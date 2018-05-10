#include "ftp.h"
int main(int argc, char* argv[])
{
	if(argc <= 2)
	{
		printf("Usage:ip ,port\n");
		exit(-1);
	}
	
	char* ip_server = argv[1] ;
	char* port_server = argv[2];
	char recv_buf[BUF_SIZE], send_buf[BUF_SIZE];
	int recv_len ,send_len ,read_len, write_len;
	;
	int cfd = socket_client(ip_server, port_server);
	if(cfd == -1)
	{
		perror("socket");
		exit(-1);
	}
	char cmd[128];
	while(bzero(cmd, 128),(read_len = read(0, cmd, 128)) > 0 )
	{
		delete_space(cmd);
		send_len = strlen(cmd);
		send(cfd, &send_len, sizeof(int), 0);
		send_n(cfd, cmd, send_len);
		if(strncmp("cd", cmd, 2) == 0)
		{
			recv(cfd, &recv_len, sizeof(int), 0);
			recv_n(cfd, recv_buf, recv_len );
			printf("%s\n", recv_buf);
		}else if(strncmp("ls", cmd, 3) == 0)
		{
			while(1)
			{
				recv(cfd, &recv_len, sizeof(int), 0);
				if(recv_len == 0)
				{
					break ;
				}
				recv_n(cfd, recv_buf, recv_len );
				printf("%s\n", recv_buf);
			}
		}else if(strncmp("puts", cmd, 4) == 0)
		{
			char file_name[256];
			int file_pos = 5 ;
			while(bzero(file_name, 256),sscanf(cmd + file_pos,"%s", file_name ) == 1)
			{
				file_pos += strlen(file_name) + 1; 
				if(upload(cfd, file_name) == 0)
				{
					printf(" file upload %s success\n", file_name);
				}else 
				{
					printf(" file upload %s failed\n", file_name);
				}
				
			}
		}else if( strncmp("gets", cmd, 4)== 0)
		{
				char file_name[256];
				int file_pos = 5 ;
				while(bzero(file_name, 256), sscanf(cmd + file_pos,"%s", file_name) == 1)
				{
					file_pos += strlen(file_name) + 1 ;
					if(download(cfd,file_name )== 0)
					{
						printf("file download %s success \n", file_name);
					}else
					{
					
						printf("file download %s failed \n", file_name);
					}
				}
			
		}else if( strncmp("remove", cmd, 6) == 0)
		{
			recv(cfd, &recv_len, sizeof(int), 0);
			recv_n(cfd, recv_buf, recv_len );
			printf("%s\n", recv_buf);
			
		}else if(strncmp("pwd", cmd, 3) == 0)
		{
			recv(cfd, &recv_len, sizeof(int), 0);
			recv_n(cfd, recv_buf, recv_len );
			printf("%s\n", recv_buf);
		}else 
		{
			continue ;
		}
	}
	return 0 ;
}
