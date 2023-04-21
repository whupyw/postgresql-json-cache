#include <time.h>
#include <bits/time.h>
#include "utils/json_cache_utils.h"

StringInfo path = NULL; // 存储path(12->address->postcode)
StringInfo primaryKey = NULL; // 存储 121_3:12&4:12
StringInfo pathPrev = NULL;

struct ARRAY_PARSE_INFO *saveInfo = NULL;

struct KeyInfo *keyInfoHeader = NULL;

struct ARRAY_PARSE_LIST *arrayParseMapHead = NULL;

void saveParseInfo(struct ARRAY_PARSE_INFO *info) {
    saveInfo = info;
}

void insert_parse_info(char *parseKey, int pathIndex, struct ARRAY_PARSE_INFO *insertPosition) {
    struct ARRAY_PARSE_LIST *mapPtr;
    if (saveInfo == NULL)
        return;
    saveInfo->pathIndex = pathIndex;
    HASH_FIND_STR(arrayParseMapHead, parseKey, mapPtr);
    if (mapPtr == NULL) {
        struct ARRAY_PARSE_LIST *parseMap = (struct ARRAY_PARSE_LIST*) malloc(sizeof (struct ARRAY_PARSE_LIST));
        parseMap->parse_key = (char*) malloc(strlen(parseKey)+1);
        strcpy(parseMap->parse_key, parseKey);
        parseMap->head = parseMap->tail = saveInfo;
        saveInfo->prev = saveInfo->next = NULL;
        HASH_ADD_STR(arrayParseMapHead, parse_key, parseMap);
    } else if (insertPosition == NULL) {
        // 插入队首
        saveInfo->next = mapPtr->head;
        saveInfo->prev = NULL;
        if (mapPtr->head != NULL) {
            mapPtr->head->prev = saveInfo;
            mapPtr->head = saveInfo;
        } else {
            mapPtr->head = mapPtr->tail = saveInfo;
        }
    } else {
        if (insertPosition->next != NULL) {
            saveInfo->next = insertPosition->next;
            saveInfo->prev = insertPosition;
            insertPosition->next = saveInfo;
            saveInfo->next->prev = saveInfo;
        } else {
            saveInfo->next = NULL;
            saveInfo->prev = insertPosition;
            insertPosition->next = saveInfo;
            mapPtr->tail = saveInfo;
        }
    }
    saveInfo = NULL;
}

struct ARRAY_PARSE_INFO *search_the_parse_info(char *parse_key, int targetIndex) {
    struct ARRAY_PARSE_LIST *ptr;
    struct ARRAY_PARSE_INFO *node, *result;

    HASH_FIND_STR(arrayParseMapHead, parse_key, ptr);
    if (ptr == NULL || ptr->head == NULL || ptr->tail == NULL)
        return NULL;
    if (ptr->head->pathIndex > targetIndex)
        return NULL;
    if (ptr->tail->pathIndex <= targetIndex)
        return ptr->tail;
    // 从前到后抑或是从后到前
    if (targetIndex - ptr->head->pathIndex <= ptr->tail->pathIndex - targetIndex) {
        node = ptr->head;
        while (node->pathIndex <= targetIndex) {
            result = node;
            node = node->next;
        }
    } else {
        node = ptr->tail;
        while (node->pathIndex > targetIndex) {
            node = node->prev;
            result = node;
        }
    }
    return result;
}

/*
 * transform_primary_keys
 * 输出唯一标识一个JSON类型属性的key
 * @param nkeys 主键数量
 * @param keysIdx 主键的列号数组
 * @param slot 正在扫描的元组
 * @return 标识一个JSON类型属性的key, tableOid_pkAttNum1:pkVal1&pkAttNum2:pkVal2...pkAttNumK:pkValK
 */
extern StringInfo transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot) {

    StringInfo result = NULL;
    uint relidUint = (unsigned int) relid;

    if (pkinfo == NULL || pkinfo->nkeys == 0)
        return NULL;

    for (int i = 0; i < pkinfo->nkeys; i++) {
        Datum  value; // 主键的属性value
        bool   isnull;
        int  key_index;
        int32  key_value;

        key_index = (int) pkinfo->keyAttno[i];

        /* Get the attribute value from the tuple slot */
        value = slot_getattr(slot, key_index, &isnull);

        if (isnull)
            continue;

        key_value = DatumGetInt32(value); // todo:yyh 临时方案, 需要switch语句判断类型

        if (result == NULL) {
            result = makeStringInfo();
            appendStringInfo(result, "%u_%d:%d", relidUint, key_index, key_value);
        } else {
            appendStringInfo(result, "&%d:%d", key_index, key_value);
        }
    }

    return result;
}

/*
 * get_primary_keys_index_old
 * 获取关系表中的主键列信息, 从给定表的元数据中获取主键的列号数组。
 * @param relid 表的对象 ID。
 * @param nkeys 指向主键数量的指针，该函数会更新其值。
 * @return 如果存在主键，则返回主键的列号数组；如果不存在，则返回 NULL。
 */
