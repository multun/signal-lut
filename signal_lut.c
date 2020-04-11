#include "signal_lut.h"
#include "signal_pipe.h"

#include <stdio.h>


static struct signal_lut state;


void signal_lut_handler(int signum)
{
    /* acquire the handler lock */
    while (atomic_flag_test_and_set_explicit(&state.producer_lock, memory_order_acquire))
        continue;

    /* update the signal revision */
    lsig_atomic_t sig_id = ++state.lut_producer_revision;
    state.producer_signal_revision[signum] = sig_id;

    /* release the handler lock */
    atomic_flag_clear_explicit(&state.producer_lock, memory_order_release);
}

void signal_pipe_lut_handler(int signum)
{
    if (signal_pipe_write(signum) == 0)
        return;

    signal_lut_handler(signum);
}

static int __signal_lut_read(struct signal_list *events)
{
    /* read events from the array */
    lsig_atomic_t cached_lut_producer_revision = state.lut_producer_revision;

    /* stop if no new event was received */
    if (cached_lut_producer_revision == state.lut_consumer_revision)
        return events->count;

    for (size_t i = 0; i < MAX_SIGNAL_NUMBER; i++) {
        if (state.consumer_signal_revision[i] == state.producer_signal_revision[i])
            continue;

        signal_list_add(events, i);
        state.consumer_signal_revision[i] = state.producer_signal_revision[i];
    }

    state.lut_consumer_revision = cached_lut_producer_revision;
    return events->count;
}


#ifndef SIG_ATOMIC_MISSING
int signal_lut_read(struct signal_list *events)
{
    return __signal_lut_read(events);
}
#else
/* some exotic architectures can't write anything but a byte in one go.
   for these architectures, we need to prevent signals from coming in
   while we read the producer's data.
*/
int signal_lut_read(struct signal_list *events)
{
    sigset_t oldset;
    sigset_t newset;
    sigfillset(&newset);

    if (sigprocmask(SIG_SETMASK, &newset, &oldset) == -1)
        return -1;

    int rc = __signal_lut_read(events);

    if (sigprocmask(SIG_SETMASK, &oldset, NULL) == -1)
        return -1;

    return rc;
}
#endif // SIG_ATOMIC_MISSING



int signal_lut_setup_handler(struct sigaction *oldact, int signum, int flags, void (*handler)(int))
{
    struct sigaction action = {
        .sa_handler = handler,
        .sa_flags = flags,
    };

    sigfillset(&action.sa_mask);
    return sigaction(signum, &action, oldact);
}
