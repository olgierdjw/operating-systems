CC=gcc
CFLAGS=-Wall -Werror

all: static_lib dynamic_lib

static_lib: my_lib.o
	ar rcs libmy_lib.a my_lib.o

dynamic_lib: my_lib.o
	$(CC) -shared -o libmy_lib.so my_lib.o

my_lib.o: my_lib.c
	$(CC) $(CFLAGS) -fPIC -c my_lib.c

clean:
	rm -f *.o *.a *.so my_lib
