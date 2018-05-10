/**
*	Time:2015.12.05
*	Auther:ChenLi
**/
#include "MyFtp.h"

pthread_mutex_t Count_mutex,Client_mutex;

typedef pthread_t tThread;

typedef struct ThreadInfo{
	UserInfo User;
	struct sockaddr_in ClientAddress;
	int sock;
}ThreadInfo;
//多线程管理-------
typedef struct ThreadList{
	int Flag;//0为空.1为占用
	char Name[256];
	pthread_t ThreadId;
}ThreadList;

ThreadList ThreadArray[10];//最多允许十个线程同时在线

//打印用户列表
int PrintList();

//结束线程
int KillUser(char *user);

//Server命令线程
int ServerCommandThread(int socket);

//Server命令函数
int ServerCommand(void *Param);

//监听端口函数
void get_salt(char *salt,char *passwd);

int ServerThread(int socket);
//用户线程
void *Client_Thread(void* Param);

//返回线程id
int InsertThread(char *Name,pthread_t ID,ThreadList List[] );

