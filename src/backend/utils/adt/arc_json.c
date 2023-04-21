#include "utils/arc_json.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

struct JSON_ARC_LIST *t1 = NULL;
struct JSON_ARC_LIST *b1 = NULL;
struct JSON_ARC_LIST *t2 = NULL;
struct JSON_ARC_LIST *b2 = NULL;

struct JSON_CACHE_PTR *json_map_head = NULL;

const double t_b_total = 1000; // c 24+40+1 = 65
double t1_cap = 0.0; // p

uint json_hit_count = 0, json_get_count = 0, json_phantom_hit_count = 0, json_delete_count = 0;

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
                delete_from_list_and_free(b1->tail);
                replace(false);
            } else {
                delete_from_list_and_free(t1->tail);
            }
        } else {
            double listSize = t1->length + t2->length + b1->length + b2->length;
            if (listSize >= t_b_total) {
                if (listSize == t_b_total * 2) {
                    delete_from_list_and_free(b2->tail);
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
            json_phantom_hit_count++;
            extendSize = b1->length >= b2->length ? 1 : b2->length / b1->length;
            t1_cap = min(t1_cap + extendSize, t_b_total);
            replace(false);
            move_to_mru(node, T2);
            // parse 后 复制 result 到 T2->head->json_value
            return HitB;
        case B2:
            json_phantom_hit_count++;
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
        if (result == NULL) return; // 可以是空的
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
    if (result == NULL) {
        node->json_value = NULL;
    } else {
        node->json_value = (text*) malloc(sizeof(text) + VARSIZE(result) - VARHDRSZ);
        memcpy(node->json_value, result, sizeof(text) + VARSIZE(result) - VARHDRSZ);
    }
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

void remove_from_list(struct JSON_CACHE_PTR *node) {
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
        if (t1->tail->json_value != NULL)
            free(t1->tail->json_value);
        move_to_mru(t1->tail, B1);
    } else {
        if (t2->tail->json_value != NULL)
            free(t2->tail->json_value);
        move_to_mru(t2->tail, B2);
    }
}

// @Return: next node of the one to be deleted
struct JSON_CACHE_PTR *delete_from_list_and_free(struct JSON_CACHE_PTR *node) {
    struct JSON_CACHE_PTR *returnVal = node->next;
    HASH_DEL(json_map_head, node);
    remove_from_list(node);
    free_node(node);
    json_delete_count++;
    return returnVal;
}
void free_node(struct JSON_CACHE_PTR *node) {
    if (node->composite_key != NULL)
        free(node->composite_key);
    if (node->list_type != B1 && node->list_type != B2 && node->json_value != NULL)
        free(node->json_value);
    free(node);
}

extern void delete_json(char *key, enum KeyType keyType) {
    struct JSON_ARC_LIST *listArr[4] = {t1, t2, b1, b2};
    // 未初始化缓存系统
    if (t1 == NULL || t2 == NULL || b1 == NULL || b2 == NULL)
        return;
    // 这种情况对JSON是不存在的? 因为不能删除或更新单个Key, 如果JSONB可以？
    if (keyType == Relid_Tuple_Attnum_Path) {
        return;
    }
    for (int i = 0; i < 4; i++) {
        if (listArr[i] == NULL)
            return;
        for (struct JSON_CACHE_PTR *node = listArr[i]->head; node != NULL;) {
            if (strncmp(key, node->composite_key, strlen(key)) != 0) {
                node = node->next;
                continue;
            }
            node = delete_from_list_and_free(node);
        }
    }
}

extern void print_json_hit_rate(void) {
    ereport(LOG,
            (errmsg("get_count: %u, hit_count: %u, phantom_hit_count: %u, delete_count: %u",
                    json_get_count, json_hit_count,
                    json_phantom_hit_count, json_delete_count)));
}