extern PrimaryKeyInfo *get_primary_keys_att_no(Oid relid) {
    PrimaryKeyInfo *pkinfo = NULL;
    Relation	pg_constraint;
    HeapTuple	tuple;
    SysScanDesc scan;
    ScanKeyData skey[1];

    /* Scan pg_constraint for constraints of the target rel */
    pg_constraint = table_open(ConstraintRelationId, AccessShareLock);

    ScanKeyInit(&skey[0],
                Anum_pg_constraint_conrelid,
                BTEqualStrategyNumber, F_OIDEQ,
                ObjectIdGetDatum(relid));

    scan = systable_beginscan(pg_constraint, ConstraintRelidTypidNameIndexId, true,
                              NULL, 1, skey);

    while (HeapTupleIsValid(tuple = systable_getnext(scan)))
    {
        Form_pg_constraint con = (Form_pg_constraint) GETSTRUCT(tuple);
        Datum		adatum;
        bool		isNull;
        ArrayType  *arr;
        int16	   *attnums;
        int			numkeys;

        /* Skip constraints that are not PRIMARY KEYs */
        if (con->contype != CONSTRAINT_PRIMARY)
            continue;


        /* Extract the conkey array, ie, attnums of PK's columns */
        adatum = heap_getattr(tuple, Anum_pg_constraint_conkey,
                              RelationGetDescr(pg_constraint), &isNull);
        if (isNull)
            return NULL;

        arr = DatumGetArrayTypeP(adatum);	/* ensure not toasted */
        numkeys = ARR_DIMS(arr)[0];
        if (ARR_NDIM(arr) != 1 ||
            numkeys < 0 ||
            ARR_HASNULL(arr) ||
            ARR_ELEMTYPE(arr) != INT2OID)
            return NULL; // conkey is not a 1-D smallint array
        attnums = (int16 *) ARR_DATA_PTR(arr);

        pkinfo= (PrimaryKeyInfo*)malloc(sizeof *pkinfo); // json内的单个数据

        pkinfo->nkeys = numkeys;
        pkinfo->relid = (int)relid;

        pkinfo->keyAttno = malloc(numkeys * sizeof(int16_t));
        for (int i = 0; i < numkeys; i++) {
            pkinfo->keyAttno[i] = attnums[i];
        }
        /* No need to search further */
        break;
    }
    systable_endscan(scan);

    table_close(pg_constraint, AccessShareLock);

    return pkinfo;
}

// 121_3:12&4:12_3->address->postcode
extern StringInfo get_composite_key(Oid relid, TupleTableSlot *slot, char *name, int attNum, enum KeyType keyType) {

    StringInfo compositeKey = NULL;
    PrimaryKeyInfo *keyAttnos = NULL; // 主键的列号数组

    int relidInt = (int) relid;

    if (keyType == FastFetch && primaryKey != NULL) {
        compositeKey = makeStringInfo();
        appendStringInfo(compositeKey, "%s_%d->%s", primaryKey->data, attNum, name);
        return compositeKey;
    }

    if (keyType == Relid) {
        compositeKey = makeStringInfo();
        appendStringInfo(compositeKey, "%u", (unsigned int) relid);
        return compositeKey;
    }

    if (primaryKey != NULL)
        goto LABEL;

    HASH_FIND_INT(keyInfoHeader, &relidInt, keyAttnos);
    if (keyAttnos == NULL) {
        keyAttnos = get_primary_keys_att_no(relid);
        if (keyAttnos == NULL) {
            keyAttnos= (PrimaryKeyInfo*)malloc(sizeof *keyAttnos);
            keyAttnos->relid = relidInt;
            keyAttnos->keyAttno = NULL;
            keyAttnos->nkeys = 0;
        }
        HASH_ADD_INT(keyInfoHeader, relid, keyAttnos);
    }

    if (keyAttnos == NULL)
        return NULL;

    primaryKey = transform_primary_keys(relid, keyAttnos, slot);

//    free(keyAttnos);

LABEL:

    if (keyType == FastFetch) {
        compositeKey = makeStringInfo();
        appendStringInfo(compositeKey, "%s_%d->%s", primaryKey->data, attNum, name);
        return compositeKey;
    }

    if (primaryKey == NULL || keyType == Relid_Tuple)
        return primaryKey;

    if (keyType == Relid_Tuple_Attnum) {
        appendStringInfo(primaryKey, "_%d", attNum);
        return primaryKey;
    }

    if (path == NULL) {
        path = makeStringInfo();
        pathPrev = makeStringInfo();
        appendStringInfo(pathPrev, "%d", attNum);
        appendStringInfo(path, "%d->%s", attNum, name);
    } else {
        appendBinaryStringInfo(pathPrev, path->data + pathPrev->len, path->len - pathPrev->len);
        appendStringInfo(path, "->%s", name);
    }

    compositeKey = makeStringInfo();
    appendStringInfo(compositeKey, "%s_%s", primaryKey->data, path->data);

    return compositeKey;
}


extern char *getParseKey() {
    char *res;
    if (primaryKey == NULL || pathPrev == NULL)
        return NULL;
    res = psprintf("%s_%s", primaryKey->data, pathPrev->data);
    return res;
}

extern void free_path(void) {
    if (path == NULL)
        return;
    pfree(path->data);
    pfree(path);
    path = NULL;
    pfree(pathPrev->data);
    pfree(pathPrev);
    pathPrev = NULL;
}

extern void free_primary_key(void) {
    if (primaryKey == NULL)
        return;
    pfree(primaryKey->data);
    pfree(primaryKey);
    primaryKey = NULL;
}