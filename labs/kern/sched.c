#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>


// Choose a user environment to run and run it.
void
sched_yield(void)
{
	// Implement simple round-robin scheduling.
	// Search through 'envs' for a runnable environment,
	// in circular fashion starting after the previously running env,
	// and switch to the first such environment found.
	// It's OK to choose the previously running env if no other env
	// is runnable.
	// But never choose envs[0], the idle environment,
	// unless NOTHING else is runnable.
	int fpid = curenv != NULL ? curenv->env_id : 0;
	int pid = curenv != NULL ? (curenv->env_id + 1) % NENV : 1;
	for (; pid != fpid; pid = (pid + 1) % NENV) {
		if (pid == 0) continue;
		if (envs[pid].env_status == ENV_RUNNABLE)
			env_run(&envs[pid]);
	}

	// Run the special idle environment when nothing else is runnable.
	if (envs[0].env_status == ENV_RUNNABLE)
		env_run(&envs[0]);
	else {
		cprintf("Destroyed all environments - nothing more to do!\n");
		while (1)
			monitor(NULL);
	}
}
