make: program

clean:
	rm -fr program report.txt

program:
	gcc -Wall -o program main.c

test: clean program
	./program 0.00000001 5 >> report.txt
	./program 0.00000001 10 >> report.txt
	./program 0.000000001 1 >> report.txt
	./program 0.000000001 5 >> report.txt
	./program 0.000000001 10 >> report.txt
