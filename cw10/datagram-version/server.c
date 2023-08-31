#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include "messenger.h"
#include <pthread.h>

// udp client_udp_socket
int udp_socket;
struct sockaddr_in server_udp_address;

// unix client_udp_socket
struct sockaddr_un unix_server_address;
int unix_socket;

// epoll
int epoll_fd;
struct epoll_event events[MAX_EPOLL_EVENTS];

// create mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// server database
server_user clients[MAX_USERS];

typedef struct {
    struct sockaddr_un unix_address;
    struct sockaddr_in web_address;
    uint address_len;
} client_address;


void unix_setup() {
    unlink(UNIX_SERVER_SOCKET_PATH);

    // Create a client_udp_socket
    unix_socket = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (unix_socket == -1) {
        perror("Failed to create server client_udp_socket");
        exit(EXIT_FAILURE);
    }

    // Save server address
    memset(&unix_server_address, 0, sizeof(unix_server_address));
    unix_server_address.sun_family = AF_UNIX;
    strncpy(unix_server_address.sun_path, UNIX_SERVER_SOCKET_PATH, sizeof(unix_server_address.sun_path) - 1);

    // Bind the client_udp_socket to the address
    if (bind(unix_socket, (struct sockaddr *) &unix_server_address, sizeof(unix_server_address)) == -1) {
        perror("Failed to bind server client_udp_socket");
        exit(EXIT_FAILURE);
    }

    printf("[unix client_udp_socket] started\n");

}

void udp_setup(int port_address) {
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("udp client_udp_socket");
        exit(1);
    }

    uint udp_server_address_len = sizeof(server_udp_address);
    memset(&server_udp_address, 0, sizeof(server_udp_address));
    server_udp_address.sin_family = AF_INET;
    server_udp_address.sin_addr.s_addr = INADDR_ANY;
    server_udp_address.sin_port = htons(port_address);

    if (bind(udp_socket, (struct sockaddr *) &server_udp_address, udp_server_address_len) == -1) {
        perror("udp bind");
        close(udp_socket);
        exit(1);
    }
    printf("[udp client_udp_socket] started on port %d\n", port_address);

}

void epoll_setup() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Failed to create epoll instance");
        exit(1);
    }

    // Add udp_socket to epoll
    struct epoll_event udp_event;
    udp_event.events = EPOLLIN;
    udp_event.data.fd = udp_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, udp_socket, &udp_event) == -1) {
        perror("Failed to add udp_socket to epoll");
        exit(1);
    }

    // Add unix_socket to epoll
    struct epoll_event unix_event;
    unix_event.events = EPOLLIN;
    unix_event.data.fd = unix_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &unix_event) == -1) {
        perror("Failed to add unix_socket to epoll");
        exit(1);
    }
}

void cleanup() {
    close(unix_socket);
    close(udp_socket);
    close(epoll_fd);
    unlink(UNIX_SERVER_SOCKET_PATH);
}

void invalid_argument() {
    printf("for udp client_udp_socket use ./udp-unix_sock <udp port>\n");
    exit(1);
}

void create_user_database() {
    for (int i = 0; i < MAX_USERS; i++) {
        clients[i].free_slot = true;
    }
    printf("Empty %d place(s) created.\n", MAX_USERS);
}

int next_user_index() {
    for (int i = 0; i < MAX_USERS; i++) {
        if (clients[i].free_slot) {
            memset(&clients[i], 0, sizeof(server_user));
            return i;
        }
    }
    printf("Server is full. Free index not found.\n");
    return -1;
}

int user_by_name(char *username) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (clients[i].free_slot)
            continue;
        if (strcmp(clients[i].username, username) == 0) {
            printf("User '%s' found.\n", username);
            return i;
        }
    }
    printf("User '%s' not found.\n", username);
    return -1;
}

void send_message(server_user client, message mes) {
    printf("Send message to '%s'\n", client.username);
    if (client.is_unix_client) {
        printf("to address unix: '%s'\n", client.unix_address.sun_path);

        if (sendto(unix_socket, &mes, sizeof(message), 0,
                   (struct sockaddr *) &client.unix_address, client.address_len) == -1) {
            //printf("sending to unix address: '%s'\n", client.unix_address.sun_path);
            printf("User is offline. Sending failed\n");

        }
    } else {
        printf("address udp: '%u'\n", client.ipv4_address.sin_addr.s_addr);
        if (sendto(udp_socket, &mes, sizeof(message), 0,
                   (struct sockaddr *) &client.ipv4_address, client.address_len) == -1) {
            perror("Failed to send udp response to client");
        }
    }
    printf("\n");
}

