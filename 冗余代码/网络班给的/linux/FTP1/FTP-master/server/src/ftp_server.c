#include "../include/ftp.h"



int main(int argc, char* argv[])// conf_file path
{
	int pid ;
	int conn_num = 0;
	char log_buf[128];
	char ip[16] = "" ;
	char port[10]= "";
	pinfo_t info ;

	ftp_init_from_file(argv[1], ip, port );

	int sfd = socket_server(ip, port);
	int cfd ;
	if(sfd == -1)
	{
		perror("socket");
		exit(-1);
	}


	while(1)
	{
		cfd = accept(sfd, NULL, NULL);
		if(cfd == -1)
		{
			perror("accept");
			exit(-1);
		}
		bzero(log_buf, 128);
		sprintf(log_buf, "%d connect\n",++conn_num );
		write(1, log_buf, strlen(log_buf));

		info = (pinfo_t)calloc(1, sizeof(*info));
		info ->info_sfd = cfd ;
		pid = fork() ;
		if(pid == 0)
		{
			client_handle(info);
			exit(1);
		}
		free(info);
	}

}
