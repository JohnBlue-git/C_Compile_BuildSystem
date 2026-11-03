/* A union shares one storage for all members. Writing one member and
 * reading another reinterprets the bytes, so the result depends on
 * endianness. A tagged union records which member is active.
 * Note: 01_data_types_and_sizeof.md */
#include <stdint.h>
#include <stdio.h>

union Bytes {
    uint32_t word;
    unsigned char byte[4];
};

/* Tagged union: the enum says which member is valid. */
struct Value {
    enum { VALUE_INT, VALUE_FLOAT } kind;
    union {
        int integer;
        float real;
    } as;
};

static void print_value(struct Value value) {
    switch (value.kind) {
    case VALUE_INT:
        printf("int %d\n", value.as.integer);
        break;
    case VALUE_FLOAT:
        printf("float %.2f\n", value.as.real);
        break;
    }
}

int main(void) {
    union Bytes bytes;
    bytes.word = 0;
    bytes.byte[1] = 1; /* set the second byte in memory */

    /* Little-endian (x86, most ARM): byte[1] is bits 8..15 -> 256.
     * Big-endian: byte[1] is bits 16..23 -> 65536. */
    printf("word = %u (%s-endian)\n", (unsigned)bytes.word,
           bytes.word == 256u ? "little" : "big");
    printf("sizeof(union Bytes) = %zu\n", sizeof bytes);

    print_value((struct Value){.kind = VALUE_INT, .as.integer = 42});
    print_value((struct Value){.kind = VALUE_FLOAT, .as.real = 1.5f});
    return 0;
}