bool is_online(server_user client) {
    int try = 3;
    message mes = {.type=PING};
    if (client.is_unix_client) {
        for (int i = 0; i < try; i++) {
            bool offline = (sendto(unix_socket, &mes, sizeof(message), 0,
                                   (struct sockaddr *) &client.unix_address, client.address_len) == -1);
            if (!offline) return true;
        }
        printf("User '%s' is offline\n", client.username);

        return false;
    } else {
        for (int i = 0; i < try; i++) {
            bool offline = sendto(udp_socket, &mes, sizeof(message), 0,
                                  (struct sockaddr *) &client.ipv4_address, client.address_len) == -1;
            if (!offline) return true;
        }
    }
    printf("User '%s' is online\n", client.username);
    return false;
}

message empty_message() {
    message new_message;
    memset(&new_message, 0, sizeof(new_message));
    strcpy(new_message.username, "server");
    return new_message;
}

void cant_send_message_notice(server_user sender, char *not_found_username) {
    message mes = empty_message();
    mes.type = JUST_PRINT;
    char *message = calloc(MAX_MESSAGE_SIZE, sizeof(char));
    sprintf(message, "User '%s' tried to send message to '%s' but user not found\n", sender.username,
            not_found_username);
    printf("%s", message);
    strcpy(mes.text, message);
    send_message(sender, mes);
}

void deep_copy_sockaddrUn(struct sockaddr_un *dest, const struct sockaddr_un *src) {
    dest->sun_family = src->sun_family;
    strncpy(dest->sun_path, src->sun_path, sizeof(dest->sun_path) - 1);
    dest->sun_path[sizeof(dest->sun_path) - 1] = '\0';
}

void deep_copy_sockaddrIn(struct sockaddr_in *dest, const struct sockaddr_in *src) {
    dest->sin_family = src->sin_family;
    dest->sin_port = src->sin_port;
    dest->sin_addr.s_addr = src->sin_addr.s_addr;
}

client_address read_message(bool unix_sender, message *mes) {
    client_address message_sender;
    memset(&message_sender, 0, sizeof(message_sender));

    *mes = empty_message();
    ssize_t num_bytes;

    if (unix_sender) {
        struct sockaddr_un unix_address;
        uint address_size;
        memset(&unix_address, 0, sizeof(struct sockaddr_un));

        num_bytes = recvfrom(unix_socket, mes, sizeof(message), 0,
                             (struct sockaddr *) &unix_address, &address_size);
        deep_copy_sockaddrUn(&message_sender.unix_address, &unix_address);
        message_sender.address_len = address_size;

    } else {
        struct sockaddr_in web_address;
        uint address_size;

        memset(&web_address, 0, sizeof(struct sockaddr_in));
        num_bytes = recvfrom(udp_socket, mes, sizeof(message), 0, (struct sockaddr *) &web_address,
                             &address_size);
        deep_copy_sockaddrIn(&message_sender.web_address, &web_address);
        message_sender.address_len = address_size;

    }
    if (num_bytes == -1) {
        printf("%zd bits from address %s\n", num_bytes, message_sender.unix_address.sun_path);
        perror("Failed to receive message from client");
        exit(1);
    }
    return message_sender;
}

int create_client_in_db(client_address client_data, message client_request, bool unix_user) {
    int user_db_index = user_by_name(client_request.username);
    bool user_registered = user_db_index != -1;
    bool user_first_request = client_request.type == INIT;

    if (user_registered) {
        if (user_first_request) {
            char *text_back = calloc(50, sizeof(char));
            sprintf(text_back, "Welcome back %s!", client_request.username);
            printf("User '%s' already registered.\n", client_request.username);
            message mes = empty_message();
            mes.type = INIT;
            strcpy(mes.text, text_back);
            send_message(clients[user_db_index], mes);
        }
        return user_db_index;
    } else {
        server_user new_client;
        strcpy(new_client.username, client_request.username);
        int free_index = next_user_index();
        new_client.free_slot = false;
        new_client.is_unix_client = unix_user;
        if (unix_user) {
            deep_copy_sockaddrUn(&new_client.unix_address, &client_data.unix_address);
        } else {
            deep_copy_sockaddrIn(&new_client.ipv4_address, &client_data.web_address);
        }
        new_client.address_len = client_data.address_len;
        clients[free_index] = new_client;
        printf("User '%s' registered. Database index: %d\n", client_request.username, free_index);
        return free_index;
    }
}


