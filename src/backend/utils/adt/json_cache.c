/*-------------------------------------------------------------------------
 *
 * json_cache.c
 *	  Implementations of functions for caching json data.
 *
 * Copyright (c) 2023, Yuhan Yuan, Wuhan University
 * All rights reserved.
 *
 * src/backend/utils/adt/json_cache.c
 *
 *-------------------------------------------------------------------------
 */

#include "utils/json_cache.h"

struct json_cache *jsonCache = NULL;

// 根据 json
void add_json_data(char *primaryKey, char *pathName, text *value) {
    struct json_cache *cache; // 整个json

    struct json_data *datas = NULL; // 初始化map, 相对于cache的value

    struct json_data *data = (struct json_data*)malloc(sizeof *data); // json内的单个数据
    // todo:yyh 缓存淘汰策略
    data->path_name = (char*)malloc(strlen(pathName) + 1);
    strcpy(data->path_name, pathName);

    data->value = (text*) malloc(sizeof(text) + VARSIZE(value) - VARHDRSZ);
    memcpy(data->value, value, sizeof(text) + VARSIZE(value) - VARHDRSZ);

    // 查找map中是否已经存在此key
    HASH_FIND_STR(jsonCache, primaryKey, cache);
    // 存在则在cache的内部map更新
    if (cache != NULL) {
        /* 我们实际使用时会先传入json_addr, field_name查询是否
         * 存在value, 若不存在value再调用add_json_data, 因此
         * 调用add_json_data时必然找不到field_name对应的value,
         * 插入value前不必查询
         */
//        HASH_ADD_KEYPTR(hh, cache->datas, data->path_name, strlen(data->path_name), data);
        HASH_ADD_STR(cache->datas, path_name, data);
        return;
    }
//    HASH_ADD_KEYPTR(hh, datas, data->path_name, strlen(data->path_name), data);
    HASH_ADD_STR(datas, path_name, data);
    cache = (struct json_cache*)malloc(sizeof *cache);

    cache->primary_key = (char*)malloc(strlen(primaryKey) + 1);
    strcpy(cache->primary_key, primaryKey);// 赋值: json的起始地址
    cache->datas = datas;
    HASH_ADD_STR(jsonCache, primary_key, cache);
}

text *find_json_data(char *primaryKey, char *pathName) {
    struct json_cache *cache;
    struct json_data *data;
    HASH_FIND_STR(jsonCache, primaryKey, cache);
    if (cache == NULL) {
        return NULL;
    }
    HASH_FIND_STR(cache->datas, pathName, data);
    if (data == NULL) {
        return NULL;
    }
    return data->value;
}

void delete_datas_for_json(text *json) {

}
