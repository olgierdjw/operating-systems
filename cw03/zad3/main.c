#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

char *cmp_string;

void create_directory_process(char *);

void directory_process_task(char *);

char *folder_item_path(char *, char *);

int should_consider(char *folder_item_name) {
    // '..'
    if (folder_item_name[0] == '.' && folder_item_name[1] == '.')
        return 0;
    // block '.'
    if (folder_item_name[0] == '.' && folder_item_name[1] == 0)
        return 0;
    return 1;
}


void create_directory_process(char *dir) {
    id_t pid = fork();

    int is_child_process = pid == 0;
    if (is_child_process) {
        directory_process_task(dir);
    }

    if (pid != 0) {
        wait(NULL);
    }
}

void directory_process_task(char *dir) {
    DIR *wd = opendir(dir);
    if (wd == NULL) {
        return;
    }
    printf(ANSI_COLOR_GREEN  "new path (%s) process pid: %d\n"  ANSI_COLOR_RESET, dir, getpid());

    struct dirent *folder_item = readdir(wd);
    while (folder_item != NULL) {
        char *full_path = folder_item_path(dir, folder_item->d_name);

        struct stat folder_item_stats;
        stat(folder_item->d_name, &folder_item_stats);
        if (stat(full_path, &folder_item_stats) == -1) {
            perror("ERROR! stat function failed.\n");
            continue;
        }

        // ignore .. and .
        if (should_consider(folder_item->d_name)) {
            // new process
            if (S_ISDIR(folder_item_stats.st_mode)) {
                char *item_path = folder_item_path(dir, folder_item->d_name);
                create_directory_process(item_path);
                free(item_path);
            }

            // just a file
            else {
                FILE *file = fopen(full_path, "r");
                if (file != NULL) {
                    char *bufor = calloc(255, 1);
                    fread(bufor, 255, 1, file);
                    int byte_to_cmp = (int) strlen(cmp_string);
                    if (strncmp(cmp_string, bufor, byte_to_cmp) == 0) {
                        printf(ANSI_COLOR_CYAN "pid %d, file: %s\n" ANSI_COLOR_RESET, getpid(), full_path);
                    }
                }
                fclose(file);
            }
        }
        folder_item = readdir(wd);
    }
    closedir(wd);
    exit(0);
}

char *folder_item_path(char *folder_name, char *file_name) {
    char *result = (char *) calloc(PATH_MAX, sizeof(char));
    int free_index = 0;
    for (int i = 0; i < PATH_MAX; i++) {
        char c = *(folder_name + i);
        if (c == '\0') {
            free_index = i;
            break;
        }
        result[i] = c;
    }
    result[free_index++] = '/';
    for (int i = 0; i < PATH_MAX; i++) {
        char c = *(file_name + i);
        if (c == '\0')
            break;
        result[free_index++] = c;
    }
    result[free_index] = '\0';
    return result;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Argument required!\n");
        return 1;
    }

    char *directory_path = *(argv + 1);
    cmp_string = *(argv + 2);

    create_directory_process(directory_path);
    exit(0);
}
