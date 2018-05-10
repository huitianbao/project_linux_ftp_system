//
// Created by yobol on 16-11-7.
//
#include "server.h"
#include <signal.h>

void handleTermSig();
int createNewThreadForClient(pthread_t *thread,int c_sockfd,in_addr_t *clientIP);
void listCurrentUsers();
int killUser(char *username);
/**
 * 执行   ftpserver [IP地址] port   启动FTP服务端
 * 若不提供IP地址，则FTP服务自动绑定到本地网络的IP
 *
 * @param argc
 * @param argv
 * @return
 */
int currentClients = 0;//当前连接客户数
ClientInfo clients[FD_SIZE];//当前在线用户
pthread_mutex_t count_mutex;/* 保护count 和 clients */

int s_sockfd,c_sockfd;//服务器和客户端的socket fd
int sockfd;

fd_set sockfds,sockfds_tmp;
int select_result = 0;

const char *clientRootDir = "/usr/myftp";/* 服务器客户根目录 */
const char *rootDir = "/usr/myftp/root";/* 服务器文件根目录 */
const char *upldDir = "/usr/myftp/root/upload";/* 上传目录 */
const char *dwldDir = "/usr/myftp/root/download";/* 下载目录 */
char *currDir;/* 当前目录 */

int main(int argc,char **argv)
{
    char *s_ipaddr;//服务器IP地址
    int s_port;//服务器端口号

    in_addr_t *clientIP = NULL;//可以保存客户端IP

    currDir = (char *)malloc(PATH_LEN);
    strcpy(currDir,"/usr/myftp/root");

    signal(SIGTERM,handleTermSig);//调用handleTermSig函数，处理SIGTERM信号
    createMutex(&count_mutex);

    s_port = getServerIPAndPortFromArguments(argc,argv,s_ipaddr);

    s_sockfd = startFTPServer(s_ipaddr,s_port);
    if (s_sockfd == INVALID_SOCKET)
    {
        exit(1);
    }

    FD_ZERO(&sockfds);
    FD_SET(s_sockfd,&sockfds);
    FD_SET(0,&sockfds);
    while(1)
    {
        sockfds_tmp = sockfds;

        if ((select_result = select(FD_SIZE,&sockfds_tmp,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *)NULL)) < 1)
        {
            printf("%s",SM);
            perror("Select error");
            exit(1);
        }

        if (FD_ISSET(s_sockfd,&sockfds_tmp))//发生在套接字s_sockfd上，肯定是一个新的连接请求
        {
            //将相关的c_sockfd添加到描述符集合中
            c_sockfd = acceptClientRequest(s_sockfd,clientIP);
            if (c_sockfd == INVALID_SOCKET)
            {
                exit(1);
            }

            pthread_t thread;//客户线程
            if (createNewThreadForClient(&thread,c_sockfd,clientIP) == -1)
            {
                printf("%sFail to create new thread for client\n",SM);
            }

            FD_SET(c_sockfd,&sockfds);
        }

        if (FD_ISSET(0,&sockfds_tmp))
        {
            int sread;
            char buffer[COMMAND_MAX];
            ioctl(0,FIONREAD,&sread);
            if (sread == 0) {
                exit(0);
            }

            sread = read(0, buffer, sread);//当用户按下回车键，会在输入字符后添加\r或者\n
            buffer[sread - 1] = '\0';

            if (strcmp(buffer, "quit") == 0)//退出服务器
            {
                printf("%sServer quit\n", SM);
                exit(1);
            }
            else
            {
                if (strcmp(buffer, "list") == 0)//列出当前在线用户
                {
                    listCurrentUsers();
                }
                else if (strncmp(buffer,"kill",4) == 0)//踢出用户
                {
                    char *name = (char *)malloc(USERNAME_LEN);
                    strcpy(name,&buffer[5]);
                    int cfd;
                    if ((cfd = killUser(name) )!= -1)
                    {
                        printf("%sKill user success\n",SM);
                        lockMutex(&count_mutex);
                        clients[cfd].c_sockfd = 0;
                        unlockMutex(&count_mutex);
                        FD_CLR(cfd,&sockfds);
                    }
                }
                else if (strcmp(buffer, "pwd") == 0)//查看当前目录路径
                {
                    fm_pwd(currDir);
                }
                else if (strcmp(buffer, "ls") == 0)//查看当前目录下的目录项目
                {
                    printf("%s",fm_ls(currDir));
                }
                else if (strncmp(buffer, "cd",2) == 0)//查看当前目录下的目录项目
                {
                    char *target = (char *)malloc(PATH_LEN);
                    strcpy(target,&buffer[3]);
                    currDir = fm_cd(currDir,target);
                    fm_pwd(currDir);
                }
                else if (strncmp(buffer, "mkdir",5) == 0)//在当前目录下创建新目录
                {
                    char *target = (char *)malloc(PATH_LEN);
                    strcpy(target,&buffer[6]);
                    fm_mkdir(currDir,target,S_IRUSR | S_IWUSR | S_IXUSR);
                }
                else if (strncmp(buffer, "rmdir",5) == 0)//查看当前目录下的删除指定目录
                {
                    char *target = (char *)malloc(PATH_LEN);
                    strcpy(target,&buffer[6]);
                    fm_rmdir(currDir,target);
                }
                else
                {
                    if (buffer[0] != '\0')
                    {
                        printf("%sDon't understand this command\n",SM);
                    }
                }
            }

            printf("ftpServer > ");
            fflush(stdout);
        }

    }
}

