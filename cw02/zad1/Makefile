clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

all: clean program
	cat test_commands.txt | ./program
	tr O + < big.txt > tmp ; diff -s tmp output.txt

