#ifndef _FACTOR_LIST_H
#define _FACTOR_LIST_H

typedef long long ll;

typedef struct
{
    ll num;
    int occur;
    char *filename;
}
factor_t;

typedef struct
{
    factor_t *list;
    int size;
    int cont_size;
}
factor_list_t;

factor_list_t *list_new();
void list_free(factor_list_t *list);
void list_push(factor_list_t *list, factor_t factor);
void list_remove(factor_list_t *list, factor_t *to_remove);
factor_t *list_begin(factor_list_t *list);
factor_t *list_end(factor_list_t *list);

#endif
