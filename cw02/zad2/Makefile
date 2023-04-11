make: test1

clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

test1: clean program
	#cat test_commands.txt | ./program
	#tr O + < big.txt > tmp ; diff -s tmp output.txt
	./program
	#cat b.txt

test:
	tac < start.txt | rev > b.txt