#ifndef _ROC_COMMON_H_
#define _ROC_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <stdlib.h>

#define FMT_STR "%s\n"
#define pr_strerror(extra) fprintf(stdout, "%s "FMT_STR, extra, strerror(errno))

#endif
