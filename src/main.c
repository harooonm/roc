#define _GNU_SOURCE
#include <getopt.h>
#include <sysexits.h>
#include <sys/stat.h>
#include "roc.h"
#include "roc_common.h"
#include "runner.h"
#include <dirent.h>
#include "sig_handler.h"
static char *usage =
                "Usage:\n\
 roc -[fchmd] [FILE...] [DIR..]\n\
 -f,    do not run command on change of files matching this regex\n\
 -c,    run this command\n\
 -h,    print help and exit\n\
 -m,    mask\n\
 -d,    do not execute command on change of directories matching this regex\n\
 -i,    ignore case while matching regex\n\n\
 -r,    recursively add watch for directory DIR\n\n\
by default reports all events to stdout\n\n\
valid values for mask\n\
a:    IN_ACCESS\n\
b:    IN_MODIFY\n\
c:    IN_ATTRIB\n\
d:    IN_CLOSE_WRITE\n\
e:    IN_CLOSE_NOWRITE\n\
f:    IN_CREATE\n\
g:    IN_DELETE\n\
h:    IN_DELETE_SELF\n\
i:    IN_MODIFY\n\
j:    IN_MOVE_SELF\n\
k:    IN_MOVED_FROM\n\
l:    IN_MOVED_TO\n\
m:    IN_OPEN\n\
n:    IN_DONT_FOLLOW\n\
o:    IN_EXCL_UNLINK\n\
p:    IN_ONESHOT\n\
q:    IN_ONLYDIR\n\
r:    IN_ALLEVENTS";

void on_sigs(int nr)
{
	watcher_stop();
}

//TODO:XXX:: i hate this function its gigantic and repetitive , FIX THIS
static int process_path(char *path, uint32_t mask, regex_t *file_rgx,
                regex_t *dir_regex)
{
	struct stat st;
	if (stat(path, &st)) {
		pr_strerror("path");
		return 0;
	}

	switch (st.st_mode & S_IFMT) {
	case S_IFBLK:
		fprintf(stderr, "Not a file or directory %s\n", path);
		return 0;
		break;
	case S_IFDIR:
	{
		if (dir_regex && !match_rgx(dir_regex, path))
			break;

		DIR *dir_stream = NULL;
		dir_stream = opendir(path);

		if (!dir_stream) {
			pr_strerror(path);
			return 0;
		}

		struct dirent *dir_entry = NULL;
		while ((dir_entry = readdir(dir_stream))) {

			if (!strcmp(dir_entry->d_name, ".")
			                || !strcmp(dir_entry->d_name, ".."))
				continue;

			char *complete_path = NULL;
			int __attribute__((unused))foo =
				asprintf(&complete_path, "%s%s%s", path,
					path[strlen(path) - 1] != '/' ? "/" :
						"", dir_entry->d_name);

			switch (dir_entry->d_type) {
			case DT_BLK:
			case DT_UNKNOWN:
			case DT_WHT:
				free(complete_path);
				closedir(dir_stream);
				return 0;
			case DT_DIR:
			case DT_CHR:
			case DT_FIFO:
			case DT_LNK:
			case DT_REG:
			case DT_SOCK:
				if (!process_path(complete_path, mask, file_rgx, dir_regex)) {
					free(complete_path);
					closedir(dir_stream);
					return 0;
				}
				break;
			}
			free(complete_path);
		}
		closedir(dir_stream);
		break;
	}
	case S_IFREG:
		if (file_rgx && !match_rgx(file_rgx, path))
			break;
		if (!watcher_add(strdup(path), mask))
			return 0;
		break;
	}
	return 1;
}

int main(int argc, char **argv)
{
	uint32_t inotify_mask = IN_ALL_EVENTS;
	int rgx_flags = (REG_NOSUB | REG_NEWLINE);
	int recur = 0;
	int optc = -1;
	int ret_code = 0;
	regex_t *rgx_file = NULL;
	regex_t*rgx_dir = NULL;
	char *dir_rgx_pattern = NULL;
	char *file_rgx_pattern = NULL;
	char *cmd_str = NULL;
	char **args_arr = NULL;

	while (1) {

		if (-1 == (optc = getopt(argc, argv, "c:f:him:d:")))
			break;

		switch (optc) {
		case 'c':
			prep_cmd(optarg, &cmd_str, &args_arr);
			break;
		case 'd':
			dir_rgx_pattern = optarg;
			break;
		case 'f':
			file_rgx_pattern = optarg;
			break;
		case 'i':
			rgx_flags |= REG_ICASE;
			break;
		case 'm':
			set_mask(&inotify_mask, optarg);
			break;
		case 'h':
			fprintf(stdout, "%s\n", usage);
			goto cleanup_exit;
		case '?':
			ret_code = 1;
			goto cleanup_exit;
		}
	}

	ret_code = 1;

	if (file_rgx_pattern
	                && !comp_rgx(rgx_file, file_rgx_pattern, rgx_flags))
		goto cleanup_exit;

	if (dir_rgx_pattern && !comp_rgx(rgx_dir, dir_rgx_pattern, rgx_flags))
		goto cleanup_exit;

	argv += optind;

	if (!*argv) {
		fprintf(stderr, "%s\n", usage);
		goto cleanup_exit;
	}

	if (!watcher_init())
		goto cleanup_exit;

	runner_init(cmd_str, args_arr);

	while (*argv) {
		if (!process_path(*argv, inotify_mask, rgx_file, rgx_dir))
			goto cleanup_exit;
		++argv;
	}

	if (!reg_sig(SIGINT, on_sigs, 0, 0)) {
		pr_strerror("reg_sig(SIGINT)");
		goto cleanup_exit;
	}

	if (!reg_sig(SIGQUIT, on_sigs, 0, 0)) {
		pr_strerror("reg_sig(SIGQUIT)");
		goto cleanup_exit;
	}

	watcher_start();
	watcher_stop();
	ret_code = 0;

cleanup_exit:
	if (rgx_file)
		regfree(rgx_file);

	if (rgx_dir)
		regfree(rgx_dir);

	if (cmd_str)
		free(cmd_str);

	if (args_arr) {
		char **p = args_arr;
		while (*p) {
			free(*p);
			++p;
		}
		free(args_arr);
	}
	return ret_code;
}
