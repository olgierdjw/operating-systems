make: test

clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

test: clean program
	touch destination.txt
	rm destination.txt
	./program . '#include'