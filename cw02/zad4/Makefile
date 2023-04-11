make: test

clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

test: clean program
	./program .
	#find . | xargs wc --bytes
