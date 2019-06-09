CC = ezcc
CFLAGS = -Wall

all : host client

host : host.c
	$(CC) $(CFLAGS) $? -o $@ -lEzGraph

client : client.c
	$(CC) $(CFLAGS) $? -o $@ -lEzGraph

clean :
	rm -rf client host
