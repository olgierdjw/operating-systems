make: clean client server

clean:
	rm -fr server client

client:
	gcc -Wall -o client client.c

server:
	gcc -Wall -o server server.c

run-client: clean client server
	./client SERVER-ID 2 3 4
