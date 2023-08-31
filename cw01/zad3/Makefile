OPTFLAGS =

make: static shared dll
	LD_LIBRARY_PATH=.

test: delete_results clean delete_results test_all test_all_01 test_all_02 test_all_03

test_static_shared_dll: test_static test_shared test_dll

static: clean
	gcc $(OPTFLAGS) -c my_lib.c -o my_lib.o #object file my_lib.o
	ar rcs libmylib.a my_lib.o #static library file libmylib.a
	gcc $(OPTFLAGS)  main.c -L. -lmylib -o program_static  #compile main.c with the static library

test_static: static
	cat test_commands.txt | ./program_static 2>&1 | tee results_static_$(OPTFLAGS).txt


shared: clean
	gcc $(OPTFLAGS) -c -fPIC my_lib.c -o my_lib.o
	gcc $(OPTFLAGS) -shared -o libmylib.so my_lib.o
	gcc $(OPTFLAGS) main.c -o program_shared -L. -lmylib -Wl,-rpath=`pwd`

test_shared: shared test_commands.txt
	cat test_commands.txt | ./program_shared 2>&1 | tee results_shared_$(OPTFLAGS).txt

dll: clean
	gcc $(OPTFLAGS) -c  main.c
	gcc $(OPTFLAGS) -c  my_lib.c
	gcc $(OPTFLAGS) -o  main main.o my_lib.o
	gcc $(OPTFLAGS) -c -Wall -fpic  my_lib.c
	gcc $(OPTFLAGS) -shared  -o libmylib.so my_lib.o
	gcc $(OPTFLAGS) -o program_dll main.c -L. -lmylib

run_dll: dll
	LD_LIBRARY_PATH=. ./program_dll

test_dll: dll test_commands.txt
	cat test_commands.txt | LD_LIBRARY_PATH=. ./program_dll 2>&1 | tee results_dll_$(OPTFLAGS).txt

test_all:
	make OPTFLAGS= test_static_shared_dll

test_all_01:
	make OPTFLAGS=-O1 test_static_shared_dll

test_all_02:
	make OPTFLAGS=-O2 test_static_shared_dll

test_all_03:
	make OPTFLAGS=-O3 test_static_shared_dll

clean:
	rm -fr *.o *.a static shared program_static results_static.txt
	rm -fr libmylib.so results_shared.txt program_shared
	rm -fr program_dll results_dll.txt

delete_results:
	find . -name "results_*.txt" -type f -delete
