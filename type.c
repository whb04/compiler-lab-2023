#include "type.h"

type *new_type(enum typetag tag, type *inner_type, int arr_size){
    type *t = (type *)malloc(sizeof(type));
    t->tag = tag;
    t->arr_size = arr_size;
    t->inner_type = inner_type;
    return t;
}

void del_type(type *t){
    if(t->inner_type)
        del_type(t->inner_type);
    free(t);
}

void print_type(type *t){
    printf("%s", type_name[t->tag]);
    if(t->inner_type){
        printf("(");
        print_type(t->inner_type);
        printf(")");
    }
}

int type_size(type *t){
    if(t->tag == T_ARR)
        return t->arr_size * type_size(t->inner_type);
    return 1;
}

int type_equal(type *t1, type *t2){
    if(t1->tag != t2->tag)
        return 0;
    if(t1->tag == T_ARR)
        return t1->arr_size == t2->arr_size && type_equal(t1->inner_type, t2->inner_type);
    if(t1->tag == T_PTR)
        return type_equal(t1->inner_type, t2->inner_type);
    return 1;
}

int type_compatible(type *t1, type *t2){
    if(t1->tag == T_ARR || t1->tag == T_PROC)
        return 0;
    if(t1->tag == T_INT)
        return t2->tag == T_INT;
    if(t1->tag == T_PTR)
        return t2->inner_type && type_compatible(t1->inner_type, t2->inner_type);
    return 0; // Unreachable
}