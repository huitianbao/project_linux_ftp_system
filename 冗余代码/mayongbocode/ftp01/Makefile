CC=gcc
FLAG=-g
OBJS=clientftp.c serverftp.c
CLIENT=client.c
SERVER=server.c
MESSAGE=message.c
all:  
	$(CC) $(FLAG) -c $(OBJS) 
	$(CC) $(FLAG) $(CLIENT) clientftp.o -o client
	$(CC) $(FLAG) $(SERVER) serverftp.o -o server -lpthread
	$(CC) $(FLAG) $(MESSAGE) -o message
clean:
	rm -f serverftp.o 
	rm -f clientftp.o
	rm -f client
	rm -f server
	rm -f message
