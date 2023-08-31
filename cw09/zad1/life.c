#include <locale.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

typedef struct {
    int elf_id;
    int time;
} elf_task;

typedef struct {
    int thread_id;
} thread_init_message;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define REINDEER_NUM 9
#define ELF_NUM 10
#define ELF_QUEUE_LIMIT 3

// santa tasks
sem_t santa_jobs_cnt;
bool santa_is_working;

// delivery
pthread_cond_t santa_delivering = PTHREAD_COND_INITIALIZER;
pthread_t santa;
pthread_t reindeers[REINDEER_NUM];
pthread_t elfs[ELF_NUM];

// problems
pthread_cond_t santa_fixing_problems = PTHREAD_COND_INITIALIZER;
int reindeers_in_queue = 0;
int santa_delivery_cnt = 0;
int santa_working_time = 0;

// elfs
int elfs_in_queue = 0;
elf_task elf_tasks[ELF_QUEUE_LIMIT];

int random_time(int min, int max) {
    int range = max - min + 1;
    return rand() % range + min;
}


void simulation_end_check() {
    if (santa_delivery_cnt < 3)
        return;

    printf("[SIMULATION]: END\n");
    exit(0);
}

void handle_elfs() {
    for (int i = 0; i < ELF_QUEUE_LIMIT; i++) {
        elf_task problem = elf_tasks[i];
        printf("[SANTA]: FIXING PROBLEM FROM ELF:%d \n", problem.elf_id);
        pthread_mutex_unlock(&mutex);
        sleep(problem.time);
        pthread_mutex_lock(&mutex);
    }
}

_Noreturn void *santa_task(void *args) {
    new_iteration:

    sem_wait(&santa_jobs_cnt);
    printf("[SANTA]: WAKING UP\n");
    pthread_mutex_lock(&mutex);

    if (elfs_in_queue == ELF_QUEUE_LIMIT) {
        santa_is_working = true;
        elfs_in_queue = 0;
        pthread_cond_broadcast(&santa_fixing_problems);
        handle_elfs();
    } else if (reindeers_in_queue >= 9) {
        santa_is_working = true;
        reindeers_in_queue -= 9;
        pthread_cond_broadcast(&santa_delivering);
        printf("[SANTA]: WORKING [%d/%d]\n", ++santa_delivery_cnt, 3);
        santa_working_time = random_time(2, 4);
        sleep(santa_working_time);
    }

    printf("[SANTA]: FALLING ASLEEP\n");

    santa_is_working = false;
    simulation_end_check();
    pthread_mutex_unlock(&mutex);


    goto new_iteration;
}

_Noreturn void *elf_taks(void *args) {
    thread_init_message *message = args;
    int elf_id = message->thread_id;
    free(message);

    new_iteration:
    sleep(random_time(2, 5));

    pthread_mutex_lock(&mutex);

    bool can_wait_in_queue = elfs_in_queue + 1 <= 3;
    if (can_wait_in_queue && !santa_is_working) {
        int task_insert_index = elfs_in_queue;
        elfs_in_queue++;
        printf("[ELF %d]: %d elf(s) waiting\n", elf_id, elfs_in_queue);

        elf_task my_task = {elf_id, random_time(1, 2)};
        elf_tasks[task_insert_index] = my_task;

        if (elfs_in_queue == 3) {
            printf("[ELF %d]: santa notified\n", elf_id);
            sem_post(&santa_jobs_cnt);
        }

    } else {
        printf("[ELF %d]: fixing on my own\n", elf_id);
        pthread_mutex_unlock(&mutex);
        goto new_iteration;
    }


    pthread_cond_wait(&santa_fixing_problems, &mutex);
    int santa_time = santa_working_time;

    pthread_mutex_unlock(&mutex);
    sleep(santa_time);
    //printf("[ELF %d]: santa fixed my problem\n", elf_id);
    goto new_iteration;
}

_Noreturn void *reindeer_taks(void *args) {
    thread_init_message *message = args;
    int reindeer_id = message->thread_id;
    free(message);

    new_iteration:

    sleep(random_time(5, 10));
    pthread_mutex_lock(&mutex);
    printf("[REINDEER %d]: %d reindeer(s) waiting\n", reindeer_id, ++reindeers_in_queue);
    if (reindeers_in_queue == 9) {
        printf("[REINDEER %d]: santa notified\n", reindeer_id);
        //pthread_cond_broadcast(&santa_consider_working);
        sem_post(&santa_jobs_cnt);
    }

    bool will_work_next_santa_iteration = reindeers_in_queue <= 9;
    if (will_work_next_santa_iteration) {
        //printf("[REINDEER %d]: WAITING FOR SANTA START\n", reindeer_id);
        pthread_cond_wait(&santa_delivering, &mutex);

    } else {
        //printf("[REINDEER %d]: QUEUE FULL, WAITING FOR NEXT SANTA START\n", reindeer_id);
        pthread_cond_wait(&santa_delivering, &mutex);
        //printf("[REINDEER %d]: WAITING FOR SANTA START\n", reindeer_id);
        pthread_cond_wait(&santa_delivering, &mutex);
    }
    int sleep_time = santa_working_time;
    pthread_mutex_unlock(&mutex);
    //printf("[REINDEER %d]: working with santa\n", reindeer_id);
    sleep(sleep_time);

    goto new_iteration;
}

void create_threads() {
    pthread_create(&santa, NULL, santa_task, NULL);

    for (int r = 0; r < REINDEER_NUM; r++) {
        thread_init_message *message = malloc(sizeof(message));
        message->thread_id = r;
        pthread_create(reindeers + r, NULL, reindeer_taks, message);
    }

    for (int e = 0; e < ELF_NUM; e++) {
        thread_init_message *message = malloc(sizeof(message));
        message->thread_id = e;
        pthread_create(elfs + e, NULL, elf_taks, message);
    }
}


int main() {
    srand(time(NULL));
    sem_init(&santa_jobs_cnt, 0, 0);

    santa_is_working = false;
    create_threads();
    printf("[SIMULATION]: START\n");
    pthread_join(santa, NULL);
}
