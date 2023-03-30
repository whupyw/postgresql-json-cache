#include "../postgres.h"
#include "../catalog/pg_constraint.h"
#include "../access/relation.h"
#include "../access/tableam.h"
#include "../../backend/utils/fmgrprotos.h"
#include "../access/skey.h"
#include "../access/genam.h"
#include "../../backend/utils/fmgroids.h"
#include "../access/table.h"

typedef struct KeyInfo {
    int nkeys;
    int16* keyAttno;
} PrimaryKeyInfo;

extern PrimaryKeyInfo *get_primary_keys_att_no(Oid relid);

extern int *get_primary_keys_index_old(Oid relid, int *nkeys);

extern char *transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot);

extern Datum
json_object_field_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int curAttNum, char **path);
