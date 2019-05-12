#ifndef _RUNNER_H_
#define _RUNNER_H_

#include "roc_common.h"

extern void runner_init(char *cmd, char **args);
extern void runner_run(char *f_path, char *tag);
extern void runner_stop(void);

#endif
