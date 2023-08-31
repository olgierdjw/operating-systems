#include <stdio.h>
#include <ftw.h>

long long global_sum = 0;

int list_files(const char *path, const struct stat *statbuf, int typeflag) {
    if (typeflag == FTW_F) {
        printf("%s -> %ld bytes\n", path, statbuf->st_size);
        global_sum += statbuf->st_size;
    }
    return 0;
}

void sum_bytes(char *start_path) {
    global_sum = 0;
    int max_depth = 10000;
    int ret = ftw(start_path, list_files, max_depth);
    if (ret == -1) {
        printf("Error.");
    }
    printf("TOTAL BYTES: %lld\n", global_sum);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("use %s [path]\n", argv[0]);
        return 1;
    }
    sum_bytes(argv[1]);
}
