#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>

void char_by_char(char *read_from, char *save_to) {
    FILE *file_read = fopen(read_from, "r");
    if (file_read == NULL) {
        printf("FILE DOES NOT EXIST.");
        return;
    }

    int free_line_index = 0;
    char **lines = calloc(1000, sizeof(char *));
    int *sizes = calloc(1000, sizeof(int));

    int i = 0;
    char *tmp_line = (char *) malloc(1000 * sizeof(char));
    while (1) {
        char red_char;
        fread(&red_char, 1, 1, file_read);
        if (feof(file_read)) break;

        if (red_char == '\n') {
            char *reversed_line = (char *) malloc((i + 1) * sizeof(char));
            *(sizes + free_line_index) = i + 1;
            *(lines + free_line_index++) = reversed_line;
            for (int y = 0; y < i; y++)
                *(reversed_line + i - y - 1) = tmp_line[y];
            *(reversed_line + i) = '\n';
            i = 0;
            continue;
        }
        *(tmp_line + i++) = red_char;
    }
    FILE *file_save = fopen(save_to, "w+");
    for (int l = free_line_index - 1; l >= 0; l--) {
        fwrite(*(lines + l), sizeof(char), *(sizes + l), file_save);
        free(*(lines + l));
    }
    free(lines);
    free(sizes);
    free(tmp_line);
    fclose(file_read);
    fclose(file_save);

}

void read_1024(char *read_from, char *save_to) {
    int block_size = 1024;
    FILE *file_read = fopen(read_from, "r");
    if (file_read == NULL) {
        printf("FILE DOES NOT EXIST.");
        return;
    }

    int current_line = 0;
    int empty_char_i = 0;
    char **to_save = calloc(2000, sizeof(char *));
    int *line_size = malloc(2000 * sizeof(int));


    char *reading_line = (char *) malloc(2000 * sizeof(char));
    while (1) {
        char *file_block = malloc(block_size * sizeof(char));
        if (file_block == NULL)
            printf("FILE BLOCK NOT CREATED");
        fread(file_block, sizeof(char), block_size, file_read);

        for (int char_index = 0; char_index < block_size; char_index++) {
            char red_char = *(file_block + char_index);

            if (red_char == '\n') { // new_line push
                *(line_size + current_line) = empty_char_i + 1; // +1 for '\n'
                *(to_save + current_line) = reading_line;

                char *rotate = malloc(block_size * sizeof(char));
                for (int i = 0; i < empty_char_i; i++)
                    *(rotate + i) = *(reading_line + i);

                for (int y = 0; y < empty_char_i; y++)
                    *(reading_line + empty_char_i - y - 1) = *(rotate + y);

                free(rotate);

                *(reading_line + empty_char_i) = '\n';
                empty_char_i = 0;

                reading_line = (char *) malloc(block_size * sizeof(char)); // reset bufor
                current_line += 1;
            } else // append to existing new_line
                *(reading_line + empty_char_i++) = red_char;
        }

        if (feof(file_read)) break;
    }

    FILE *file_save = fopen(save_to, "w+");
    for (int l = current_line - 1; l >= 0; l--) {
        fwrite(*(to_save + l), sizeof(char), *(line_size + l), file_save);
        free(*(to_save + l));
    }
    free(to_save);
    free(line_size);
    fclose(file_save);
    fclose(file_read);

}

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

int main() {
    int tests = 10000;
    char filename[] = "main.c";
    timeStart();
    for (int i = 0; i < tests; i++)
        char_by_char(filename, "char_by_char.txt");
    timeEnd();
    timeStart();
    for (int i = 0; i < tests; i++)
        read_1024(filename, "block_1024.txt");
    timeEnd();
    return 0;
}
