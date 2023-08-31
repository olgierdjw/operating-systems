make: clean client server

clean:
	rm -fr client server client-logs.txt

server:
	gcc -Wall -o server server.c

client:
	gcc -Wall -o client client.c

test: clean client
	./client
