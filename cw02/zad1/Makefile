clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

test1: clean program
	cat TEST_COMMANDS.txt | ./program
	#tr O + < big.txt > tmp ; diff -s tmp output.txt

