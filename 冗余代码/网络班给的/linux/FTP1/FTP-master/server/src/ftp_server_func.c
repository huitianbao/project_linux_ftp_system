#include "../include/ftp.h"


int socket_server(char* ip, char* port)
{
	int sfd ;
	struct sockaddr_in server_addr ;
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd == -1)
	{
		return -1;
	}
	memset(&server_addr, 0, sizeof(server_addr) );
	server_addr.sin_family = AF_INET ;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = inet_addr(ip);
	int reuse = 1 ;
	int buf_num = BUF_SIZE;
	if(0 != setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR,  (void*)&reuse, sizeof(int)) )
	{
		close(sfd);
		return -1;
	}
	if(0 != setsockopt(sfd, SOL_SOCKET, SO_SNDBUF,  (void*)&buf_num, sizeof(int)) )
	{
		close(sfd);
		return -1 ;	
	}
	if(0 != setsockopt(sfd, SOL_SOCKET, SO_RCVBUF,  (void*)&buf_num, sizeof(int)) )
	{
		close(sfd);
		return -1 ;	
	}

	if(-1 == bind(sfd, (struct sockaddr*)&server_addr, sizeof(server_addr) ) )
	{
	    perror("bind");
		close(sfd);
		return -1 ;	
	}
	if(-1 == listen(sfd, 5))
	{
	    perror("listen");
		close(sfd);
		return -1 ;
	}
	return sfd ;

}


int socket_client(char* ip, char* port)
{
	int cfd ;
	struct sockaddr_in server_addr ;
	cfd = socket(AF_INET, SOCK_STREAM, 0);
	if(cfd == -1)
	{
		return -1;
	}
	memset(&server_addr, 0, sizeof(server_addr) );
	server_addr.sin_family = AF_INET ;
	server_addr.sin_port = htons(atoi(port));
	server_addr.sin_addr.s_addr = inet_addr(ip);
	int reuse = 1 ;
	int buf_num = BUF_SIZE;
	if(0 != setsockopt(cfd, SOL_SOCKET, SO_REUSEADDR,  (void*)&reuse, sizeof(int)) )
	{
		close(cfd);
		return -1;
	}
	if(0 != setsockopt(cfd, SOL_SOCKET, SO_SNDBUF,  (void*)&buf_num, sizeof(int)) )
	{
		close(cfd);
		return -1 ;	
	}
	if(0 != setsockopt(cfd, SOL_SOCKET, SO_RCVBUF,  (void*)&buf_num, sizeof(int)) )
	{
		close(cfd);
		return -1 ;	
	}
	if(-1 == connect(cfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)))
	{
	    perror("connect");
		close(cfd);
		return -1 ;
	}
	return cfd ;

}

int recv_n(int fd_recv,char* recv_buf, int len)
{
	int sum = 0 ;
	int nrecv ;
	while(sum < len)
	{
		nrecv = recv(fd_recv, &recv_buf[ sum], len - sum, 0);
		sum += nrecv ;
	}
	recv_buf[sum] = '\0';
	return sum ;
}

int read_n(int fd_read, char* read_buf, int len)
{
	int sum = 0 ;
	int nread ;
	while(sum < len)
	{
		nread = read(fd_read, &read_buf[ sum], len - sum);
		if(nread == 0)
		{
			break ;
		}
		sum += nread ;
	}
	read_buf[sum] = '\0';
	return sum ;
		
}
int send_n(int fd_send, char* send_buf, int len)
{
	int sum = 0 ;
	int nsend ;
	while(sum < len)
	{
		nsend = send(fd_send, send_buf + sum, len - sum, 0);
		sum += nsend ;
	}
	return sum ;
}


int write_n(int fd_write, char* write_buf, int len)
{
	int sum = 0 ;
	int nwrite ;
	while(sum < len)
	{
		nwrite = write(fd_write, write_buf + sum, len - sum);
		sum += nwrite ;
	}
	return sum ;

}


