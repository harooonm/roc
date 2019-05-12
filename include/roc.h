#ifndef _ROC_H_
#define _ROC_H_

#include <stdint.h>
#include <regex.h>

extern void set_mask(uint32_t *mask, char *mask_str);
extern void prep_cmd(char *from , char **cmd_str, char ***args_arr);
extern int comp_rgx(regex_t *rgx, char *pattern, int flags);
extern int match_rgx(regex_t *rgx, char *to_match);
extern int watcher_init(void);
extern int watcher_add(char *path, uint32_t mask);
extern void watcher_start(void);
extern void watcher_stop(void);

#endif
