#include "ftp.h"
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
		close(sfd);
		return -1 ;	
	}
	if(-1 == listen(sfd, 5))
	{
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
