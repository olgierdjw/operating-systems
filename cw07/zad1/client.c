#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>


#define SHARED_MEMORY_KEY 9148403

#define NUM_HAIRDRESSERS 5
#define NUM_CHAIRS 2
#define NUM_WAITING 4

#define MEM_CAN_ACCESS 0
#define MEM_CLIENT_WAITING 1
#define MEM_FREE_WORKER 2
#define MEM_FREE_CHAIR 3

int shared_memory_id;
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
} shared_memory;


void increment(int sem_num) {
    struct sembuf semop_signal = {sem_num, 1, 0};
    if (semop(sem_list_id, &semop_signal, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

void decrement(int sem_num) {
    struct sembuf semop_signal = {sem_num, -1, 0};
    if (semop(sem_list_id, &semop_signal, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

void create_shared_memory(shared_memory **shared) {
    int shmid = shmget(SHARED_MEMORY_KEY, sizeof(shared_memory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    shared_memory_id = shmid;

    *shared = shmat(shmid, NULL, 0);
    if (*shared == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    (*shared)->free_chairs = NUM_CHAIRS;
    (*shared)->clients_waiting_cnt = 0;
    (*shared)->free_workers_cnt = 0;
    (*shared)->jobs = 0;


    printf("INIT DONE.\n");
}

void create_sem(int *sem_id) {
    key_t key = ftok(".", 'o');
    if (key == -1) {
        perror("ftok");
        exit(1);
    }

    *sem_id = semget(key, 4, IPC_CREAT | 0666);
    if (*sem_id == -1) {
        perror("semgett");
        exit(1);
    }


    unsigned short semvals[4] = {1, 0, NUM_HAIRDRESSERS, NUM_CHAIRS};
    if (semctl(*sem_id, 0, SETALL, semvals) == -1) {
        perror("semctl: SETALL");
        exit(1);
    }
}

void worker() {
    printf("[WORKER %d] CREATED.\n", getpid());
    while (1) {
        sleep(10);
    }

}

void client() {
    shared_memory *shared = shmat(shared_memory_id, NULL, 0);
    if (shared == (void *) -1) {
        perror("shmat");
        exit(1);
    }

    decrement(MEM_CAN_ACCESS);
    if (shared->clients_waiting_cnt >= NUM_WAITING) {
        printf("[CLIENT %d] New client. Queue full. Kill.\n", getpid());
        increment(MEM_CAN_ACCESS);
        exit(0);
    } else {
        client_order new_order = {getpid(), 10};
        shared->clients_waiting[shared->clients_waiting_cnt] = new_order;
        shared->clients_waiting_cnt++;
        increment(MEM_CLIENT_WAITING);
        printf("[CLIENT %d] New client. Waiting in queue.\n", getpid());
    }
    increment(MEM_CAN_ACCESS);
}

void client_generator() {
    int spawned_clients = 0;
    for (int iteration = 0; iteration < 9; iteration++) {
        srand(time(0));
        int pause_time = rand() % 4 + 1;
        sleep(pause_time);
        //printf("SPAWNING ITERATION %d/%d\n", iteration + 1, 5);

        srand(time(0));
        int people = rand() % 2 + 1;
        //printf("SPAWN %d MORE PEOPLE\n", people);
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
    shared_memory *shared_data = shmat(shared_memory_id, NULL, 0);
    int worker = shared_data->free_workers[0];
    for (int i = 0; i < shared_data->free_workers_cnt - 1; i++)
        shared_data->free_workers[i] = shared_data->free_workers[i + 1];
    shared_data->free_workers_cnt--;
    return worker;
}

void worker_add(int worker_id) {
    shared_memory *shared_data = shmat(shared_memory_id, NULL, 0);
    int add_worker_back_index = shared_data->free_workers_cnt;
    shared_data->free_workers[add_worker_back_index] = worker_id;
    shared_data->free_workers_cnt++;
    increment(MEM_FREE_WORKER);
}


client_order order_pop() {
    shared_memory *shared_data = shmat(shared_memory_id, NULL, 0);
    client_order order = shared_data->clients_waiting[0];
    for (int i = 0; i < shared_data->clients_waiting_cnt - 1; i++)
        shared_data->clients_waiting[i] = shared_data->clients_waiting[i + 1];
    shared_data->clients_waiting_cnt--;
    return order;
}

void start_job(client_order order, int worker_id) {
    decrement(MEM_CAN_ACCESS);
    shared_memory *shared_data = shmat(shared_memory_id, NULL, 0);
    printf("+++[%d-%d] START\n", worker_id, order.client_pid);
    // consume resources
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
}


int main(int argc, char **argv) {
    shared_memory *shared_data = NULL;
    create_shared_memory(&shared_data);

    int sem;
    create_sem(&sem);
    sem_list_id = sem;

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
