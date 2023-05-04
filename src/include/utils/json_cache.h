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

// @ Deprecated -> Check the newest codes in 'arc_json.h'

#ifndef JSON_CACHE_H
#define JSON_CACHE_H

#include "uthash.h"
#include "../postgres.h"
#include <time.h>

#define EXPIRE_TIME_JSON 43200

struct json_data {
    char *path_name; // json 中的键
    text *value; // 存储 text 地址
    long updateTime; // 更新时间， presentTime - updateTime >= expireTime, 删除数据
    UT_hash_handle hh; /* makes this structure hashable */
};

struct json_cache {
    char *primary_key;  // 唯一标识元组中的一个json数据
    struct json_data *datas; // json_data 也是一个 map
    UT_hash_handle hh; /* makes this structure hashable */
};

void add_json_data(char *primaryKey, char *pathName, text *value);

text *find_json_data(char *primaryKey, char *pathName);

// 根据关系表oid删除缓存
void delete_json_by_relid(Oid relid);

void delete_json_by_primary_key(char *primaryKey);

void delete_json_by_pk_and_attnum(char *primaryKey, int attnum);

void delete_json_by_pk_and_path(char *primaryKey, char *pathName);

void destroy_json_cache();

void clean_expired_cache();

#endif
