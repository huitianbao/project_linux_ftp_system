ftp
=========

A simple ftp software including server and client in Linux operating system


## Usage

##### **compile**

	gcc -o ftp_server ftp_server.c
	gcc -o ftp_client ftp_client.c

##### **start ftp server** (default host: 127.0.0.1)

	./ftp_server <port>

##### **start ftp client**

	./ftp_client <host> <port>

## Client commads

	get     get a file from server.
	put     send a file to server.
	pwd     get the present directory on server.
	dir     list the directory on server.
	cd      change the directory on server.
	?/help  help to know how to use the commands.
	quit    quit client.
