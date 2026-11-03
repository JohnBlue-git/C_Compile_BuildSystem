/* enum values, typedef struct, and sizeof with padding.
 * Note: 01_data_types_and_sizeof.md */
#include <stdio.h>
#include <string.h>

/* An enumerator without "=" is the previous value + 1. */
typedef enum {
    SUNDAY = 0,
    MONDAY,         /* 1 */
    TUESDAY,        /* 2 */
    WEDNESDAY,      /* 3 */
    THURSDAY = 14,
    FRIDAY,         /* 15 */
    SATURDAY        /* 16 */
} DayOfWeek;

typedef struct {
    char name[50];
    int age;
    char gender;
} Person;

int main(void) {
    DayOfWeek today = SATURDAY;
    printf("SATURDAY = %d, WEDNESDAY = %d\n", today, WEDNESDAY);

    Person person;
    /* strcpy is fine here: the literal is known to fit in name[50]. */
    strcpy(person.name, "John");
    person.age = 30;
    person.gender = 'M';
    printf("Person: %s, %d, %c\n", person.name, person.age, person.gender);

    /* 50 + 4 + 1 = 55 bytes of members, but sizeof includes padding. */
    printf("sizeof(Person) = %zu (members sum to %zu)\n", sizeof(Person),
           sizeof person.name + sizeof person.age + sizeof person.gender);
    return 0;
}
