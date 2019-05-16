#include "roc_common.h"
#include <unistd.h>
#include <sys/wait.h>
static char *_cmd = NULL;
static char **_cmd_args = NULL;
static pid_t child_pid = -1;

void runner_init(char *cmd, char **args)
{
	_cmd = cmd;
	_cmd_args = args;
}

void runner_run(char *s_path, char *e_path, char *tag)
{
	if (!_cmd) {
		tag += 3; /*SKIP "IN_" */
		fprintf(stdout, "FILE %s/%s [%s]\n", s_path, e_path, tag);
	} else {
		 child_pid = fork();
		switch(child_pid){
		case 0:
			if (-1 == execv(_cmd, _cmd_args))
				pr_strerror("execv ");
			break;
		case -1:
			pr_strerror("fork()");
			break;
		default:
			waitpid(child_pid, 0, 0);
			break;
		}
	}
}

void runner_stop()
{
	if (-1 != child_pid) {
		if (0  == kill(child_pid, 0))
			kill(child_pid, SIGKILL);
		child_pid = -1;
	}
}
