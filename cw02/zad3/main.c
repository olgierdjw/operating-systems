#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

int main() {

    DIR *wd = opendir(".");
    if (wd == NULL) {
        printf("PWD CAN NOT BE OPENED\n");
        return 0;
    }
    long long sum = 0;

    struct dirent *folder_item = readdir(wd);
    while (folder_item != NULL) {
        struct stat folder_item_stat;
        stat(folder_item->d_name, &folder_item_stat);
        if (!S_ISDIR(folder_item_stat.st_mode)) {
            printf("%s -> %ld bytes\n", folder_item->d_name, folder_item_stat.st_size);
            sum += folder_item_stat.st_size;
        }


        folder_item = readdir(wd);
    }

    printf("SUM: %lld\n", sum);
    closedir(wd);
    return 0;
}
