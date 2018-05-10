//
// Created by yobol on 16-11-7.
//
#include "client.h"

/**
 * 执行   myftp:username:password@ftp-server-ip:ftp-server-port   登陆ftpserver
 *
 * @param argc
 * @param argv
 * @return
 */
char *currDir;/* 当前目录 */

int
main(int argc,char **argv)
{
    ClientLoginInfo *clientLoginInfo;//用户登陆信息

    int tMode = 0;/* 传输模式，默认为ASCII文本模式 */
    int seek = 0;//从文件指定位置开始传输

    char buffer[COMMAND_MAX];
    int nread;

    //socket fd集
    fd_set inputfds,tmpfds;
    //最大等候时长
    struct timeval timeout;

    int result;

    currDir = (char *)malloc(PATH_LEN);
    strcpy(currDir,getenv("HOME"));

    Msg *sMsg,*rMsg;
    char *username,*passwd;
    char *login;

    clientLoginInfo = parseArguments(argc,argv);

    int s_sockfd = connectFtpServer(clientLoginInfo->ipaddr,clientLoginInfo->port);
    if (s_sockfd == INVALID_SOCKET)
    {
        exit(1);
    }

    //发送用户名和密码到服务器
    login = malloc(LOGININFO_LEN);
    username = clientLoginInfo->username;
    passwd = clientLoginInfo->passwd;
    strcat(login,username);
    strcat(login,":");
    strcat(login,passwd);

    sMsg = malloc(MSG_LEN);
    sMsg->msg_header = malloc(MSG_HEADER_LEN);
    sMsg->msg_data = malloc(sizeof(ClientLoginInfo));
    rMsg = malloc(MSG_LEN);
    rMsg->msg_header = malloc(MSG_HEADER_LEN);
    sMsg->msg_header->msg_type = FTP_LOGIN;
    sMsg->msg_header->msg_bytes = strlen(login);
    sMsg->msg_data = login;

    if (sendMsg(s_sockfd,sMsg) != 0)
    {
        exit(1);
    }
    if (recvMsg(s_sockfd,rMsg) != 0)
    {
        exit(1);
    }
    if (rMsg->msg_header->msg_type == FTP_RESULT_LOGINFAILURE)
    {
        printf("%sLogin fail,username doesn't exist or password is error\n",CM);
        close(s_sockfd);
        exit(1);
    }

    displayConnectionInfo(clientLoginInfo->ipaddr,clientLoginInfo->port,atoi((char *)rMsg->msg_data));

    FD_ZERO(&inputfds);
    FD_SET(0,&inputfds);

    while (615)
    {
        tmpfds = inputfds;
        //两分钟内无操作则退出断开连接
        timeout.tv_sec = 120;

        printf("myFtp > ");
        fflush(stdout);

        result = select(FD_SIZE,&tmpfds,(fd_set *)NULL,(fd_set *)NULL,&timeout);

        switch (result)
        {
            case 0:
                printf("\n%sTimeout\n",CM);
                exit(1);
            case -1:
                printf("%sSelect error\n",CM);
                exit(1);
            default:
                if (FD_ISSET(0,&tmpfds)) {
                    ioctl(0, FIONREAD, &nread);
                    if (nread == 0) {
                        exit(0);
                    }

                    nread = read(0, buffer, nread);//当用户按下回车键，会在输入字符后添加\r或者\n
                    buffer[nread - 1] = '\0';

                    if (strcmp(buffer, "quit") == 0)//退出客户端
                    {
                        printf("%sClient quit\n", CM);
                        close(s_sockfd);
                        exit(0);
                    }
                    else
                    {
                        if (strcmp(buffer,"pwd") == 0)//显示当前路径
                        {
                            sMsg->msg_header->msg_type = FTP_PWD;
                            sMsg->msg_header->msg_bytes = strlen(buffer);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = buffer;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }
                            /* 根据结果执行响应处理 */
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_PWD)
                            {
                                fm_pwd(rMsg->msg_data);
                            }
                        }
                        else if (strcmp(buffer,"ls") == 0)//查看当前目录下的所有文件
                        {
                            sMsg->msg_header->msg_type = FTP_LS;
                            sMsg->msg_header->msg_bytes = strlen(buffer);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = buffer;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }
                            /* 根据结果执行响应处理 */
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_LS)
                            {
                                printf("%s",rMsg->msg_data);
                            }

                        }
                        else if (strncmp(buffer,"cd",2) == 0)//切换目录
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[3]);

                            if (strlen(buffer) == 2 || strlen(buffer) == 3 || strncmp(target," ",1) == 0 || strncmp(target,"  ",2) == 0)
                            {
                                continue;
                            }

                            sMsg->msg_header->msg_type = FTP_CD;
                            sMsg->msg_header->msg_bytes = strlen(target);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = target;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            /* 根据结果执行响应处理 */
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_CD)
                            {
                                fm_pwd((char *)rMsg->msg_data);
                            }
                            else
                            {
                                printf("\tPermission denied\n");
                            }
                        }
                        else if (strncmp(buffer,"mkdir",5) == 0)//创建文件夹
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[6]);

                            if (strlen(buffer) == 5 || strlen(buffer) == 6 || strncmp(target," ",1) == 0 || strncmp(target,"  ",2) == 0)
                            {
                                continue;
                            }

                            sMsg->msg_header->msg_type = FTP_MKDIR;
                            sMsg->msg_header->msg_bytes = strlen(target);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = target;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            /* 根据结果执行响应处理 */
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_MKDIR)
                            {
                                printf("\tCreate success\n");
                            }
                            else
                            {
                                printf("\tCreate fail\n");
                            }
                        }
                        else if (strncmp(buffer,"rmdir",5) == 0)//删除文件夹
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[6]);

                            if (strlen(buffer) == 5 || strlen(buffer) == 6 || strncmp(target," ",1) == 0 || strncmp(target,"  ",2) == 0)
                            {
                                continue;
                            }

                            sMsg->msg_header->msg_type = FTP_RMDIR;
                            sMsg->msg_header->msg_bytes = strlen(target);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = target;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            /* 根据结果执行响应处理 */
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_RMDIR)
                            {
                                printf("\tDelete success\n");
                            }
                            else
                            {
                                printf("\tPermission denied,delete fail\n");
                            }
                        }
                        else if (strncmp(buffer,"put",3) == 0)//上传文件
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[4]);

                            if (!isExisted(target))
                            {
                                printf("\tFile is not existed\n");
                                continue;
                            }

                            //发送文件名到服务器
                            sMsg->msg_header->msg_type = FTP_PUT;
                            sMsg->msg_header->msg_bytes = strlen(target);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = target;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            /* 根据结果执行响应处理 */
                            //文件是否存在于服务器
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_FILE_NOTEXISTED)
                            {
                                seek = 0;
                            }
                            else if (rMsg->msg_header->msg_type == FTP_FILE_EXISTED)
                            {
                                seek = rMsg->msg_header->msg_bytes;
                            }

                            /* 根据tMode确定写文件模式 */
                            if (fm_put(s_sockfd,currDir,target,seek,tMode) == -1)
                            {
                                printf("\tPut fail\n");
                            }
                        }
                        else if (strncmp(buffer,"get",3) == 0)//下载文件
                        {
                            FILE *file;
                            struct stat statbuf;
                            int filesize = 0;
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[4]);

                            chdir(currDir);
                            lstat(target,&statbuf);
                            //判断本地是否存在文件
                            if (isExisted(target))
                            {
                                //本地存在文件
                                filesize = statbuf.st_size;
                            }
                            else
                            {
                                //本地不存在文件
                                filesize = 0;
                            }

                            sMsg->msg_header->msg_type = FTP_GET;
                            sMsg->msg_header->msg_bytes = strlen(target);//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = target;

                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            //发送文件大小
                            sMsg->msg_header->msg_type = FTP_GET_FLSZ;
                            sMsg->msg_header->msg_bytes = filesize;
                            sMsg->msg_data = "\0";
                            sendMsg(s_sockfd,sMsg);

                            /* 根据结果执行响应处理 */
                            recvMsg(s_sockfd,rMsg);
                            if (rMsg->msg_header->msg_type == FTP_RESULT_FAILURE)
                            {
                                printf("\tFile is not existed\n");
                                continue;
                            }

                            /* 根据tMode确定读文件模式 */
                            if (fm_get(s_sockfd,currDir,target,filesize,tMode) == -1)
                            {
                                printf("\tGet fail\n");
                            }
                        }
                        else if (strcmp(buffer,"ascii") == 0)//设置传输模式为文本模式
                        {
                            sMsg->msg_header->msg_type = FTP_ASCII;
                            sMsg->msg_header->msg_bytes = 1;//发送的命令没有'\0',服务器接收到须根据实际需要添加'\0'或其他标识结束的字符
                            sMsg->msg_data = "\0";
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            /* 根据结果执行响应处理 */
                            if (recvMsg(s_sockfd,rMsg) == 0)
                            {
                                if (rMsg->msg_header->msg_type == FTP_RESULT_SUCCESS)
                                {
                                    tMode = 0;
                                }
                            }

                            if (tMode == 0)
                            {
                                printf("\tCurrent file transmission mode is : ASCII\n");
                            }
                            else
                            {
                                printf("\tCurrent file transmission mode is : BINARY\n");
                            }
                        }
                        else if (strcmp(buffer,"bin") == 0)//设置传输模式为二进制模式
                        {
                            sMsg->msg_header->msg_type = FTP_BINARY;
                            sMsg->msg_header->msg_bytes = 1;
                            sMsg->msg_data = "\0";
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }

                            /* 根据结果执行响应处理 */
                            if (recvMsg(s_sockfd,rMsg) == 0)
                            {
                                if (rMsg->msg_header->msg_type == FTP_RESULT_SUCCESS)
                                {
                                    tMode = 1;
                                }
                            }

                            if (tMode == 0)
                            {
                                printf("\tCurrent file transmission mode is : ASCII\n");
                            }
                            else
                            {
                                printf("\tCurrent file transmission mode is : BINARY\n");
                            }
                        }
                        else if (strcmp(buffer, "lpwd") == 0)//查看当前目录路径
                        {
                            fm_pwd(currDir);
                        }
                        else if (strcmp(buffer, "dir") == 0)//查看当前目录下的目录项目
                        {
                            printf("%s",fm_ls(currDir));
                        }
                        else if (strncmp(buffer, "lcd",3) == 0)//查看当前目录下的目录项目
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[4]);
                            currDir = fm_cd(currDir,target);
                        }
                        else if (strncmp(buffer, "lmkdir",6) == 0)//在当前目录下创建新目录
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[7]);
                            fm_mkdir(currDir,target,S_IRUSR | S_IWUSR | S_IXUSR);
                        }
                        else if (strncmp(buffer, "lrmdir",6) == 0)//查看当前目录下的删除指定目录
                        {
                            char *target = (char *)malloc(PATH_LEN);
                            strcpy(target,&buffer[7]);
                            fm_rmdir(currDir,target);
                        }
                        else
                        {
                            sMsg->msg_header->msg_type = FTP_NONE;
                            sMsg->msg_header->msg_bytes = strlen(buffer);
                            sMsg->msg_data = buffer;
                            if (sendMsg(s_sockfd,sMsg) == -1)
                            {
                                //与服务器断开连接
                                printf("%sDisconnect with server!\n",CM);
                                exit(0);
                            }
                            printf("%sDon't understand this command\n",CM);
                            nread = 0;
                        }
                    }
                }
                break;
        }
    }
}