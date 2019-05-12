#include "roc.h"
#include "roc_common.h"

struct inotify_mask *get_mask_info(char c, uint32_t mask)
{
	struct inotify_mask *m = imasks;
	while (m->abbrv != '\0') {
		if ((c != '\0' && c == m->abbrv)
		                || (mask != -1 && mask == m->mask))
			break;
		++m;
	}
	return m;
}

void set_mask(uint32_t *mask, char *mask_str)
{
	while (mask_str && *mask_str != '\0') {
		struct inotify_mask *m = get_mask_info(*mask_str, -1);
		if (m)
			*mask |= m->mask;
		++mask_str;
	}
}

void prep_cmd(char *from, char **cmd_str, char ***args_arr)
{
	char *tok = strtok(from, " ");
	*cmd_str = strdup(tok);
	int len = 0;
	while ((tok = strtok(NULL, " "))) {
		(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
		((*args_arr)[len - 1]) = strdup(tok);
	}
	if (len) {
		(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
		(*args_arr)[len - 1] = NULL;
	}
}

int comp_rgx(regex_t *rgx, char *pattern, int flags)
{
	char *err = calloc(4096, 1);
	rgx = malloc(sizeof(regex_t));
	int rgx_err = 0;
	if ((rgx_err = regcomp(rgx, pattern, flags))) {
		regerror(rgx_err, rgx, err, 4096);
		fprintf(stderr, "%s\n", err);
	}
	free(err);
	return (0 == rgx_err);
}

inline int match_rgx(regex_t *rgx, char *to_match)
{
	return (0 == regexec(rgx, to_match, 0, 0, 0));
}
