#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <signal.h>
#include <time.h>
#include "common.h"

mqd_t server_queue;
char **clients_queues_names;
FILE *log_file;


char* current_time_string() {
    time_t current_time = time(NULL);
    struct tm* time_info = localtime(&current_time);
    char* time_str = malloc(9 * sizeof(char));
    strftime(time_str, 9, "%H:%M:%S", time_info);
    return time_str;
}

void file_log(char *log_row) {
    char *time = current_time_string();
    char log_file_row[100];
    sprintf(log_file_row, "[%s] %s", time, log_row);
    fwrite(log_file_row, sizeof(char), strlen(log_file_row), log_file);
    printf("%s", log_file_row);
}

void open_queue() {

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MAX_MESSAGES;
    attr.mq_msgsize = MAX_REQUEST_SIZE;
    attr.mq_curmsgs = 0;

    mq_unlink(SERVER_QUEUE_NAME);
    if ((server_queue = mq_open(SERVER_QUEUE_NAME, O_RDONLY | O_CREAT, QUEUE_PERMISSIONS, &attr)) == -1) {
        perror("Server: mq_open (server)");
        exit(1);
    }
    file_log("SERVER QUEUE CREATED.\n");
}

void read_queue(request *result_place) {
    if (mq_receive(server_queue, (char *) result_place, MAX_REQUEST_SIZE, NULL) == -1) {
        perror("Server: mq_receive");
        exit(1);
    }

    char log_mes[200];
    sprintf(log_mes, "DECIMAL '%d', TYPE '%d', TEXT '%s', SENDER '%d'\n", result_place->decimal_message,
           result_place->message_type,
           result_place->text_message, result_place->message_sender);
    file_log(log_mes);
}

void to_client(request *req, int client_id) {
    // validate user
    char *client_queue_name = clients_queues_names[client_id];
    if (client_queue_name == NULL) {
        printf("CANT SEND MESSAGE. NO USER DATA.\n");
        return;
    }

    // open user queue
    mqd_t client_queue;
    if ((client_queue = mq_open(client_queue_name, O_WRONLY)) == -1) {
        perror("Client: mq_open (server)");
        exit(1);
    }

    // send message
    if (mq_send(client_queue, (char *) req, MAX_REQUEST_SIZE, 0) == -1) {
        perror("Client: Not able to send message to server");
    }

    // file_log
    char log_mes[200];
    sprintf(log_mes, "[OUTPUT] ( ID = '%d'): TYPE '%d', DECIMAL '%d', TEXT '%s', SENDER '%d'\n", client_id, req->message_type,
           req->decimal_message, req->text_message, req->message_sender);
    file_log(log_mes);


    if (mq_close(client_queue) == -1) {
        perror("Client: mq_close");
        exit(1);
    }
}

void client_kill(int client_id) {
    request kill_order;
    kill_order.message_type = STOP;
    to_client(&kill_order, client_id);
}

void empty_client_base() {
    clients_queues_names = calloc(CLIENTS_LIMIT, sizeof(char *));
    for (int i = 0; i < CLIENTS_LIMIT; i++)
        clients_queues_names[i] = NULL;
    file_log("EMPTY CLIENT DATABASE CREATED.\n");
}


void server_kill() {
    file_log("\nCTRL + C DETECTED.\n");

    // free database and kill clients
    for (int i = 0; i < CLIENTS_LIMIT; i++)
        if (clients_queues_names[i] != NULL) {
            client_kill(i);
            free(clients_queues_names[i]);
        }
    free(clients_queues_names);

    mq_unlink(SERVER_QUEUE_NAME);
    printf("BYE!.\n");
    fclose(log_file);
    exit(0);
}

int register_client(char *client_queue_name) {
    int find_client_id = -1;
    for (int i = 0; i < CLIENTS_LIMIT; i++)
        if (clients_queues_names[i] == NULL) {
            find_client_id = i;
            break;
        }
    if (find_client_id == -1) {
        printf("CLIENT DATABASE FULL.\n");
        exit(0);
    }

    char *name = calloc(MAX_CLIENT_QUEUE_NAME_LENGTH, sizeof(char));
    strcpy(name, client_queue_name);
    clients_queues_names[find_client_id] = name;
    char log_mes[200];
    sprintf(log_mes, "NEW CLIENT <%d, '%s'>\n", find_client_id, name);
    file_log(log_mes);
    return find_client_id;
}

void db_client_remove(request *remove_request) {
    int client_i = remove_request->message_sender;
    if (clients_queues_names[client_i] != NULL) {
        clients_queues_names[client_i] = NULL;
    }

    char log_mes[200];
    sprintf(log_mes, "USER %d DELETED.\n", client_i);
    file_log(log_mes);
}

void init_command(request *client_request) {
    request server_response;
    char *user_queue_name = client_request->text_message;

    // add <new-client-id, client-queue-name> to db
    int new_client_id = register_client(user_queue_name);
    server_response.message_type = INIT;
    server_response.decimal_message = new_client_id;
    strcpy(server_response.text_message, "Registered.");
    to_client(&server_response, new_client_id);
}

void list_command(int client_id) {
    char user_list[19 * CLIENTS_LIMIT] = "";
    printf("USERS LIST:\n");
    for (int i = 0; i < CLIENTS_LIMIT; i++)
        if (clients_queues_names[i] != NULL) {
            char user_row[50];
            sprintf(user_row, "client_id: %d\n", i);
            printf("{client_id: %d, q_name: %s}\n", i, clients_queues_names[i]);
            strcat(user_list, user_row);
        }
    request user_list_message;
    user_list_message.message_type = LIST_USERS;
    strncpy(user_list_message.text_message, user_list, 100);
    to_client(&user_list_message, client_id);
}

void message_to_all_command(request *client_request) {
    char *text_message = calloc(20, sizeof(char));
    sscanf(client_request->text_message, "%*s %s", text_message);
    request message_to_all;
    strcpy(message_to_all.text_message, text_message);
    message_to_all.message_type = SEND_TO_ALL;
    message_to_all.message_sender = client_request->message_sender;


    for (int i = 0; i < CLIENTS_LIMIT; i++)
        if (clients_queues_names[i] != NULL) {
            to_client(&message_to_all, i);
        }

}

void message_to_one_command(request *client_request) {

    request message;
    strcpy(message.text_message, client_request->text_message);
    message.message_type = SEND_TO_ONE;
    message.message_sender = client_request->message_sender;

    int message_receiver = client_request->decimal_message;
    if (clients_queues_names[message_receiver] != NULL) {
        to_client(&message, message_receiver);
        printf("MESSAGE SENT.\n");
    }
}


int main(int argc, char **argv) {
    log_file = fopen(SERVER_LOG_FILE, "w");
    empty_client_base();
    open_queue();

    char *colors[4] = {"\x1b[2;37m", "\x1b[1;34m", "\x1b[1;31m", "\x1b[1;32m"};
    int color = 0;

    signal(SIGINT, server_kill);

    while (1) {
        request client_request;
        read_queue(&client_request);
        printf("%s", colors[color++ % 4]);
        switch ((request_type) client_request.message_type) {

            case INIT:
                init_command(&client_request);
                break;
            case LIST_USERS:
                list_command(client_request.message_sender);
                break;
            case SEND_TO_ALL:
                message_to_all_command(&client_request);
                break;
            case SEND_TO_ONE:
                message_to_one_command(&client_request);
                break;
            case STOP:
                client_kill(client_request.message_sender);
                db_client_remove(&client_request);
                break;
        }
    }
}
