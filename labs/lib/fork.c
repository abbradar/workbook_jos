// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	void *pgaddr = (void *)PTE_ADDR(addr);
	uint32_t err = utf->utf_err;
	int perm, nperm;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at vpt
	//   (see <inc/memlayout.h>).
	cprintf("oh hai %08x, va %08x, page %08x, ip %08x sp %08x\n", sys_getenvid(), addr, pgaddr, utf->utf_eip, utf->utf_esp);
	if ((err | FEC_PR | FEC_WR) != err)
		panic("[%08x] user fault in library handler va %08x ip %08x: nonexistent or protected page\n", sys_getenvid(), addr, utf->utf_eip);

	perm = vpt[PPN(pgaddr)] & PTE_USER;
	if ((perm | PTE_U | PTE_COW) != perm)
		panic("[%08x] user fault in library handler va %08x ip %08x: not a user or COW page\n", sys_getenvid(), addr, utf->utf_eip);

	nperm = (perm & (~PTE_COW)) | PTE_W;

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	//   No need to explicitly delete the old page's mapping.
	r = sys_page_alloc(0, PFTEMP, nperm);
	if (r < 0)
		panic("sys_page_alloc failed with %d\n", r);
	// Copy the page at whole
	memmove(PFTEMP, pgaddr, PGSIZE);
	// Remap!
	r = sys_page_map(0, PFTEMP, 0, pgaddr, nperm);
	if (r < 0)
		panic("sys_page_map failed with %d\n", r);
	r = sys_page_unmap(0, PFTEMP);
	if (r < 0)
		panic("sys_page_unmap failed with %d\n", r);
	cprintf("oh bye %08x, va %08x, page %08x, ip %08x sp %08x byte %08x\n", sys_getenvid(), addr, pgaddr, utf->utf_eip, utf->utf_esp, *(unsigned *)addr);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why might we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
// 
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	int perm = vpt[pn] & PTE_USER;
	void *addr = (void *)(pn * PGSIZE);

	if (perm & PTE_W || perm & PTE_COW) {
		perm &= ~PTE_W;
		perm |= PTE_COW;
		r = sys_page_map(0, addr, 0, addr, perm);
		if (r < 0)
			return r;
	}

	return sys_page_map(0, addr, envid, addr, perm);
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use vpd, vpt, and duppage.
//   Remember to fix "env" and the user exception stack in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	envid_t cpid;

	set_pgfault_handler(pgfault);

	cpid = sys_exofork();
	if (cpid < 0)
		return cpid;
	if (cpid == 0) {
		cprintf("test test %08x\n", sys_getenvid());
		env = envs + ENVX(sys_getenvid());
		return 0;
	} else {
		int pdi;
		int r;

		r = sys_page_alloc(cpid, (void *)(UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W);
		if (r < 0) {
			// ignore an error, if any
			sys_env_destroy(cpid);
			return r;
		}
		r = sys_env_set_pgfault_upcall(cpid, env->env_pgfault_upcall);
		if (r < 0) {
			sys_env_destroy(cpid);
			return r;
		}

		for (pdi = 0; pdi < VPD(UTOP); pdi++) {
			int cpti;
			int dentry = vpd[pdi];

			if (!(dentry & PTE_P) || !(dentry & PTE_U)) {
				continue;
			}

			for (cpti = 0; cpti < NPTENTRIES; cpti++) {
				int pn = cpti + pdi * NPTENTRIES;
				int entry = vpt[pn];

				if (pn == PPN(UXSTACKTOP - PGSIZE)) {
					continue;
				}

				if (!(entry & PTE_P) || !(entry & PTE_U)) {
					continue;
				}

				r = duppage(cpid, pn);
				if (r < 0) {
					// any copied already pages will be restored later by our page
					// fault handler.
					sys_env_destroy(cpid);
					return r;
				}
			}
		}

		r = sys_env_set_status(cpid, ENV_RUNNABLE);
		if (r < 0) {
			sys_env_destroy(cpid);
			return r;
		}
		return cpid;
	}
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
