# C Notes 01: Data Types and `sizeof`

## `struct`, `union`, `enum`

- Each `struct` member has its own storage. The size depends on alignment and padding.
- All `union` members share the same storage. Only the most recently written member holds a meaningful value; the union does not record which member that is.
- An `enum` gives names to integer constants. To fix the underlying type, use the C23 syntax (`enum E : unsigned char { ... }`); in older standards, constrain it in the interface yourself.

```c
struct PacketHeader {
    unsigned short length;
    unsigned char type;
};

enum PacketType {
    PACKET_DATA = 1,
    PACKET_CONTROL = 2
};

union Value {
    int integer;
    float real;
};
```

### `enum` values and `typedef`

An enumerator without `=` gets the previous value plus 1, so explicit values can create gaps:

```c
typedef enum {
    SUNDAY = 0, MONDAY, TUESDAY, WEDNESDAY,   /* 0, 1, 2, 3 */
    THURSDAY = 14, FRIDAY, SATURDAY           /* 14, 15, 16 */
} DayOfWeek;

typedef struct {
    char name[50];
    int age;
    char gender;
} Person;                                     /* use as: Person p; */
```

`typedef` only creates an alias; it does not create a new type. Enumeration constants have type `int` in C (before C23), and an enum variable accepts any integer value without a diagnostic.

Run it: `make run EX=c_types`

### Reading a different `union` member

Writing one member and reading another reinterprets the same bytes. C allows this type punning (C++ does not: there it is UB), but the result depends on the representation, for example endianness:

```c
union Bytes { uint32_t word; unsigned char byte[4]; } b;
b.word = 0;
b.byte[1] = 1;
/* little-endian: b.word == 256 (0x00000100)
 * big-endian:    b.word == 65536 (0x00010000) */
```

To know which member is active, store a tag next to the union (a *tagged union*):

```c
struct Value {
    enum { VALUE_INT, VALUE_FLOAT } kind;
    union { int integer; float real; } as;
};
```

Run it: `make run EX=c_union`

## `sizeof`, padding, and arrays

`sizeof` gives the storage size of a type or an expression, in units of `char`. It is not the string length and not the capacity of the buffer a pointer points to.

```c
int values[4];
size_t array_bytes = sizeof values;
size_t element_count = sizeof values / sizeof values[0];

struct PacketHeader header;
size_t header_bytes = sizeof header;    /* can be larger than the sum of members */
size_t pointer_bytes = sizeof &header;  /* size of a pointer */
```

A `struct` can have padding between members and at the end for alignment, so do not compute `sizeof(struct PacketHeader)` by adding member sizes. For example, the `Person` above has 55 bytes of members but `sizeof(Person)` is 60 on x86-64 (`int` needs 4-byte alignment). A function parameter `int values[]` is adjusted to `int *`, so `sizeof values` inside the function is the pointer size; pass the array length separately.

Strings also need the array/pointer distinction:

```c
char name[] = "John";              /* array of size 5, includes '\0' */
const char *label = "John";        /* label is a pointer; sizeof label is the pointer size */
size_t name_length = strlen(name); /* 4: does not include '\0' */
```
