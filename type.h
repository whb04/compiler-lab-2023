#include <stddef.h>

enum typetag
{
    T_INT,
    T_PTR,
    T_ARR,
    T_PROC
};

char *type_name[] = {"int", "ptr", "arr", "proc"};

typedef struct type
{
    enum typetag tag;
    struct type *inner_type;
    int arr_size;
} type;

type int_type = {T_INT, NULL, 0};
type proc_type = {T_PROC, NULL, 0};

type *new_type(enum typetag tag, type *inner_type, int arr_size);
void del_type(type *t);
type *copy_type(type *t);
void print_type(type *t); // for debug
int type_size(type *t);
int type_equal(type *t1, type *t2);
int type_compatible(type *t1, type *t2); // t2 can be assigned to t1