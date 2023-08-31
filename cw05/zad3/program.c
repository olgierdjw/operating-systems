#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/times.h>
#include <fcntl.h>
#include <sys/stat.h>

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


int main(int argc, char **argv) {

    if (argc != 3) {
        printf("Invalid arguments.");
        exit(0);
    }

    setbuf(stdout, NULL);

    double dx = strtod(argv[1], NULL);
    int n = atoi(argv[2]);

    printf("%d %.10f\n", n, dx);

    double width = 1.0 / n;


    if (mkfifo("myFIFO", 0777) == -1) {
        printf("Can't create pipe.\n");
        exit(1);
    }


    double sum = 0.0;


    time_start();
    for (int repeat = 0; repeat < 10; repeat++) {
        sum = 0;
        for (int i = 0; i < n; i++) {
            if (fork() == 0) {
                char newArg1[20];
                double a = i * width;
                sprintf(newArg1, "%f", a);
                char newArg2[20];
                double b = (i + 1) * width;
                sprintf(newArg2, "%f", b);

                execl("./worker", "worker", newArg1, newArg2, argv[1], NULL);
                exit(0);
            }
        }


        int fd;
        if ((fd = open("myFIFO", O_RDONLY)) == -1) {
            printf("Pipe open failed.\n");
            exit(1);
        }
        for (int i = 0; i < n; i++)
            wait(NULL);


        double number;
        for (int i = 0; i < n; i++) {
            read(fd, &number, sizeof(double));
            sum += number;
        }

        close(fd);
    }
    time_end();


    printf("RESULT: %f\n\n", sum);
    unlink("myFIFO");

    return 0;
}
