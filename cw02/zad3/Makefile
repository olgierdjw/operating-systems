make: test1

clean:
	rm -fr program

program:
	gcc -Wall -o program main.c

test: clean program
	#cat test_commands.txt | ./program
	#tr O + < big.txt > tmp ; diff -s tmp output.txt
	./program
	wc --bytes *
