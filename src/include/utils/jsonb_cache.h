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

#ifndef JSONB_CACHE_H
#define JSONB_CACHE_H

#include "uthash.h"
#include "../postgres.h"
#include "jsonb.h"

struct jsonb_data {
    char *path_name; // jsonb 中的键
    JsonbValue *value; // 存储 JsonbValue
    UT_hash_handle hh; /* makes this structure hashable */
};

struct jsonb_cache {
    char *primary_key;  // 唯一标识元组中的一个jsonb数据
    struct jsonb_data *datas; // json_data 也是一个 map
    UT_hash_handle hh; /* makes this structure hashable */
};

void add_jsonb_data(char *primaryKey, char *pathName, JsonbValue *value);

JsonbValue *find_jsonb_data(char *primaryKey, char *pathName);

void delete_jsonb_by_primary_key(char *primaryKey);

#endif
