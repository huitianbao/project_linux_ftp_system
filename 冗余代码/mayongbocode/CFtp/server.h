//
// Created by yobol on 16-11-7.
//

#include "common.h"
#include <netdb.h>
#include <net/if.h>

#include <crypt.h>
#include <shadow.h>
#include <pwd.h>
#include <grp.h>
#include "thread.h"

#define NT_IF_NAME1 "lo"
#define NT_IF_NAME2 "eth0"
#define NT_IF_NAME3 "ens33"


int startControlThread(pthread_t *serverThread,void *parm);
/**
 * 根据网卡接口名字，获取对应接口的IP地址
 *
 * @param ifname 网卡接口名
 * @param ip IP地址
 * @return 成功返回0；失败则返回-1
 */
int getLocalIP(char *ifname, char *ip)
{
    char *temp;
    int inet_sock;
    struct ifreq ifr;

    inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

    memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    memcpy(ifr.ifr_name, ifname, strlen(ifname));

    if(0 != ioctl(inet_sock, SIOCGIFADDR, &ifr))//没有对应的设备
    {
        return -1;
    }

    temp = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
    strcpy(ip,temp);

    close(inet_sock);

    return 0;
}

/**
 * 获取当前处于活跃状态的主机的IP
 *
 * @return IP地址
 */
char *getHostIPAddr()
{
    char *s_hostname,**addrs;
    struct hostent *s_hostinfo;

    char hostname[256];
    gethostname(hostname,255);
    s_hostname = hostname;
    s_hostinfo = gethostbyname(s_hostname);
    if (s_hostinfo == NULL)
    {
        perror("server > Fail to get default host ip address");
        exit(1);
    }

    if (s_hostinfo -> h_addrtype != AF_INET)
    {
        printf("server > Not an IP host\n");
        exit(1);
    }

    addrs = s_hostinfo -> h_addr_list;
    while(*addrs)
    {
        printf(" %s",inet_ntoa(*(struct in_addr *)*addrs));
        addrs++;
    }
    printf("\n");

    return inet_ntoa(*(struct in_addr *)*s_hostinfo->h_addr_list);
}

/**
 * 分析程序入口提供的参数，获取服务器IP地址和端口号
 *
 * @param argc
 * @param argv
 * @param s_ipaddr 服务器IP地址
 * @return 服务器端口号
 */
int getServerIPAndPortFromArguments(const int argc, char **argv,char *s_ipaddr)
{
    int s_port;
    switch (argc){
        case 1:
            printf("%sNot provide port\n",SM);
            exit(1);
        case 2:
            //将本地网络的IP的作为服务器IP
            if (getLocalIP(NT_IF_NAME2,s_ipaddr) == -1)
            {
                if (getLocalIP(NT_IF_NAME3,s_ipaddr) == -1){
                    if (getLocalIP(NT_IF_NAME1,s_ipaddr) == -1){
                        exit(1);
                    }
                }
            }
            s_port = atoi(argv[1]);
            break;
        case 3:
            //将argv[1]提供的IP作为服务器IP
            if (verifyIPAddrFormat(argv[1]) != 1)
            {
                printf("%sIP format error\n",SM);
                exit(1);
            }
            strcpy(s_ipaddr,argv[1]);
            s_port = atoi(argv[2]);//atoi函数将字符串转换为整数
            break;
        default:
            printf("%sArguments exception\n",SM);
            exit(1);
    }
    return s_port;
}

/**
 * 打印帮助信息
 */
void help()
{
    printf("--------------------->  Help        List  <---------------------\n");
    printf("\t\tlist\t\t列出在线用户\n");
    printf("\t\tkill\t\t踢出在线用户\n");
    printf("\t\tpwd\t\t显示当前路径\n");
    printf("\t\tls\t\t查看当前目录下的所有文件\n");
    printf("\t\tcd\t\t切换目录\n");
    printf("\t\tmkdir\t\t创建文件夹\n");
    printf("\t\trmdir\t\t删除文件夹\n");
    printf("\t\tquit\t\t退出服务器\n");
    printf("----------------------------------------------------------------\n");
}

/**
 * 显示服务器信息
 *
 * @param port 服务器端口号
 */
void displayServerInfo(char * ipaddr,int port)
{
    char **s_ipaddr;
    struct hostent *s_hostinfo;

    struct sockaddr_in s_sockaddr;

    if (!inet_aton(ipaddr,&s_sockaddr.sin_addr))//调用inet_aton将点分十进制转化为in_addr
    {
        printf("%sFail to parse IP address\n",SM);
        exit(1);
    }

    if ((s_hostinfo = gethostbyaddr((void *)&s_sockaddr.sin_addr,4,AF_INET)) == NULL)
    {
        printf("%sFail to get server info\n",SM);
        exit(1);
    }

    printf("----------------------------------------------------------------\n");
    printf("----------------------Start server success----------------------\n");
    if (s_hostinfo -> h_addrtype != AF_INET)//查询的主机不是一个IP主机
    {
        printf("%sFail to resolve the current host\n",SM);
        exit(1);
    }

    printf("\tHost\tName\t: %s\n",s_hostinfo->h_name);

    s_ipaddr = s_hostinfo -> h_addr_list;
    while(*s_ipaddr)
    {
        printf("\tHost\tIP\t: %s\n",inet_ntoa(*(struct in_addr *)*s_ipaddr));
        s_ipaddr++;
    }
    printf("\tHost\tPort\t: %d\n",port);
    help();
}

