#ifndef __SIGNAL_LUT_H_
#define __SIGNAL_LUT_H_


#include "signal_list.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>


/*

Reading end:
 - read and cache the producer revision
 - if the cached producer and consumer revision are the same, stop there
 - otherwise, scan through the signaled array:
   - skip signals where producer revision == consumer revision
   - add the remaining signals to the result pool
   - copy the signal producer revision into the signal consumer revision
 - copy the cached producer revision into the consumer revision

Writing end:
 - acquire the producer lock (if MT support enabled)
 - increment the signal revision
 - set the producer signal revision for the received signal
 - release the producer lock (if MT support enabled)

*/


/* we need a big integer type that can be written in one go
   unfortunately, sig_atomic_t is not the best we can get. */
typedef uintptr_t usize_atomic_t;


struct signal_lut {
    /* when a signal is received, the handler sets this flag. other signal
       handlers have to spin, waiting for the lock to be released */
    atomic_flag producer_lock;

    /* increased by one each time a signal is added to the lookup table */
    volatile usize_atomic_t lut_producer_revision;

    /* the revision of last processed signal */
    volatile usize_atomic_t lut_consumer_revision;

    /* each cell stores the revision of the most recently received signal */
    volatile usize_atomic_t producer_signal_revision[MAX_SIGNAL_NUMBER];

    /* each cell stores the revision of the last processed signal */
    volatile usize_atomic_t consumer_signal_revision[MAX_SIGNAL_NUMBER];
};

#endif // __SIGNAL_LUT_H_
