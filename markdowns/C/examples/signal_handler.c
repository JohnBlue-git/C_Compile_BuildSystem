/* Install a handler with sigaction, keep the handler async-signal-safe,
 * ignore a signal with SIG_IGN, and restore SIG_DFL.
 * Note: 05_error_handling_and_signals.md */
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* The only object type the C standard lets a handler write. */
static volatile sig_atomic_t last_signal = 0;

static void on_signal(int signum) {
    /* Only async-signal-safe calls here: write() yes, printf()/exit() no. */
    static const char message[] = "  [handler] caught a signal\n";
    ssize_t ignored = write(STDOUT_FILENO, message, sizeof message - 1);
    (void)ignored;
    last_signal = signum;
}

int main(void) {
    struct sigaction action;
    memset(&action, 0, sizeof action);
    action.sa_handler = on_signal;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGUSR1, &action, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* raise() delivers the signal to this process before it returns. */
    printf("raise(SIGUSR1)\n");
    fflush(stdout); /* the handler uses unbuffered write(); keep output in order */
    raise(SIGUSR1);
    printf("main sees last_signal = %d (SIGUSR1 = %d)\n", (int)last_signal, SIGUSR1);

    /* SIG_IGN: the signal is discarded. */
    last_signal = 0;
    signal(SIGUSR1, SIG_IGN);
    raise(SIGUSR1);
    printf("after SIG_IGN, last_signal = %d\n", (int)last_signal);

    /* SIG_DFL restores the default action (terminate, for SIGUSR1),
     * so do not raise it again here. */
    signal(SIGUSR1, SIG_DFL);
    printf("SIGUSR1 restored to SIG_DFL\n");
    return EXIT_SUCCESS;
}
