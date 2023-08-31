#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>


#define UNIX_SERVER_SOCKET_PATH "/tmp/stream_socket"

#define SERVER_IP "localhost"

#define MAX_MESSAGE_SIZE 500
#define MAX_USERNAME_SIZE 40
#define MAX_USERS 20

#define MAX_EPOLL_EVENTS 10

typedef enum {
    PING,
    LIST,
    TO_ALL,
    TO_ONE,
    INIT,
    STOP,
    JUST_PRINT
} message_type;


typedef struct {
    message_type type;
    char username[MAX_USERNAME_SIZE];
    char text[MAX_MESSAGE_SIZE];
    char additional_text[MAX_MESSAGE_SIZE];
} message;

typedef struct {
    bool free_slot;
    char username[MAX_USERNAME_SIZE];
    bool is_unix_client;
    int unix_socket;
    struct sockaddr_in ipv4_address;
    struct sockaddr_un unix_address;
    uint address_len;
} server_user;


