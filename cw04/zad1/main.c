#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

typedef enum {
    INVALID,
    IGNORE,
    HANDLER,
    MASK,
    PENDING,
} TASK_TYPE;

TASK_TYPE str_to_task_type(const char *str) {
    if (strcmp(str, "ignore") == 0) return IGNORE;
    if (strcmp(str, "handler") == 0) return HANDLER;
    if (strcmp(str, "mask") == 0) return MASK;
    if (strcmp(str, "pending") == 0) return PENDING;
    return INVALID;
}

void signal_handler(int sig) {
    printf(ANSI_COLOR_GREEN "pid: %d -> signal %d\n" ANSI_COLOR_RESET, getpid(), sig);
    fflush(NULL);
}

void mask_signal(int sig, bool block) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);

    if (block) {
        sigprocmask(SIG_BLOCK, &mask, NULL);
        printf(ANSI_COLOR_GREEN "signal masked\n" ANSI_COLOR_RESET);
    } else
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void test_signals(TASK_TYPE task_type, int sig, bool parent_process) {

    switch (task_type) {
        case IGNORE:
            signal(sig, SIG_IGN);
            kill(getpid(), sig);
            break;
        case HANDLER:
            if (parent_process)
                signal(sig, signal_handler);
            kill(getpid(), sig);
            break;
        case MASK:
        case PENDING:
            if (parent_process) {
                signal(sig, signal_handler);
                kill(getpid(), sig);
                mask_signal(sig, parent_process);
                kill(getpid(), sig);
            } else {
                mask_signal(sig, false);
                kill(getpid(), sig);
            }
            break;
        case INVALID:
            printf(ANSI_COLOR_CYAN "INVALID OPERATION TYPE\n" ANSI_COLOR_RESET);
            break;
    }
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "use: program [task_type] [signal]\n");
        exit(1);
    }

    TASK_TYPE task_type = str_to_task_type(*(argv + 1));
    int signal = atoi(*(argv + 2));

    // exec process
    if (argc == 4) {
        test_signals(task_type, signal, false);
        fflush(NULL);
        exit(0);
    } else {

        test_signals(task_type, signal, true);
        fflush(NULL);

        pid_t pid = fork();
        bool parent_process = pid != 0;

        // parent and fork child
        if (parent_process) {
            wait(NULL);
            test_signals(task_type, signal, true);

            if (execl(argv[0], argv[0], "handler", "10", "hi!", NULL) == -1)
                printf("execl error occurred\n");
            exit(0);
        } else {
            test_signals(task_type, signal, false);
            exit(0);
        }
    }
}
