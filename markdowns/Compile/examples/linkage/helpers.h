// helpers.h - static vs inline functions, each with a local static counter.
static int next_static() { static int n = 0; return ++n; } // one copy per TU
inline int next_inline() { static int n = 0; return ++n; } // one copy per program
