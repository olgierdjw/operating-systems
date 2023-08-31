#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define SIGNAL 10


void siginfo_handler(int signo, siginfo_t *info, void *context) {
    printf("Signal number:%d\n", info->si_signo);
    printf("PID:%d\n", info->si_pid);
    printf("UID:%u\n", info->si_uid);
    printf("POSIX timer ID:%d\n", info->si_timerid);
}


void testSIGINFO() {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = siginfo_handler;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGNAL, &action, NULL);
    printf("PARENT:\n");
    kill(getpid(), SIGNAL);


    printf("CHILD:\n");

    bool child_process = fork() == 0;

    if (child_process) {
        kill(getpid(), SIGNAL);
        exit(0);
    } else
        wait(NULL);

}

void resethand_handler(int sig, siginfo_t *info, void *context) {
    printf("[%d] signal: %d\n", getpid(), sig);
}

void testRESETHAND() {
    printf(ANSI_COLOR_GREEN "TEST RESETHAND\n" ANSI_COLOR_RESET, getpid());
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = resethand_handler;
    action.sa_flags = SA_RESETHAND;
    sigaction(SIGNAL, &action, NULL);
    printf("SIGNAL SENT!\n");
    kill(getpid(), SIGNAL);
    printf("SIGNAL SENT!\n");
    printf("EXPECTING ERROR, SIGNAL %d!\n", SIGNAL);
    kill(getpid(), SIGNAL);
}


void onehot_handler(int signum) {
    printf("Signal %d.\n", signum);

}

void testONESHOT() {
    struct sigaction action;
    action.sa_handler = onehot_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_ONESHOT;
    sigaction(SIGNAL, &action, NULL);

    bool child_process = fork() == 0;
    if (child_process) {
        for (int i = 0; i < 40; i++) {
            kill(getpid(), SIGNAL);
        }
    } else
        wait(NULL);
}

int main(int argc, char **argv) {
    printf(ANSI_COLOR_CYAN "parent pid: %d\n" ANSI_COLOR_RESET, getpid());

    testSIGINFO();
    testONESHOT();
    testRESETHAND();

}
