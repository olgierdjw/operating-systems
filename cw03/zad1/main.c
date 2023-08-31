#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Argument required!\n");
        return 1;
    }

    int cnt = 0;
    if (sscanf(*(argv + 1), "%d", &cnt) != 1) {
        printf("Argument required!\n");
        return 1;
    }

    printf("Hello world!\n");

    id_t parent_id = getpid();

    for (int i = 0; i < cnt; i++) {
        id_t pid = fork();

        int is_child_process = pid == 0;
        if (is_child_process) {
            printf("CHILD: parent id: %d, my id: %d\n", (int) getppid(), (int) getpid());
            return 0;
        }
    }


    for (int i = 0; i < cnt; i++)
        wait(NULL);

    printf("PARENT: %s, id: %d\n", *argv, parent_id);
    return 0;

}
