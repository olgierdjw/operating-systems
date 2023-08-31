CC = gcc
CFLAGS = -Wall

all: clean server client
	
server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f client server

# SERVER
server-start: clean all
	./server 1234

# UDP socket client
client-udp: clean all
	./client 'udp-user1' 1234

# Unix socket client
client-unix: clean all
	./client 'unix-user-1'