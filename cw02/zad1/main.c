#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define MEASURE_OPERATIONS 10000
#define TERMINAL_COMMAND_MAX_LEN 1024
#define FILENAME_MAX_LEN 100
#define MAX_FILE_SIZE 100000


static clock_t st_time;
static clock_t en_time;
static struct tms st_cpu;
static struct tms en_cpu;


void printTime() {
    printf("Real Time: %f, User Time %f, System Time %f\n",
           (double) (en_time - st_time),
           (double) (en_cpu.tms_utime - st_cpu.tms_utime),
           (double) (en_cpu.tms_stime - st_cpu.tms_stime));
}

void timeStart() {
    st_time = times(&st_cpu);
}

void timeEnd() {
    en_time = times(&en_cpu);
    printTime();
}


long get_file_size(char *filename) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}


void sys_version(char before, char after, char *sourceFile, char *destinationFile) {
    char c;
    int we, wy;
    we = open(sourceFile, O_RDONLY);
    wy = open(destinationFile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    while (read(we, &c, 1) == 1) {
        if (c == before)
            write(wy, &after, 1);
        else
            write(wy, &c, 1);
    }
}

void lib_version(char before, char after, char *sourceFile, char *destinationFile) {
    FILE *file_read = fopen(sourceFile, "r");
    FILE *file_save = fopen(destinationFile, "w+");

    if (file_read == NULL || file_save == NULL) {
        printf("FILE DOES NOT EXIST.\n");
        return;
    }

    size_t written_characters = 0;
    for (int i = 0; i < get_file_size(sourceFile); i++) {
        char red;
        size_t red_characters = fread(&red, sizeof(char), 1, file_read);
        if (red == before)
            written_characters = fwrite(&after, sizeof(char), 1, file_save);
        else
            written_characters = fwrite(&red, sizeof(char), 1, file_save);
        if (written_characters != red_characters) {
            printf("error: written_characters != red_characters\n");
            exit(1);
        }

    }
    fclose(file_read);
    fclose(file_save);
}


int main() {
    char command[TERMINAL_COMMAND_MAX_LEN];
    char char_before;
    char char_after;
    char *filenameSource = calloc(FILENAME_MAX_LEN, sizeof(char));
    char *filenameDestination = calloc(FILENAME_MAX_LEN, sizeof(char));

    while (1) {
        printf("COMMAND: ");
        fgets(command, TERMINAL_COMMAND_MAX_LEN, stdin);
        if (strncmp(command, "change1 ", 8) == 0) {
            if (sscanf(command + 8, "%c %c %s %s", &char_before, &char_after, filenameSource, filenameDestination) !=
                4) {
                printf("INVALID ARGUMENTS.\n");
                continue;
            }
            printf("MEASURE LIB VERSION FREAD() FWRITE()\n");
            timeStart();
            for (unsigned int i = 0; i < MEASURE_OPERATIONS; i++)
                lib_version(char_before, char_after, filenameSource, filenameDestination);
            timeEnd();

        } else if (strncmp(command, "change2 ", 8) == 0) {
            if (sscanf(command + 8, "%c %c %s %s", &char_before, &char_after, filenameSource, filenameDestination) !=
                4) {
                printf("INVALID ARGUMENTS.\n");
                continue;
            }
            printf("MEASURE SYS VERSION READ() WRITE()\n");
            timeStart();
            for (unsigned int i = 0; i < MEASURE_OPERATIONS; i++)
                sys_version(char_before, char_after, filenameSource, filenameDestination);
            timeEnd();

        } else if (strncmp(command, "exit", 4) == 0) {
            printf("BYE!.\n");
            exit(0);
        } else {
            printf("INVALID COMMAND.\n");
            printf("Use |change1 [char] [char] [filename.txt] [filename.txt]|\n");
            printf("or Use |change2 [char] [char] [filename.txt] [filename.txt]| to run system version.\n");
        }
    }
}