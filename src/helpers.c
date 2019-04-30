#include <string.h>
#include "roc.h"

#define to_str(x) #x

void set_mask(uint32_t *mask, char *mask_str)
{
	char *mask_tok = NULL;
	while((mask_tok = strtok(mask_str, ","))) {

		if (!strcmp(mask_tok, to_str(IN_MOVED_TO)))
			*mask |= IN_MOVED_TO;

		if (!strcmp(mask_tok, to_str(IN_MOVED_FROM)))
			*mask |= IN_MOVED_FROM;

		if (!strcmp(mask_tok, to_str(IN_MOVE_SELF)))
			*mask |= IN_MOVE_SELF;

		if (!strcmp(mask_tok, to_str(IN_DELETE_SELF)))
			*mask |= IN_DELETE_SELF;

		if (!strcmp(mask_tok, to_str(IN_DELETE)))
			*mask |= IN_DELETE;

		if (!strcmp(mask_tok, to_str(IN_CREATE)))
			*mask |= IN_CREATE;

		if (!strcmp(mask_tok, to_str(IN_CLOSE_NOWRITE)))
			*mask |= IN_CLOSE_NOWRITE;

		if (!strcmp(mask_tok, to_str(IN_ATTRIB)))
			*mask |= IN_ATTRIB;

		if (!strcmp(mask_tok, to_str(IN_ACCESS)))
			*mask |= IN_ACCESS;

		if (!strcmp(mask_tok, to_str(IN_DONT_FOLLOW)))
			*mask |= IN_DONT_FOLLOW;

		if (!strcmp(mask_tok, to_str(IN_EXCL_UNLINK)))
			*mask |= IN_EXCL_UNLINK;

		if (!strcmp(mask_tok, to_str(IN_ONESHOT)))
			*mask |= IN_ONESHOT;

		if (!strcmp(mask_tok, to_str(IN_ONLYDIR)))
			*mask |= IN_ONLYDIR;

		if (!strcmp(mask_tok, to_str(IN_ALL_EVENTS)))
			*mask |= IN_ALL_EVENTS;
	}
}

void prep_cmd(char *from, char **cmd_str, char ***args_arr)
{
	char *tok = strtok(from, " ");
	*cmd_str = strdup(tok);
	int len  = 0;
	while((tok = strtok(NULL, " "))) {
		(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
		((*args_arr)[len - 1]) = strdup(tok);
	}
	if (len) {
		(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
		(*args_arr) [len - 1] = NULL;
	}
}

int comp_rgx(regex_t *rgx, char *pattern, int flags)
{
	char *err = calloc(4096, 1);
	rgx = malloc(sizeof(regex_t));
	int rgx_err  = 0;
	if ((rgx_err = regcomp(rgx, pattern, flags))) {
		regerror(rgx_err, rgx, err, 4096);
		fprintf(stderr, "%s\n", err);
	}
	free (err);
	return rgx_err;
}

inline int match_rgx(regex_t *rgx, char *to_match)
{
	return (0 == regexec(rgx, to_match, 0, 0, 0));
}
