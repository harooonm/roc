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
 -c,    run this command\n\
 -h,    print help and exit\n\
 -m,    mask\n\
by default reports all events to stdout\n\n\
valid values for mask\n\
a:    IN_ACCESS\n\
b:    IN_ATTRIB\n\
c:    IN_CLOSE_WRITE\n\
d:    IN_CLOSE_NOWRITE\n\
e:    IN_CREATE\n\
f:    IN_DELETE\n\
g:    IN_DELETE_SELF\n\
h:    IN_MODIFY\n\
i:    IN_MOVE_SELF\n\
j:    IN_MOVED_FROM\n\
k:    IN_MOVED_TO\n\
l:    IN_OPEN\n\
m:    IN_DONT_FOLLOW\n\
n:    IN_EXCL_UNLINK\n\
o:    IN_ONESHOT\n\
p:    IN_ONLYDIR\n\
q:    IN_ALLEVENTS";

void on_sigs(int nr)
{
	watcher_stop();
}

enum{
	ERR,
	IGNORED,
	ADDED
};

//TODO:REFINE THIS MAMOTH XXX
static int process_dir(char *path, uint32_t mask)
{
	/*add the directory to watch list*/
	if (!watcher_add(path, mask, 1))
		return ERR;

	/*now iterate the directory*/
	DIR *dir_stream = opendir(path);

	if (!dir_stream) {
		pr_strerror(path);
		return ERR;
	}

	struct dirent *dir_entry = NULL;
	while ((dir_entry = readdir(dir_stream))) {
		if (!strcmp(dir_entry->d_name, ".") ||
			!strcmp(dir_entry->d_name, ".."))
			continue;
		char *complete_path = NULL;
		int __attribute__((unused)) len =
			asprintf(&complete_path, "%s%s%s", path,
				path[strlen(path) - 1] != '/' ? "/" :
						"", dir_entry->d_name);
		switch (dir_entry->d_type) {
		case DT_BLK:
		case DT_UNKNOWN:
		case DT_WHT:
			fprintf(stderr, "Not a file or directory %s",
				complete_path);
			free(complete_path);
			closedir(dir_stream);
			return ERR;
		case DT_DIR:
			if (ERR == process_dir(complete_path, mask)) {
				free(complete_path);
				closedir(dir_stream);
				return ERR;
			}
			break;
		case DT_CHR:
		case DT_FIFO:
		case DT_LNK:
		case DT_REG:
		case DT_SOCK:
			if (!watcher_add(complete_path, mask, 1)) {
				free(complete_path);
				closedir(dir_stream);
				return ERR;
			}
			break;
		}
		free(complete_path);
	}
	closedir(dir_stream);
	return ADDED;
}

int main(int argc, char **argv)
{
	uint32_t inotify_mask = IN_MODIFY;
	int optc = -1;
	int ret_code = 0;
	char *cmd_str = NULL;
	char **args_arr = NULL;

	while (1) {

		if (-1 == (optc = getopt(argc, argv, "c:hm:")))
			break;

		switch (optc) {
		case 'c':
			prep_cmd(optarg, &cmd_str, &args_arr);
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

	argv += optind;

	if (!*argv) {
		fprintf(stderr, "%s\n", usage);
		goto cleanup_exit;
	}

	if (!watcher_init())
		goto cleanup_exit;

	runner_init(cmd_str, args_arr);

	struct stat st;
	while (*argv) {
		if (stat(*argv, &st)) {
			pr_strerror(*argv);
			return 0;
		}

		switch (st.st_mode & S_IFMT) {
		case S_IFBLK:
			fprintf(stderr, "Not a file or directory %s\n", *argv);
			goto cleanup_exit;
		case S_IFDIR:
			if (ERR == process_dir(*argv, inotify_mask))
				goto cleanup_exit;
			break;
		default:
			if (ERR == watcher_add(*argv, inotify_mask, 1))
				goto cleanup_exit;
			break;
		}
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
