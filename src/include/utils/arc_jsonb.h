#ifndef ARC_JSONB_H
#define ARC_JSONB_H

#include "uthash.h"
#include "../postgres.h"
#include "jsonb.h"

enum JsonbListType {
    T1Jsonb,
    B1Jsonb,
    T2Jsonb,
    B2Jsonb,
};

enum JsonbHitCase {
    HitTJsonb,
    HitBJsonb,
    HitNoneJsonb,
};

struct JSONB_ARC_LIST {
    double length;
    struct JSONB_CACHE_PTR *head;
    struct JSONB_CACHE_PTR *tail;
};

struct JSONB_CACHE_PTR {
    char *composite_key;
    JsonbValue *jsonb_value;
    enum JsonbListType list_type;
    struct JSONB_CACHE_PTR *next;
    struct JSONB_CACHE_PTR *prev;
    UT_hash_handle hh;
};

enum JsonbHitCase get_jsonb_data(char *compositeKey, JsonbValue **result);

void insert_jsonb_data(char *compositeKey, JsonbValue *result, enum JsonbHitCase hitCase);

void jsonb_init_arc_lists(void);

void jsonb_init_specific_list(struct JSONB_ARC_LIST **list);

void jsonb_move_to_mru(struct JSONB_CACHE_PTR *node, enum JsonbListType desType);

struct JSONB_CACHE_PTR *jsonb_remove_from_list(struct JSONB_CACHE_PTR *node);

void jsonb_add_to_list_head(struct JSONB_CACHE_PTR *node, enum JsonbListType desType);

struct JSONB_ARC_LIST *jsonb_fetch_list_by_type(enum JsonbListType listType);

void jsonb_replace(bool in_b2);

struct JSONB_CACHE_PTR *jsonb_delete_from_list_and_free(struct JSONB_CACHE_PTR *node);

void jsonb_free_node(struct JSONB_CACHE_PTR *node);

extern void delete_jsonb(void);

#endif
