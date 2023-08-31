#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"

char client_queue_name[100];
int client_id;
mqd_t server_queue, private_queue;

void open_queues() {

    struct mq_attr attr;

    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_REQUEST_SIZE;
    attr.mq_curmsgs = 0;

    if ((private_queue = mq_open(client_queue_name, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Client: mq_open (client)");
        exit(1);
    }

    if ((server_queue = mq_open(SERVER_QUEUE_NAME, O_WRONLY)) == -1) {
        perror("Client: mq_open (server)");
        exit(1);
    }
}

void close_queues() {

    if (mq_close(private_queue) == -1) {
        perror("Client: mq_close");
        exit(1);
    }

    if (mq_unlink(client_queue_name) == -1) {
        perror("Client: mq_unlink");
        exit(1);
    }
    printf("Client: bye\n");
    exit(0);
}

void read_queue(request *result_place) {
    if (mq_receive(private_queue, (char *) result_place, MAX_REQUEST_SIZE, NULL) == -1) {
        perror("Client: mq_receive");
        exit(1);
    }
}

void send_request(request *to_send) {
    if (mq_send(server_queue, (char *) to_send, MAX_REQUEST_SIZE, 0) == -1) {
        perror("Client: Not able to send message to server");
    }
}

void client_register() {
    // send message to server
    request client_request;
    client_request.message_type = INIT;
    strcpy(client_request.text_message, client_queue_name);
    send_request(&client_request);

    request server_response;
    read_queue(&server_response);
    printf("Client: Token received from server: %d\n\n", server_response.decimal_message);
    client_id = server_response.decimal_message;
    printf("NEW CLIENT ID = '%d'\n", client_id);
}

void list_command() {
    request client_request;
    client_request.message_type = LIST_USERS;
    client_request.message_sender = client_id;
    send_request(&client_request);
    printf("USER LIST REQUESTED\n");
}

void message_to_one(char *command) {
    request message;
    message.message_sender = client_id;
    message.message_type = SEND_TO_ONE;
    sscanf(command, "%*s %d %s", &message.decimal_message, message.text_message);
    send_request(&message);
}

void message_to_all(char *command) {
    request message;
    message.message_sender = client_id;
    message.message_type = SEND_TO_ALL;
    strcpy(message.text_message, command);
    printf("SENT TO ALL CLIENTS.\n");
    send_request(&message);
}

void request_stop() {
    request message;
    message.message_sender = client_id;
    message.message_type = STOP;
    send_request(&message);
}

void client_end() {
    close_queues();
    mq_unlink(client_queue_name);
    printf("BYE!\n");
    exit(0);
}

int main(int argc, char **argv) {
    strcpy(client_queue_name, timestamp());
    open_queues();
    client_register();


    pid_t pid = fork();
    if (pid == 0) {
        request server_response;
        while (1) {
            read_queue(&server_response);
            printf("SERVER MESSAGE RECEIVED.\n");
            switch ((request_type) server_response.message_type) {
                case SEND_TO_ALL:
                    printf("[%d TO ALL] %s\n", server_response.message_sender,
                           server_response.text_message);
                    break;
                case SEND_TO_ONE:
                    printf("[%d] %s\n", server_response.message_sender, server_response.text_message);
                    break;
                case STOP:
                    kill(getppid(), SIGINT);
                    exit(0);
                case LIST_USERS:
                    printf("USERS: >>>F \n %s\n <<<\n", server_response.text_message);;
                default:
                    break;
            }
        }
    } else {
        signal(SIGINT, client_end);

        char user_input[100];
        while (1) {
            printf("ENTER COMMAND:\n");
            fgets(user_input, 1024, stdin);
            if (user_input[0] == 'q') {
                break;
            }
            char *command = calloc(30, sizeof(char));
            sscanf(user_input, "%s ", command);
            if (strcmp(command, "LIST") == 0)
                list_command();
            else if (strcmp(command, "TO_ALL") == 0)
                message_to_all(user_input);
            else if (strcmp(command, "TO_ONE") == 0)
                message_to_one(user_input);
            else if (strcmp(command, "STOP") == 0) {
                request_stop();
                printf("STOP COMMAND REQUESTED.\n");
            } else {
                printf("COMMAND NOT FOUND.\n");
                printf("LIST, TO_ALL [message], TO_ONE [who] [message], STOP\n");
            }
        }
    }
}
