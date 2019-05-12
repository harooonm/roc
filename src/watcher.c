#include "roc_common.h"
#include <sys/inotify.h>
#include "runner.h"
#include <limits.h>
#include <unistd.h>
#include <poll.h>
#include "libbtree.h"

struct wd {
	int descriptor;
	int mask;
	char *path;
}__attribute__((packed));

static struct pollfd in_fd [1];
static btree_t *watches = NULL;

int watcher_init(void)
{
	if ((in_fd[0].fd = inotify_init()) == -1)
		pr_strerror("inotify init");
	return -1 != in_fd[0].fd;
}

int comp_wd(void *old, void *new)
{
	struct wd *old_wd = (struct wd *) old;
	struct wd *new_wd = (struct wd *) new;
	if (new_wd->descriptor == old_wd->descriptor)
		return 0;
	if (new_wd->descriptor > old_wd->descriptor)
		return 1;
	return -1;
}

int watcher_add(char *path, uint32_t mask)
{
	assert(path);
	int descriptor = inotify_add_watch(in_fd[0].fd, path, mask);
	if (-1 == descriptor) {
		pr_strerror("inotify_add_watch");
		return 0;
	}
	struct wd *w = calloc(1, sizeof(struct wd));
	w->descriptor = descriptor;
	w->mask = mask;
	w->path = path;
	add_tree_node(&watches, w, comp_wd);
	return 1;
}

void free_wd(void *data)
{
	struct wd * w = (struct wd *)data;
	inotify_rm_watch(in_fd[0].fd, w->descriptor);
	if(w->path)
		free(w->path);
	free(w);
}

void watcher_start(void)
{
	if (!watches)
		return;

	in_fd[0].events = POLLIN | POLLNVAL | POLLERR;
	while(1) {
		int poll_res = poll(&in_fd[0], 1, -1);
		if (-1 == poll_res){
			if (EINTR != errno)
				pr_strerror("pollu");
			break;
		}

		if (poll_res <= 0 || !(in_fd[0].revents & POLLIN))
			continue;

		if (in_fd[0].revents & (POLLERR | POLLNVAL)) {
			pr_strerror("POLLERR|POLLNVAL");
			break;
		}

		size_t min_read_sz = sizeof(struct inotify_event) + NAME_MAX + 1;

		char buf [min_read_sz];

		size_t bytes_in = read(in_fd[0].fd, buf, min_read_sz);

		if (bytes_in <= 0)
			continue;

		char *buf_p = (char *)&buf;
		while(bytes_in > 0) {
			struct inotify_event *ev = (struct inotify_event *)buf_p;

			/*check exceptional masks*/
			if (ev->mask & (IN_IGNORED | IN_UNMOUNT)){
				del_tree_node(&watches,
					&((struct wd){ev->wd, 0, 0}),
						comp_wd, free_wd);
				continue;
			}

			if (ev->mask & IN_Q_OVERFLOW)
				continue;

			btree_t *descriptor = find_tree_node(&watches,
				&(struct wd){ev->wd, 0, 0}, comp_wd);
			if (!descriptor ||
				(descriptor &&
					(!(((struct wd *)(descriptor->data))->mask
						& ev->mask)))) {
				continue;
			}

			if (ev->mask & IN_ISDIR)
				ev->mask &= ~IN_ISDIR;

			runner_run(((struct wd *)descriptor->data)->path,
				get_mask_info('\0', ev->mask)->name);

			size_t step = sizeof(struct inotify_event) + ev->len;
			bytes_in -= step;
			buf_p += step;
		}
	}
}

void watcher_stop(void)
{
	if (-1 != in_fd[0].fd)
		close(in_fd[0].fd);
	runner_stop();
	free_tree(&watches, free_wd);
}
