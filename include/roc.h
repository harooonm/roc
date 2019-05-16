#ifndef _ROC_H_
#define _ROC_H_

#include <stdint.h>

extern void set_mask(uint32_t *mask, char *mask_str);
extern void prep_cmd(char *from , char **cmd_str, char ***args_arr);
extern int watcher_init(void);
extern int watcher_add(char *path, uint32_t mask, int pr_err);
extern void watcher_start(void);
extern void watcher_stop(void);

#endif
