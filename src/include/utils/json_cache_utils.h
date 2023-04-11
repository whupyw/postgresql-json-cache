#include "../postgres.h"
#include "../catalog/pg_constraint.h"
#include "../access/relation.h"
#include "../access/tableam.h"
#include "../../backend/utils/fmgrprotos.h"
#include "../access/skey.h"
#include "../access/genam.h"
#include "../../backend/utils/fmgroids.h"
#include "../access/table.h"

char *path; // 存储path(address_postcode)

typedef struct KeyInfo {
    int nkeys;
    int16* keyAttno;
} PrimaryKeyInfo;

extern PrimaryKeyInfo *get_primary_keys_att_no(Oid relid);

extern char *transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot);

extern inline char *get_composite_key(Oid relid, TupleTableSlot *slot, char *fnamestr, int attNum);

extern Datum
json_object_field_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
jsonb_object_field_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int curAttNum);