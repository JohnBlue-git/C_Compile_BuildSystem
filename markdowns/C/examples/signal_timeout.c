/* Timeout pattern: alarm() sends SIGALRM; SIGINT/SIGTERM end the wait early.
 * The handler only sets a flag. sigprocmask + sigsuspend avoid the race
 * where the signal arrives between checking the flag and going to sleep.
 * Note: 05_error_handling_and_signals.md */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static volatile sig_atomic_t stop_signal = 0;

static void on_stop(int signum) {
    stop_signal = signum;
}

int main(void) {
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_handler = on_stop;
    sigemptyset(&action.sa_mask);

    const int signals[] = {SIGALRM, SIGINT, SIGTERM};
    sigset_t blocked;
    sigemptyset(&blocked);
    for (size_t i = 0; i < sizeof signals / sizeof signals[0]; i++) {
        if (sigaction(signals[i], &action, NULL) == -1) {
            perror("sigaction");
            return EXIT_FAILURE;
        }
        sigaddset(&blocked, signals[i]);
    }

    /* Block the signals first, so none is lost before sigsuspend. */
    sigset_t previous;
    sigprocmask(SIG_BLOCK, &blocked, &previous);

    alarm(1); /* timeout: SIGALRM in 1 second */
    printf("waiting up to 1 second (Ctrl+C ends it early)...\n");

    while (stop_signal == 0) {
        /* Atomically unblock and sleep until a signal handler has run. */
        sigsuspend(&previous);
    }
    sigprocmask(SIG_SETMASK, &previous, NULL);

    printf("stopped by %s\n", stop_signal == SIGALRM ? "SIGALRM (timeout)"
                              : stop_signal == SIGINT ? "SIGINT" : "SIGTERM");
    return EXIT_SUCCESS;
}
