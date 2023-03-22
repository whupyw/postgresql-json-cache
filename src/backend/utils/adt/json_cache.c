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
void add_json_data(text *json, char *fieldName, text *value) {
    struct json_cache *cache; // 整个json

    struct json_data *datas = NULL; // 初始化map, 相对于cache的value

    struct json_data *data = (struct json_data*)malloc(sizeof *data); // json内的单个数据

    data->field_name = (char*)malloc(strlen(fieldName) + 1);
    strcpy(data->field_name, fieldName);
    data->value = value;

    // 查找map中是否已经存在此key
    HASH_FIND_PTR(jsonCache, &json, cache);
    // 存在则在cache的内部map更新
    if (cache != NULL) {
        /* 我们实际使用时会先传入json_addr, field_name查询是否
         * 存在value, 若不存在value再调用add_json_data, 因此
         * 调用add_json_data时必然找不到field_name对应的value,
         * 插入value前不必查询
         */
        HASH_ADD_KEYPTR(hh, cache->datas, data->field_name, strlen(data->field_name), data);
        return;
    }
    HASH_ADD_KEYPTR(hh, datas, data->field_name, strlen(data->field_name), data);

    cache = (struct json_cache*)malloc(sizeof *cache);
    cache->json_addr = json; // 赋值: json的起始地址
    cache->datas = datas;

    HASH_ADD_PTR(jsonCache, json_addr, cache);
}

text *find_json_data(text *json, char *fieldName) {
    struct json_cache *cache;
    struct json_data *data;
    HASH_FIND_PTR(jsonCache, &json, cache);
    if (cache == NULL) {
        return NULL;
    }
    HASH_FIND_STR(cache->datas, fieldName, data);
    if (data == NULL) {
        return NULL;
    }
    return data->value;
}

void delete_json_data(text *json, char *fieldName) {

}
