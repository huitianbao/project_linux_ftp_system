/**
*	Time:2015.12.05
*	Auther:ChenLi
**/
#include "Thread.h"

//Server命令线程
int ServerCommandThread(int socket)
{
	tThread CommandThread;
	pthread_create(&CommandThread,NULL,(void*)(&ServerCommand),&socket);
}
//Server命令函数
int ServerCommand(void *Param)
{
	int socket = *((int *)Param);
	char cmd[BUFFER],buf[BUFFER];

	while(1)
	{
		printf("Server>");
		fgets(cmd,2048,stdin);

		cmd[strlen(cmd)-1] = '\0';//字符串末尾加上结尾符号

		if(!strcmp(cmd,"count all")){
 			printf("访问用户总数:%d\n",CountAll);
			continue;
		}else if(!strcmp(cmd,""))
		{
			continue;
		}else if(!strcmp(cmd,"count current"))
		{
   			printf("当前在线用户总数:%d\n",CountNow);
			continue;
		}else if(!strcmp(cmd,"list"))
		{
			int i;
			for(i = 0;i<10;i++)
			{
				if(ThreadArray[i].Flag == 1)
				{
					fputs(ThreadArray[i].Name,stdout);
				}
			}
			fputs("\n",stdout);
			continue;
		}else if(!strncmp(cmd,"kill",4))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}
			char *p = strrchr(cmd,' ');//取有最后的空格
			if(p == NULL)//有最后的空格
			{
				//
				continue;
			}
			p++;
			printf("Warnning: You Will kill: %s\n",p);
			int i;
			pthread_mutex_lock(&Count_mutex);//对修改全局变量加锁
			for(i = 0;i<10;i++)
			{
				if(ThreadArray[i].Flag == 1)
				{
					
					if(!strncmp(p,ThreadArray[i].Name,strlen(ThreadArray[i].Name)))
					{
						void *thread_recv;//通知客户端
						if(pthread_cancel(ThreadArray[i].ThreadId) != 0)
						{
							printf("Kill %s Fail1,Please Try Again!\n",p);
						}
						if(pthread_join(ThreadArray[i].ThreadId,&thread_recv) != 0)
						{
							printf("Kill %s Fail2,Please Try Again!\n",p);
						}
						ThreadArray[i].Flag = 0;//删除线程记录
						printf("Kill %s Successful!\n",ThreadArray[i].Name);
						CountNow--;
						break;
					}
				}//Flag
			}//for
			pthread_mutex_unlock(&Count_mutex);
			continue;
		}else if(!strcmp(cmd,"help"))
		{
			printf("count all:		统计访问用户总数\n");
			printf("count current:		统计在线用户总数\n");
			printf("list:			打印在线用户总数\n");
			printf("kill:			结束进程 a\n");
			printf("help:			获取帮助\n");
			continue;
		}else if(!strcmp(cmd,"quit"))
		{
			char* tmp = (char *)malloc(2048);
			char tmp2[256];
			strcpy(tmp,"count all=");
			sprintf(tmp2,"%d",CountAll);
			strcat(tmp,tmp2);
			if(RWDataText("config/count.x","w",tmp,NULL) == -1)
			{
				printf("写入配置文件失败!\n");
				continue;
			}
			free(tmp);
			int i;
			for(i = 0;i<10;i++)
			{
				ThreadArray[i].Flag =0;				
			}
			printf("今日访问用户数:%d,访问用户总数:%d\n",CountNow,CountAll);
			printf("Quit Successful! GoodBye!\n");
			exit(0);
		}else
		{
			printf("Unknow input:%s\n",cmd);
			continue;
		}
	}
	return 0;
}
//---------------------------------------------------
//------------------监听端口函数-----------------------
//由这个函数负责接受客户连接,并为其创建用户线程
//监听端口函数
//--------------------------------------------------
void get_salt(char *salt,char *passwd)
{
	int i,j;
	for(i = 0,j=0;passwd[i] && j !=3;++i)
	{
		if(passwd[i] == '$')
		{
			++j;
		}
	}
	strncpy(salt,passwd,i-1);
}
int ServerThread(int socket)
{
	fputs("\n",stdout);
	printf("Server Launched And Waiting Users !\n");
	while(1)
	{

		struct sockaddr_in ClientAddress;
		int AddressSizeof = sizeof(ClientAddress);
		int ClientSocket = accept(socket,(struct sockaddr *)&ClientAddress, &AddressSizeof);
		if(ClientSocket == -1)//这个socket已经连上用户啦   客户端知道这个socket
		{
			fputs("Cannot Accept New User!\n",stdout);
			continue;
		}
		
		//接收并验证用户是否合法
		UserInfo user;
		char send[BUFFER];	
		SRMsg pSendMsg,pRecvMsg;

		//pRecvMsg.Data = &user;

		Recv_Message(ClientSocket,&pRecvMsg);
		bcopy(pRecvMsg.Data,&user,sizeof(UserInfo));

		//验证能否接受新用户
		if(Count>=10)
		{
			strcpy(send,"Server Crowed,Try Later!\n");
			pSendMsg.Header.Type = FTP_FALSE;
			pSendMsg.Data = send;
			pSendMsg.Header.Bytes = strlen(send)+1;
			Send_Message(ClientSocket,&pSendMsg);
			close(ClientSocket);
			continue;
		}

		if(pRecvMsg.Data !=NULL)
		{
			if(!(pRecvMsg.Header.Type == FTP_USERAUTH))
			{
				strcpy(send,"Password or UserName Not True.Login Experimental Fail!");
				pSendMsg.Header.Type = FTP_FALSE;
				pSendMsg.Data = send;
				pSendMsg.Header.Bytes = strlen(send)+1;
				Send_Message(ClientSocket,&pSendMsg);
				close(ClientSocket);
				continue;
			}
			struct spwd *sp;
			user.Name[strlen(user.Name)-1] = '\0';
			sp = getspnam(user.Name);
			if(sp == NULL)
			{
				printf("New User Info Experimental Fail!\n");
				strcpy(send,"Password or UserName Not True.Login Experimental Fail!");
				pSendMsg.Header.Type = FTP_FALSE;
				pSendMsg.Data = send;
				pSendMsg.Header.Bytes = strlen(send)+1;
				Send_Message(ClientSocket,&pSendMsg);
				close(ClientSocket);
				continue;
			}
			char salt[512] = {0};
			char passwd[256];
			//printf("sp->sp_pwdp:%s\n",sp->sp_pwdp);
			get_salt(salt,sp->sp_pwdp);
			user.Passwd[strlen(user.Passwd)-1] = '\0';
			if(strcmp(sp->sp_pwdp,(char *)crypt(user.Passwd,salt)) == 0)
			{  
				strcpy(send,"Login Experimental Successful!	  Welcome!");
				pSendMsg.Header.Type = FTP_TRUE;
				pSendMsg.Data = send;
				pSendMsg.Header.Bytes = strlen(send)+1;
				Send_Message(ClientSocket,&pSendMsg);
				//写在这儿
				ThreadInfo tClientThread;
				tClientThread.User = user;
				tClientThread.ClientAddress = ClientAddress;
				tClientThread.sock = ClientSocket;
				//开辟用户会话线程
				pthread_t Thread[10];
				if(pthread_create(&Thread[Count],NULL,Client_Thread,&tClientThread) != 0)
				{	
					strcpy(send,"Cannot Create New Client Thread! Please Relogin in !");
					pSendMsg.Header.Type = FTP_FALSE;
					pSendMsg.Data = send;
					pSendMsg.Header.Bytes = strlen(send)+1;
					Send_Message(ClientSocket,&pSendMsg);
					printf("Cannot Create New Client Thread!\n");
					close(ClientSocket);
					continue;
				}

				pthread_mutex_lock(&Count_mutex);
				if(InsertThread(tClientThread.User.Name,Thread[Count]/*pthread_self()*/,ThreadArray) != 0)
				{
					strcpy(send,"Cannot Create New Client Thread! Please Relogin in !");
					pSendMsg.Header.Type = FTP_FALSE;
					pSendMsg.Data = send;
					pSendMsg.Header.Bytes = strlen(send)+1;
					Send_Message(ClientSocket,&pSendMsg);
					printf("Cannot Create New Client Thread!\n");
					close(ClientSocket);
					continue;
				}
				pthread_mutex_unlock(&Count_mutex);

				CountAll = CountAll+1;
				CountNow = CountNow+1;
				Count = Count+1;
				printf("New User accept in Server\n");
				printf("username:%s\n",user.Name);
				printf("Server>");
			}//登录失败
			else{
				strcpy(send,"Password or UserName Not True.Login Experimental Fail!");
				pSendMsg.Header.Type = FTP_FALSE;
				pSendMsg.Data = send;
				pSendMsg.Header.Bytes = strlen(send)+1;
				if(Send_Message(ClientSocket,&pSendMsg) == -1)
				{
					printf("Send Error\n");
				}
				close(ClientSocket);
				continue;
			}
		}//RecvMsg->Data !=NULL
	}//while
}
//------------------------用户线程-------------------

