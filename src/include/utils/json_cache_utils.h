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

// 存储 primaryKeyInfo 的 Hash表
typedef struct KeyInfo {
    int relid; // key
    int nkeys;
    int16* keyAttno;
    UT_hash_handle hh;
} PrimaryKeyInfo;

struct ARRAY_PARSE_INFO {
    int pathIndex;
    int token_start; // ptr
    int token_terminator; // ptr
    int prev_token_terminator; // ptr
//    JsonTokenType token_type;
    int lex_level;
    int line_number;
    int line_start; // ptr
    char* strVal;
    int array_cur_index;
    int result_start; // ptr
    struct ARRAY_PARSE_INFO *prev;
    struct ARRAY_PARSE_INFO *next;
};

struct ARRAY_PARSE_LIST {
    char *parse_key;
    struct ARRAY_PARSE_LIST *next;
    struct ARRAY_PARSE_LIST *prev;

    struct ARRAY_PARSE_INFO *head;
    struct ARRAY_PARSE_INFO *tail;
    UT_hash_handle hh;
};

struct ARRAY_PARSE_INFO *search_the_parse_info(char *parse_key, int targetIndex);

void insert_parse_info(char *parseKey, int pathIndex, struct ARRAY_PARSE_INFO *insertPosition);

void delete_parse_info(char *parseKey);

void saveParseInfo(struct ARRAY_PARSE_INFO *info);

extern PrimaryKeyInfo *get_primary_keys_att_no(Oid relid);

extern StringInfo transform_primary_keys(Oid relid, PrimaryKeyInfo *pkinfo, TupleTableSlot *slot);

extern StringInfo get_composite_key(Oid relid, TupleTableSlot *slot, char *name, int attNum, enum KeyType keyType);

extern char *getParseKey(void);

extern void free_primary_key(void);

extern void free_path(void);

extern Datum
json_object_field_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
json_object_field_text_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
json_array_element_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

extern Datum
json_array_element_text_with_cache(FunctionCallInfo fcinfo, TupleTableSlot *slot, Oid relid, int attNum);

text *
get_worker_with_cache(text *json, char **tpath, int *ipath, int npath, bool normalize_results, char *parseKey);