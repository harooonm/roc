#define _GNU_SOURCE
#include <getopt.h>
#include <sysexits.h>
#include <sys/stat.h>
#include "roc.h"
#include "roc_common.h"
#include "runner.h"
#include <dirent.h>
#include "sig_handler.h"

char *cmd_str = NULL;
char **args_arr = NULL;

static char *usage = "Usage:\n\
 roc -[fchmd] [FILE...] [DIR..]\n\
 -c,    run this command\n\
 -h,    print help and exit\n\
 -m,    mask\n\
by default reports all events to stdout\n\n\
valid values for mask\n\
a:    IN_ACCESS\n\
b:    IN_MODIFY\n\
c:    IN_ATTRIB\n\
d:    IN_CLOSE_WRITE\n\
e:    IN_CLOSE_NOWRITE\n\
f:    IN_OPEN\n\
g:    IN_MOVED_FROM\n\
h:    IN_MOVED_TO\n\
i:    IN_CREATE\n\
j:    IN_DELETE\n\
k:    IN_DELETE_SELF\n\
l:    IN_MOVE_SELF\n\
m:    IN_ONLYDIR\n\
n:    IN_DONT_FOLLOW\n\
o:    IN_EXCL_UNLINK\n\
p:    IN_ONESHOT\n\
q:    IN_ALLEVENTS";

/*
	"a" IN_ACCESS 0      a = 97 , 2^0
	"b" IN_MODIFY 1      b = 98 , 2^1
	"c" IN_ATTRIB 2      c = 99 , 2^2
	"d" IN_CLOSE_WRITE   d = 100, 2^3
	"e" IN_CLOSE_NOWRITE e = 101, 2^4
	"f" IN_OPEN          f = 102, 2^5
	"g" IN_MOVED_FROM    g = 103, 2^6
	"h" IN_MOVED_TO      h = 104, 2^7
	"i" IN_CREATE        i = 105, 2^8
	"j" IN_DELETE        j = 106, 2^9
	"k" IN_DELETE_SELF   k = 107, 2^10
	"l" IN_MOVE_SELF     l = 108, 2^11
	"m" IN_ONLYDIR       m = 109, 2^24
	"n" IN_DONT_FOLLOW   n = 110  2^25
	"o" IN_EXCL_UNLINK   o = 111  2^26
	"p" IN_ONSESHOT      p = 112  2^31;
*/

void on_sigs(int nr)
{
	watcher_stop();
}

static int process_dir(char *path, uint32_t mask)
{
	/*add the directory to watch list*/
	if (!watcher_add(path, mask, 1))
		return 0;

	/*now iterate the directory*/
	DIR *dir_stream = opendir(path);

	if (!dir_stream) {
		pr_strerror(path);
		return 0;
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
			goto break_loop;
		case DT_DIR:
			if (!process_dir(complete_path, mask))
				goto break_loop;
			break;
		case DT_CHR:
		case DT_FIFO:
		case DT_LNK:
		case DT_REG:
		case DT_SOCK:
			if (!watcher_add(complete_path, mask, 1))
				goto break_loop;
			break;
		}
		free(complete_path);
		continue;
	break_loop:
		free(complete_path);
		closedir(dir_stream);
		return 0;
	}
	closedir(dir_stream);
	return 1;
}


int main(int argc, char **argv)
{
	uint32_t inotify_mask = IN_MODIFY;
	int optc = -1;
	int ret_code = 0;

	while (-1 != (optc = getopt(argc, argv, "c:hm:"))) {
		switch (optc) {
		case 'c':
			prep_cmd(optarg, &cmd_str, &args_arr);
			break;
		case 'm':
			if(!set_mask(&inotify_mask, optarg, NR_MASKS))
				goto cleanup_exit;
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

	struct stat st;
	while (*argv) {
		if (stat(*argv, &st)) {
			pr_strerror(*argv);
			goto cleanup_exit;
		}

		switch (st.st_mode & S_IFMT) {
		case S_IFBLK:
			fprintf(stderr, "Not a file or directory %s\n", *argv);
			goto cleanup_exit;
		case S_IFDIR:
			if (!process_dir(*argv, inotify_mask))
				goto cleanup_exit;
			break;
		default:
			if (!watcher_add(*argv, inotify_mask, 1))
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
	char **p = args_arr;
	while (p && *p)
		free(*p++);
	if (args_arr)
		free(args_arr);
	return ret_code;
}