/**
 * 退出客户线程
 */
void
exitClientTHread(int c_sockfd)
{
    FD_CLR(c_sockfd,&sockfds);
    close(c_sockfd);
    printf("%sDetach the thread of c_sockfd %d\n",SM,c_sockfd);
    printf("%sRemoving client on c_sockfd %d\n",SM,c_sockfd);
    lockMutex(&count_mutex);
    if (currentClients > 0) currentClients--;
    printf("%sCurrent clients : %d\n",SM,currentClients);
    unlockMutex(&count_mutex);
    pthread_exit(NULL);
}

/**
 * 开启与客户端的会话
 *
 * @param parm
 * @return
 */
void *
startSessionWithClient(void *parm)
{
    char data[MSG_LEN];//在服务器和客户端之间传递数据的缓冲区
    char *command;
    int real_copy_num;//实际从recv中获取的字符个数
    char *clientCurrPath = (char *)malloc(PATH_LEN);//客户在服务器的当前路径
    strcpy(clientCurrPath,clientRootDir);
    char *lsRes = (char *)malloc(MSG_DATA_LEN);/* 向客户端发送ls的结果 */
    char *target = (char *)malloc(PATH_LEN);

    Msg *sMsg,*rMsg;
    sMsg = malloc(MSG_LEN);
    sMsg->msg_header = malloc(MSG_HEADER_LEN);
    sMsg->msg_data = malloc(MSG_DATA_LEN);
    rMsg = malloc(MSG_LEN);
    rMsg->msg_header = malloc(MSG_HEADER_LEN);

    int tMode = 0;/* 文件传输模式，默认为0，表示ASCII文本传输模式 */
    int filesize = 0;//文件大小
    int recvsize = 0;//已经接收的文件字节数
    int items = 0;//要接收的次数
    int curritem = 0;//当前已接收的次数
    int clientfilesize = 0;//当客户端GET时，客户端文件大小
    char *filename = (char *)malloc(PATH_LEN);/* 若target为相对路径，则用path保存其绝对路径 */
    FILE *file;
    unsigned char buffer[MSG_DATA_LEN];

    ClientInfo *clientInfo = (ClientInfo *)parm;
    int c_sockfd = clientInfo->c_sockfd;
    printf("%sNew client thread started\n",SM);

    //从客户端接收登陆信息
    if (recvMsg(c_sockfd,rMsg) != 0)
    {
        //接收登陆信息失败，向客户端传递失败信号
        sMsg->msg_header->msg_type = FTP_RESULT_LOGINFAILURE;
        sMsg->msg_header->msg_bytes = sizeof(uint32_t);
        sendMsg(c_sockfd,sMsg);
        exitClientTHread(c_sockfd);
    }

    char *username = (char *)malloc(USERNAME_LEN);
    char *passwd = (char *)malloc(PASSWORD_LEN);

    parseLoginInfo((char *)rMsg->msg_data,username,passwd);

    if (checkUserInfo(username,passwd) == 1)
    {

        strcat(clientCurrPath,"/");
        strcat(clientCurrPath,getGroupName(username));//进入客户所在组目录
        strcat(clientCurrPath,"/");
        strcat(clientCurrPath,username);//进入到客户主目录

        printf("%sAdd client on c_sockfd %d\n",SM,c_sockfd);
        lockMutex(&count_mutex);
        currentClients++;
        clients[c_sockfd].c_sockfd = c_sockfd;
        clients[c_sockfd].username = username;
        printf("%sCurrent clients : %d\n",SM,currentClients);
        unlockMutex(&count_mutex);

        //向客户端返回登陆结果
        sMsg->msg_header->msg_type = FTP_RESULT_LOGINSUCCESS;
        char num[128];
        sprintf(num,"%d",currentClients);
        sMsg->msg_data = num;
        sMsg->msg_header->msg_bytes = strlen(num);
        if (sendMsg(c_sockfd,sMsg) != 0)
        {
            exitClientTHread(c_sockfd);
        }

        for(;;)
        {
            //获取客户端传递的命令
            //当获取到来自客户端的退出命令后，退出线程
            if(recvMsg(c_sockfd,rMsg) != 0)
            {
                exitClientTHread(c_sockfd);
            }

            //客户端发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
//            command = (char *)rMsg->msg_data;
//            command[rMsg->msg_header->msg_bytes] = '\0';
            real_copy_num = rMsg->msg_header->msg_bytes;
            if (rMsg->msg_header->msg_type == FTP_PWD)
            {
                //向客户端返回用户所在服务器目录
                sMsg->msg_header->msg_type = FTP_PWD;
                sMsg->msg_header->msg_bytes = strlen(clientCurrPath) + 1;
                sMsg->msg_data = clientCurrPath;
                sendMsg(c_sockfd,sMsg);
            }
            else if (rMsg->msg_header->msg_type == FTP_LS)
            {
                if ((lsRes = fm_ls(clientCurrPath)) != NULL)
                {
                    //向客户端返回用户所在服务器目录下的文件内容
                    sMsg->msg_header->msg_type = FTP_LS;
                    sMsg->msg_header->msg_bytes = strlen(lsRes) + 1;
                    sMsg->msg_data = lsRes;
                    sendMsg(c_sockfd,sMsg);
                }
            }
            else if (rMsg->msg_header->msg_type == FTP_CD)
            {
                char *previous = (char *)malloc(PATH_LEN);
                strcpy(previous,clientCurrPath);
                target = (char *)rMsg->msg_data;
                target[real_copy_num] = '\0';

                if ((clientCurrPath = fm_cd(clientCurrPath,target)) != NULL)
                {

                    if (isAuthorized(username,clientCurrPath) == 0)
                    {
                        strcpy(clientCurrPath,previous);
                        sMsg->msg_header->msg_type = FTP_RESULT_FAILURE;
                        sMsg->msg_header->msg_bytes = 1;
                        sMsg->msg_data = "\0";
                    }
                    else
                    {
                        sMsg->msg_header->msg_type = FTP_CD;
                        sMsg->msg_header->msg_bytes = strlen(clientCurrPath) + 1;
                        sMsg->msg_data = clientCurrPath;
                    }
                    sendMsg(c_sockfd,sMsg);
                }
            }
            else if (rMsg->msg_header->msg_type == FTP_MKDIR)
            {
                char *aPath = (char *)malloc(PATH_LEN);/* 若target为相对路径，则用path保存其绝对路径 */

                target = (char *)rMsg->msg_data;
                target[real_copy_num] = '\0';

                strcpy(aPath,clientCurrPath);
                strcat(aPath,"/");
                strcat(aPath,target);

                if (isAuthorized(username,target) == 0)
                {
                    if (isAuthorized(username,aPath) == 0)
                    {
                        sMsg->msg_header->msg_type = FTP_RESULT_FAILURE;
                    }
                    else
                    {
                        fm_mkdir(clientCurrPath,target,USER_AUTH);
                        sMsg->msg_header->msg_type = FTP_MKDIR;
                    }
                }
                else
                {
                    fm_mkdir(clientCurrPath,target,USER_AUTH);
                    sMsg->msg_header->msg_type = FTP_MKDIR;
                }
                sMsg->msg_header->msg_bytes = 1;
                sMsg->msg_data = "\0";
                sendMsg(c_sockfd,sMsg);
            }
            else if (rMsg->msg_header->msg_type == FTP_RMDIR)
            {
                char *aPath = (char *)malloc(PATH_LEN);/* 若target为相对路径，则用path保存其绝对路径 */
                char *pre = (char *)malloc(PATH_LEN);

                target = (char *)rMsg->msg_data;
                target[real_copy_num] = '\0';
                strcpy(pre,target);

                if (strncmp(target,"-",1) == 0)//判断模式
                {
                    strncpy(target,&target[3],strlen(target) - 3);
                    target[strlen(target) - 3] = '\0';
                }

                strcpy(aPath,clientCurrPath);
                strcat(aPath,"/");
                strcat(aPath,target);

                if (isAuthorized(username,target) == 0)
                {
                    if (isAuthorized(username,aPath) == 0)
                    {
                        sMsg->msg_header->msg_type = FTP_RESULT_FAILURE;
                    }
                    else
                    {
                        if (fm_rmdir(clientCurrPath,pre) == -1)
                        {
                            sMsg->msg_header->msg_type = FTP_RESULT_FAILURE;
                        }
                        else
                        {
                            sMsg->msg_header->msg_type = FTP_RMDIR;
                        }
                    }
                }
                else
                {
                    if (fm_rmdir(clientCurrPath,pre) == -1)
                    {
                        sMsg->msg_header->msg_type = FTP_RESULT_FAILURE;
                    }
                    else
                    {
                        sMsg->msg_header->msg_type = FTP_RMDIR;
                    }
                }

                sMsg->msg_header->msg_bytes = 1;
                sMsg->msg_data = "\0";
                sendMsg(c_sockfd,sMsg);
            }
            else if (rMsg->msg_header->msg_type == FTP_ASCII)
            {
                sMsg->msg_header->msg_type = FTP_RESULT_SUCCESS;
                sMsg->msg_header->msg_bytes = 1;
                sMsg->msg_data = "\0";
                if (sendMsg(c_sockfd,sMsg) == 0)
                {
                    tMode = 0;
                }

            }
            else if (rMsg->msg_header->msg_type == FTP_BINARY)
            {
                sMsg->msg_header->msg_type = FTP_RESULT_SUCCESS;
                sMsg->msg_header->msg_bytes = 1;
                sMsg->msg_data = "\0";
                if (sendMsg(c_sockfd,sMsg) == 0)
                {
                    tMode = 1;
                }
            }
            else if (rMsg->msg_header->msg_type == FTP_GET)
            {
                //接收文件名
                target = (char *)rMsg->msg_data;
                target[real_copy_num] = '\0';
                strcpy(filename,target);
                chdir(clientCurrPath);

                if (isExisted(filename) == 0)//文件不存在
                {
                    sMsg->msg_header->msg_type = FTP_RESULT_FAILURE;
                    sMsg->msg_header->msg_bytes = 1;
                    sMsg->msg_data = "\0";
                    sendMsg(c_sockfd,sMsg);
                    continue;
                }
                else
                {
                    sMsg->msg_header->msg_type = FTP_RESULT_SUCCESS;
                    sMsg->msg_header->msg_bytes = 1;
                    sMsg->msg_data = "\0";
                    sendMsg(c_sockfd,sMsg);
                }
            }
            else if (rMsg->msg_header->msg_type == FTP_GET_FLSZ)
            {
                //接收文件大小
                clientfilesize = real_copy_num;
                fm_put(c_sockfd,clientCurrPath,filename,clientfilesize,tMode);
            }
            else if (rMsg->msg_header->msg_type == FTP_PUT)
            {
                //接收文件名
                target = (char *)rMsg->msg_data;
                target[real_copy_num] = '\0';
                strcpy(filename,target);
                chdir(clientCurrPath);
                if (!isExisted(filename))
                {
                    //文件不存在，以可写形式打开文件
                    if ((file = fopen(filename,"w+")) == NULL)
                    {
                        exitClientTHread(c_sockfd);
                    }
                    sMsg->msg_header->msg_type = FTP_FILE_NOTEXISTED;
                    sMsg->msg_header->msg_bytes = 1;
                }
                else
                {
                    struct stat statbuf;
                    //文件存在，以附加形式打开文件
                    if ((file = fopen(filename,"a+")) == NULL)
                    {
                        exitClientTHread(c_sockfd);
                    }
                    lstat(filename,&statbuf);
                    sMsg->msg_header->msg_type = FTP_FILE_EXISTED;
                    sMsg->msg_header->msg_bytes = statbuf.st_size;
                }
                sMsg->msg_data = "\0";
                sendMsg(c_sockfd,sMsg);
            }
            else if (rMsg->msg_header->msg_type == FTP_PUT_FLSZ)
            {
                filesize = real_copy_num;
                items = (filesize > 0) ? (filesize / MSG_DATA_LEN + 1) : 0;
//                printf("\t要接收的文件大小 : %d\n",filesize);//test
            }
            else if(rMsg->msg_header->msg_type == FTP_PUT_FILE)
            {
                switch (tMode)
                {
                    case 0:/* 文本模式 */
                        target = (char *)rMsg->msg_data;
                        target[real_copy_num] = 0;

                        if(recvsize < filesize)
                        {
                            fputs(target,file);
                            fflush(file);
                            memset(target,0,MSG_DATA_LEN);
                            recvsize += real_copy_num;
                        }
                        fflush(file);
                        memset(target,0,MSG_DATA_LEN);
                    break;
                    case 1:/* 二进制模式 */
                        target = (char *)rMsg->msg_data;
                        if(curritem < items)
                        {
                            fwrite(target,real_copy_num,1,file);
                            fflush(file);
                            memset(target,0,MSG_DATA_LEN);
                            curritem++;
                        }
                        fflush(file);
                        memset(target,0,MSG_DATA_LEN);
                    break;
                }
            }
        }
    }
    else
    {
        printf("%sFail to login in\n",SM);
        //向客户端返回登陆结果
        sMsg->msg_header->msg_type = FTP_RESULT_LOGINFAILURE;
        sMsg->msg_header->msg_bytes = sizeof(uint32_t);
        sendMsg(c_sockfd,sMsg);
        exitClientTHread(c_sockfd);
    }
}

