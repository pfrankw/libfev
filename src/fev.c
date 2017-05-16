#include <string.h>

#include "fev/fev.h"
#include "fev/time.h"

static void io_free_all(fev_t *fev);
static void timer_free_all(fev_t *fev)


int fev_init(fev_t *fev)
{
    memset(fev, 0, sizeof(fev_t));
    fev->io = flist_new();
    fev->timers = flist_new();
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

    for (cur = fev->timer->start; cur; cur = cur->next)
        timer_free(cur->item);
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
}

static void run_timers(fev_t *fev)
{
    size_t i;
    uint64_t cl;
    flist_node_t *cur = 0;

    cl = fev_clock();


    for (cur = fev->timers->start; cur; cur = cur->next) {

        struct fev_timer *timer = cur->item;

        if (cl-timer->last_call > timer->interval)

    }


}

void fev_run(fev_t *fev)
{
    run_timers(fev);
    //fev_io_poll(fev);
}
