#include <stdio.h>


#define CLIENTS_LIMIT 100
#define MAX_MESSAGE_BUFOR 1000

typedef enum {
    INIT = 400,
    LIST,
    ALL2,
    ONE2,
    STOP
} request_type;

typedef struct message {
    long type;
    char text_message[MAX_MESSAGE_BUFOR-1];
} message;