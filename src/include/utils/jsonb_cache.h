/*-------------------------------------------------------------------------
 *
 * jsonb_cache.h
 *	  Declarations of structs and functions for caching jsonb data.
 *
 * Copyright (c) 2023, Yuhan Yuan, Wuhan University
 * All rights reserved.
 *
 * src/include/utils/jsonb_cache.h
 *
 *-------------------------------------------------------------------------
 */

// @ Deprecated -> Check the newest codes in 'arc_json.h'

#ifndef JSONB_CACHE_H
#define JSONB_CACHE_H

#include "uthash.h"
#include "../postgres.h"
#include "jsonb.h"
#include <time.h>

struct JSONB_INDEX_CACHE {
    char *composite_key;
    int jsonb_index;
    struct JSONB_INDEX_CACHE *next;
    struct JSONB_INDEX_CACHE *prev;
    UT_hash_handle hh;
};

void insert_jsonb_index(char *compositeKey, int index);

int find_jsonb_index(char *compositeKey);

void delete_lru(void);

extern void print_jsonb_lru_hit_rate(void);

#endif
