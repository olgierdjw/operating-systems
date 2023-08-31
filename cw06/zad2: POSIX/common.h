#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define SERVER_QUEUE_NAME "/SERVER_QUEUE"
#define SERVER_LOG_FILE "server.file_log.txt"
#define CLIENTS_LIMIT 20
#define MAX_REQUEST_SIZE sizeof(request)
#define QUEUE_PERMISSIONS 0660
#define MAX_MESSAGES 10
#define MAX_CLIENT_QUEUE_NAME_LENGTH 100

typedef enum {
    INIT = 400,
    LIST_USERS,
    SEND_TO_ALL,
    SEND_TO_ONE = 403,
    STOP
} request_type;

typedef struct request {
    int message_type;
    int message_sender;
    char text_message[100];
    int decimal_message;
} request;

char *timestamp() {
    time_t current_time = time(NULL);
    int size = snprintf(NULL, 0, "%ld", current_time);
    char* str = malloc(size + 2);
    snprintf(str, size + 3, "/%ldu", current_time);
    printf("CLIENT QUEUE NAME = '%s'\n", str);
    return str;
}
