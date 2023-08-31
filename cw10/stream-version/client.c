#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include "messenger.h"
#include <pthread.h>

// unix socket
int unix_socket;

// server udp socket
bool use_unix_socket;
struct sockaddr_in udp_server_address;
int client_udp_socket;

// client
char username[MAX_USERNAME_SIZE];

void unix_setup() {
    struct sockaddr_un unix_server_address;

    // Create a client_udp_socket
    unix_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (unix_socket == -1) {
        perror("Failed to create client client_udp_socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    memset(&unix_server_address, 0, sizeof(unix_server_address));
    unix_server_address.sun_family = AF_UNIX;
    strncpy(unix_server_address.sun_path, UNIX_SERVER_SOCKET_PATH, sizeof(unix_server_address.sun_path) - 1);

    // Connect the client_udp_socket to the server address
    if (connect(unix_socket, (struct sockaddr *) &unix_server_address, sizeof(unix_server_address)) == -1) {
        perror("Failed to connect to server");
        exit(1);
    }

    printf("[Unix socket] Connected to server\n");
}

message empty_message() {
    message new_message;
    memset(&new_message, 0, sizeof(new_message));
    strcpy(new_message.username, username);
    return new_message;
}

void send_message(message_type type, char *text) {
    message msg = empty_message();
    msg.type = type;
    strcpy(msg.text, text);

    printf("Sending as '%s': %s\n", msg.username, msg.text);

    if (use_unix_socket) {
        if (write(unix_socket, &msg, sizeof(msg)) == -1) {
            perror("write unix socket stream");
            exit(1);
        }
    }
    if (!use_unix_socket) {
        ssize_t n = sendto(client_udp_socket, &msg, sizeof(msg), 0, (struct sockaddr *) &udp_server_address,
                           sizeof(udp_server_address));
        if (n < 0) {
            perror("sendto udp");
            exit(0);
        }
    }

    //printf("Sent message to server: type = '%d', text = '%s'\n", msg.type, msg.text);
}

void close_sockets() {
    if (close(unix_socket) == -1 || close(client_udp_socket) == -1) {
        perror("Failed to close the client socket");
        exit(1);
    }
    exit(EXIT_SUCCESS);
}

void signal_handler(int signum) {
    // notify server
    send_message(STOP, "Bye!");

    printf("\nBye!\n");
    close_sockets();
}

message read_message() {
    message buffer;
    int num_bytes;
    if (use_unix_socket) {
        num_bytes = recv(unix_socket, &buffer, sizeof(message), 0);

    } else {
        num_bytes = recv(client_udp_socket, &buffer, sizeof(message), 0);
    }
    if (num_bytes == -1) {
        perror("Failed to receive message");
        exit(1);
    }
    return buffer;
}

void print_message(message mes) {
    printf("Message: type = '%d', text = '%s'\n", mes.type, mes.text);
}

void *receive_messages() {
    printf("Reading thread is running...\n");

    while (1) {
        message mes = read_message();
        printf("\033[34m");
        print_message(mes);
        switch (mes.type) {
            case INIT:
                printf("Registered! %s\n", mes.text);
                break;
            case LIST:
                printf("Users online: %s\n", mes.text);
                break;
            case TO_ALL:
                printf("Message from %s to all users: %s\n", mes.additional_text, mes.text);
                break;
            case TO_ONE:
                printf("Message from %s: %s\n", mes.additional_text, mes.text);
                break;
            case STOP:
                close_sockets();
                break;
            default:
                break;
        }
        printf("\033[0m");

    }

}

void start_reading_thread() {
    pthread_t thread;
    int result = pthread_create(&thread, NULL, receive_messages, NULL);
    if (result != 0) {
        perror("Thread creation failed");
        exit(1);
    }
}


int main(int argc, char *argv[]) {
    int udp_port = 1234;
    if (argc == 2) {
        use_unix_socket = true;
        unix_setup();
        printf("Using unix socket.\n");
    } else if (argc == 3) {
        exit(1);
        use_unix_socket = false;
        udp_port = atoi(argv[2]);
        //udp_init(udp_port);
        printf("Using udp socket.\n");
    } else {
        printf("For unix socket use: ./client <username>\n");
        printf("For udp socket use: ./client <username> <port>\n");
        exit(1);
    }

    signal(SIGINT, signal_handler);

    // parse username to variable username
    strcpy(username, argv[1]);
    printf("Hi, '%s'\n", username);

    start_reading_thread();
    sleep(1);

    send_message(INIT, "Hi!");

    while (1) {
        //printf("ENTER COMMAND:\n");
        char *user_input = calloc(1024, sizeof(char));
        fgets(user_input, 1024, stdin);
        if (user_input[0] == 'q') {
            break;
        }

        char *command = calloc(30, sizeof(char));
        sscanf(user_input, "%s ", command);
        if (strcmp(command, "LIST") == 0) {
            send_message(LIST, "Hi!");
        } else if (strcmp(command, "TO_ALL") == 0) {
            printf("Sending message to all users... \n");
            send_message(TO_ALL, user_input + strlen(command) + 1);
        } else if (strcmp(command, "TO_ONE") == 0) {
            send_message(TO_ONE, user_input + strlen(command) + 1);
        } else if (strcmp(command, "STOP") == 0) {
            signal_handler(SIGINT);
        } else {
            printf("COMMAND NOT FOUND.\n");
            printf("LIST, TO_ALL <message>, TO_ONE <who> <message>, STOP\n");
        }
    }

    close(unix_socket);
    close(client_udp_socket);
    return 0;
}
