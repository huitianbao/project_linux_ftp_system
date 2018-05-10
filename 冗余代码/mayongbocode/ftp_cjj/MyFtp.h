/**
*	Time:2015.12.05
*	Auther:ChenLi
**/
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <shadow.h>
#include <crypt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

//用户验证处
#define _XOPEN_SOURCE /* See feature_test_macros(7) */


#define BUFFER 2048

int CountAll;
int CountNow;
int Count;

//{消息----------------------------------------------
typedef enum Type{//枚举消息类型
	FTP_BINARY,
	FTP_ASCII,
	FTP_OK,
	FTP_FALSE,
	FTP_TRUE,
	FTP_DOWNLOAD,
	FTP_UPLOAD,
	FTP_CD,
	FTP_MKDIR,
	FTP_RMDIR,
	FTP_LS,
	FTP_QUIT,
	FTP_USERAUTH,
	FTP_PWD
}Type;

typedef struct Header{//消息头
	Type Type;
	int Bytes;
}Header;

typedef struct SRMsg{//消息体
	Header Header;
	void* Data;
}SRMsg;
//}--------------------------------------------------
typedef struct UserInfo{
	char Name[256];
	char Passwd[256];
}UserInfo;
//------------------用户信息结构体----------------------
//--------------------------公共函数--------------------
//发送消息体
int Send_Message(int sockfd,const SRMsg* SMsg);
//发送消息头
int Send_Header(int sockfd,const Header* Header);
//阻塞式发送数据
int Send_AllUnit(int sockfd,const void *buffer,int Size);
//阻塞式接收数据
int Receive_AllUnti(int sockfd,void *buffer,int Size);
//接收消息体
int Recv_Message(int sockfd,SRMsg* RMsg);

int Run_Server(char *ip,int nPoirt);

int RWDataText(char* Filename,char* Model,char* WString,char* RString);

int Connect_ToServer(char *IP_Server,int nPort);

int PutsDir(char *cmd,char *workpath);//回显目录信息

int printfdir(char *cmd,int mode);

int SPutsDir(char *cmd,char *workpath,int sockfd);//回显目录信息

int Sprintfdir(char *cmd,int mode,int sockfd);

void CSStringPut(int sockfd,char *p,char *workpath);

void CSStringGet(int sockfd,char *workpath);

int PutFile(int sockfd,char *filename,int model,char *workpath);

int GetFile(int sockfd,char *workpath);
