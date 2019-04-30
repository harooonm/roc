#ifndef _ROC_H_
#define _ROC_H_

#include <sys/inotify.h>
#include <stdint.h>
#include <stdlib.h>
#include <regex.h>
#include <stdio.h>

extern void set_mask(uint32_t *mask, char *mask_str);
extern void prep_cmd(char *from , char **cmd_str, char ***args_arr);
extern int comp_rgx(regex_t *rgx, char *pattern, int flags);
#endif
