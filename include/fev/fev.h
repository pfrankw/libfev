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



typedef struct fev_io fev_io_t;
typedef struct fev_timer fev_timer_t;
typedef struct fev fev_t;

typedef void (*fev_io_cb)(fev_t *fev, fev_io_t *io, uint8_t rev, void *arg);
typedef void (*fev_timer_cb)(void *arg);

struct fev_io {
    int fd; // File descriptor
    uint8_t ev; // Events to be checked for
    uint8_t rev; // Events set by select()/poll() wrapper
    fev_io_cb cb; // Callback if event
    void *arg;
    struct fev_io *next;
};

struct fev_timer {
    uint64_t last_call;
    uint32_t interval; // Interval in milliseconds to call the function. Max interval around 49 days.
    uint16_t flags;
    fev_timer_cb cb;
    void *arg;
    struct fev_timer *next;
};

struct fev {

    int min_interval;
    uint64_t cl;
    fev_io_t *io;
    fev_timer_t *timers;

};

int fev_init(fev_t *fev);
void fev_free(fev_t *fev);

struct fev_io* fev_add_io(fev_t *fev, int fd, uint8_t ev, fev_io_cb cb, void *arg);
void fev_del_io(fev_t *fev, struct fev_io *io);

struct fev_timer* fev_add_timer(fev_t *fev, uint32_t interval, uint16_t flags, fev_timer_cb cb, void *arg);
void fev_del_timer(fev_t *fev, struct fev_timer *timer);

int fev_run(fev_t *fev);

#endif
