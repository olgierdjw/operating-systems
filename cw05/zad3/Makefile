make: program worker

clean:
	rm -fr program worker report.txt myFIFO

program:
	gcc -Wall -o program program.c

worker:
	gcc -Wall -o worker worker.c

test: clean make
	./program 0.00000001 5 >> report.txt
	./program 0.00000001 10 >> report.txt
	./program 0.000000001 1 >> report.txt
	./program 0.000000001 5 >> report.txt
	./program 0.000000001 10 >> report.txt
