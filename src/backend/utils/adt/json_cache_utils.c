#include "utils/json_cache_utils.h"

/*
 * get_primary_keys_index_old
 * 获取关系表中的主键列信息, 从给定表的元数据中获取主键的列号数组。
 * @param relid 表的对象 ID。
 * @param nkeys 指向主键数量的指针，该函数会更新其值。
 * @return 如果存在主键，则返回主键的列号数组；如果不存在，则返回 NULL。
 */
extern int *get_primary_keys_index_old(Oid relid, int *nkeys) {

    Relation rel;
    TupleDesc tupdesc;
    int keynum = 0; // 主键数量
    int *keysIdx = NULL; // 主键的列号数组

    /*
     * relation_open 函数打开了表，返回了一个 Relation 结构体 rel，
     * 该结构体包含了关于表的元数据信息，例如列的数量、列的类型等。
     */
    rel = relation_open(relid, AccessShareLock);
    tupdesc = RelationGetDescr(rel);

    // 遍历所有约束，找到主键约束
    for (int i = 0; i < tupdesc->natts; i++) {
        // Form_pg_attribute 里存储了约束信息
        Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
        Oid relationConstraintOid = get_relation_constraint_oid(relid, NameStr(attr->attname), true);

        if (OidIsValid(relationConstraintOid)) {
            // 如果找到主键约束，则将对应的属性编号加入到数组中
            keysIdx = realloc(keysIdx, (++keynum) * sizeof(int));
            keysIdx[keynum - 1] = attr->attnum;
        }
    }

    // 关闭关系表
    relation_close(rel, AccessShareLock);

    // 如果没有找到主键约束，则返回 NULL
    if (keynum == 0) {
        *nkeys = 0;
        return NULL;
    }
    // 如果有两个或多个主键，则返回主键数量长度的整数数组
    *nkeys = keynum;
    return keysIdx; // 记得调用完后free(keysIdx)
}

/*
 * transform_primary_keys
 * 输出唯一标识一个JSON类型属性的key
 * @param nkeys 主键数量
 * @param keysIdx 主键的列号数组
 * @param slot 正在扫描的元组
 * @return 标识一个JSON类型属性的key, tableOid_pkColumn1_pkVal1_pkColumn2_pkVal2...pkColumnK_pkValK
 */
extern char *transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot) {

    char *result = psprintf("%u", (unsigned int) relid);

    if (pkinfo->nkeys == 0)
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

        new_result = psprintf("%s_%d_%d", result, key_index, key_value);
        pfree(result);
        result = new_result;
    }

    return result;
}

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
