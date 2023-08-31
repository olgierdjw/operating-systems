#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "my_lib.h"
#include <time.h>

#include <sys/times.h>

#define MAX_LINE_LENGTH 100
#define MAX_ARRAY_SIZE 10000

//cp ../zad1/my_lib.c ./my_lib.c && make all && ./prog_static

typedef struct MainBlock MainBlock;
int array[MAX_ARRAY_SIZE];
int array_size = 0;

int MEASURE_OPERATIONS = 1000;

MainBlock **firstMainBlock = NULL;

struct timespec start, end;

static clock_t st_time;
static clock_t en_time;
static struct tms st_cpu;
static struct tms en_cpu;

void printTime() {
    double elapsed_s = (double) (end.tv_sec - start.tv_sec) +
                       (double) (end.tv_nsec - start.tv_nsec) / 1000000000.0 / MEASURE_OPERATIONS;
    printf("Elapsed time: %f s\n", elapsed_s);

    printf("Real Time: %f, User Time %f, System Time %f\n",
        (double )(en_time - st_time),
        (double)(en_cpu.tms_utime - st_cpu.tms_utime),
        (double)(en_cpu.tms_stime - st_cpu.tms_stime));
}

void timeStart() {
    clock_gettime(CLOCK_REALTIME, &start);
    st_time = times(&st_cpu);
}

void timeEnd() {
    clock_gettime(CLOCK_REALTIME, &end);
    en_time = times(&en_cpu);
    printTime();
}


void init(int size) {
    firstMainBlock = (MainBlock **) calloc(MEASURE_OPERATIONS, sizeof(MainBlock *));
    if (size <= 0 || size > MAX_ARRAY_SIZE) {
        printf("Invalid array size: %d\n", size);
        printf("MIN: 1, MAX_ARRAY_SIZE: %d\n", MAX_ARRAY_SIZE);
        return;
    }
    printf("Initializing array of size %d.\n", size);

    timeStart();
    for (int i = 0; i < MEASURE_OPERATIONS; i++) {
        *(firstMainBlock + i) = initMainBlock(size);
    }
    timeEnd();


}

void count(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Cannot open file: %s\n", filename);
        return;
    }
    fclose(fp);
    printf("Load file.\n");

    timeStart();
    for (int i = 0; i < MEASURE_OPERATIONS; i++) {
        addTextBlock(*(firstMainBlock + i), filename);
    }
    timeEnd();
}

void show(int index) {
    if (firstMainBlock == NULL) {
        printf("MainBlock init required.\n");
        return;
    }
    if (index < 0 || index >= (*firstMainBlock)->mainBlockLimit) {
        printf("Invalid index: %d\n", index);
        return;
    }

    timeStart();
    for (int i = 0; i < MEASURE_OPERATIONS; i++) {
        printTextBlock(*(firstMainBlock + i), index);
    }
    timeEnd();
}

void delete(int index) {
    if (firstMainBlock == NULL) {
        printf("MainBlock init required.\n");
        return;
    }
    printf("Deleting values from index %d.\n", index);
    timeStart();
    for (int i = 0; i < MEASURE_OPERATIONS; i++) {
        deleteTextBlock(*(firstMainBlock + i), index);
    }
    timeEnd();

}

void destroy() {
    if (firstMainBlock != NULL) {
        printf("Destroying array.\n");
        timeStart();
        for (int i = 0; i < MEASURE_OPERATIONS; i++) {
            deleteMainBlock(*(firstMainBlock + i));
        }
        firstMainBlock = NULL;
        timeEnd();
    }
}

int main() {
    char command[MAX_LINE_LENGTH];
    int arg1;

    while (1) {
        printf("Enter command: ");
        fgets(command, MAX_LINE_LENGTH, stdin);

        if (strncmp(command, "init", 4) == 0) {
            if (sscanf(command + 4, "%d", &arg1) != 1) {
                printf("Invalid argument: %s\n", command + 4);
                continue;
            }
            init(arg1);
        } else if (strncmp(command, "count", 5) == 0) {
            char fileName[20];
            if (sscanf(command + 5, "%s", fileName) != 1) {
                printf("Invalid argument: %s\n", command + 5);
                continue;
            }
            count(fileName);
        } else if (strncmp(command, "show", 4) == 0) {
            if (sscanf(command + 4, "%d", &arg1) != 1) {
                printf("Invalid argument: %s\n", command + 4);
                continue;
            }
            show(arg1);
        } else if (strncmp(command, "delete", 6) == 0) {
            if (sscanf(command + 6, "%d", &arg1) != 1) {
                printf("Invalid argument: %s\n", command + 6);
                continue;
            }
            delete(arg1);
        } else if (strncmp(command, "destroy", 7) == 0) {
            destroy();

        } else if (strncmp(command, "exit", 4) == 0) {
            exit(1);
        } else {
            printf("Invalid command.\n");
            printf("init size, count file, show index, delete index index, destroy\n");
        }
    }
}