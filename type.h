#include <stddef.h>

enum typetag
{
    T_INT,
    T_PTR,
    T_ARR,
    T_PROC
};

char *type_name[] = {"int", "ptr", "arr", "proc"};

typedef struct
{
    enum typetag tag;
    int arr_size;
    type *inner_type;
} type;

type int_type = {T_INT, 0, NULL};
type proc_type = {T_PROC, 0, NULL};

type *new_type(enum typetag tag, int arr_size, type *inner_type);
void del_type(type *t);
void print_type(type *t); // for debug
int typesize(type *t);
int type_equal(type *t1, type *t2);
int type_compatible(type *t1, type *t2); // t2 can be assigned to t1