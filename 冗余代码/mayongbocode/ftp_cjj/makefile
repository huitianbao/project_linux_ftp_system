

#MyFtp.o:MyFtp.c
	gcc -c MyFtp.c -o MyFtp.o

#Thread.o:Thread.c
	gcc -c Thread.c -o Thread.o

#client.o:client.c
	gcc -c client.c -o client.o

#server.o:server.c
	gcc -c server.c -o server.o

#all:Thread.o MyFtp.o client.o server.o



	gcc -O -o s server.o Thread.o -lpthread MyFtp.o -lcrypt

	gcc -O -o c client.o Thread.o -lpthread MyFtp.o -lcrypt

#clean:
	#rm *.o
	#rm s
	#rm c