void delete_space(char* src)
{
	int index, cur ;
	for(cur = -1, index = 0 ; index < strlen(src); index ++)
	{
		if(src[index] !=' ' && src[index] != '\n')
		{
			src[++cur] = src[index];
			
		}else 
		{
			if(cur != -1 && src[cur] != ' '&& src[cur] !='\n' )
			{
				src[++cur] = ' ';
			}
		}

	}
	for(; cur >= 0 ; cur --)
	{
		if(src[cur] != ' ')
		{
			break ;
		}
	}
	src[++cur] = '\0';
}
int upload(int fd_up, char* file_name)
{
	int fd_file = open(file_name, O_RDONLY);
	if(fd_file == -1)
	{
		return -1 ;
	}
	char *read_buf = (char*)malloc(8 * 1024);
	bzero(read_buf, 8 * 1024);
	int nread ;
	while(1)
	{
		nread = read_n(fd_file, read_buf, 8192);
		if(nread < 8192)
		{
			send(fd_up, &nread, sizeof(int), 0);
			send_n(fd_up, read_buf, nread);
			break ;
		}else
		{
			
			send(fd_up, &nread, sizeof(int), 0);
			send_n(fd_up, read_buf, nread);
		}
	}
	int flag = 0 ;
	send(fd_up, &flag, sizeof(flag), 0);
	close(fd_file);
	return 0 ;
}



int download(int fd_down, char* file_name)
{
	int fd_file = open(file_name, O_WRONLY|O_CREAT,0666 );
	if(fd_file == -1)
	{
		return -1 ;
	}
	char* write_buf = (char*)malloc(8192);
	bzero(write_buf, 8192);
	int nwrite ;
	while(1)
	{
		recv(fd_down, &nwrite, sizeof(int), 0);
		if(nwrite == 0)
		{
			break ;
		}
		recv_n(fd_down, write_buf, nwrite);
		write_n(fd_file, write_buf, nwrite);
	}
	close(fd_file);
	return 0 ;
}



char* file_type(mode_t md)
{
	if(S_ISREG(md))
	{
		return "-";	
	}else if(S_ISDIR(md))
	{
		return "d";
	}else if(S_ISFIFO(md))
	{
		return "p";
	}else 
	{
		return "o" ;
	}
}


void do_ls(pinfo_t info) 
{
	DIR* pdir = opendir("./");
	if(pdir == NULL)
	{
		int flag = -1 ;
		send(info ->info_sfd, &flag, sizeof(int), 0);
	}else
	{
		struct dirent* mydir ;
		int len ;
		while( (mydir = readdir(pdir)) != NULL)
		{
			if(strncmp(mydir->d_name, ".", 1) == 0 || strncmp(mydir->d_name,"..", 2) == 0)
			{
				continue ;
			}
			struct stat mystat;
			bzero(&mystat, sizeof(stat));
			stat(mydir->d_name, &mystat);
			bzero(info ->info_buf, 1024);
			sprintf(info ->info_buf, "%-5s %-15s %20dB", file_type(mystat.st_mode),mydir->d_name, (int)mystat.st_size);
			len =  strlen(info ->info_buf);
			send(info ->info_sfd, &len, sizeof(int), 0);
			send_n(info ->info_sfd, info ->info_buf, len);
		}
		len = 0 ;
		send(info ->info_sfd, &len, sizeof(int), 0);
	}

}

void do_cd(pinfo_t info) 
{
	char dir[128]= "";
	sscanf(info ->info_buf +3, "%s", dir);
	chdir(dir);
	getcwd(dir, 128);
	int len = strlen(dir);
	send(info ->info_sfd, &len, sizeof(int), 0);
	send_n(info ->info_sfd, dir, strlen(dir));
}

