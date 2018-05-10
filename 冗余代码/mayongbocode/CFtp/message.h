//
// Created by yobol on 16-11-9.
//
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MSG_END_MARKER 0x1234

#define MSG_TYPE_LEN 8
#define MSG_HEADER_LEN 16
#define MSG_DATA_LEN 1024
#define MSG_LEN 1040


/*
 * C/S之间传输内容的数据类型
 */
typedef enum _MsgType {
    FTP_LOGIN,
    FTP_PUT,
    FTP_GET,
    FTP_PWD,
    FTP_LS,
    FTP_CD,
    FTP_MKDIR,
    FTP_RMDIR,
    FTP_ASCII,
    FTP_BINARY,
    FTP_NONE,
    FTP_FILE_NOTEXISTED,
    FTP_FILE_EXISTED,
    FTP_PUT_FLSZ,
    FTP_PUT_FILE,
    FTP_GET_FLSZ,
    FTP_GET_FILE,
    FTP_RESULT_SUCCESS,
    FTP_RESULT_FAILURE,
    FTP_RESULT_LOGINSUCCESS,
    FTP_RESULT_LOGINFAILURE
} MsgType;

/*
 * MsgHeader - size : sizeof(unit32_t) * 4
 *
 * msg_type - size : sizeof(unit32_t) * 2
 */
typedef struct _MsgHeader {
    uint32_t msg_version;
    MsgType msg_type;
    uint32_t msg_bytes;
} MsgHeader;

/*
 * Msg - size : sizeof(unit32_t) * 4 + sizeof(char) * 1024
 *
 * msg_data - size : sizeof(char) * 1024
 */
typedef struct _Msg {
    MsgHeader *msg_header;
    void *msg_data;
} Msg;

/**
 * 傻瓜式发送消息
 * send函数有时并不能按照预想的那样发送size长度的数据
 * 该函数返回的值可能会小于size
 * 这种情况下，应该比较send函数返回值与size，发送所有数据
 *
 * @param sockfd
 * @param buffer
 * @param size
 * @return 失败返回小于等于0的数
 */
int
sendFool(int sockfd, const void *buffer, int size)
{
    return (int)send(sockfd,buffer,(size_t)size,0);
}

/**
 * 防止数据丢失，阻塞式发送数据
 *
 * @param sockfd
 * @param buffer
 * @param size
 * @return 成功返回0；失败则返回-1
 */
int
sendChoke(int sockfd,const void* buffer,int size)
{
    int sent = 0;
    int sentBytes;
    for(;;)
    {
        sentBytes = sendFool(sockfd, (void *) &((char *) buffer)[sent], size - sent);
        if (sentBytes <= 0)
        {
            return -1;
        }
        sent += sentBytes;
        if (sent == size)
        {
            return 0;
        }
    }
}

/**
 * 发送消息头
 *
 * @param sockfd
 * @param msgHeader
 * @return 成功返回0；失败则返回-1
 */
int
sendMsgHeader(int sockfd,const MsgHeader *msgHeader)
{
    uint32_t header[4];
    header[0] = htonl(msgHeader->msg_version);
    header[1] = htonl(msgHeader->msg_type);
    header[3] = htonl(msgHeader->msg_bytes);
    if (sendChoke(sockfd,header,MSG_HEADER_LEN) == -1)
    {
        printf("Fail to send message header:sendMsgHeader(%d)\n",sockfd);
        return -1;
    }
    return 0;
}

/**
 * 发送数据体
 *
 * @param sockfd
 * @param msg
 * @return 成功返回0；失败则返回-1
 */
int
sendMsg(int sockfd,const Msg *msg)
{

    uint32_t msg_end_marker = htonl(MSG_END_MARKER);

    if (!msg || sockfd == -1 || (msg->msg_header->msg_bytes != 0 && !msg->msg_data))
    {
        return -1;
    }

    if (sendMsgHeader(sockfd,msg->msg_header) == -1)
    {
        return -1;
    }

    if (msg->msg_header->msg_bytes != 0)
    {
        if (sendChoke(sockfd,msg->msg_data,msg->msg_header->msg_bytes) == -1)
        {
            printf("Fail to send message header:sendMsgHeader(%d)\n",sockfd);
            return -1;
        }
    }

    if (sendChoke(sockfd,&msg_end_marker,sizeof(uint32_t)) == -1)
    {
        printf("Fail to send message end marker:sendMsgHeader(%d)\n",sockfd);
        return -1;
    }

    return 0;
}


/**
 * 傻瓜式接收数据
 *
 * @param socket
 * @param buffer
 * @param size
 * @return 失败返回小于等于0的数
 */
