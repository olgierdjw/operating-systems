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
struct epoll_event event, events[MAX_EPOLL_EVENTS];

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
    unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_socket == -1) {
        perror("Failed to create server stream socket");
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

    if (listen(unix_socket, 5) == -1) {
        perror("Błąd przy nasłuchiwaniu na gnieździe");
        exit(1);
    }

    printf("[unix client_udp_socket] listening\n");

}

void epoll_setup() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll create");
        exit(1);
    }

    event.events = EPOLLIN;
    event.data.fd = unix_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, unix_socket, &event) == -1) {
        perror("adding socket to epoll");
        exit(1);
    }
}


void invalid_argument() {
    printf("use ./udp-unix_sock <udp port>\n");
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
            printf("Free index found: %d\n", i);
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


int user_by_socket(int socket) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (clients[i].free_slot)
            continue;
        if (clients[i].unix_socket == socket) {
            return i;
        }
    }
    return -1;
}


void send_message(server_user client, message mes) {
    if (client.is_unix_client) {
        if (write(client.unix_socket, &mes, sizeof(mes)) == -1) {
            perror("Błąd przy wysyłaniu wiadomości");
            exit(1);
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
        if (write(client.unix_socket, &mes, sizeof(mes)) == -1) {
            perror("Błąd przy wysyłaniu wiadomości");
            return false;
        }
        printf("User '%s' is offline\n", client.username);
        return true;
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

void cleanup() {
    message kill_request;
    kill_request.type = STOP;
    for (int i = 0; i < MAX_USERS; i++) {
        if (clients[i].free_slot == false) {
            send_message(clients[i], kill_request);
        }
    }
    printf("Stop signal sent to all users.\n");

    close(unix_socket);
    close(udp_socket);
    close(epoll_fd);
    unlink(UNIX_SERVER_SOCKET_PATH);
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

void print_users() {
    printf("=================\n");
    for (int i = 0; i < 5; i++) {
        printf("%d> username: '%s', free slot: %d", i, clients[i].username, clients[i].free_slot);
        if (clients[i].is_unix_client) {
            printf("    unix socket: %d\n", clients[i].unix_socket);
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
        case INIT:
            strcpy(response.text, "User online.");
            send_message(client, response);
            break;
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
        //printf("Pinging finished\n");
        pthread_mutex_unlock(&mutex);

    }
}


int main(int argc, char *argv[]) {
    int port_address = 1234;
    unix_setup();
//    udp_setup(port_address);
    epoll_setup();


    signal(SIGINT, cleanup);

    create_user_database();

//    pthread_t thread;
//    pthread_create(&thread, NULL, ping_thread, NULL);


    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
        if (num_events == -1) {
            perror("Failed during epoll_wait");
            exit(1);
        }

        for (int i = 0; i < num_events; i++) {
            printf("\n");
            pthread_mutex_lock(&mutex);

            bool first_connection = events[i].data.fd == unix_socket;
            if (first_connection) {
                printf("Accept new connection.\n");
                int newsockfd = accept(unix_socket, NULL, NULL);
                if (newsockfd == -1) {
                    perror("Błąd przy akceptowaniu połączenia");
                    exit(1);
                }

                // add new client to epoll
                event.events = EPOLLIN;
                event.data.fd = newsockfd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, newsockfd, &event) == -1) {
                    perror("Błąd przy dodawaniu gniazda klienta do epoll");
                    exit(1);
                }

                // user profile
                server_user new_user;
                memset(&new_user, 0, sizeof(server_user));
                new_user.free_slot = false;
                new_user.unix_socket = newsockfd;
                new_user.is_unix_client = true;
                strcpy(new_user.username, "TBA");

                // add to the db
                int free_slot = next_user_index();
                clients[free_slot] = new_user;
                printf("User with socket '%d' added. Index %d.\n", new_user.unix_socket, free_slot);

            } else {
                printf("I KNOW YOUR SOCKET!\n");
                int socket_incoming = events[i].data.fd;
                message msg;
                ssize_t bytes_read = read(socket_incoming, &msg, sizeof(msg));
                if (bytes_read == -1) {
                    perror("Can't read message.");
                    exit(1);
                }

                int user_db = user_by_socket(socket_incoming);
                if (user_db == -1) {
                    printf("User registered, but not found in db.");
                    exit(1);
                }


                printf("Received (type = '%d') from '%s': %s\n", msg.type, msg.username, msg.text);

                if (msg.type == INIT) {
                    // add missing name
                    strcpy(clients[user_db].username, msg.username);
                }

                client_action(user_db, msg);


            }
            pthread_mutex_unlock(&mutex);
            print_users();
        }
    }

}
