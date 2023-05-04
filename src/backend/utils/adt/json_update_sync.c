#include "utils/json_update_sync.h"

void sendInvalidMessage(char *key, enum KeyType keyType) {
    struct InvalidMessage message;
    message.key = key;
    message.keyType = keyType;

    // 获取消息队列 ID
    int msgid = msgget(IPC_PRIVATE, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    // 向消息队列中发送消息
    if (msgsnd(msgid, &message, sizeof(message), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

// 接收函数，从消息队列中读取未处理的消息并删除对应的缓存
void receiveInvalidMessage(void) {
    struct InvalidMessage message;

    // 获取消息队列 ID
    int msgid = msgget(IPC_PRIVATE, 0666);
    if (msgid == -1) {
        perror("msgget");
        exit(1);
    }

    // 从消息队列中接收消息
    if (msgrcv(msgid, &message, sizeof(message), 0, IPC_NOWAIT) == -1) {
        // 如果队列为空，则直接返回
        return;
    }

    // 根据消息中的信息删除对应的缓存
    delete_json(message.key, message.keyType);
    delete_parse_info(message.key);
}