int
recvFool(int sockfd,void *buffer,int size)
{
    return (int)recv(sockfd,buffer,(size_t)size,0);
}

/**
 * 阻塞式接收数据
 *
 * @param socket
 * @param msg
 * @return 成功返回0；失败则返回-1
 */
static int
recvChoke(int sockfd,void *buffer,int size)
{
    int received = 0;
    int gotBytes;
    for(;;)
    {
        gotBytes = recvFool(sockfd,(void *)&((char *)buffer)[received],size - received);
        if (gotBytes <= 0)
        {
            return -1;
        }

        received += gotBytes;

        if (received == size)
        {
            return 0;
        }
    }
}

/**
 * 接收消息体
 *
 * @param sockfd
 * @param msg
 * @return 成功返回0；失败返回-1
 */
int
recvMsg(int sockfd,Msg *msg)
{
    uint32_t header[4];
    uint32_t msg_end_marker;
    if (!msg || sockfd == -1)
    {
        return -1;
    }

    msg->msg_data = NULL;

    if (recvChoke(sockfd,header,MSG_HEADER_LEN) == -1)
    {
        printf("Fail to receive message header:recvMsg(%d)\n",sockfd);
        return -1;
    }

    msg->msg_header->msg_version = ntohl(header[0]);
    msg->msg_header->msg_type = ntohl(header[1]);
    msg->msg_header->msg_bytes = ntohl(header[3]);

    if (msg->msg_header->msg_bytes == 0)
    {
        msg->msg_data = NULL;
    }
    else
    {
        msg->msg_data = malloc(msg->msg_header->msg_bytes);
        if (!msg->msg_data)
        {
            printf("Fail to malloc %d bytes for getting data:recvMsg(%d)\n",msg->msg_header->msg_bytes,sockfd);
            return -1;
        }

        if (recvChoke(sockfd,msg->msg_data,msg->msg_header->msg_bytes) == -1)
        {
            printf("Fail to receive message header:recvMsg(%d)\n",sockfd);
            free(msg->msg_data);
            return -1;
        }
    }

    if (recvChoke(sockfd,&msg_end_marker,sizeof(uint32_t)) == -1)
    {
        printf("Fail to receive message header:recvMsg(%d)\n",sockfd);
        free(msg->msg_data);
        return -1;
    }

    msg_end_marker = ntohl(msg_end_marker);
    if (msg_end_marker != MSG_END_MARKER)
    {
        printf("Fail to receive message end marker %x:recvMsg(%d)\n",msg_end_marker,sockfd);
        free(msg->msg_data);
        return -1;
    }

    return 0;
}

/**
 * 创建名为fileName的文件，从选定的sockfd中读取内容写入到该文件中
 *
 * @param sockfd
 * @param currPath
 * @param fileName
 * @param mode
 * @return
 */
int
fm_get(int sockfd,char *currPath,char *fileName,int localfilesize,int mode)
{
    FILE *file;

    int filesize = 0;//文件大小
    int counter = 0;//决定调用多少次recvMsg
    int one_recv = 0;//本次从sockfd中读取的字节数
    int recvsize = 0;//一共从sockfd中读取的字节数
    int items = 0;//要从sockfd中recv的次数
    int curritem = 0;

    Msg *rMsg = (Msg *)malloc(MSG_LEN);
    rMsg->msg_header = malloc(MSG_HEADER_LEN);
    rMsg->msg_data = malloc(MSG_DATA_LEN);
    unsigned char *target = (unsigned char *)malloc(MSG_DATA_LEN);

    //获取文件大小
    recvMsg(sockfd,rMsg);
    filesize = rMsg->msg_header->msg_bytes;
    items = (filesize > 0) ? (filesize / MSG_DATA_LEN + 1) : 0;

    chdir(currPath);
    //可写形式打开文件
    if (filesize == 0)
    {
        if ((file = fopen(fileName,"w+")) == NULL)
        {
            return -1;
        }
    }
    else if (filesize > 0)
    {
        if ((file = fopen(fileName,"a+")) == NULL)
        {
            return -1;
        }
    }

    switch (mode)
    {
        case 0:/* 文本模式 */
            while (recvsize < filesize)
            {
                recvMsg(sockfd,rMsg);
                one_recv = rMsg->msg_header->msg_bytes;
                target = (char *)rMsg->msg_data;
                target[one_recv] = 0;
                fputs(target,file);
                fflush(file);
                memset(target,0,MSG_DATA_LEN);
                recvsize += one_recv;
            }
            fflush(file);
            memset(target,0,MSG_DATA_LEN);
            break;
        case 1:/* 二进制模式 */
            while(curritem < items)
            {
                recvMsg(sockfd,rMsg);
                one_recv = rMsg->msg_header->msg_bytes;
                target = (char *)rMsg->msg_data;
                fwrite(target,one_recv,1,file);
                fflush(file);
                memset(target,0,MSG_DATA_LEN);
                curritem++;
            }
            fflush(file);
            memset(target,0,MSG_DATA_LEN);
            break;
    }

    return 1;
}

