#include <limits.h>
#include <string.h>
#include <poll.h>

#include "fev/fev.h"
#include "fev/time.h"

static void io_free_all(fev_t *fev);
static void timer_free_all(fev_t *fev);
static int get_min_interval(flist_t *timers);

int fev_init(fev_t *fev)
{
    memset(fev, 0, sizeof(fev_t));
    fev->io = flist_new();
    fev->timers = flist_new();

    // With a 0 element list, the function returns INT_MAX
    fev->min_interval = get_min_interval(fev->timers);

    return 0;
}

void fev_free(fev_t *fev)
{
    if (!fev)
        return;

    io_free_all(fev);
    timer_free_all(fev);
    flist_free(fev->io);
    flist_free(fev->timers);

    memset(fev, 0, sizeof(fev_t));

}

static struct fev_io* io_new(int fd, uint8_t ev, fev_io_cb cb, void *arg)
{
    struct fev_io *io = calloc(1, sizeof(struct fev_io));

    io->fd = fd;
    io->ev = ev;
    io->cb = cb;
    io->arg = arg;

    return io;
}

static struct fev_timer* timer_new(uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg)
{
    struct fev_timer *timer = calloc(1, sizeof(struct fev_timer));

    timer->last_call = fev_clock();
    timer->interval = interval;
    timer->flags = flags;
    timer->cb = cb;
    timer->arg = arg;

    return timer;
}

static void io_free(struct fev_io *io)
{
    if (!io)
        return;

    free(io);
}

static void timer_free(struct fev_timer *timer)
{
    if (!timer)
        return;

    free(timer);
}

static void io_free_all(fev_t *fev)
{

    flist_node_t *cur = 0;

    for (cur = fev->io->start; cur; cur = cur->next)
        io_free(cur->item);
}

static void timer_free_all(fev_t *fev)
{
    flist_node_t *cur = 0;

    for (cur = fev->timers->start; cur; cur = cur->next)
        timer_free(cur->item);
}

static int get_min_interval(flist_t *timers)
{
    int min = INT_MAX;
    flist_node_t *cur = 0;

    for (cur=timers->start; cur; cur=cur->next) {
        struct fev_timer *timer = cur->item;
        if (timer->interval < min)
            min = timer->interval;
    }

    return min;
}

struct fev_io* fev_add_io(fev_t *fev, int fd, uint8_t ev, fev_io_cb cb, void *arg)
{
    struct fev_io *io = 0;

    if (!cb)
        goto cleanup;

    io = io_new(fd, ev, cb, arg);

    flist_insert(fev->io, io);

cleanup:
    return io;
}

struct fev_timer* fev_add_timer(fev_t *fev, uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg)
{
    struct fev_timer *timer = 0;

    if (!cb)
        goto cleanup;

    timer = timer_new(interval, flags, cb, arg);
    flist_insert(fev->timers, timer);

    fev->min_interval = get_min_interval(fev->timers);

cleanup:
    return timer;
}

void fev_del_io(fev_t *fev, struct fev_io *io)
{
    flist_node_t *io_node = flist_search(fev->io, io);
    if (!io_node)
        return;

    io_free(io);
    flist_remove(fev->io, io);
}

void fev_del_timer(fev_t *fev, struct fev_timer *timer)
{

    flist_node_t *timer_node = flist_search(fev->timers, timer);
    if (!timer_node)
        return;

    timer_free(timer);
    flist_remove(fev->timers, timer);

    fev->min_interval = get_min_interval(fev->timers);

}

static int timer_run_cb(flist_node_t *timer_node, void *arg)
{
    fev_t *fev = arg;
    struct fev_timer *timer = timer_node->item;


    if ((fev->cl - timer->last_call) >= timer->interval) {

        timer->cb(timer->arg);

        if (timer->flags & FEV_TIMER_FLAG_ONCE)
            fev_del_timer(fev, timer);
        else
            timer->last_call = fev->cl;

    }

    return 0;
}

static void timers_run(fev_t *fev)
{
    flist_iterate(fev->timers, timer_run_cb, fev);
}

static void set_pollfd_by_io(struct pollfd *pfd, struct fev_io *io)
{
    pfd->fd = io->fd;

    if (io->ev & FEV_EVENT_READ)
        pfd->events |= POLLIN;

    if (io->ev & FEV_EVENT_WRITE)
        pfd->events |= POLLOUT;
}

static void set_io_by_pollfd(struct fev_io *io, struct pollfd *pfd)
{
    if (pfd->revents & POLLIN)
        io->rev |= FEV_EVENT_READ;

    if (pfd->revents & POLLOUT)
        io->rev |= FEV_EVENT_WRITE;

    if (pfd->revents & POLLERR)
        io->rev |= FEV_EVENT_ERROR;
}

static int io_poll(fev_t *fev)
{

    int r = -1, rp;
    struct pollfd *pfd = 0;
    flist_node_t *cur = 0;
    size_t i = 0;

    pfd = calloc(fev->io->n_nodes, sizeof(struct pollfd));

    for (cur = fev->io->start; cur; cur = cur->next, i++) {
        struct fev_io *io = cur->item;
        set_pollfd_by_io(&pfd[i], io);
    }

    rp = poll(pfd, i, fev->min_interval);

    if (rp == 0)
        goto success_cleanup;
    else if (rp < 0)
        goto cleanup;

    for (cur = fev->io->start, i=0; cur && rp; cur = cur->next, i++) {

        struct fev_io *io = cur->item;

        if (pfd[i].revents != 0) { // If there are revents
            set_io_by_pollfd(io, &pfd[i]);
            rp--; // rp is the counter of pollfd structures with revents set.
            // We can use rp as counter.
        }

    }

success_cleanup:
    r = 0;
cleanup:
    free(pfd);
    return r;
}

static int io_run_cb(flist_node_t *io_node, void *arg)
{
    fev_t *fev = arg;
    struct fev_io *io = 0;
    uint8_t rev;

    io = io_node->item;

    if (io->rev != 0) {
    	rev = io->rev;
    	io->rev = 0;
        io->cb(fev, io, rev, io->arg);
        // io might have been freed now
    }

    return 0;
}

static void io_run(fev_t *fev)
{
    flist_iterate(fev->io, io_run_cb, fev);
}

int fev_run(fev_t *fev)
{
    fev->cl = fev_clock();
    timers_run(fev);

    if (io_poll(fev) != 0)
        return -1;

    io_run(fev);

    return 0;
}
