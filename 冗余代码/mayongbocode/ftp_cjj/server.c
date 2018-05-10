/**
*	Time:2015.12.05
*	Auther:ChenLi
**/
#include "Thread.h"

int main(int argc,char *argv[])
{//XX PORT
	char IP[20];
	char port[20];
	int Port;
	if(argc == 1)
	{
		printf("请输入IP:");
		fgets(IP,20,stdin);
		printf("Port:");
		fgets(port,20,stdin);
		Port = atol(port);
	}else if(argc == 2)
	{
		if(strchr(argv[1],':')==NULL)
		{
			if(strchr(argv[1],'.')==NULL)
			{
				//仅port
				Port = atol(argv[1]);
				printf("请输入IP:");
				fgets(IP,20,stdin);
			}else
			{
				//ip模式
				strcpy(IP,argv[1]);
				printf("请输入Port:");
				fgets(port,20,stdin);
				Port = atol(port);
			}
		}else
		{
			strcpy(IP, strtok(argv[1],":"));//取用户名密码
			Port = atol(strtok(NULL,":"));
		}
	}
	int client_max = 10;

	int sockfd = Run_Server(IP,Port);//第一个socket 用作监听
	if(sockfd == -1)
	{
		printf("启动失败，请重试！\n");
		return -1;
	}
	//开server命令线程
	if(ServerCommandThread(sockfd) == -1)//其实用不到端口
	{
		printf("启动失败，请重试！\n");
		return -1;
	}
	//监听端口
	if(ServerThread(sockfd) == -1)
	{
		printf("监听任务失败,即将推出程序!\n");
		close(sockfd);
		exit(0);
	 }
}

