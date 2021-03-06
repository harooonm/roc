#include "roc_common.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <regex.h>

extern char *cmd_str;
extern char **args_arr;
extern regex_t *rgx_file;
extern regex_t *rgx_dir;
static pid_t child_pid = -1;


void runner_run(char *s_path, char *e_path, char mask_char)
{
	if (rgx_file)
		if (0 == regexec(rgx_file, s_path, 0, 0, 0))
			return;

	if (rgx_dir)
		if (0 == regexec(rgx_dir, s_path, 0, 0, 0))
			return;

	if (!cmd_str) {
		fprintf(stdout, "FILE %s/%s [%c]\n", s_path, e_path, mask_char);
	} else {
		child_pid = fork();
		switch(child_pid){
		case 0:
			if (-1 == execv(cmd_str, args_arr))
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
	if (-1 == child_pid)
		return;
	if (0  == kill(child_pid, 0))
		kill(child_pid, SIGKILL);
	child_pid = -1;
}
