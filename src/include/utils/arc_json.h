
#ifndef ARC_JSON_H
#define ARC_JSON_H

#include "json_cache.h"
#include "uthash.h"

struct JSON_T1 {
    int capacity;
    int length;
    struct JSON_CACHE_PTR *head;
    struct JSON_CACHE_PTR *tail;
};

struct JSON_B1 {
    int capacity;
    int length;
    struct JSON_PHANTOM_PTR *head;
    struct JSON_PHANTOM_PTR *tail;
};

struct JSON_T2 {
    int capacity;
    int length;
    struct JSON_CACHE_PTR *head;
    struct JSON_CACHE_PTR *tail;
};

struct JSON_B2 {
    int capacity;
    int length;
    struct JSON_PHANTOM_PTR *head;
    struct JSON_PHANTOM_PTR *tail;
};

struct JSON_CACHE_PTR {
    char *composite_key;
    struct json_data *data_ptr;
    struct JSON_CACHE_PTR *next;
    struct JSON_CACHE_PTR *prev;
    UT_hash_handle hh;
};

struct JSON_PHANTOM_PTR {
    char *composite_key;
    struct JSON_PHANTOM_PTR *next;
    struct JSON_PHANTOM_PTR *prev;
    UT_hash_handle hh;
};


#endif
