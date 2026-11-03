/* loader.c - load two plugins at run time and look up "add" by name.
 *
 * Usage: compile_dlopen_loader [mangled-plugin.so] [extern-c-plugin.so]
 * The defaults are next to the executable. A path with a '/' makes dlopen
 * open that file directly instead of searching the library path. */
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

typedef int (*add_fn)(int, int);

/* Returns add(2, 3), or -1 if the library or the symbol is not found. */
static int call_add(const char *path, const char *symbol) {
    void *handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        printf("dlopen(%s): %s\n", path, dlerror());
        return -1;
    }

    int result = -1;
    void *sym = dlsym(handle, symbol);
    if (!sym) {
        printf("dlsym(\"%s\"): %s\n", symbol, dlerror());
    } else {
        /* ISO C has no void* -> function pointer conversion; POSIX
         * guarantees it works, and memcpy keeps -Wpedantic quiet. */
        add_fn fn;
        memcpy(&fn, &sym, sizeof fn);
        result = fn(2, 3);
        printf("dlsym(\"%s\") found it: add(2, 3) = %d\n", symbol, result);
    }
    dlclose(handle);
    return result;
}

int main(int argc, char **argv) {
    const char *mangled = argc > 1 ? argv[1] : "./libcompile_plugin_mangled.so";
    const char *extern_c = argc > 2 ? argv[2] : "./libcompile_plugin_extern_c.so";

    printf("-- %s (no extern \"C\")\n", mangled);
    int plain = call_add(mangled, "add");          /* expected: not found */
    int by_mangled = call_add(mangled, "_Z3addii"); /* works, but fragile */

    printf("-- %s (extern \"C\")\n", extern_c);
    int by_c_name = call_add(extern_c, "add");     /* expected: found */

    return (plain == -1 && by_mangled == 5 && by_c_name == 5) ? 0 : 1;
}
