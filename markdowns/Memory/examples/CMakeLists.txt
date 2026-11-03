set(EXAMPLE_LABEL memory)

# Safe examples: normal tests.
add_example(memory_storage_duration SOURCES storage_duration.c)
add_example(memory_object_lifetime SOURCES object_lifetime.cpp)
add_example(memory_callback_lifetime SOURCES callback_lifetime.c)

# Deliberate bugs: build only. Run them with the sanitizer preset, for example
#   make run PRESET=asan EX=memory_bug_use_after_free
add_example(memory_bug_use_after_free SOURCES bugs/use_after_free.c NO_TEST)
add_example(memory_bug_stack_use_after_return SOURCES bugs/stack_use_after_return.c NO_TEST)
add_example(memory_bug_heap_overflow SOURCES bugs/heap_overflow.c NO_TEST)
add_example(memory_bug_leak SOURCES bugs/leak.c NO_TEST)
