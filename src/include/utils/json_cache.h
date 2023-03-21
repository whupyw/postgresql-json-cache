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
    char *field_name; //
    text *value;
    UT_hash_handle hh; /* makes this structure hashable */
};

struct json_cache {
    text *json;  // 唯一标识元组中的一个json数据
    struct json_data *data;
    UT_hash_handle hh; /* makes this structure hashable */
};

void add_json_data(text *json, char *fnamestr, text *result);

struct json_data *find_json_data(text *json, char *fnamestr);

void delete_json_data(text *json, char *fnamestr);

void delete_data_in_json(text *json);

void destroy_cache();

#endif
