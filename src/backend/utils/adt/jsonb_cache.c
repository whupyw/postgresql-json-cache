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

struct jsonb_cache *jsonbCache = NULL;

void add_jsonb_data(char *primaryKey, char *pathName, JsonbValue *value) {
    struct jsonb_cache *cache;

    struct jsonb_data *datas = NULL;

    struct jsonb_data *data = (struct jsonb_data*) malloc(sizeof *data);
    // todo:yyh 缓存淘汰策略
    data->path_name = (char *) malloc(strlen(pathName) + 1);
    strcpy(data->path_name, pathName);

    data->value = (JsonbValue *) malloc(sizeof(JsonbValue) + VARSIZE(value) - VARHDRSZ);
    memcpy(data->value, value, sizeof(JsonbValue) + VARSIZE(value) - VARHDRSZ);

    HASH_FIND_STR(jsonbCache, primaryKey, cache);
    // if already init specific cache
    if (cache != NULL) {
        HASH_ADD_STR(cache->datas, path_name, data);
        return;
    }
    // else
    HASH_ADD_STR(datas, path_name, data);
    cache = (struct jsonb_cache*) malloc(sizeof *cache);

    cache->primary_key = (char *) malloc(strlen(primaryKey) + 1);
    strcpy(cache->primary_key, primaryKey);
    cache->datas = datas;
    HASH_ADD_STR(jsonbCache, primary_key, cache);
}

JsonbValue *find_jsonb_data(char *primaryKey, char *pathName) {
    struct jsonb_cache *cache;
    struct jsonb_data *data;

    HASH_FIND_STR(jsonbCache, primaryKey, cache);
    if (cache == NULL)
        return NULL;

    HASH_FIND_STR(cache->datas, pathName, data);
    if (data == NULL)
        return NULL;

    return data->value;
}

void delete_jsonb_by_primary_key(char *primaryKey) {
    struct jsonb_cache *cache;
    HASH_FIND_STR(jsonbCache, primaryKey, cache);
    if (cache == NULL)
        return;
    HASH_DEL(jsonbCache, cache); // 从Hash Table中删除
    if (cache->hh.prev == NULL) {
        jsonbCache = (struct jsonb_cache*)cache->hh.next;
    }
    free(cache);
}
