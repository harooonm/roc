#ifndef _ROC_COMMON_H_
#define _ROC_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <stdlib.h>
#include <regex.h>

#define FMT_STR "%s\n"
#define pr_strerror(extra) fprintf(stdout, "%s "FMT_STR, extra, strerror(errno))
struct inotify_mask {
	char abbrv;
	char *name;
	uint32_t mask;
};

#define def_mask(a,m) {a, #m, m}

static struct inotify_mask imasks [] = {
	def_mask('a', IN_ACCESS),
	def_mask('b', IN_ATTRIB),
	def_mask('c', IN_CLOSE_WRITE),
	def_mask('d', IN_CLOSE_NOWRITE),
	def_mask('e', IN_CREATE),
	def_mask('f', IN_DELETE),
	def_mask('g', IN_DELETE_SELF),
	def_mask('h', IN_MODIFY),
	def_mask('i', IN_MOVE_SELF),
	def_mask('j', IN_MOVED_FROM),
	def_mask('k', IN_MOVED_TO),
	def_mask('l', IN_OPEN),
	def_mask('m', IN_DONT_FOLLOW),
	def_mask('n', IN_EXCL_UNLINK),
	def_mask('o', IN_ONESHOT),
	def_mask('p', IN_ONLYDIR),
	def_mask('q', IN_ALL_EVENTS),
/*err masks*/
	def_mask('r', IN_IGNORED),
	def_mask('s', IN_ISDIR),
	def_mask('t', IN_Q_OVERFLOW),
	def_mask('u', IN_UNMOUNT),
	{'\0', 0, 0}
};

extern struct inotify_mask *get_mask_info(char c, uint32_t mask);
#endif
