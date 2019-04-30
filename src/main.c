#include <getopt.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "roc.h"

#define FMT_STR "%s\n"

static char *usage = "Usage:\n\
 roc -[fch] [FILE...] [DIR..]\n\
 -f,    do not watch files matching this regex\n\
 -c,    run this command e.g [make -CFLAGS+=-g]\n\
 -h,    print help and exit\n\
 -m,    mask [comma separated list of masks] default for files [MOD] for \
directory\n        [OPEN] by default prints events to stdout\n\
 -d,    do not match directories matching this regex\n\
 -i,    ignore case while matching regex\n\n\
valid values for mask IN_OPEN, IN_MOVED_TO, IN_MOVED_FROM, IN_MOVE_SELF,\
IN_MODIFY,\n IN_DELETE_SELF, IN_DELETE, IN_CREATE, IN_CLOSE_NOWRITE,\
IN_CLOSE_WRITE, IN_ATTRIB,\n IN_ACCESS, IN_DONT_FOLLOW, IN_EXCL_UNLINK,\
IN_ONESHOT, IN_ONLYDIR, IN_ALL_EVENTS";


static int inotify_mask = (IN_OPEN | IN_MODIFY),
rgx_flags = (REG_NOSUB | REG_NEWLINE), inotify_fd = -1;

static regex_t *rgx_file = 0, *rgx_dir = 0;

static char *dir_rgx_pattern = NULL, *file_rgx_pattern = NULL, *cmd_str = NULL,
**args_arr = NULL;

void __attribute__((destructor)) clean()
{
	if (cmd_str)
		free(cmd_str);

	if (args_arr){
		char **cp = args_arr;
		while (cp && *cp)
			free(*cp++);
		free(args_arr);
	}

	if (rgx_file)
		regfree(rgx_file);

	if (rgx_dir)
		regfree(rgx_dir);

	if (-1 != inotify_fd)
		close(inotify_fd);
}

static void pr_exit(int code, char *fmt,char *msg)
{
	FILE *stream = stdout;
	if(code)
		stream = stderr;
	fprintf(stream, fmt, msg);
	exit(code);
}

#define usage_exit(code) pr_exit(code, FMT_STR, usage)

int main(int argc, char **argv)
{
	int optc = -1;
	while(1) {

		if (-1 == (optc = getopt(argc, argv, "c:f:him:")))
			break;

		switch(optc){
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
		case 'h':/*yes gcc recognises fall through as not a "comment"*/
			usage_exit(0);
			/*fall through*/
		case '?':
			usage_exit(1);
		}
	}

	if (file_rgx_pattern && 0 != comp_rgx(rgx_file, file_rgx_pattern,
		rgx_flags))
			return 1;

	if (dir_rgx_pattern &&  0 != comp_rgx(rgx_dir, dir_rgx_pattern,
		rgx_flags))
			return 1;

	argv += optind;

	if (!*argv)
		usage_exit(1);

	inotify_fd = inotify_init(IN_NONBLOCK);
	if (-1 == (inotif_fd = inotify_init(IN_NONBLOCK)))
		pr_exit(1, "inotify_init () : "FMT_STR, strerror(errno));

	struct stat st;
	while(*argv) {
		if (stat(*argv, &st))
			pr_exit(1, FMT_STR, strerror(errno));

		switch(st.st_mode & S_IFMT) {
		case S_IFBLK:
		case S_IFDIR:
		case S_IFCHR:
		case S_IFIFO:
		case S_IFREG:
		case S_IFLNK:
		case S_IFSOCK:
			break;
		default:
			pr_exit(1, "not a file or directory"FMT_STR, *argv);
		}
		++argv;
	}
	return 0;
}
