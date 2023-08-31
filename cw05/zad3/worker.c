#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>


double f(double x) {
    return 4.0 / (x * x + 1.0);
}


double integral(double x0, double x1, double dx) {
    double sum = 0.0;
    for (double x = x0; x < x1; x += dx) {
        sum += f((x + (x + dx)) / 2) * dx;
    }
    return sum;
}


int main(int argc, char **argv) {
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    double dx = atof(argv[3]);
    double result = integral(a, b, dx);
    int fifo_fd = open("myFIFO", O_WRONLY);
    write(fifo_fd, &result, sizeof(result));
    close(fifo_fd);
}

