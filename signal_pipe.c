#define _GNU_SOURCE
#include "signal_pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


static struct signal_pipe state = SIGNAL_PIPE_INIT;


static inline void sig_warn(const char *msg)
{
    write(STDERR_FILENO, msg, strlen(msg));
}


int signal_pipe_write(int signum)
{
    int errno_save = errno;
    int rc = 1;

    signum_t signum_c = signum;
    int wrote = write(state.producer_pipe, &signum_c, sizeof(signum_c));

    switch (wrote) {
    case sizeof(signum_c):
        /* it wurked, let's go!! */
        rc = 0;
        break;
    case -1:
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
        sig_warn("signal_lut_handler: unexpected write error\n");
        break;
    default:
        sig_warn("signal_lut_handler: write returned an unexpected return value\n");
        break;
    }

    /* restore errno */
    errno = errno_save;
    return rc;
}


void signal_pipe_handler(int signum)
{
    signal_pipe_write(signum);
}


int signal_pipe_read(struct signal_list *events)
{
    static signum_t sigbuf[SIGNAL_READ_SIZE];

    /* read events from the pipe */
    ssize_t read_count;
    do {
        read_count = read(state.consumer_pipe, sigbuf, sizeof(sigbuf));
        if (read_count == -1) {
            /* if the error signals the pipe is empty, that's fine */
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            /* otherwise, forward the error */
            return -1;
        }

        /* register the signals */
        for (size_t i = 0; i < read_count / sizeof(signum_t); i++)
            signal_list_add(events, sigbuf[i]);
    } while (read_count == sizeof(sigbuf));
    return 0;
}


int signal_pipe_init(void)
{
    int rc;

    struct signal_pipe *pipe = &state;

    int fds[2];
    if ((rc = pipe2(fds, O_CLOEXEC | O_NONBLOCK)))
        return rc;

    pipe->consumer_pipe = fds[0];
    pipe->producer_pipe = fds[1];
    return pipe->consumer_pipe;
}


int signal_pipe_fd(void)
{
    return state.consumer_pipe;
}
