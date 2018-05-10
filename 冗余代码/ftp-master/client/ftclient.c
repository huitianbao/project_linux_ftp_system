#include "ftclient.h"
	

int sock_control; 


/**
 * Receive a response from server
 * Returns -1 on error, return code on success  //获取sock_control的码值
 */
int read_reply(){
	int retcode = 0;
	if (recv(sock_control, &retcode, sizeof retcode, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}	
	return ntohl(retcode);
}



/**
 * Print response message
 */
void print_reply(int rc) 
{
printf("%d\n",rc);
	switch (rc) {
		case 220:
			printf("220 Welcome, server ready.\n");
			break;
		case 221:
			printf("221 Goodbye!\n");
			break;
		case 226:
			printf("226 Closing data connection. Requested file action successful.\n");
			break;
		case 550:
			printf("550 Requested action not taken. File unavailable.\n");
			break;
		case 251:
			printf("251 Closing data connection. mkdir action successful.\n");
			break;
		case 551:
			printf("551 Requested action not taken. mkdir error.\n");
			break;
		case 252:
			printf("252 Closing data connection. rmdir action successful.\n");
			break;
		case 552:
			printf("552 Requested action not taken. rmdir error.\n");
			break;
		case 253:
			printf("253 Closing data connection. pwd action successful.\n");
			break;
		case 553:
			printf("553 Requested action not taken. pwd error.\n");
			break;
		case 254:
			printf("254 Closing data connection. cd action successful.\n");
			break;
		case 554:
			printf("554 Requested action not taken. cd error.\n");
			break;
		case 255:
			printf("255 Closing data connection. put action successful.\n");
			break;
		case 555:
			printf("555 Requested action not taken. put error.\n");
			break;
		case 256:
			printf("256 Closing data connection. mput action successful.\n");
			break;
		case 556:
			printf("556 Requested action not taken. mput error.\n");
			break;
		case 257:
			printf("257 Closing data connection. mget action successful.\n");
			break;
		case 557:
			printf("557 Requested action not taken. mget error.\n");
			break;
		//default:break;
	}
	
}


/**
 * Parse command in cstruct，前四个字节，命令，后边全部为数据
 */ 
int ftclient_read_command(char* buf, int size, struct command *cstruct)
{
	memset(cstruct->code, 0, sizeof(cstruct->code));
	memset(cstruct->arg, 0, sizeof(cstruct->arg));
	
	printf("ftclient> ");	// prompt for input		
	fflush(stdout); 	

	// wait for user to enter a command
	read_input(buf, size);
		
	char *arg = NULL;
	arg = strtok (buf," ");
	arg = strtok (NULL, " ");

	if (arg != NULL){
		// store the argument if there is one
		strncpy(cstruct->arg, arg, strlen(arg));
	}

	// buf = command，把命令长度改为“4”
	if (strcmp(buf, "ls") == 0) {
		strcpy(cstruct->code, "LIST");		
	}
	else if (strcmp(buf, "get") == 0) {
		strcpy(cstruct->code, "RETR");//retr		
	}
	else if (strcmp(buf, "quit") == 0) {
		strcpy(cstruct->code, "QUIT");		
	}
	else if(strcmp(buf, "mkdir") == 0){//*********新建目录
		strcpy(cstruct->code, "MKDI");
	}
	else if(strcmp(buf, "rmdir") == 0){//*********删除目录
		strcpy(cstruct->code, "RMDI");
	}
	else if(strcmp(buf, "pwd") == 0){//*********获取路经
		strcpy(cstruct->code, "PWDS");
	}
	else if(strcmp(buf, "cd") == 0){//*********修改路径
		strcpy(cstruct->code, "CDCD");
	}
	else if(strcmp(buf, "put") == 0){//*********上传单个文件
		strcpy(cstruct->code, "PUTS");
	}
	else if(strcmp(buf, "mput") == 0){//*********上传多个文件
		strcpy(cstruct->code, "MPUT");
	}
	else if(strcmp(buf, "mget") == 0){//*********下载多个文件
		strcpy(cstruct->code, "MGET");
	}
	else {//invalid
		return -1;
	}

	// store code in beginning of buffer
	memset(buf, 0, 400);
	strcpy(buf, cstruct->code);

	// if there's an arg, append it to the buffer
	if (arg != NULL) {
		strcat(buf, " ");
		strncat(buf, cstruct->arg, strlen(cstruct->arg));
	}
	
	return 0;
}



/**
 * Do get <filename> command 
 */
int ftclient_get(int data_sock, int sock_control, char* arg)
{
    char data[MAXSIZE];
    int size;
    FILE* fd = fopen(arg, "w");
    
    while ((size = recv(data_sock, data, MAXSIZE, 0)) > 0) {
        fwrite(data, 1, size, fd);
    }

    if (size < 0) {
        perror("error\n");
    }

    fclose(fd);
    return 0;
}




/*********新建目录**************/
int ftclient_mkdir(int data_sock, int sock_control, char* arg)
{
   return 0;
}


/*********删除目录**************/
int ftclient_rmdir(int data_sock, int sock_control, char* arg)
{
   return 0;
}


/*********获取路经**************/
int ftclient_pwd(int sock_data, int sock_con)
{
   return 0;
}


/*********修改路径**************/
int ftclient_cd(int data_sock, int sock_control, char* arg)
{
   return 0;
}


/*********上传单个文件**************/
int ftclient_put(int data_sock, int sock_control, char* arg)
{
   return 0;
}


/*********上传多个文件**************/
int ftclient_mput(int data_sock, int sock_control, char* arg)
{
   return 0;
}


/*********下载多个文件**************/
int ftclient_mget(int data_sock, int sock_control, char* arg)
{
   return 0;
}





/**
 * Open data connection
 */
int ftclient_open_conn(int sock_con)
{
	int sock_listen = socket_create(CLIENT_PORT_ID);

	// send an ACK on control conn
	int ack = 1;
	if ((send(sock_con, (char*) &ack, sizeof(ack), 0)) < 0) {
		printf("client: ack write error :%d\n", errno);
		exit(1);
	}		

	int sock_conn = socket_accept(sock_listen);
	close(sock_listen);
	return sock_conn;
}




/** 
 * Do list commmand
 */
int ftclient_list(int sock_data, int sock_con)
{
	size_t num_recvd;			// number of bytes received with recv()
	char buf[MAXSIZE];			// hold a filename received from server
	int tmp = 0;

	// Wait for server starting message
	if (recv(sock_con, &tmp, sizeof tmp, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}
	
	memset(buf, 0, sizeof(buf));
	while ((num_recvd = recv(sock_data, buf, MAXSIZE, 0)) > 0) {//服务端是放在将DOS命令放在“tmp.txt”里，再发送
        	printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}
	
	if (num_recvd < 0) {
	        perror("error");
	}

	// Wait for server done message
	if (recv(sock_con, &tmp, sizeof tmp, 0) < 0) {
		perror("client: error reading message from server\n");
		return -1;
	}
	return 0;
}



/**
 * Input: cmd struct with an a code and an arg
 * Concats code + arg into a string and sends to server
 */
int ftclient_send_cmd(struct command *cmd)
{
	char buffer[MAXSIZE];
	int rc;

	sprintf(buffer, "%s %s", cmd->code, cmd->arg);
	
	// Send command string to server
	rc = send(sock_control, buffer, (int)strlen(buffer), 0);	
	if (rc < 0) {
		perror("Error sending command to server");
		return -1;
	}
	
	return 0;
}



/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login()
{
	struct command cmd;
	char user[256];
	memset(user, 0, 256);

	// Get username from user
	printf("Name: ");	
	fflush(stdout); 		
	read_input(user, 256);

	// Send USER command to server
	strcpy(cmd.code, "USER");
	strcpy(cmd.arg, user);
	ftclient_send_cmd(&cmd);
	
	// Wait for go-ahead to send password
	int wait;
	recv(sock_control, &wait, sizeof wait, 0);

	// Get password from user
	fflush(stdout);	
	char *pass = getpass("Password: ");//密码，不可见	

	// Send PASS command to server
	strcpy(cmd.code, "PASS");
	strcpy(cmd.arg, pass);
	ftclient_send_cmd(&cmd);
	
	// wait for response
	int retcode = read_reply();
	switch (retcode) {
		case 430:
			printf("Invalid username/password.\n");
			exit(0);
		case 230:
			printf("Successful login.\n");
			break;
		default:
			perror("error reading message from server");
			exit(1);		
			break;
	}
}





int main(int argc, char* argv[]) 
{		
	int data_sock, retcode, s;
	char buffer[MAXSIZE];
	struct command cmd;	
	struct addrinfo hints, *res, *rp;//和sockaddr_in用法一样

	if (argc != 3) {
		printf("usage: ./ftclient hostname port\n");
		exit(0);
	}

	char *host = argv[1];
	char *port = argv[2];

	// Get matching addresses
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;//返回的是适用于指定主机名和服务名且适合任何协议族的地址
	hints.ai_socktype = SOCK_STREAM;
	
	s = getaddrinfo(host, port, &hints, &res); //getaddrinfo函数能够处理名字到地址以及服务到端口这两种转换，返回的是一个sockaddr 结构的链而 不是一个地址清单。它具有协议无关性。
	if (s != 0) {
		printf("getaddrinfo() error %s", gai_strerror(s));
		exit(1);
	}
	
	// Find an address to connect to & connect
	for (rp = res; rp != NULL; rp = rp->ai_next) {
		sock_control = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);//！！！！！！！！！sock_control！！！！！！！！！！！

		if (sock_control < 0)
			continue;

		if(connect(sock_control, res->ai_addr, res->ai_addrlen)==0) {
			break;
		} else {
			perror("connecting stream socket");
			exit(1);
		}
		close(sock_control);
	}
	freeaddrinfo(rp);


	// Get connection, welcome messages
	printf("Connected to %s.\n", host);
	print_reply(read_reply()); //获取sock_control的码值
	

	/* Get name and password and send to server */
	ftclient_login();

	while (1) { // loop until user types quit

		// Get a command from user
		if ( ftclient_read_command(buffer, sizeof buffer, &cmd) < 0) {
			printf("Invalid command\n");
			continue;	// loop back for another command
		}

		// Send command to server
		if (send(sock_control, buffer, (int)strlen(buffer), 0) < 0 ) {
			close(sock_control);
			exit(1);
		}
                retcode = read_reply();	
		if (retcode == 221) {
			/* If command was quit, just exit */
			print_reply(221);		
			break;
		}
		
		if (retcode == 502) {
			// If invalid command, show error message
			printf("%d Invalid command.\n", retcode);
		} 
		else {			
			// Command is valid (RC = 200), process command
		
			// open data connection
			if ((data_sock = ftclient_open_conn(sock_control)) < 0) {//先登录，如果发送的命令里有LIST(list)/RETR(retr)再初始化接收数据，否则不初始化
				perror("Error opening socket for data connection");
				exit(1);
			}			
			
			// execute command
			if (strcmp(cmd.code, "LIST") == 0) {
				ftclient_list(data_sock, sock_control);
			} 
			else if (strcmp(cmd.code, "RETR") == 0) {//*******get
				// wait for reply (is file valid)
				if (read_reply() == 550) {
					print_reply(550);		
					close(data_sock);
					continue; 
				}
				ftclient_get(data_sock, sock_control, cmd.arg);
				print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "MKDI") == 0) {//*********新建目录
				// wait for reply (is file valid)
				if (read_reply() == 551) {//服务端还没写
					print_reply(551);		
					close(data_sock);
					continue; 
				}
				ftclient_mkdir(data_sock, sock_control, cmd.arg);
				//print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "RMDI") == 0) {//*********删除目录
				// wait for reply (is file valid)
				if (read_reply() == 552) {//服务端还没写
					print_reply(552);		
					close(data_sock);
					continue; 
				}
				ftclient_rmdir(data_sock, sock_control, cmd.arg);
				//print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "PWDS") == 0) {//*********获取路经
				ftclient_pwd(data_sock, sock_control);
			}
			else if (strcmp(cmd.code, "CDCD") == 0) {//*********修改路径
				// wait for reply (is file valid)
				if (read_reply() == 554) {//服务端还没写
					print_reply(554);		
					close(data_sock);
					continue; 
				}
				ftclient_cd(data_sock, sock_control, cmd.arg);
				print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "PUTS") == 0) {//*********上传单个文件
				// wait for reply (is file valid)
				if (read_reply() == 555) {//服务端还没写
					print_reply(555);		
					close(data_sock);
					continue; 
				}
				ftclient_put(data_sock, sock_control, cmd.arg);
				print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "MPUT") == 0) {//*********上传多个文件
				// wait for reply (is file valid)
				if (read_reply() == 556) {//服务端还没写
					print_reply(556);		
					close(data_sock);
					continue; 
				}
				ftclient_mput(data_sock, sock_control, cmd.arg);
				print_reply(read_reply()); 
			}
			else if (strcmp(cmd.code, "MGET") == 0) {//*********下载多个文件
				// wait for reply (is file valid)
				if (read_reply() == 557) {//服务端还没写
					print_reply(557);		
					close(data_sock);
					continue; 
				}
				ftclient_mget(data_sock, sock_control, cmd.arg);
				print_reply(read_reply()); 
			}
			close(data_sock);
		}
	} // loop back to get more user input

	// Close the socket (control connection)
	close(sock_control);
    return 0;  
}
