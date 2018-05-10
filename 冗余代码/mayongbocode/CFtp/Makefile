# Don't edit this file using CLion Text Editor!!!
# Try using vim!!!

# Thread libraries
THREADLIBS = -lpthread
SHADOWLIBS = -lcrypt

all: server.o client.o

server.o: server.c common.h server.h thread.h
	gcc $(THREADLIBS) $(SHADOWLIBS) server.c -o server


client.o: client.c
	gcc -o client client.c
