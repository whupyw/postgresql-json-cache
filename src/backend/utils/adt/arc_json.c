#include "utils/arc_json.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

struct JSON_ARC_LIST *t1 = NULL;
struct JSON_ARC_LIST *b1 = NULL;
struct JSON_ARC_LIST *t2 = NULL;
struct JSON_ARC_LIST *b2 = NULL;

struct JSON_CACHE_PTR *json_map_head = NULL;

const double t_b_total = 1000.0; // c
double t1_cap = 0.0; // p

uint json_hit_count, json_get_count;

void init_arc_lists(void) {
    init_specific_list(&t1);
    init_specific_list(&t2);
    init_specific_list(&b1);
    init_specific_list(&b2);
}

void init_specific_list(struct JSON_ARC_LIST **list) {

    *list = (struct JSON_ARC_LIST*) malloc(sizeof(struct JSON_ARC_LIST));
    (*list)->head = NULL;
    (*list)->tail = NULL;
    (*list)->length = 0;
}

enum HitCase get_json_data(char *compositeKey, text **result) {
    struct JSON_CACHE_PTR *node;
    double extendSize;
    if (t1 == NULL) {
        init_arc_lists();
    }
    json_get_count++;
    HASH_FIND_STR(json_map_head, compositeKey, node);
    if (node == NULL) {
        if (t1->length + b1->length == t_b_total) {
            if (t1->length < t_b_total) {
                HASH_DEL(json_map_head, b1->tail);
                free(remove_from_list(b1->tail));
                replace(false);
            } else {
                HASH_DEL(json_map_head, t1->tail);
                free(remove_from_list(t1->tail));
            }
        } else {
            double listSize = t1->length + t2->length + b1->length + b2->length;
            if (listSize >= t_b_total) {
                if (listSize == t_b_total * 2) {
                    HASH_DEL(json_map_head, b2->tail);
                    free(remove_from_list(b2->tail));
                }
                replace(false);
            }
        }
        return HitNone;
    }
    switch (node->list_type) {
        case T1:
        case T2:
            move_to_mru(node, T2);
            json_hit_count++;
            *result = node->json_value;
            return HitT;
        case B1:
            extendSize = b1->length >= b2->length ? 1 : b2->length / b1->length;
            t1_cap = min(t1_cap + extendSize, t_b_total);
            replace(false);
            move_to_mru(node, T2);
            // parse 后 复制 result 到 T2->head->json_value
            return HitB;
        case B2:
            extendSize = b2->length >= b1->length ? 1 : b1->length / b2->length;
            t1_cap = max(t1_cap - extendSize, 0);
            replace(true);
            move_to_mru(node, T2);
            // parse 后 复制 result 到 T2->head->json_value
            return HitB;
    }
}

void insert_json_data(char *compositeKey, text *result, enum HitCase hitCase) {
    struct JSON_CACHE_PTR *node;

    if (hitCase == HitB) {
        t2->head->json_value = (text*) malloc(sizeof(text) + VARSIZE(result) - VARHDRSZ);
        memcpy(t2->head->json_value, result, sizeof(text) + VARSIZE(result) - VARHDRSZ);
        return;
    }
    // HitNone
    node = (struct JSON_CACHE_PTR*) malloc(sizeof(struct JSON_CACHE_PTR));
    // node->composite_key
    node->composite_key = (char *) malloc(strlen(compositeKey) + 1);
    strcpy(node->composite_key, compositeKey);
    // node->json_value
    node->json_value = (text*) malloc(sizeof(text) + VARSIZE(result) - VARHDRSZ);
    memcpy(node->json_value, result, sizeof(text) + VARSIZE(result) - VARHDRSZ);
    // node->list_type
    node->list_type = T1;
    add_to_list_head(node, T1);
    HASH_ADD_STR(json_map_head, composite_key, node);
}

void move_to_mru(struct JSON_CACHE_PTR *node, enum ListType desType) {
    // 从原来的list移除
    remove_from_list(node);
    // 添加到新的list头部
    add_to_list_head(node, desType);
    node->list_type = desType;
}

struct JSON_CACHE_PTR * remove_from_list(struct JSON_CACHE_PTR *node) {
    struct JSON_ARC_LIST *list = fetch_list_by_type(node->list_type);
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

void add_to_list_head(struct JSON_CACHE_PTR *node, enum ListType desType) {
    struct JSON_ARC_LIST *desList = fetch_list_by_type(desType);
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

inline struct JSON_ARC_LIST *fetch_list_by_type(enum ListType listType) {
    if (listType == T1)
        return t1;
    if (listType == T2)
        return t2;
    if (listType == B1)
        return b1;
    if (listType == B2)
        return b2;
}

// 把 T1/T2尾部的数据移动到B1/B2的头部
void replace(bool in_b2) {
    if (t1->length != 0 && ((t1->length > t1_cap) || (in_b2 && t1->length == t1_cap))) {
        free(t1->tail->json_value);
        move_to_mru(t1->tail, B1);
    } else {
        free(t2->tail->json_value);
        move_to_mru(t2->tail, B2);
    }
}