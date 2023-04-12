#include "utils/arc_jsonb.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

struct JSONB_ARC_LIST *t1_jb = NULL;
struct JSONB_ARC_LIST *b1_jb = NULL;
struct JSONB_ARC_LIST *t2_jb = NULL;
struct JSONB_ARC_LIST *b2_jb = NULL;

struct JSONB_CACHE_PTR *jsonb_map_head = NULL;

const double t_b_total_jsonb = 10.0; // c
double t1_cap_jsonb = 0.0; // p

uint jsonb_hit_count, jsonb_get_count;

void jsonb_init_arc_lists(void) {
    jsonb_init_specific_list(&t1_jb);
    jsonb_init_specific_list(&t2_jb);
    jsonb_init_specific_list(&b1_jb);
    jsonb_init_specific_list(&b2_jb);
}

void jsonb_init_specific_list(struct JSONB_ARC_LIST **list) {
    *list = (struct JSONB_ARC_LIST*) malloc(sizeof(struct JSONB_ARC_LIST));
    (*list)->head = NULL;
    (*list)->tail = NULL;
    (*list)->length = 0;
}

enum JsonbHitCase get_jsonb_data(char *compositeKey, JsonbValue **result) {
    struct JSONB_CACHE_PTR *node;
    double extendSize;
    if (t1_jb == NULL) {
        jsonb_init_arc_lists();
    }
    jsonb_get_count++;
    HASH_FIND_STR(jsonb_map_head, compositeKey, node);
    if (node == NULL) {
        if (t1_jb->length + b1_jb->length == t_b_total_jsonb) {
            if (t1_jb->length < t_b_total_jsonb) {
                HASH_DEL(jsonb_map_head, b1_jb->tail);
                free(jsonb_remove_from_list(b1_jb->tail));
                jsonb_replace(false);
            } else {
                HASH_DEL(jsonb_map_head, t1_jb->tail);
                free(jsonb_remove_from_list(t1_jb->tail));
            }
        } else {
            double listSize = t1_jb->length + t2_jb->length + b1_jb->length + b2_jb->length;
            if (listSize >= t_b_total_jsonb) {
                if (listSize == t_b_total_jsonb * 2) {
                    HASH_DEL(jsonb_map_head, b2_jb->tail);
                    free(jsonb_remove_from_list(b2_jb->tail));
                }
                jsonb_replace(false);
            }
        }
        return HitNoneJsonb;
    }
    switch (node->list_type) {
        case T1Jsonb:
        case T2Jsonb:
            jsonb_move_to_mru(node, T2Jsonb);
            jsonb_hit_count++;
            *result = node->jsonb_value;
            return HitTJsonb;
        case B1Jsonb:
            extendSize = b1_jb->length >= b2_jb->length ? 1 : b2_jb->length / b1_jb->length;
            t1_cap_jsonb = min(t1_cap_jsonb + extendSize, t_b_total_jsonb);
            jsonb_replace(false);
            jsonb_move_to_mru(node, T2Jsonb);
            // parse 后 复制 result 到 t2_jb->head->json_value
            return HitBJsonb;
        case B2Jsonb:
            extendSize = b2_jb->length >= b1_jb->length ? 1 : b1_jb->length / b2_jb->length;
            t1_cap_jsonb = max(t1_cap_jsonb - extendSize, 0);
            jsonb_replace(true);
            jsonb_move_to_mru(node, T2Jsonb);
            // parse 后 复制 result 到 t2_jb->head->json_value
            return HitBJsonb;
    }
}

void insert_jsonb_data(char *compositeKey, JsonbValue *result, enum JsonbHitCase hitCase) {
    struct JSONB_CACHE_PTR *node;

    if (hitCase == HitBJsonb) {
        t2_jb->head->jsonb_value = (JsonbValue *) malloc(sizeof(JsonbValue) + VARSIZE(result) - VARHDRSZ);
        memcpy(t2_jb->head->jsonb_value, result, sizeof(JsonbValue) + VARSIZE(result) - VARHDRSZ);
        return;
    }
    // HitNone
    node = (struct JSONB_CACHE_PTR*) malloc(sizeof(struct JSONB_CACHE_PTR));
    // node->composite_key
    node->composite_key = (char *) malloc(strlen(compositeKey) + 1);
    strcpy(node->composite_key, compositeKey);
    // node->json_value
    node->jsonb_value = (JsonbValue *) malloc(sizeof(JsonbValue) + VARSIZE(result) - VARHDRSZ);
    memcpy(node->jsonb_value, result, sizeof(JsonbValue) + VARSIZE(result) - VARHDRSZ);
    // node->list_type
    node->list_type = T1Jsonb;
    jsonb_add_to_list_head(node, T1Jsonb);
    HASH_ADD_STR(jsonb_map_head, composite_key, node);
}


void jsonb_move_to_mru(struct JSONB_CACHE_PTR *node, enum JsonbListType desType) {
    // 从原来的list移除
    jsonb_remove_from_list(node);
    // 添加到新的list头部
    jsonb_add_to_list_head(node, desType);
    node->list_type = desType;
}

struct JSONB_CACHE_PTR * jsonb_remove_from_list(struct JSONB_CACHE_PTR *node) {
    struct JSONB_ARC_LIST *list = jsonb_fetch_list_by_type(node->list_type);
    if (list->head == node && list->tail == node) {
        list->head = NULL;
        list->tail = NULL;
    } else if (list->head == node) {
        list->head = list->head->next;
        node->next->prev = NULL;
    } else if (list->tail == node) {
        list->tail = list->tail->prev;
        node->prev->next = NULL;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    list->length--;
    return node;
}

void jsonb_add_to_list_head(struct JSONB_CACHE_PTR *node, enum JsonbListType desType) {
    struct JSONB_ARC_LIST *desList = jsonb_fetch_list_by_type(desType);
    node->prev = NULL;
    node->next = desList->head;
    if (desList->head != NULL) {
        desList->head->prev = node;
    } else {
        desList->tail = node;
    }
    desList->head = node;
    desList->length++;
}

inline struct JSONB_ARC_LIST *jsonb_fetch_list_by_type(enum JsonbListType listType) {
    if (listType == T1Jsonb)
        return t1_jb;
    if (listType == T2Jsonb)
        return t2_jb;
    if (listType == B1Jsonb)
        return b1_jb;
    if (listType == B2Jsonb)
        return b2_jb;
}

// 把 t1_jb/t2_jb尾部的数据移动到b1_jb/b2_jb的头部
void jsonb_replace(bool in_b2_jb) {
    if (t1_jb->length != 0 && ((t1_jb->length > t1_cap_jsonb) || (in_b2_jb && t1_jb->length == t1_cap_jsonb))) {
        free(t1_jb->tail->jsonb_value);
        jsonb_move_to_mru(t1_jb->tail, B1Jsonb);
    } else {
        free(t2_jb->tail->jsonb_value);
        jsonb_move_to_mru(t2_jb->tail, B2Jsonb);
    }
}