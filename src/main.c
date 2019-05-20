#define _GNU_SOURCE
#include <getopt.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <regex.h>
#include "roc.h"
#include "roc_common.h"
#include "runner.h"
#include <dirent.h>
#include "sighandler.h"

char *cmd_str = NULL;
char **args_arr = NULL;
regex_t *rgx_dir = NULL;
regex_t *rgx_file = NULL;

static char *usage = "Usage:\n\
 roc -[chmfd] [FILE...] [DIR..]\n\
 -c,    run this command\n\
 -h,    print help and exit\n\
 -m,    mask\n\
 -f,    ignore files matching this regex\n\
 -d,    ignore directories matching this regex\n\
 -i,    case insensitive regex\n\
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
	"p" IN_ONSESHOT      p = 112  2^31
	"q" IN_ALL_EVENTS    q = 113;
*/

void on_sigs(int __attribute__((unused)) nr)
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
		default:
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
	int rgx_flags = (REG_NOSUB | REG_NEWLINE);
	char *opt_f = NULL;
	char *opt_d = NULL;

	while (-1 != (optc = getopt(argc, argv, "c:hm:d:fi"))) {
		switch (optc) {
		case 'c':
			prep_cmd(optarg, &cmd_str, &args_arr);
			break;
		case 'm':
			if(!set_mask(&inotify_mask, optarg, NR_MASKS))
				goto cleanup_exit;
			break;
		case 'd':
			rgx_dir = calloc(1, sizeof(regex_t));
			opt_d = optarg;
			break;
		case 'f':
			rgx_file = calloc(1, sizeof(regex_t));
			opt_f = optarg;
			break;
		case 'i':
			rgx_flags |= REG_ICASE;
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

	if (rgx_dir) {
		if (regcomp(rgx_dir, opt_d, rgx_flags)) {
			fprintf(stderr, "invalid regex %s\n", opt_d);
			goto cleanup_exit;
		}
	}

	if (rgx_file) {
		if (regcomp(rgx_file, opt_f, rgx_flags)) {
			fprintf(stderr, "invalid regex %s\n", opt_d);
			goto cleanup_exit;
		}
	}

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

	if (rgx_dir)
		regfree(rgx_dir);

	if (rgx_file)
		regfree(rgx_file);

	return ret_code;
}
