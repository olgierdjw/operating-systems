#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/times.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

static clock_t start_time, end_time;
static struct tms start_cpu, end_cpu;

void time_start() {
    start_time = times(&start_cpu);
}

void time_end() {
    end_time = times(&end_cpu);
    printf("real time: %lfs\nuser time: %lfs\nsystem time: %lfs\n",
           (double) (end_time - start_time) / sysconf(_SC_CLK_TCK),
           (double) (end_cpu.tms_utime - start_cpu.tms_utime) / sysconf(_SC_CLK_TCK),
           (double) (end_cpu.tms_stime - start_cpu.tms_stime) / sysconf(_SC_CLK_TCK));
}

double f(double x) {
    return 4.0 / (x * x + 1.0);
}


double integral(double x0, double x1, double dx) {
    double sum = 0.0;
    for (double x = x0; x < x1; x += dx) {
        sum += f((x + x + dx) / 2) * dx;
    }
    return sum;
}

void sendResult(int pipe_write, double value) {
    char value_string[20];
    size_t size = sprintf(value_string, "%f", value);
    write(pipe_write, value_string, size);
}

double calculate(double n, double dx, double intervals) {
    double sum = 0.0;
    int *pipe_read = calloc(n, sizeof(int));

    for (int i = 0; i < n; i++) {
        int fd[2];
        pipe(fd);
        pid_t pid = fork();
        if (pid == 0) {
            close(fd[0]);
            sendResult(fd[1], integral(i * intervals, (i + 1) * intervals, dx));
            close(fd[1]);
            exit(0);
        } else {
            close(fd[1]);
            pipe_read[i] = fd[0];
        }
    }

    while (wait(NULL) > 0);

    for (int i = 0; i < n; i++) {
        char message[20];
        read(pipe_read[i], message, 20);
        close(pipe_read[i]);
        double partial = atof(message);
        sum += partial;
    }
    free(pipe_read);
    return sum;
}


int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    if (argc < 2) {
        printf("INVALID ARGUMENTS\n");
        return -1;
    }
    double dx = atof(argv[1]);
    int n = atoi(argv[2]);
    double intervals = 1.0 / n;
    double sum = 0.0;

    printf("%d %.10f\n", n, dx);

    time_start();
    for (int i = 0; i < 10; i++)
        sum = calculate(n, dx, intervals);
    time_end();

    printf("RESULT: %f\n\n", sum);
    return 0;
}