/**
 * 启动FTP服务端
 *
 * @param s_ipaddr 服务器IP地址
 * @param s_port 服务器端口号
 * @return 服务器socket的fd ; 失败则返回INVALID_SOCKET
 */
int startFTPServer(char *s_ipaddr,int s_port) {
    int s_sockfd;
    int s_socklen;
    struct sockaddr_in s_sockaddr;
    pthread_t server_thread;

    s_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_sockfd == INVALID_SOCKET) {
        printf("%s",SM);
        perror("socket error");
        return INVALID_SOCKET;
    }

    s_sockaddr.sin_family = AF_INET;
    s_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
//    s_sockaddr.sin_addr.s_addr = htonl(inet_addr(s_ipaddr));
    s_sockaddr.sin_port = htons((unsigned int) s_port);
    s_socklen = sizeof(s_sockaddr);

    if (bind(s_sockfd, (struct sockaddr *) &s_sockaddr, s_socklen) == INVALID_SOCKET)
    {
        printf("%s",SM);
        perror("bind error");
        return INVALID_SOCKET;
    }

    displayServerInfo(s_ipaddr,s_port);
//    startControlThread(&server_thread,NULL);

    if (listen(s_sockfd,SOMAXCONN) == INVALID_SOCKET)
    {
        printf("%s",SM);
        perror("server > listen error");
        return INVALID_SOCKET;
    }

    printf("%sWaiting......\n",SM);

    return s_sockfd;

}

/**
 * 接收客户请求函数
 *
 * @param s_sockfd 服务器的socket的fd
 * @param clientIP 客户端ip地址
 * @return
 */
int
acceptClientRequest(int s_sockfd,in_addr_t *clientIP)
{
    struct sockaddr_in c_sockaddr;
    int c_socklen= sizeof(c_sockaddr);

    int c_sockfd = accept(s_sockfd,(struct sockaddr *)&c_sockaddr,(socklen_t *)&c_socklen);

    if (c_sockfd < 0)
    {
        printf("%s",SM);
        perror("accept error\n");
        return INVALID_SOCKET;
    }

    if (clientIP)
    {
        *clientIP = c_sockaddr.sin_addr.s_addr;
    }

    return c_sockfd;

}

/**
 * 从logininfo登陆信息字符串中解析出用户名和密码
 *
 * @param logininfo
 * @param username
 * @param passwd
 */
void parseLoginInfo(char *logininfo,char *username,char *passwd)
{
    int len = strlen(logininfo);
    char separator = ':';
    int i;
    for (i = 0; i < len; i++) {
        if (logininfo[i] == separator)
        {
            strncpy(username,&logininfo[0],i);
            break;
        }
    }

    strcpy(passwd,&logininfo[i + 1]);
}

/**
 * 获取salt
 *
 * @param salt
 * @param passwd
 */
void getSalt(char *salt,char *passwd)
{
    int i,j;

    //取出salt，i记录密码字符下标，j记录密码$出现次数
    for(i = 0,j = 0;passwd[i] && j != 3;++i)
    {
        if(passwd[i] == '$')
        {
            ++j;
        }

    }

    strncpy(salt,passwd,i-1);
}

/**
 * 验证用户信息信息
 *
 * @param username 用户名
 * @param password 用户密码
 * @return 成功返回1，否则返回-1
 */
int checkUserInfo(char *username,char *password)
{
    struct spwd *sp;
    char salt[512] = {0};

    //根据用户名获取spwd结构类型的数据
    if((sp = getspnam(username)) == NULL)
    {
        printf("%sPermission denied or user{%s} does't exist\n",SM,username);
        return -1;
    }

    getSalt(salt,sp->sp_pwdp);

    //进行密码验证
    if (strcmp(sp->sp_pwdp,crypt(password,salt)) != 0)
    {
        printf("%sUser{%s} password is error\n",SM,username);
        return -1;
    }

    //将指针移动到passwd文件开头
    setspent();

    //关闭passwd文件
    endspent();

    return 1;
}

/**
 * 根据用户名获得用户所在组名
 *
 * @param username
 * @return
 */
char * getGroupName(char *username)
{
    struct passwd *pw = getpwnam(username);
    struct group *grp = getgrgid(pw->pw_gid);
    return grp->gr_name;
}

/**
 * 用户是否有权访问某个目录
 *
 * @param username
 * @param path
 * @return 有权返回1；无权力返回0
 */
int isAuthorized(char *username,char *path)
{
    struct passwd *pw = getpwnam(username);
    struct group *grp = getgrgid(pw->pw_gid);

    if (strlen(path) < strlen(pw->pw_dir))//越权访问
    {
        return 0;
    }

    if (strncmp(path,pw->pw_dir,strlen(pw->pw_dir)) == 0)
    {
        return 1;
    }
    return 0;
}
