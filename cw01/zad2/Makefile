CFLAGS = -g

make: static/program_static shared/program_shared

test: static/program_static
	./static/program_static init 10

# static
my_lib.o: my_lib.c
	gcc $(CFLAGS) -c my_lib.c -o my_lib.o

my_lib.a: my_lib.o
	ar rcs my_lib.a my_lib.o

main.o: main.c
	gcc $(CFLAGS) -c main.c -o main.o

static/program_static: my_lib.a main.o
	mkdir -p static
	gcc $(CFLAGS) main.o my_lib.a -o static/program_static

# shared
shared/my_lib.o: my_lib.c
	mkdir -p shared
	gcc -c -fPIC my_lib.c -o shared/my_lib.o

libmylib.so: shared/my_lib.o
	gcc -shared -o libmylib.so shared/my_lib.o

shared/program_shared: libmylib.so
	gcc main.c -o shared/program_shared -L. -lmylib -Wl,-rpath=`pwd`

clean:
	rm -fr *.o *.a static shared results_static.txt
	rm -fr libmylib.so results_shared.txt

