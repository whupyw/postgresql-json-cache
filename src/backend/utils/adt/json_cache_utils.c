#include "utils/json_cache_utils.h"

char *path; // 存储path(12->address->postcode)

/*
 * transform_primary_keys
 * 输出唯一标识一个JSON类型属性的key
 * @param nkeys 主键数量
 * @param keysIdx 主键的列号数组
 * @param slot 正在扫描的元组
 * @return 标识一个JSON类型属性的key, tableOid_pkAttNum1:pkVal1&pkAttNum2:pkVal2...pkAttNumK:pkValK
 */
extern char *transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot) {

    char *result = NULL;
    uint relidUint = (unsigned int) relid;

    if (pkinfo == NULL || pkinfo->nkeys == 0)
        return NULL;

    for (int i = 0; i < pkinfo->nkeys; i++) {
        Datum  value; // 主键的属性value
        bool   isnull;
        int  key_index;
        int32  key_value;
        char *new_result;

        key_index = (int) pkinfo->keyAttno[i];

        /* Get the attribute value from the tuple slot */
        value = slot_getattr(slot, key_index, &isnull);

        if (isnull)
            continue;

        key_value = DatumGetInt32(value); // todo:yyh 临时方案, 需要switch语句判断类型

        if (result == NULL) {
            result = psprintf("%u_%d:%d", relidUint, key_index, key_value);
        } else {
            new_result = psprintf("%s&%d:%d", result, key_index, key_value);
            pfree(result);
            result = new_result;
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
        pkinfo->keyAttno = attnums;
        /* No need to search further */
        break;
    }
    systable_endscan(scan);

    table_close(pg_constraint, AccessShareLock);

    return pkinfo;
}

// 121_3:12&4:12_3->address->postcode
extern char * get_composite_key(Oid relid, TupleTableSlot *slot, char *name, int attNum, enum KeyType keyType) {
//    StringInfo compositeKey = NULL;
    char *primaryKey = NULL; // The key for the map of json cache
    char *compositeKey = NULL;
    PrimaryKeyInfo *keyAttnos = NULL; // 主键的列号数组

    if (keyType == Relid) {
        compositeKey = psprintf("%u", (unsigned int) relid);
        return compositeKey;
    }

    keyAttnos = get_primary_keys_att_no(relid);

    if (keyAttnos == NULL)
        return NULL;

    primaryKey = transform_primary_keys(relid, keyAttnos, slot);

    free(keyAttnos);

    if (primaryKey == NULL || keyType == Relid_Tuple)
        return primaryKey;

    if (keyType == Relid_Tuple_Attnum) {
        compositeKey = psprintf("%s_%d", primaryKey, attNum);
        return compositeKey;
    }

    if (path == NULL) {
        appendStringInfo(result, "%d->%s", attNum, name);
    } else {
        char *newStr = psprintf("%s->%s", path, name);
        pfree(path);
        path = newStr;
    }

    compositeKey = psprintf("%s_%s", primaryKey, path);
    pfree(primaryKey);

    return compositeKey;
}

extern void free_path(void) {
    if (path != NULL) {
        pfree(path);
        path = NULL;
    }
}