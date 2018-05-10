#include "common.h"

typedef struct _ClientLoginInfo{
    char *ipaddr;
    int port;
    char *username;
    char *passwd;
} ClientLoginInfo;
/**
 * 解析命令行参数，从第二个参数中获取用户登陆信息
 *
 * @param argc
 * @param argv
 * @return 返回用户登陆信息
 */
ClientLoginInfo *
parseArguments(int argc,char **argv){

    ClientLoginInfo *c_info = (ClientLoginInfo *)malloc(sizeof(ClientLoginInfo));
    if (!c_info)
    {
        printf("%sFail to parse arguments\n",CM);
        exit(1);
    }

    char *target = argv[1];
    char *temp = (char *)malloc(32 * sizeof(char));
    char signal[4] = {':','@',':','\0'};
    char sig_index = 0;
    int len = strlen(target);
    for(int i = 0;i < len,sig_index < 4;i++)
    {
        if (target[i] != signal[sig_index])
        {
            strncat(temp,&target[i],1);
        }
        else
        {
            if(sig_index == 0)
            {
                char *ipaddr = (char *)malloc(IPADDR_LEN);
                strcpy(ipaddr,temp);
                c_info->ipaddr = ipaddr;
            }
            else if (sig_index == 1)
            {
                c_info->port = atoi(temp);
            }
            else if (sig_index == 2)
            {
                char *username = (char *)malloc(USERNAME_LEN);
                strcpy(username,temp);
                c_info->username = username;
            }
            else
            {
                char *passwd = (char *)malloc(PASSWORD_LEN);
                strcpy(passwd,temp);
                c_info->passwd = passwd;
            }
            strcpy(temp,"");
            sig_index++;
        }
    }
    return c_info;
}

/**
 * 打印客户端帮助信息
 */
void
help()
{
    printf("--------------------->  Help        List  <---------------------\n");
    printf("\t\t\tremote operation : \n");
    printf("\t\tpwd\t\t显示当前路径\n");
    printf("\t\tls\t\t查看当前目录下的所有文件\n");
    printf("\t\tcd\t\t切换目录\n");
    printf("\t\tmkdir\t\t创建文件夹\n");
    printf("\t\trmdir\t\t删除文件夹\n");
    printf("\t\tput\t\t上传文件\n");
    printf("\t\tget\t\t下载文件\n");

    printf("\t\t\tlocal operation : \n");
    printf("\t\tlpwd\t\t显示当前路径\n");
    printf("\t\tdir\t\t查看当前目录下的所有文件\n");
    printf("\t\tlcd\t\t切换目录\n");
    printf("\t\tlmkdir\t\t创建文件夹\n");
    printf("\t\tlrmdir\t\t删除文件夹\n");
    printf("\t\tquit\t\t退出客户端\n");
    printf("----------------------------------------------------------------\n");
}

void
displayConnectionInfo(char *serverIP,in_port_t serverPort,int number)
{
    printf("----------------------------------------------------------------\n");
    printf("%sConnected to %s:%d\n",CM,serverIP,serverPort,number);
    printf("%sYou are client No.%d\n",CM,number);
    help();
}

/**
 * 客户端连接服务器
 *
 * @param serverIP
 * @param serverPort
 * @return 成功返回连接到的服务器的socket fd；失败返回INVALID_SOCKET
 */
int
connectFtpServer(char *serverIP,in_port_t serverPort){
	int s_sockfd;
	socklen_t s_socklen;
	struct sockaddr_in s_sockaddr;

	s_sockfd = socket(AF_INET,SOCK_STREAM,0);

	if (s_sockfd == INVALID_SOCKET)
	{
		printf("%sSocket error\n",CM);
		return INVALID_SOCKET;
	}

	s_sockaddr.sin_family = AF_INET;
	s_sockaddr.sin_addr.s_addr = inet_addr(serverIP);
	s_sockaddr.sin_port = htons(serverPort);

	s_socklen = sizeof(s_sockaddr);

	if(connect(s_sockfd,(struct sockaddr *)&s_sockaddr,s_socklen) == INVALID_SOCKET)
	{
		printf("%sConnect error\n",CM);
		return INVALID_SOCKET;
	}
	return s_sockfd;
}