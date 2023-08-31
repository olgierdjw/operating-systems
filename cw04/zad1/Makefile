make: test

clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

test: clean program
	#./program ignore 10 // using just fork()
	./program pending 10
