#ifndef __FTP_H__
#define __FTP_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#define BUF_SIZE 1024

typedef struct information
{
	int  info_sfd ;
	char info_buf[BUF_SIZE] ;

}info_t, *pinfo_t;


//初始化从文件读取ip和port
void ftp_init_from_file(char* path, char* ip, char* port);

//服务器初始化函数封装
int socket_server(char* ip, char* port);

//服务器处理客户端函数
void client_handle(pinfo_t arg);

//接收，读取，写入，发送函数重写
int recv_n(int fd_recv, char* recv_buf, int len );
int read_n(int fd_read, char* read_buf, int len );
int send_n(int fd_send, char* send_buf, int len );
int write_n(int fd_write, char* write_buf, int len );

//客户端初始化函数
int socket_client(char* ip, char* port);

//格式化字符串
void delete_space(char* src);

//上传函数
int upload(int fd_up, char* file_name);

//下载函数
int download(int fd_down, char* file_name);
#endif


