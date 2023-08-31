#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>


#define SHARED_MEMORY_NAME "/shared-memory-lab7"

#define NUM_HAIRDRESSERS 5
#define NUM_CHAIRS 2
#define NUM_WAITING 4

#define MEM_CAN_ACCESS 0
#define MEM_CLIENT_WAITING 1
#define MEM_FREE_WORKER 2
#define MEM_FREE_CHAIR 3

int sem_list_id;

typedef struct {
    int client_pid;
    int required_time;
} client_order;

typedef struct {
    int free_workers[NUM_HAIRDRESSERS];
    int free_workers_cnt;
    client_order clients_waiting[NUM_WAITING];
    int clients_waiting_cnt;
    int free_chairs;
    int jobs; // should be 0 at the end
} shared_data;

typedef struct {
    sem_t *sem[4];
} semaphores;

semaphores *global_sems;


void increment(int sem_num) {
    sem_post(global_sems->sem[sem_num]);
}

void decrement(int sem_num) {
    sem_wait(global_sems->sem[sem_num]);
}

shared_data *load_shared_memory() {
    int shared_memory_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR, 0666);
    if (shared_memory_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    shared_data *shared_object = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd,
                                      0);
    if (shared_object == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    return shared_object;
}

shared_data *create_shared_memory() {
    int shared_memory_fd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (shared_memory_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    ftruncate(shared_memory_fd, sizeof(shared_data));

    shared_data *shared_object = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_fd,
                                      0);
    if (shared_object == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    (shared_object)->free_chairs = NUM_CHAIRS;
    (shared_object)->clients_waiting_cnt = 0;
    (shared_object)->free_workers_cnt = 0;
    (shared_object)->jobs = 0;

    printf("INIT DONE.\n");
    return shared_object;
}

char *concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

char *sem_name(int index) {
    char sem_code[2];
    snprintf(sem_code, 2, "%d", index);
    return concat("/semaphore-", sem_code);
}

void create_sem() {

    global_sems = calloc(1, sizeof(semaphores));
    unsigned short sem_init_values[4] = {1, 0, NUM_HAIRDRESSERS, 2};
    for (int i = 0; i < 4; i++) {
        char *i_sem_name = sem_name(i);
        sem_t *my_semaphore = sem_open(i_sem_name, O_CREAT | O_EXCL, 0666, sem_init_values[i]);
        if (my_semaphore == SEM_FAILED)
            perror("sem_open() failed");

        int sem_value = 0;
        sem_getvalue(my_semaphore, &sem_value);
        printf("NEW SEMAPHORE: %s; WITH VALUE: %d\n", i_sem_name, sem_value);

        global_sems->sem[i] = my_semaphore;
        free(i_sem_name);
    }


}

void worker() {
    printf("[WORKER %d] CREATED.\n", getpid());
    while (1) {
        sleep(10);
    }

}

void client() {
    shared_data *shared = load_shared_memory();

    decrement(MEM_CAN_ACCESS);
    if (shared->clients_waiting_cnt >= NUM_WAITING) {
        printf("[CLIENT %d] New client. Queue full. Kill.\n", getpid());
        increment(MEM_CAN_ACCESS);
        exit(0);
    } else {
        srand(time(NULL));
        int random_value = rand() % 10 + 2;
        printf("Random value: %d\n", random_value);
        client_order new_order = {getpid(), random_value};
        shared->clients_waiting[shared->clients_waiting_cnt] = new_order;
        shared->clients_waiting_cnt++;
        increment(MEM_CLIENT_WAITING);
        printf("[CLIENT %d] New client. Waiting in queue. Queue: %d\n", getpid(), shared->clients_waiting_cnt);
    }
    increment(MEM_CAN_ACCESS);
}

void client_generator() {
    int spawned_clients = 0;
    for (int iteration = 0; iteration < 9; iteration++) {
        srand(time(0));
        int pause_time = rand() % 4 + 1;
        sleep(pause_time);
        srand(time(0));
        int people = rand() % 2 + 1;
        for (int c = 0; c < people; c++) {
            spawned_clients++;
            if (fork() == 0) {
                client();
                exit(0);
            }
            sleep(1);
        }

    }
    printf("SENT %d CLIENTS. GENERATOR STOPPED.\n", spawned_clients);
}


int worker_pop() {
    shared_data *shared_data = load_shared_memory();
    int worker = shared_data->free_workers[0];
    for (int i = 0; i < shared_data->free_workers_cnt - 1; i++)
        shared_data->free_workers[i] = shared_data->free_workers[i + 1];
    shared_data->free_workers_cnt--;
    return worker;
}

void worker_add(int worker_id) {
    shared_data *shared_data = load_shared_memory();
    int add_worker_back_index = shared_data->free_workers_cnt;
    shared_data->free_workers[add_worker_back_index] = worker_id;
    shared_data->free_workers_cnt++;
    increment(MEM_FREE_WORKER);
}


client_order order_pop() {
    shared_data *shared_data = load_shared_memory();
    client_order order = shared_data->clients_waiting[0];
    for (int i = 0; i < shared_data->clients_waiting_cnt - 1; i++)
        shared_data->clients_waiting[i] = shared_data->clients_waiting[i + 1];
    shared_data->clients_waiting_cnt--;
    return order;
}

void start_job(client_order order, int worker_id) {
    decrement(MEM_CAN_ACCESS);
    shared_data *shared_data = load_shared_memory();
    printf("NEW JOB: worker: %d, client: %d\n", worker_id, order.client_pid);
    shared_data->jobs++;
    shared_data->free_chairs--;
    printf("orders left: %d, free workers: %d, free chairs: %d\n",
           shared_data->clients_waiting_cnt,
           shared_data->free_workers_cnt,
           shared_data->free_chairs);

    increment(MEM_CAN_ACCESS);

    sleep(order.required_time);


    // restore resources
    decrement(MEM_CAN_ACCESS);
    shared_data->jobs--;
    printf("---[%d-%d] END (status: %d)\n", worker_id, order.client_pid, shared_data->jobs);

    worker_add(worker_id);

    shared_data->free_chairs++;
    increment(MEM_FREE_CHAIR);


    increment(MEM_CAN_ACCESS);

    munmap(shared_data, sizeof(shared_data));
}

void clean() {
    for (int i = 0; i < 4; ++i) {
        sem_close(global_sems->sem[i]);
        sem_unlink(sem_name(i));
    }
    free(global_sems);
    exit(1);
}


int main(int argc, char **argv) {
    signal(SIGINT, clean);
    for (int i = 0; i < 4; ++i) {
        sem_unlink(sem_name(i));
    }
    shared_data *shared_mem = create_shared_memory();
    printf("shared data test: %d\n", shared_mem->free_chairs);

    create_sem();

    for (int worker_id = 0; worker_id < NUM_HAIRDRESSERS; worker_id++) {
        if (fork() != 0) {
            worker_add(getpid());
            worker();
            exit(0);
        } else {
            wait(NULL);
        }
    }


    if (fork() == 0) {
        client_generator();
        exit(0);
    }


    printf("MAIN LOOP START.\n");
    while (1) {
        decrement(MEM_CLIENT_WAITING);
        decrement(MEM_FREE_WORKER);
        decrement(MEM_FREE_CHAIR);
        decrement(MEM_CAN_ACCESS);


        int worker = worker_pop();
        client_order order = order_pop();
        increment(MEM_CAN_ACCESS);

        if (fork() == 0) {
            start_job(order, worker);
            exit(0);
        }
    }

}
