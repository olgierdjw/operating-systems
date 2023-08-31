make: clean client

clean:
	rm -fr client

client:
	gcc -Wall -o client client.c -lrt

test: clean client
	./client
