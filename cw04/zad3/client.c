#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"


void send_message(pid_t destination, int message_code) {
    union sigval message;
    message.sival_int = message_code;
    if (sigqueue(destination, 10, message) == -1)
        printf("[CLIENT] ERROR: CAN'T REACH SERVER\n");
}

int read_client_id_from_file(char *file_name) {
    FILE *read_from = fopen(file_name, "r");
    if (read_from == NULL) {
        fprintf(stderr, "CANT FIND FILE %s\n", file_name);
        exit(0);
    }
    char *line = calloc(100, sizeof(char));
    size_t str_size = 0;
    getline(&line, &str_size, read_from);
    fclose(read_from);
    int number = -1;
    if (sscanf(line, "%d", &number) == 1)
        return number;
    else {
        fprintf(stderr, "CAN'T PARSE CLIENT ID \n");
        exit(0);
    }
}

void handle_signal(int sig) {
    printf("[CLIENT] SERVER CONFIRMED.\n");
}

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "INVALID ARGUMENTS. USE: sender [PID]\n");
        exit(1);
    }

    pid_t client = read_client_id_from_file("server_process_id.txt");
    printf("[CLIENT] PID: %d\n", getpid());
    printf("CONNECTED TO %d\n", client);

    signal(10, handle_signal);

    for (int i = 2; i < argc; i++) {
        int task_id = atoi(*(argv + i));
        printf("[CLIENT] SEND TODO: %d\n", task_id);
        send_message(client, task_id);
        pause();
    }
}
