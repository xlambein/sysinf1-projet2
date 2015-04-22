typedef long long ll;

typedef struct
{
    ll num;
    char *file;
}
factor_t;

typedef struct
{
    factor_t *list;
    int size;
    int cont_size;
}
factor_list_t;

factor_list_t *new_list();
void free_list(factor_list_t *list);
void push(factor_list_t *list, factor_t fact);
void remove(factor_list_t *list, *factor_t to_remove);
factor_t *begin(factor_list_t *list);
factor_t *end(factor_list_t *list);