void print_users() {
    printf("=================\n");
    for (int i = 0; i < 5; i++) {
        //if (clients[i].free_slot) continue;

        printf("%d> USER '%s', free slot: %d", i, clients[i].username, clients[i].free_slot);
        if (clients[i].is_unix_client) {
            printf("    unix address: %s\n", clients[i].unix_address.sun_path);
        } else {
            printf("    udp client port: %u\n", ntohs(clients[i].ipv4_address.sin_port));
        }
    }
    printf("=================\n");

}

void delete_user(char *username) {
    int user_index = user_by_name(username);
    if (user_index == -1)
        return;
    printf("User '%s' deleted from database (index '%d')\n", clients[user_index].username, user_index);
    memset(&clients[user_index], 0, sizeof(server_user));
    clients[user_index].free_slot = true;
}

void client_action(int client_db_index, message user_request) {
    server_user client = clients[client_db_index];
    message response = empty_message();

    switch (user_request.type) {
        case LIST:
            response.type = LIST;
            strcpy(response.text, "List of users:\n");
            for (int i = 0; i < MAX_USERS; i++) {
                if (clients[i].free_slot == false) {
                    strcat(response.text, clients[i].username);
                    strcat(response.text, "\n");
                }
            }
            send_message(client, response);
            break;
        case TO_ALL:
            response.type = TO_ALL;
            strcpy(response.text, user_request.text);
            strcpy(response.additional_text, client.username);
            for (int i = 0; i < MAX_USERS; i++) {
                if (clients[i].free_slot == false) {
                    send_message(clients[i], response);
                }
            }
            printf("User '%s' sent message to all users\n", client.username);
            break;
        case TO_ONE:
            response.type = TO_ONE;
            char *username = strtok(user_request.text, " ");
            int user_index = user_by_name(username);
            char *message = strtok(NULL, "");
            strcpy(response.text, message);
            strcpy(response.additional_text, client.username);
            if (user_index != -1) {
                send_message(clients[user_index], response);
                printf("User '%s' sent message to '%s'\n", client.username, username);
            } else {
                cant_send_message_notice(clients[client_db_index], username);
            }
            break;
        case STOP:
            delete_user(client.username);
            break;
        default:
            printf("Unknown request type\n");
            break;
    }
}

void ping_all() {
    for (int i = 0; i < MAX_USERS; i++) {
        if (clients[i].free_slot)
            continue;
        if (!is_online(clients[i])) {
            printf("PING. JEST OFFLINE -> DELETE\n");
            delete_user(clients[i].username);
        }
    }
}

void *ping_thread(void *arg) {
    while (1) {
        sleep(1);
        pthread_mutex_lock(&mutex);

        printf("Pinging clients...\n");
        ping_all();

        pthread_mutex_unlock(&mutex);

    }
}



int main(int argc, char *argv[]) {
    int port_address = 1234;
    if (argc == 2)
        port_address = atoi(argv[1]);
    else
        invalid_argument();


    unix_setup();
    udp_setup(port_address);
    epoll_setup();


    signal(SIGINT, cleanup);

    create_user_database();

    pthread_t thread;
    pthread_create(&thread, NULL, ping_thread, NULL);

    while (1) {

        print_users();

        printf("\n");

        int num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        if (num_events == -1) {
            perror("Failed during epoll_wait");
            exit(1);
        }

        pthread_mutex_lock(&mutex);
        for (int i = 0; i < num_events; i++) {
            // request socket type
            int current_socket = events[i].data.fd;
            bool is_unix_request = events[i].data.fd == unix_socket;

            if (current_socket == udp_socket) {
                printf("\nCLIENT UDP REQUEST\n");
            } else if (current_socket == unix_socket) {
                printf("\nCLIENT UNIX REQUEST\n");

            }

            // get message, nickname and client address
            message request;
            client_address request_sender = read_message(is_unix_request, &request);
            //if (is_unix_request)
            //    printf("    unix: %s\n", request_sender.unix_address.sun_path);
            //else {
            //    printf("    udp client port: %u\n", ntohs(request_sender.web_address.sin_port));
            //}


            // find or add user to the db
            int user_db_index = create_client_in_db(request_sender, request, is_unix_request);

            // handle client request
            if (request.type != INIT) {
                client_action(user_db_index, request);
            }
        }
        pthread_mutex_unlock(&mutex);
    }
}