/**
 * 从当前文件夹中将fileName指定的文件send到选定的sockfd中
 *
 * @param currPath
 * @param fileName
 * @param mode
 * @return 成功返回0;失败返回-1
 */
int
fm_put(int sockfd,char *currPath,char *fileName,int fileseek,int mode)
{
    FILE *file;
    struct stat statbuf;

    int filesize = 0;//文件大小
    int counter = 0;//调用sendMsg次数
    int sentsize = 0;//已发送的大小

    int seek = 0;//从文件指定位置开始传输

    /* 用于BINARY传输模式 */
    int items = 0;//要发送的次数
    int sendsize = 0;//本次要发送的数据长度

    Msg *sMsg = (Msg *)malloc(MSG_LEN);
    sMsg->msg_header = malloc(MSG_HEADER_LEN);
    sMsg->msg_data = malloc(MSG_DATA_LEN);
    unsigned char buffer[MSG_DATA_LEN];
    unsigned char *target = (unsigned char *)malloc(MSG_DATA_LEN);
    unsigned char previous[MSG_DATA_LEN];

    lstat(fileName,&statbuf);

    if (!S_ISREG(statbuf.st_mode))
    {
        //不是一个文件
        return -1;
    }

    seek = fileseek;
    filesize = statbuf.st_size;
    //防止文件没有信息更改时，也会sendMsg
    if ((filesize - seek) <= 0)
    {
        return 1;
    }
    items = ((filesize - seek) > 0) ? ((filesize - seek) / MSG_DATA_LEN + 1) : 0;

    //发送文件大小
    sMsg->msg_header->msg_type = FTP_PUT_FLSZ;
    sMsg->msg_header->msg_bytes = filesize - seek;//发送文件剩余大小
    sendMsg(sockfd,sMsg);
    //只读形式打开文件
    if ((file = fopen(fileName,"r")) == NULL)
    {
        return -1;
    }
//    printf("\t剩余文件大小 : %d\n",filesize - seek);//test
    fseek(file,seek,SEEK_SET);

    switch (mode)
    {
        case 0:/* 文本模式 */
            //发送文件内容
            while(fgets(buffer,MSG_DATA_LEN,file) != NULL)
            {
                strcpy(previous,target);
                if ((strlen(strcat(target,buffer)) + strlen(buffer)) > MSG_DATA_LEN)
                {
                    strcpy(target,previous);
                    sMsg->msg_header->msg_type = FTP_PUT_FILE;
                    sMsg->msg_header->msg_bytes = strlen(target);
                    sMsg->msg_data = target;
//                    printf("\t数据 : %s\n",target);//test
                    sendMsg(sockfd,sMsg);
                    sentsize += strlen(target);
                    counter++;
                    memset(target,0,MSG_DATA_LEN);
                    memset(previous,0,MSG_DATA_LEN);
                    //防止buffer中的内容丢失
                    strcat(target,buffer);
                }
            }
            sMsg->msg_header->msg_type = FTP_PUT_FILE;
            sMsg->msg_header->msg_bytes = strlen(target);
            sMsg->msg_data = target;
//            printf("\t数据 : %s\n",target);//test
            sendMsg(sockfd,sMsg);
            sentsize += strlen(target);
            memset(target,0,MSG_DATA_LEN);
            counter++;
            break;
        case 1:/* 二进制模式 */
            for (int i = 1; i <= items; ++i) {
                if (i == items)
                {
                    sendsize = (filesize - seek) - (i - 1) * MSG_DATA_LEN;
                    if (fread(target,sendsize,1,file) < 1)
                    {
                        return -1;
                    }
                }
                else
                {
                    sendsize = MSG_DATA_LEN;
                    if (fread(target,sendsize,1,file) < 1)
                    {
                        return -1;
                    }
                }
                sMsg->msg_header->msg_type = FTP_PUT_FILE;
                sMsg->msg_header->msg_bytes = sendsize;
                sMsg->msg_data = target;
                sendMsg(sockfd,sMsg);
                memset(target,0,MSG_DATA_LEN);
            }
            break;
    }

    return 1;

}