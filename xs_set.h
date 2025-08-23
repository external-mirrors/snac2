/* copyright (c) 2022 - 2025 grunfink et al. / MIT license */

#ifndef _XS_SET_H

#define _XS_SET_H

typedef struct _xs_set {
    int elems;              /* number of hash entries */
    int used;               /* number of used hash entries */
    int *hash;              /* hashed offsets */
    xs_list *list;          /* list of stored data */
} xs_set;

void xs_set_init(xs_set *s);
xs_list *xs_set_result(xs_set *s);
void xs_set_free(xs_set *s);
int xs_set_in(const xs_set *s, const xs_val *data);
int xs_set_add(xs_set *s, const xs_val *data);


#ifdef XS_IMPLEMENTATION


void xs_set_init(xs_set *s)
/* initializes a set */
{
    /* arbitrary default */
    s->elems = 256;
    s->used  = 0;
    s->hash  = xs_realloc(NULL, s->elems * sizeof(int));
    s->list  = xs_list_new();

    memset(s->hash, '\0', s->elems * sizeof(int));
}


xs_list *xs_set_result(xs_set *s)
/* returns the set as a list and frees it */
{
    xs_list *list = s->list;
    s->list = NULL;
    s->hash = xs_free(s->hash);

    return list;
}


void xs_set_free(xs_set *s)
/* frees a set, dropping the list */
{
    xs_free(xs_set_result(s));
}


static int _store_hash(xs_set *s, const char *data, int value)
{
    unsigned int hash, i;
    int sz = xs_size(data);

    hash = xs_hash_func(data, sz);

    while (s->hash[(i = hash % s->elems)]) {
        /* get the pointer to the stored data */
        const char *p = &s->list[s->hash[i]];

        /* already here? */
        if (memcmp(p, data, sz) == 0)
            return 0;

        /* try next value */
        hash++;
    }

    /* store the new value */
    s->hash[i] = value;

    s->used++;

    return 1;
}


int xs_set_in(const xs_set *s, const xs_val *data)
/* returns 1 if the data is already in the set */
{
    unsigned int hash, i;
    int sz = xs_size(data);

    hash = xs_hash_func(data, sz);

    while (s->hash[(i = hash % s->elems)]) {
        /* get the pointer to the stored data */
        const char *p = &s->list[s->hash[i]];

        /* already here? */
        if (memcmp(p, data, sz) == 0)
            return 1;

        /* try next value */
        hash++;
    }

    return 0;
}


int xs_set_add(xs_set *s, const xs_val *data)
/* adds the data to the set */
/* returns: 1 if added, 0 if already there */
{
    /* is it 'full'? */
    if (s->used >= s->elems / 2) {
        const xs_val *v;

        /* expand! */
        s->elems *= 2;
        s->used  = 0;
        s->hash  = xs_realloc(s->hash, s->elems * sizeof(int));

        memset(s->hash, '\0', s->elems * sizeof(int));

        /* add the list elements back */
        xs_list_foreach(s->list, v)
            _store_hash(s, v, v - s->list);
    }

    int ret = _store_hash(s, data, xs_size(s->list));

    /* if it's new, add the data */
    if (ret)
        s->list = xs_list_append(s->list, data);

    return ret;
}

#endif /* XS_IMPLEMENTATION */

#endif /* XS_SET_H */
