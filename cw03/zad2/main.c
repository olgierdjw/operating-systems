#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Argument required!\n");
        return 1;
    }

    char *directory_path = *(argv + 1);
    printf("directory_path: |%s| ", directory_path);
    fflush(stdout);
    int error_occurred = execl("/bin/ls", "ls", "-la", directory_path, NULL);
    if (error_occurred) {
        printf("execl error");
        return -1;
    }
    return 0;
}
