/* stack_use_after_return.c - DELIBERATE BUG. Run it with the asan preset:
 *   make run PRESET=asan EX=memory_bug_stack_use_after_return
 *
 * An asynchronous callback keeps a pointer to a local variable. The
 * function returns, its stack frame is gone, and then the callback runs. */
#include <stdio.h>

typedef void (*callback_fn)(void *context);

static callback_fn pending_callback;
/* volatile: at -O2 GCC would otherwise see the bug and warn
 * (-Wdangling-pointer) at compile time. */
static void *volatile pending_context;

static void register_timer(callback_fn callback, void *context) {
    pending_callback = callback; /* stored: runs after the caller returns */
    pending_context = context;
}

struct TimerContext {
    int fired;
};

static void on_timeout(void *context) {
    struct TimerContext *timer = context;
    timer->fired++; /* BUG: the object is dead */
    printf("timer fired %d time(s)\n", timer->fired);
}

static void start_timer_bad(void) {
    struct TimerContext context = {0};
    register_timer(on_timeout, &context);
} /* context ends here, but the timer still points to it */

int main(void) {
    start_timer_bad();
    pending_callback(pending_context); /* the "event loop" runs the timer */
    return 0;
}
