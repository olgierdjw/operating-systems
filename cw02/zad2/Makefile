make: test

clean:
	rm -fr program *.txt

program:
	gcc -Wall -o program main.c

test: clean program
	./program

