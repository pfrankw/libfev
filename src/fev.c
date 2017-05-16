#include <string.h>

#include "fev/fev.h"
#include "fev/time.h"
#include "fev/net.h"

int fev_init(fev_t *fev)
{
    memset(fev, 0, sizeof(fev_t));
    return 0;
}

void fev_free(fev_t *fev)
{
    memset(fev, 0, sizeof(fev_t));
}

struct fev_io* fev_add_io(fev_t *fev, int fd, uint8_t ev, fev_io_cb cb, void *arg)
{
    struct fev_io *io = 0;

    if (!cb)
        goto cleanup;

    io = get_free_io(fev);
    if (!io)
        goto cleanup;

    io->fd = fd;
    io->ev = ev;
    io->cb = cb;
    io->arg = arg;

cleanup:
    return io;
}

struct fev_timer* fev_add_timer(fev_t *fev, uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg)
{
    struct fev_timer *timer = 0;

    if (!cb)
        goto cleanup;

    timer = get_free_timer(fev);
    if (!timer)
        goto cleanup;

    timer->last_call = fev_clock();
    timer->interval = interval;
    timer->flags = flags;
    timer->cb = cb;
    timer->arg = arg;

cleanup:
    return timer;
}

void fev_del_timer(fev_t *fev, struct fev_timer *timer)
{
    (void)fev;
    free_timer(timer);
}

static void run_timers(fev_t *fev)
{
    size_t i;
    uint64_t cl;

    cl = fev_clock();

    for (i=0; i<FEV_TIMER_MAX; i++) {

        if (!fev_timer_is_free(&fev->timer[i]) && (cl - fev->timer[i].last_call) >= fev->timer[i].interval) {

            fev->timer[i].cb(fev->timer[i].arg);
            if (fev->timer[i].flags & FEV_TIMER_FLAG_ONCE) {
                free_timer(&fev->timer[i]);
            } else {
                fev->timer[i].last_call = cl;
            }

        }

    }

}

void fev_run(fev_t *fev)
{
    run_timers(fev);
    fev_io_poll(fev);
}
