#include "roc.h"
#include "roc_common.h"

int set_mask(uint32_t *mask, char *mask_str, int masks_len)
{
	while (*mask_str) {
		char mask_char = *mask_str;

		int id = (mask_char - 'a');

		if (id < 0 || id >= masks_len) {
			fprintf(stdout, "invalid mask %c\n", mask_char);
			return 0;
		}

		switch(id)
		{
			case 12:
			case 13:
			case 14:
				id += 12;
				break;
			case 15:
			case 16:
				*mask = (1 << 31);
				++mask_str;
				continue;
		}
		*mask |= (1 << id);
		++mask_str;
	}
	return 1;
}

char get_mask(uint32_t mask_val)
{
	uint32_t i = 0;
	while(i < NR_MASKS) {
		if (!(mask_val & (1 << i++)))
			return 'a' + i;
	}
	return 'X';
}

void prep_cmd(char *from, char **cmd_str, char ***args_arr)
{
	char *tok = strtok(from, " ");
	*cmd_str = strdup(tok);
	int len = 0;

	(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
	((*args_arr)[len - 1]) = strdup(tok);

	while ((tok = strtok(NULL, " "))) {
		(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
		((*args_arr)[len - 1]) = strdup(tok);
	}
	if (len) {
		(*args_arr) = realloc((*args_arr), (sizeof(char **) * ++len));
		(*args_arr)[len - 1] = NULL;
	}
}
