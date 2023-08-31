#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

int global_request_cnt = 0;
pid_t printing_process = 0;

void confirmation(pid_t client_id) {
    kill(client_id, 10);
}

typedef enum {
    NOTHING,
    COUNT,
    TIME,
    STATS,
    TIMER,
    EXIT
} PROGRAM_MODE;

void program_count() {
    for (int i = 1; i <= 100; i++)
        printf("[SERVER] %d\n", i);
}

void program_show_time() {
    time_t current_time;
    time(&current_time);
    printf("%s\n", ctime(&current_time));
}

void kill_me() {
    exit(0);
}

void program_timer() {
    pid_t id = fork();
    if (id == 0) {

        signal(11, kill_me);

        while (true) {
            printf("loop: ");
            program_show_time();
            sleep(1);
        }
    } else {
        printing_process = id;
    }

}

int program_mode(PROGRAM_MODE program_mode) {
    if (program_mode > 5 || program_mode < 1) {
        printf("INVALID PROGRAM MODE\n");
        exit(-1);
    }
    global_request_cnt++;
    switch (program_mode) {
        case COUNT:
            program_count();
            break;
        case TIME:
            program_show_time();
            break;
        case STATS:
            printf("TOTAL RECEIVED REQUESTS: %d\n", global_request_cnt);
            break;
        case TIMER:
            program_timer();
            break;
        default:
            printf("INVALID PROGRAM MODE\n");
            break;
    }
    return 0;
}

void signal_handler(int signo, siginfo_t *info, void *context) {
    printf(ANSI_COLOR_GREEN "[SERVER] RECEIVED SIGNAL %d with code %d\n" ANSI_COLOR_RESET, signo, info->si_status);
    if (info->si_status == EXIT) {
        confirmation(info->si_pid);
        exit(0);
    }

    if (printing_process != 0) {
        kill(printing_process, 11);
        printing_process = 0;
    }

    // handle operation
    program_mode(info->si_status);

    // and next signal pls
    confirmation(info->si_pid);
    fflush(NULL);
}

void save_pid_to_file() {
    FILE *save_pid = fopen("server_process_id.txt", "w+");
    fprintf(save_pid, "%d\n", getpid());
    fclose(save_pid);
}


int main(int argc, char **argv) {
    pid_t my_id = getpid();
    printf(ANSI_COLOR_CYAN "[SERVER] WAITING. PID: %d\n" ANSI_COLOR_RESET, my_id);
    save_pid_to_file();

    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = (void (*)(int, siginfo_t *, void *)) signal_handler;
    action.sa_flags = SA_SIGINFO;
    sigaction(10, &action, NULL);


    while (true) {
        pause();
    }

}
