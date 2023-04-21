/*-------------------------------------------------------------------------
 *
 * jsonb_cache.c
 *	  Implementations of functions for caching jsonb data.
 *
 * Copyright (c) 2023, Yuhan Yuan, Wuhan University
 * All rights reserved.
 *
 * src/backend/utils/adt/jsonb_cache.c
 *
 *-------------------------------------------------------------------------
 */

#include "utils/jsonb_cache.h"

struct JSONB_INDEX_CACHE *jsonbCacheHeader = NULL;

struct JSONB_INDEX_CACHE *jsonbListHead = NULL, *jsonbListTail = NULL;

uint jsonb_lru_hit_count = 0, jsonb_lru_get_count = 0, jsonb_lru_delete_count = 0;

int JSONB_CACHE_MAX_SIZE = 1000;
int current_size = 0;

void insert_jsonb_index(char *compositeKey, int index) {
    struct JSONB_INDEX_CACHE *jsonbCache = (struct JSONB_INDEX_CACHE*) malloc((sizeof(struct JSONB_INDEX_CACHE)));
    if (current_size >= JSONB_CACHE_MAX_SIZE) {
        delete_lru();
    }
    jsonbCache->composite_key = (char *) malloc(strlen(compositeKey) + 1);
    strcpy(jsonbCache->composite_key, compositeKey);
    jsonbCache->jsonb_index = index;
    if (jsonbListHead == NULL) {
        jsonbCache->prev = jsonbCache->next = NULL;
        jsonbListHead = jsonbListTail = jsonbCache;
    } else {
        jsonbCache->next = jsonbListHead;
        jsonbListHead->prev = jsonbCache;
        jsonbCache->prev = NULL;
        jsonbListHead = jsonbCache;
    }
    HASH_ADD_STR(jsonbCacheHeader, composite_key, jsonbCache);
    current_size++;
}

int find_jsonb_index(char *compositeKey) {
    struct JSONB_INDEX_CACHE *node;
    jsonb_lru_get_count++;
    HASH_FIND_STR(jsonbCacheHeader, compositeKey, node);
    if (node == NULL)
        return INT32_MIN;
    jsonb_lru_hit_count++;
    if (jsonbListHead != jsonbListTail && jsonbListHead != node) {
        if (jsonbListTail == node) {
            jsonbListTail = node->prev;
            node->prev->next = NULL;
        } else {
            node->next->prev = node->prev;
            node->prev->next = node->next;
        }
        node->prev = NULL;
        node->next = jsonbListHead;
        jsonbListHead->prev = node;
        jsonbListHead = node;
    }
    return node->jsonb_index;
}

void delete_lru(void) {
    struct JSONB_INDEX_CACHE * node;
    if (jsonbListHead == NULL)
        return;
    jsonb_lru_delete_count++;
    current_size--;
    if (jsonbListHead == jsonbListTail) {
        HASH_DEL(jsonbCacheHeader, jsonbListHead);
        free(jsonbListHead->composite_key);
        free(jsonbListHead);
        jsonbListHead = jsonbListTail = NULL;
        return;
    }
    node = jsonbListTail;
    node->prev->next = NULL;
    jsonbListTail = node->prev;
    HASH_DEL(jsonbCacheHeader, node);
    free(node->composite_key);
    free(node);
}

extern void print_jsonb_lru_hit_rate(void) {
    ereport(LOG,
            (errmsg("get_count: %u, hit_count: %u, delete_count: %u",
                    jsonb_lru_get_count, jsonb_lru_hit_count,
                    jsonb_lru_delete_count)));
}