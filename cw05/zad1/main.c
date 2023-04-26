#include <stdio.h>
#include <string.h>


#define MAX_LENGTH 4096

char buffer[4096];


void read_text() {
    FILE *pipe_read_file = popen("fortune", "r");
    unsigned int characters = fread(buffer, sizeof(char), MAX_LENGTH, pipe_read_file);
    buffer[characters] = '\0';
    pclose(pipe_read_file);
}

void save_text() {
    FILE *ptr_pipe_file = popen("cowsay", "w");
    fwrite(buffer, sizeof(char), strlen(buffer), ptr_pipe_file);
    pclose(ptr_pipe_file);
}

int main(int argc, char **argv) {
    read_text();
    save_text();

    return 0;
}



