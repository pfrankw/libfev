#include <limits.h>
#include <string.h>
#include <poll.h>

#include "utlist.h"

#include "fev/fev.h"
#include "fev/time.h"

static void io_free_all(fev_t *fev);
static void timer_free_all(fev_t *fev);
static int get_min_interval(fev_timer_t *timers);

int fev_init(fev_t *fev)
{
    memset(fev, 0, sizeof(fev_t));

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
    //flist_free(fev->io);
    //flist_free(fev->timers);

    memset(fev, 0, sizeof(fev_t));

}

static fev_io_t* io_new(int fd, uint8_t ev, fev_io_cb cb, void *arg)
{
    fev_io_t *io = calloc(1, sizeof(fev_io_t));

    io->fd = fd;
    io->ev = ev;
    io->cb = cb;
    io->arg = arg;

    return io;
}

static fev_timer_t* timer_new(uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg)
{
    fev_timer_t *timer = calloc(1, sizeof(fev_timer_t));

    timer->last_call = fev_clock();
    timer->interval = interval;
    timer->flags = flags;
    timer->cb = cb;
    timer->arg = arg;

    return timer;
}

static void io_free(fev_io_t *io)
{
    if (!io)
        return;

    free(io);
}

static void timer_free(fev_timer_t *timer)
{
    if (!timer)
        return;

    free(timer);
}

static void io_free_all(fev_t *fev)
{

    fev_io_t *cur, *tmp;

    LL_FOREACH_SAFE(fev->io, cur, tmp) {
        LL_DELETE(fev->io, cur);
        io_free(cur);
    }
}

static void timer_free_all(fev_t *fev)
{
    struct fev_timer *cur, *tmp;

    LL_FOREACH_SAFE(fev->timers, cur, tmp) {
        LL_DELETE(fev->timers, cur);
        timer_free(cur);
    }
}

static int get_min_interval(fev_timer_t *timers)
{
    int min = INT_MAX;
    fev_timer_t *cur = 0;

    LL_FOREACH(timers, cur) {
      if (cur->interval < min)
        min = cur->interval;
    }

    return min;
}

fev_io_t* fev_add_io(fev_t *fev, int fd, uint8_t ev, fev_io_cb cb, void *arg)
{
    fev_io_t *io = 0;

    if (!cb)
        goto cleanup;

    io = io_new(fd, ev, cb, arg);

    LL_PREPEND(fev->io, io);

cleanup:
    return io;
}

fev_timer_t* fev_add_timer(fev_t *fev, uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg)
{
    fev_timer_t *timer = 0;

    if (!cb)
        goto cleanup;

    timer = timer_new(interval, flags, cb, arg);
    LL_PREPEND(fev->timers, timer);

    fev->min_interval = get_min_interval(fev->timers);

cleanup:
    return timer;
}

void fev_del_io(fev_t *fev, fev_io_t *io)
{
    LL_DELETE(fev->io, io);
    io_free(io);
}

void fev_del_timer(fev_t *fev, fev_timer_t *timer)
{

    LL_DELETE(fev->timers, timer);

    timer_free(timer);

    fev->min_interval = get_min_interval(fev->timers);

}

static void timers_run(fev_t *fev)
{
    fev_timer_t *timer, *tmp;

    LL_FOREACH_SAFE(fev->timers, timer, tmp) {

        if ((fev->cl - timer->last_call) >= timer->interval) {

            timer->cb(timer->arg);

            if (timer->flags & FEV_TIMER_FLAG_ONCE)
                fev_del_timer(fev, timer);
            else
                timer->last_call = fev->cl;

        }
    }

}

static void set_pollfd_by_io(struct pollfd *pfd, fev_io_t *io)
{
    pfd->fd = io->fd;

    if (io->ev & FEV_EVENT_READ)
        pfd->events |= POLLIN;

    if (io->ev & FEV_EVENT_WRITE)
        pfd->events |= POLLOUT;
}

static void set_io_by_pollfd(fev_io_t *io, struct pollfd *pfd)
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
    fev_io_t *cur = 0;
    size_t i, n_io;

    LL_COUNT(fev->io, cur, n_io);

    pfd = calloc(n_io, sizeof(struct pollfd));

    i = 0;
    LL_FOREACH(fev->io, cur){
        set_pollfd_by_io(&pfd[i], cur);
        i++;
    }

    rp = poll(pfd, i, fev->min_interval);

    if (rp == 0)
        goto success_cleanup;
    else if (rp < 0)
        goto cleanup;

    //for (cur = fev->io->start, i=0; cur && rp; cur = cur->next, i++) {
    i = 0;
    LL_FOREACH(fev->io, cur) {

        if (pfd[i].revents != 0) { // If there are revents
            set_io_by_pollfd(cur, &pfd[i]);
            rp--; // rp is the counter of pollfd structures with revents set.
            // We can use rp as counter.
        }

        if (rp == 0)
            break;

    }

success_cleanup:
    r = 0;
cleanup:
    free(pfd);
    return r;
}

static void io_run(fev_t *fev)
{
    fev_io_t *io, *tmp;
    uint8_t rev;

    LL_FOREACH_SAFE(fev->io, io, tmp) {

        if (io->rev != 0) {
        	rev = io->rev;
        	io->rev = 0;
            io->cb(fev, io, rev, io->arg);
            // io might have been freed now
        }

    }

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
