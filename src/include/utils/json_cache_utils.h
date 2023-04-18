#include "../postgres.h"
#include "../catalog/pg_constraint.h"
#include "../access/relation.h"
#include "../access/tableam.h"
#include "../../backend/utils/fmgrprotos.h"
#include "../access/skey.h"
#include "../access/genam.h"
#include "../../backend/utils/fmgroids.h"
#include "../access/table.h"
#include "arc_json.h"

typedef struct KeyInfo {
    int nkeys;
    int16* keyAttno;
} PrimaryKeyInfo;

// 存储 primaryKeyInfo 的 Hash表


extern PrimaryKeyInfo *get_primary_keys_att_no(Oid relid);

extern StringInfo transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot);

extern StringInfo get_composite_key(Oid relid, TupleTableSlot *slot, char *name, int attNum, enum KeyType keyType);

extern void free_path(void);

extern Datum
json_object_field_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
jsonb_object_field_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
json_object_field_text_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
jsonb_object_field_text_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
json_array_element_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
jsonb_array_element_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
json_array_element_text_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
jsonb_array_element_text_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);