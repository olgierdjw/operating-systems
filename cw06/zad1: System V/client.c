#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "common.h"


key_t key_generator(int id) {
    char wd[2000];
    getcwd(wd, sizeof(wd));
    key_t key = ftok(wd, id);
    return key;
}


void file_log(char *message) {
    FILE *log_file = fopen("./client-logs.txt", "a+");
    if (log_file == NULL) {
        printf("CAN NOT SAVE LOGS\n");
        exit(0);
    }
    char *log_message = calloc(strlen(message) + 10, sizeof(char));
    sprintf(log_message, "%s\n", message);
    fwrite(log_message, sizeof(char), strlen(log_message), log_file);
    fclose(log_file);
}


struct message create_message(request_type req_type, const char *mtext, int private_queue) {
    struct message to_send;
    to_send.type = req_type;
    char append_sender[1000];
    sprintf(append_sender, "%d %s", private_queue, mtext);
    strcpy(to_send.text_message, append_sender);
    return to_send;
}

void incoming_message(char *text, char *sender_and_message) {
    int sender;
    char message[2000];
    sscanf(sender_and_message, "%d %[^\n]", &sender, message);
    printf("MESSAGE FROM %d: %s\n", sender, message);
}

void command_parser(char *command, int server_queue, int private_queue) {
    char parsed_text[1024];
    int parsed_client_id;
    if (strcmp(command, "LIST\n") == 0) {
        message mes = create_message(LIST, "", -1);
        msgsnd(server_queue, &mes, MAX_MESSAGE_BUFOR, 0);
        printf("LIST COMMAND REQUESTED.\n");
    } else if (sscanf(command, "2ALL %[^\n]", parsed_text) == 1) {
        message mes = create_message(ALL2, parsed_text, private_queue);
        msgsnd(server_queue, &mes, MAX_MESSAGE_BUFOR, 0);
        printf("TO ALL MESSAGE COMMAND REQUESTED.\n");
    } else if (sscanf(command, "2ONE %d %[^\n]", &parsed_client_id, parsed_text) == 2) {
        printf("SENT MESSAGE TO %d (%s)\n", parsed_client_id, parsed_text);
    } else if (strcmp(command, "STOP\n") == 0) {
        char *client_id_str = calloc(15, sizeof(char));
        sprintf(client_id_str, "%d", private_queue);
        message mes = create_message(STOP, client_id_str, private_queue);
        msgsnd(server_queue, &mes, MAX_MESSAGE_BUFOR, 0);
        printf("STOP COMMAND SENT\n");
    } else {
        printf("Nieznane polecenie: %s\n", command);
    }
}

void kill_me() {
    printf("KILL ME.\n");
    exit(0);
}

int main(int argc, char **argv) {
    key_t key = key_generator('S');
    int server_queue = msgget(key, 0666);
    if (server_queue == -1) {
        printf("SERVER NOT FOUND.\n");
        exit(0);
    }
    printf("FOUND SERVER: %d\n", server_queue);

    char letter = 'A';
    key_t my_queue_key = key_generator(letter);
    int my_queue = msgget(my_queue_key, 0666 | IPC_CREAT | IPC_EXCL);
    while (my_queue == -1) {
        letter += 1;
        my_queue_key = key_generator(letter);
        my_queue = msgget(my_queue_key, 0666 | IPC_CREAT | IPC_EXCL);
    }
    printf("PRIVATE QUEUE: %d\n", my_queue);


    char tmp[200];
    sprintf(tmp, "%d", my_queue);
    message init_message = create_message(INIT, tmp, my_queue);
    msgsnd(server_queue, &init_message, MAX_MESSAGE_BUFOR, 0);
    printf("WAITING FOR SERVER INIT RESPONSE\n");

    message confirmation;
    msgrcv(my_queue, &confirmation, MAX_MESSAGE_BUFOR, INIT, 0);
    printf("CONNECTED TO SERVER %d\n", server_queue);


    signal(SIGINT, kill_me);

    pid_t pid = fork();
    if (pid == 0) {
        message request;
        while (1) {
            msgrcv(my_queue, &request, MAX_MESSAGE_BUFOR, 0, 0);
            printf("MESSAGE RECEIVED! %s\n", request.text_message);
            switch ((request_type) request.type) {
                case ALL2:
                    incoming_message("TO ALL", request.text_message);
                    break;
                case ONE2:
                    incoming_message("TO ME", request.text_message);
                    break;
                case STOP:
                    kill(getppid(), SIGINT);
                    exit(0);
                default:
                    break;
            }
            sleep(1);
        }

    } else {
        char user_input[100];
        while (1) {
            printf("Command (or 'STOP' to quit):\n");
            fgets(user_input, 1024, stdin);
            command_parser(user_input, server_queue, my_queue);
        }
    }
}