void do_pwd(pinfo_t info) 
{
	bzero(info ->info_buf, BUF_SIZE);
	getcwd(info ->info_buf, BUF_SIZE);
	int len = strlen(info ->info_buf);
	send(info ->info_sfd, &len, sizeof(int), 0);
	send_n(info ->info_sfd, info ->info_buf, len);

}
void do_gets(pinfo_t info) 
{
	char file_name[256];
	int file_pos = 5 ;
	while(bzero(file_name, 256),sscanf(info ->info_buf + file_pos,"%s", file_name ) == 1)
	{
		file_pos += strlen(file_name) + 1; 
		if(upload(info ->info_sfd, file_name) == 0)
		{
			printf(" file-> %s upload success\n", file_name);
		}else 
		{
			printf(" file-> %s upload failed\n", file_name);
		}

	}

}
void do_puts(pinfo_t info) //puts a.txt b.txt
{
	char file_name[256];
	int file_pos = 5 ;
	while(bzero(file_name, 256), sscanf(info ->info_buf + file_pos,"%s", file_name) == 1)
	{
		file_pos += strlen(file_name) + 1 ;
		if(download(info -> info_sfd, file_name) == 0)
		{
			printf("file download %s success \n", file_name);
		}else
		{

			printf("file download %s failed \n", file_name);
		}
	}
}
void do_remove(pinfo_t info)// remove file 
{
	char cmd[256] ="" ;
	sprintf(cmd, "rm -f %s", info ->info_buf + 7);
	system(cmd);
	bzero(info ->info_buf, BUF_SIZE);
	sprintf(info ->info_buf, "%s removed", info ->info_buf + 7);
	int len = strlen(info ->info_buf);
	send(info ->info_sfd, &len, sizeof(int),0);
	send_n(info ->info_sfd, info ->info_buf, len);
}
void client_handle(pinfo_t info)
{

	int cfd = info -> info_sfd ;
	int cmd_len = 0 ;
	int recv_ret ;
	while(1)
	{
		bzero(info -> info_buf, BUF_SIZE);
		recv_ret = recv(cfd, &cmd_len, sizeof(int),0);
		if(cmd_len == 0 || recv_ret == 0)
		{
			printf("client exit !\n");
			close(info ->info_sfd);
			free(info);
			exit(1);
		}
		recv_n(cfd, info->info_buf, cmd_len);
		if(strncmp("cd", info ->info_buf, 2) == 0)
		{
			do_cd(info);
			printf("cd success!\n");
		}else if(strncmp("ls", info ->info_buf, 2) == 0)
		{
			do_ls(info);
			printf("ls success\n");
		}else if( strncmp("puts", info ->info_buf, 4)== 0)
		{
			do_puts(info);
			printf("upload success\n");
		}else if( strncmp("gets", info ->info_buf, 4)== 0)
		{
			do_gets(info);
			printf("download success\n");

		}else if( strncmp("remove", info ->info_buf, 6)== 0)
		{
			do_remove(info);
			printf("remove success\n");

		}else if(strncmp("pwd", info ->info_buf, 3) == 0) 
		{
			do_pwd(info);
			printf("pwd success\n");

		}else 
		{
			continue ;
		}


	}
}


void ftp_arg_set(char* line, char* arg) 
{	
	char* ptr = strchr(line, '=');
	if(ptr == NULL)
	{
		perror("no =");
		exit(1);
	}
	strcpy(arg, ptr + 1);
}


void ftp_init_from_file(char* path, char* ip, char* port)
{
	FILE* fp = fopen(path, "r");
	if(fp == NULL)
	{
		perror("open conf file fail!");	
		exit(1);
	}
	char buf[128];

	memset(buf, 0, 128);
	fgets(buf, 128, fp) ;
	buf[strlen(buf) - 1] = '\0' ;
	ftp_arg_set(buf, ip);

	memset(buf, 0, 128);
	fgets(buf, 128, fp) ;
	buf[strlen(buf) - 1] = '\0' ;
	ftp_arg_set(buf, port);
	fclose(fp);
}
