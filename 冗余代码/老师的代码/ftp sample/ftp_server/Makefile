CC=gcc
FLAG=-g
OBJS=ftp.c command.c
CLIENT=client.c
SERVER=server.c

all:
	$(CC) $(FLAG) -c $(OBJS) 
	$(CC) $(FLAG) $(CLIENT) ftp.o command.o -o client
	$(CC) $(FLAG) $(SERVER) ftp.o command.o -o server

clean:
	rm -f ftp.o 
	rm -f command.o
	rm -f client
	rm -f server