//---------------------------------------------------
//用户线程,用户线程接入,多线程管理
void *Client_Thread(void *Param)
{//处理参数
	ThreadInfo ClientInfo = *((ThreadInfo *)Param);
	int sockfd = ClientInfo.sock;
	//设置本线程对CANCEL信号的反应
	if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) !=0)
	{
		exit(EXIT_FAILURE);
	}
	//设置本线程取消动作的执行时机
	if(pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL) !=0 )
	{
		exit(EXIT_FAILURE);
	}
	
	char buf[BUFFER];
	SRMsg RecvMsg,SendMsg;
	char workpath[BUFFER] = ".";
	char workpath_backup[512]=".";
	while(1)
	{
		memset(buf,'\0',BUFFER);
		Recv_Message(sockfd,&RecvMsg);
		char tmp[BUFFER],send[BUFFER];
		memset(tmp,'\0',BUFFER);
		memset(send,'\0',BUFFER);
		if(RecvMsg.Header.Type == FTP_CD)
		{
			if(RecvMsg.Header.Bytes == 0)//退到根
			{
				memset(workpath,'\0',BUFFER);
				strcpy(workpath,workpath_backup);//执行
				SendMsg.Header.Type = FTP_TRUE;
				SendMsg.Header.Bytes = 0;
				SendMsg.Data = NULL;
			}else{
				strcpy(buf,RecvMsg.Data);
				if(!strcmp(buf,".."))//回退一步
				{
					if(!strcmp(workpath,workpath_backup))//根
					{
						SendMsg.Header.Type = FTP_TRUE;
						SendMsg.Header.Bytes = 0;
						SendMsg.Data = NULL;
					}else{
						char *p = strrchr(workpath,'/');
						*p  = '\0';
						SendMsg.Header.Type = FTP_TRUE;
						SendMsg.Header.Bytes = 0;
						SendMsg.Data = NULL;
					}
				}else{
					if(buf[0]=='/')//跳转目录
					{
						if(strlen(buf) == 1)
						{
							SendMsg.Header.Type = FTP_TRUE;
							SendMsg.Header.Bytes = 0;
							SendMsg.Data = NULL;
						}else{
							memset(tmp,'\0',BUFFER);
							strcpy(tmp,workpath_backup);//执行
							strcat(tmp,buf);//构造路径
							if(!access(tmp, F_OK|R_OK|W_OK))
							{
								memset(workpath,'\0',BUFFER);
								strcpy(workpath,tmp);
								SendMsg.Header.Type = FTP_TRUE;
								SendMsg.Header.Bytes = 0;
								SendMsg.Data = NULL;
							}else{
								strcpy(send,"file not exit or refuse visit!");//执行	
								SendMsg.Header.Type = FTP_FALSE;
								SendMsg.Header.Bytes = strlen(send)+1;
								SendMsg.Data = send;
							}
						}
					}else{//进入目录
						memset(tmp,'\0',BUFFER);
						strcpy(tmp,workpath);//执行
						//strcat(tmp,buf);//构造路径
						strcat(tmp,"/");
						strcat(tmp,buf);
						if(!access(tmp, F_OK|R_OK|W_OK))
						{
							strcat(workpath,"/");
							strcat(workpath,buf);
							SendMsg.Header.Type = FTP_TRUE;
							SendMsg.Header.Bytes = 0;
							SendMsg.Data = NULL;
						}else{
							strcpy(send,"file not exit or refuse visit!");//执行
							SendMsg.Header.Type = FTP_FALSE;
							SendMsg.Header.Bytes = strlen(send)+1;
							SendMsg.Data = send;
						}
					}
				}
			}
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
			}
			continue;
		}else if(RecvMsg.Header.Type == FTP_DOWNLOAD)
		{
			strcpy(buf,RecvMsg.Data);
			CSStringPut(sockfd,buf,workpath);
			continue;
		}else if(RecvMsg.Header.Type == FTP_QUIT)//结束线程关闭socket
		{
			int i;
			for(i =0;i<10;i++)//删除用户表里面的用户
			{
				if(ThreadArray[i].Flag == 1)
				{
					if(!strcmp(ThreadArray[i].Name,ClientInfo.User.Name))
					{
						
						ThreadArray[i].Flag = 0;
						CountNow--;
						close(sockfd);
						//ClientInfo.User.Name[strlen(ClientInfo.User.Name)-1]='\0';
						printf("User %s Exit\n",ClientInfo.User.Name);
						pthread_exit(0);
					}
				}
			}
			continue;
		}else if(RecvMsg.Header.Type == FTP_UPLOAD)
		{
			CSStringGet(sockfd,workpath);
			continue;
		}else if(RecvMsg.Header.Type == FTP_MKDIR)
		{
			strcpy(buf,RecvMsg.Data);
			strcpy(tmp,workpath);
			strcat(tmp,"/");
			strcat(tmp,buf);
			if(!mkdir(tmp,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH))//664
			{
				chmod(tmp,0777);
				SendMsg.Header.Type = FTP_TRUE;
			}else{
				SendMsg.Header.Type = FTP_FALSE;
			}
			SendMsg.Header.Bytes = 0;
			SendMsg.Data = NULL;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
			}
			continue;
		}else if(RecvMsg.Header.Type == FTP_RMDIR)
		{
			strcpy(buf,RecvMsg.Data);
			strcpy(tmp,workpath);
			strcat(tmp,"/");
			strcat(tmp,buf);
			if(!rmdir(tmp))//664
			{
				SendMsg.Header.Type = FTP_TRUE;
			}else{
				SendMsg.Header.Type = FTP_FALSE;
			}
			SendMsg.Header.Bytes = 0;
			SendMsg.Data = NULL;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
			}
			continue;
		}else if(RecvMsg.Header.Type == FTP_LS)//只能打印当前目录下的文件列表
		{
			SPutsDir(RecvMsg.Data,workpath,sockfd);
			continue;
		}else if(RecvMsg.Header.Type == FTP_PWD)
		{
			//getcwd(buf,2048);
			SendMsg.Header.Type = FTP_TRUE;
			SendMsg.Header.Bytes = strlen(workpath)+1;
			SendMsg.Data = workpath;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
			}
			continue;
		}
		//检查本线程是否处于Canceld状态,如果是   直接执行取消动作
		pthread_testcancel();
	}//while
}
//----------------------------------------------------
int InsertThread(char *Name,pthread_t ID,ThreadList List[] )
{
	int i;
	for(i = 0;i<10;i++)
	{
		if(List[i].Flag == 0)
		{
			strcpy(List[i].Name,Name);
			List[i].ThreadId = ID;
			List[i].Flag = 1;
			return 0;
		}
	}
}

//-----------------------------------------------------------

