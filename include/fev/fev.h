#ifndef FEV_H
#define FEV_H

#include <stdint.h>

#include <flist/flist.h>

#include <fev/config.h>

// Events
enum {
    FEV_EVENT_READ  = 1 << 0, // 00000001
    FEV_EVENT_WRITE = 1 << 1, // 00000010
    FEV_EVENT_ERROR = 1 << 2  // 00000100
};

enum {
    FEV_TIMER_FLAG_ONCE = 1 << 0,
};

struct fev_io;
struct fev;

typedef void (*fev_io_cb)(struct fev *fev, struct fev_io *io, uint8_t rev, void *arg);
typedef void (*fev_timer_cb)(void *arg);

struct fev_io {
    int fd; // File descriptor
    uint8_t ev; // Events to be checked for
    uint8_t rev; // Events set by select()/poll() wrapper
    fev_io_cb cb; // Callback if event
    void *arg;
};

struct fev_timer {
    uint64_t last_call;
    uint32_t interval; // Interval in milliseconds to call the function. Max interval around 40 days.
    uint16_t flags;
    fev_timer_cb cb;
    void *arg;
};

typedef struct fev{

    uint64_t cl;
    flist_t *io;
    flist_t *timers;

} fev_t;

int fev_init(fev_t *fev);
void fev_free(fev_t *fev);

int fev_io_is_free(struct fev_io *io);
int fev_timer_is_free(struct fev_timer *timer);

struct fev_io* fev_io_get_by_fd(fev_t *fev, int fd);

struct fev_io* fev_add_io(fev_t *fev, int fd, uint8_t ev, fev_io_cb cb, void *arg);
void fev_del_io(fev_t *fev, struct fev_io *io);


struct fev_timer* fev_add_timer(fev_t *fev, uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg);
void fev_del_timer(fev_t *fev, struct fev_timer *timer);

void fev_run(fev_t *fev);

#endif
