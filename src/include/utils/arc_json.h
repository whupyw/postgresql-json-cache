
#ifndef ARC_JSON_H
#define ARC_JSON_H

#include "uthash.h"
#include "../postgres.h"

enum KeyType {
    Relid,
    Relid_Tuple,
    Relid_Tuple_Attnum,
    Relid_Tuple_Attnum_Path,
};

enum ListType {
    T1,
    B1,
    T2,
    B2,
};

enum HitCase {
    HitT,
    HitB,
    HitNone,
};

struct JSON_ARC_LIST {
    double length;
    struct JSON_CACHE_PTR *head;
    struct JSON_CACHE_PTR *tail;
};

struct JSON_CACHE_PTR {
    char *composite_key;
    text *json_value; // 存储 text 地址
    enum ListType list_type;
    struct JSON_CACHE_PTR *next;
    struct JSON_CACHE_PTR *prev;
    UT_hash_handle hh;
};

enum HitCase get_json_data(char *compositeKey, text **result);

void insert_json_data(char *compositeKey, text *result, enum HitCase hitCase);

void init_arc_lists(void);

void init_specific_list(struct JSON_ARC_LIST **list);

void move_to_mru(struct JSON_CACHE_PTR *node, enum ListType desType);

struct JSON_CACHE_PTR * remove_from_list(struct JSON_CACHE_PTR *node);

void add_to_list_head(struct JSON_CACHE_PTR *node, enum ListType desType);

struct JSON_ARC_LIST *fetch_list_by_type(enum ListType listType);

void replace(bool in_b2);

extern void delete_json(char *key, enum KeyType keyType);

struct JSON_CACHE_PTR *delete_from_list_and_free(struct JSON_CACHE_PTR *node);

extern void print_hit_rate(void);

#endif
