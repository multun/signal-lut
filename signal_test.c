#include "signal_lut.h"
#include "signal_pipe.h"

#include <err.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>


int main(void)
{
    int rc;
    int fd;

    const int nap_duration = 5;

    if ((fd = signal_pipe_init()) < 0) {
        warn("failed to initialize the signal pipe");
        return 1;
    }

    const int handled_signals[] = {
        SIGINT,
        SIGUSR1,
        SIGUSR2,
        SIGWINCH,
    };

    for (size_t i = 0; i < sizeof(handled_signals) / sizeof(handled_signals[0]); i++)
        if ((rc = signal_lut_setup_handler(NULL, handled_signals[i], SA_RESTART, signal_pipe_lut_handler))) {
            warn("failed to setup the SIGUSR1 handler");
            return 1;
        }

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    struct signal_list events;
    bool running = true;
    do {
        signal_list_init(&events);

        printf("waiting for %d seconds, send some signals...\n", nap_duration);

        while (sleep(nap_duration) != 0 && errno == EINTR)
            puts("received a signal, restarting");

        puts("waiting for data to be available on the pipe...");
        select(fd + 1, &rfds, NULL, NULL, NULL);
        puts("done");

        signal_pipe_read(&events);
        signal_lut_read(&events);

        for (size_t i = 0; i < events.count; i++) {
            printf(">> %s\n", strsignal(events.signals[i]));
            if (events.signals[i] == SIGINT) {
                puts("received a SIGINT, stopping");
                running = false;
            }
        }
    } while (running);
    return 0;
}
