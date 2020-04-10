#ifndef __SIGNAL_LIST_H_
#define __SIGNAL_LIST_H_


#include <signal.h>
#include <assert.h>
#include <stddef.h>

/* the space efficient type internaly used to encode signal numbers */
typedef unsigned char signum_t;

#define MAX_SIGNAL_NUMBER 64


struct signal_list {
    size_t count;
    sigset_t sigset;
    signum_t signals[MAX_SIGNAL_NUMBER];
};


static inline void signal_list_init(struct signal_list *events)
{
    events->count = 0;
    sigemptyset(&events->sigset);
}


static inline void signal_list_add(struct signal_list *events, int signal)
{
    if (sigismember(&events->sigset, signal))
        return;

    sigaddset(&events->sigset, signal);
    assert(events->count != MAX_SIGNAL_NUMBER);
    events->signals[events->count++] = signal;
}

extern void signal_pipe_lut_handler(int signum);
extern void signal_lut_handler(int signum);
extern int signal_lut_read(struct signal_list *events);
extern int signal_lut_setup_handler(struct sigaction *oldact, int signum, int flags, void (*handler)(int));

#endif // __SIGNAL_LIST_H_
