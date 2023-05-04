#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "arc_json.h"
#include "json_cache_utils.h"

// 定义无效消息结构体
struct InvalidMessage {
    char *key; // 要被清除的缓存的 primaryKey
    enum KeyType keyType; // 缓存的路径类型
    char *parseKey;
};

// 发送函数，向消息队列中写入一条无效消息
void sendInvalidMessage(char *key, enum KeyType keyType, char *parseKey);

// 接收函数，从消息队列中读取未处理的消息并删除对应的缓存
void receiveInvalidMessage();