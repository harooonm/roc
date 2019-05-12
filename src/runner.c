#include "roc_common.h"

static char *_cmd = NULL;
static char **_cmd_args = NULL;

void runner_init(char *cmd, char **args)
{
	_cmd = cmd;
	_cmd_args = args;
}

void runner_run(char *f_path, char *tag)
{
	if (!_cmd) {
		tag += 3; /*SKIP "IN_" */
		fprintf(stdout, "FILE %s [%s]\n", f_path, tag);
	} else {
		/*fork exec games*/
	}
}

void runner_stop()
{
	//if there is a running command stop it immediately
}
