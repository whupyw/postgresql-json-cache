/*-------------------------------------------------------------------------
 *
 * json_cache.h
 *	  Declarations of structs and functions for caching json data.
 *
 * Copyright (c) 2023, Yuhan Yuan, Wuhan University
 * All rights reserved.
 *
 * src/include/utils/json_cache.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef JSON_CACHE_H
#define JSON_CACHE_H

#include "uthash.h"
#include "../postgres.h"

struct json_data {
    char *path_name; // json 中的键
    text *value; // 存储 text 地址
    UT_hash_handle hh; /* makes this structure hashable */
};

struct json_cache {
    char *primary_key;  // 唯一标识元组中的一个json数据
    struct json_data *datas; // json_data 也是一个 map
    UT_hash_handle hh; /* makes this structure hashable */
};

void add_json_data(char *primaryKey, char *pathName, text *value);

text *find_json_data(char *primaryKey, char *pathName);

void delete_json_data(text *json, char *fieldName);

void delete_datas_for_json(text *json);

void destroy_cache();

#endif
