/* callback_lifetime.c - the context of a callback must live as long as
 * the callback can run.
 *
 * Synchronous: the callback runs before the caller returns, so a pointer
 * to a local variable is safe.
 * Asynchronous: the callback runs later, so the context must have a longer
 * lifetime: heap (with a destroy function) or static storage. */
#include <stdio.h>
#include <stdlib.h>

typedef void (*callback_fn)(int value, void *context);
typedef void (*destroy_fn)(void *context);

/* --- synchronous --- */
static void add_to(int value, void *context) {
    *(int *)context += value;
}

static void visit_now(const int *values, int count, callback_fn callback, void *context) {
    for (int i = 0; i < count; ++i)
        callback(values[i], context);
}

/* --- asynchronous: a tiny task queue, like an event loop --- */
struct Task {
    callback_fn callback;
    void *context;
    destroy_fn destroy; /* who frees the context, or NULL */
};

static struct Task queue[8];
static int queued;

static void register_task(callback_fn callback, void *context, destroy_fn destroy) {
    queue[queued++] = (struct Task){callback, context, destroy};
}

static void run_pending(void) {
    for (int i = 0; i < queued; ++i) {
        queue[i].callback(i, queue[i].context);
        if (queue[i].destroy)
            queue[i].destroy(queue[i].context);
    }
    queued = 0;
}

struct TimerContext {
    const char *name;
    int fired;
};

static void on_timeout(int id, void *context) {
    struct TimerContext *timer = context;
    timer->fired++;
    printf("  task %d: timer '%s' fired\n", id, timer->name);
}

static struct TimerContext static_timer = {"static", 0}; /* lives for the whole program */

static void start_timers(void) {
    /* A local struct here would be dead when run_pending() calls the
     * callback (see bugs/stack_use_after_return.c). */
    struct TimerContext *heap_timer = malloc(sizeof *heap_timer);
    if (!heap_timer) exit(1);
    *heap_timer = (struct TimerContext){"heap", 0};
    register_task(on_timeout, heap_timer, free); /* the queue now owns it */

    register_task(on_timeout, &static_timer, NULL);
} /* returns before any callback runs */

int main(void) {
    int sum = 0; /* a local is fine: visit_now() calls back before it returns */
    const int values[] = {1, 2, 3, 4};
    visit_now(values, 4, add_to, &sum);
    printf("synchronous: sum = %d\n", sum);

    start_timers();
    puts("asynchronous: start_timers() returned, now run the queue");
    run_pending();

    return (sum == 10 && static_timer.fired == 1) ? 0 : 1;
}
