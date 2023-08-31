make: clean client

clean:
	rm -fr client

client:
	gcc -Wall -o client client.c

test: clean client
	./client
