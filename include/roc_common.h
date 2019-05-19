#ifndef _ROC_COMMON_H_
#define _ROC_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <stdlib.h>

#define NR_MASKS 17
#define pr_strerror(extra) fprintf(stdout, "%s %s\n", extra, strerror(errno))
#endif