/**
 * 为新登陆的客户创建新的专属线程
 *
 * @param c_sockfd
 * @param clientIP
 * @return 成功返回0；失败返回-1
 */
int
createNewThreadForClient(pthread_t *client_thread,int c_sockfd,in_addr_t *clientIP)
{
    ClientInfo *client_info = NULL;

    client_info = (ClientInfo *)malloc(sizeof(ClientInfo));
    if (client_info == NULL)
    {
        printf("%sFail to malloc memory area for ClientInfo\n",SM);
        return -1;
    }

    client_info -> c_sockfd = c_sockfd;
    client_info -> clientIp = clientIP;

    if (createNewThread(client_thread,&startSessionWithClient,client_info) != 0){
        return -1;
    }

    return 0;
}

/**
 * 处理终止信号
 */
void
handleTermSig(int signo)
{
    if (signo == SIGTERM)
    {
        return;
    }
    exit(0);
}

/**
 * 列出当前在线用户
 */
void
listCurrentUsers()
{
    printf("%sCurrent clients : %d\n",SM,currentClients);
    if (currentClients == 0)
    {
        return;
    }
    printf("----------------------------------------------------------------\n");
    printf("\t\tc_fd\t: username\n");
    for (int i = 0; i < FD_SIZE; ++i) {
        if (clients[i].c_sockfd != 0)
        {
            printf("\t\t%d\t: %s\n",clients[i].c_sockfd,clients[i].username);
        }
    }
}
