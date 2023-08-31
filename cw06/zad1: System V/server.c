#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "common.h"


int *client_queues;
int clients_cnt = 0;

int server_queue;

FILE *fp;

key_t key_generator(int id) {
    char wd[2000];
    getcwd(wd, sizeof(wd));
    key_t key = ftok(wd, id);
//    printf("pwd: %s, key: %d\n", wd, key);
    return key;
}


void file_log(char *message) {
    FILE *log_file = fopen("server-log.txt", "a+");
    if (log_file == NULL) {
        printf("CAN NOT SAVE LOGS\n");
        exit(0);
    }
    char *log_message = calloc(strlen(message) + 10, sizeof(char));
    sprintf(log_message, "%s\n", message);
    fwrite(log_message, sizeof(char), strlen(log_message), log_file);
    fclose(log_file);
}


struct message create_message(request_type req_type, const char *mtext) {
    struct message to_send;
    to_send.type = req_type;
    strcpy(to_send.text_message, mtext);
    return to_send;
}


void confirm_connection_init(int client_queue) {
    message request = create_message(INIT, "YOU ARE REGISTERED.");
    msgsnd(client_queue, &request, INIT, 0);
    printf("END\n");
}

void register_user(message mes) {
    int client_queue = atoi(mes.text_message);
    for (int i = 0; i < CLIENTS_LIMIT; i++) {
        if (!client_queues[i]) {
            client_queues[i] = client_queue;
            clients_cnt++;
            break;
        }

    }
    confirm_connection_init(client_queue);
    printf("USER (PRIVATE QUEUE: %d) REGISTERED\n", client_queue);
}

void list() {
    printf("\n> %d CLIENTS: \n", clients_cnt);
    for (int i = 0; i < clients_cnt; i++) {
        if (client_queues[i])
            printf("client id/queue: %d\n", client_queues[i]);
    }
    printf("<CLIENTS: \n\n");
}

void message_to_all(request_type req_type, message mes) {
    for (int i = 0; i < clients_cnt; i++) {
        if (client_queues[i]) {
            msgsnd(client_queues[i], &mes, req_type, 0);
            printf("SENT TO %d\n", client_queues[i]);
        }

    }
}

void text_message_to_all(message sender_and_message) {

    message mes;
    mes.type = ALL2;
    strcpy(mes.text_message, sender_and_message.text_message);
    message_to_all(ALL2, mes);
    int sender;
    char parsed_text[MAX_MESSAGE_BUFOR - 1];
    sscanf(sender_and_message.text_message, "%d %[^\n]", &sender, parsed_text);
    printf("MESSAGE FROM %d TO ALL USERS ('%s')\n", sender, parsed_text);
}

void delete_user(message proces_id) {
    int client_id = atoi(proces_id.text_message);
    client_queues[client_id] = -1;
    message mes;
    mes.type = STOP;
    msgsnd(client_id, &mes, STOP, 0);
    printf("DELETED USER WITH ID: %d\n", client_id);

}

void text_message_to_one(message req) {
    int sender;
    int destination_queue;
    if (destination_queue == server_queue)
        return;
    char parsed_text[MAX_MESSAGE_BUFOR - 1];
    sscanf(req.text_message, "%d %d %[^\n]", &sender, &destination_queue, parsed_text);


    message mes;
    mes.type = ONE2;
    char sender_and_message[1000];
    sprintf(sender_and_message, "%d %s", sender, parsed_text);
    strcpy(mes.text_message, sender_and_message);
    msgsnd(destination_queue, &mes, ONE2, 0);

    printf("MESSAGE [%d] -> [%d]: '%s'\n", sender, destination_queue, parsed_text);
}

void end_request() {
    message mes;
    mes.type = STOP;
    message_to_all(STOP, mes);

}

void server_stop() {
    message mes;
    mes.type = STOP;
    message_to_all(ALL2, mes);
    free(client_queues);
    msgctl(server_queue, IPC_RMID, NULL);
    printf("SERVER PROCESS END\n");
    exit(0);
}


int main(int argc, char **argv) {

    fp = fopen("server-log.txt", "w");
    fclose(fp);
    key_t key = key_generator('S');
    server_queue = msgget(key, 0666 | IPC_CREAT);
    if (msgctl(server_queue, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        exit(1);
    }
    server_queue = msgget(key, 0666 | IPC_CREAT);


    printf("SERVER %d CREATED\n", server_queue);

    client_queues = calloc(sizeof(int), CLIENTS_LIMIT);

    signal(SIGINT, server_stop);

    message request;
    int exit = -1;
    while (exit) {
        printf("========================\n");
        printf("WAITING.\n");
        msgrcv(server_queue, &request, MAX_MESSAGE_BUFOR, 0, 0);
        printf("RECEIVED!\n");
        file_log(request.text_message);
        switch ((request_type) request.type) {
            case STOP:
                delete_user(request);
                break;
            case INIT:
                register_user(request);
                list();
                break;
            case LIST:
                list();
                break;
            case ALL2:
                text_message_to_all(request);
                break;
            case ONE2:
                text_message_to_one(request);
                break;
            default:
                break;
        }
        printf("========================\n");
    }

}
