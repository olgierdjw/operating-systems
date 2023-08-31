#include <ncurses.h>
#include <locale.h>
#include <unistd.h>
#include <stdbool.h>
#include "grid.h"
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

char *iteration_state;
char *next_state;
char *tmp;

int threads_number = 0;
int calculated_cells_check = 0;
int requested_cells_check = 0;

typedef struct {
    pthread_t thread_index;
} thread_init_message;


typedef struct {
    int row;
    int column;
} thread_task;

sem_t *access_semaphore;

pthread_t threads[55];
bool free_threads[55];
thread_task global_task;

void increment() {
    sem_post(access_semaphore);
}

void decrement() {
    sem_wait(access_semaphore);
}

void print_sem_value(sem_t *semaphore) {
    int sem_value = 0;
    sem_getvalue(semaphore, &sem_value);
    printf("Semaphore value: %d\n", sem_value);
}

char *sem_name() {
    char *sem_code = calloc(20, sizeof(char));
    snprintf(sem_code, 20, "/%d", getpid());
    return sem_code;
}

void create_sem() {
    char *semaphore_name = sem_name();
    access_semaphore = sem_open(sem_name(), O_CREAT | O_EXCL, 0666, 1);
    if (access_semaphore == SEM_FAILED)
        perror("sem_open() failed");
    int sem_value;
    sem_getvalue(access_semaphore, &sem_value);
    printf("NEW SEMAPHORE: %s; INIT VALUE: %d\n", semaphore_name, sem_value);
    free(semaphore_name);
}

void end_game() {
    endwin(); // End curses mode
    destroy_grid(iteration_state);
    destroy_grid(next_state);
    for (int i = 0; i < threads_number; i++)
        pthread_kill(threads[i], SIGINT);
    exit(0);
}

void *thread_loop(void *arg) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // get thread id
    thread_init_message *message = (thread_init_message *) arg;
    int thread_index = message->thread_index;
    free(message);

    while (true) {
        int sig;
        sigwait(&set, &sig);

        // read available task
        decrement();
        int row = global_task.row;
        int col = global_task.column;
        increment();

        bool value = is_alive(row, col, iteration_state);

        decrement();
        calculated_cells_check++;
        next_state[row * grid_width + col] = value;
        free_threads[thread_index] = true;
        increment();
    }
}

void create_worker(int id) {
    thread_init_message *message = (thread_init_message *) malloc(sizeof(thread_init_message));
    message->thread_index = id;
    pthread_t thread;
    int result_code = pthread_create(&thread, NULL, thread_loop, message);
    if (result_code != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }
    threads[id] = thread;
}

void offer_job(int thread_id, int row, int column) {
    decrement();
    thread_task new_task = {.row = row, .column = column};
    global_task = new_task;
    pthread_kill(threads[thread_id], SIGUSR1);
    free_threads[thread_id] = false;
    requested_cells_check++;
    increment();
}

int next_free_thread(int search_from) {
    int thread_to_check = search_from;
    int result;
    while (true) {
        thread_to_check = thread_to_check % threads_number;
        decrement();
        if (free_threads[thread_to_check] == true) {
            result = thread_to_check;
            increment();
            break;
        }
        increment();
        thread_to_check++;
    }
    return result;
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Invalid argument\nUse: ./life [threads]\n");
        exit(0);
    }
    threads_number = atoi(argv[1]);
    if (threads_number > 50) {
        printf("Invalid argument\nUse: ./life [threads]\n");
        exit(0);
    }

    printf("USER PREFERENCE: THREADS: %d\n", threads_number);


    srand(time(NULL));
    setlocale(LC_CTYPE, "");
    initscr(); // Start curses mode

    signal(SIGINT, end_game);
    iteration_state = create_grid();
    next_state = create_grid();

    init_grid(iteration_state);

    create_sem();

    for (int t_id = 0; t_id < threads_number; t_id++) {
        create_worker(t_id);
        free_threads[t_id] = true;
    }

    sleep(2);


    while (true) {
        draw_grid(iteration_state);

        int last_used_thread = 0;

        // rerender each cell
        for (int r = 0; r < grid_height; r++) {
            for (int c = 0; c < grid_width; c++) {
                int free_thread = next_free_thread(last_used_thread);
                offer_job(free_thread, r, c);

                last_used_thread = free_thread;
            }
        }

        usleep(450 * 1000);
        decrement();
        calculated_cells_check = 0;
        requested_cells_check = 0;

        // all tasks assigned
        tmp = iteration_state;
        iteration_state = next_state;
        next_state = tmp;
        increment();
    }

}
