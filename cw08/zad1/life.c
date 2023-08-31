#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <stdbool.h>
#include "grid.h"
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>

char *iteration_state;
char *next_state;
char *tmp;

pthread_t threads[grid_height * grid_width];

typedef struct {
    int row;
    int column;
} thread_init_message;

void end_game() {
    endwin(); // End curses mode
    destroy_grid(iteration_state);
    destroy_grid(next_state);
    for (int i = 0; i < grid_width * grid_height; i++)
        pthread_kill(threads[i], SIGINT);

    exit(0);
}

void sig_handler(int signal) {
}

void *thread_loop(void *arg) {
    thread_init_message *init_data = (thread_init_message *) arg;
    int row = init_data->row;
    int col = init_data->column;
    signal(SIGUSR1, sig_handler);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    int sig;

    while (true) {
        sigwait(&set, &sig);
        next_state[row * grid_width + col] = is_alive(row, col, iteration_state);
    }

}

int main() {
    srand(time(NULL));
    setlocale(LC_CTYPE, "");
    initscr(); // Start curses mode

    signal(SIGINT, end_game);
    iteration_state = create_grid();
    next_state = create_grid();

    init_grid(iteration_state);

    for (int i = 0; i < grid_height; ++i) {
        for (int j = 0; j < grid_width; ++j) {
            thread_init_message args = {i, j};
            pthread_t thread;
            int rc = pthread_create(&thread, NULL, thread_loop, (void *) &args);
            if (rc != 0) {
                fprintf(stderr, "Error creating thread\n");
                exit(EXIT_FAILURE);
            }
            int map_position = i * grid_width + j;
            threads[map_position] = thread;
        }
    }

    while (true) {
        draw_grid(iteration_state);
        usleep(500 * 1000);

        // simulation step
        for (int i = 0; i < grid_width * grid_height; i++)
            pthread_kill(threads[i], SIGUSR1);


        tmp = iteration_state;
        iteration_state = next_state;
        next_state = tmp;
    }

}
