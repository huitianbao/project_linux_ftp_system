/**
*	Time:2015.12.05
*	Auther:ChenLi
**/
#include <termios.h>
#include "Thread.h"

int main(int argc,char *argv[])
{
	char IP[20];
	char Name[128];
	char Passwd[128];
	int Port;
	struct termios initialrsettings , newrsettings;
	if(argc == 1)// ./client ||./client ip||./client ip:port ./client name:passwd@ip:port
	{
		printf("程序启动错误,缺少必要参数!\n");
		return -1;
	}else if(argc == 2)
	{
		if(strchr(argv[1],'@')==NULL)/*检查argv[]不符合ftps格式   函数匹配是否有@字符*/
		{
			if(strchr(argv[1],':')==NULL)
			{
				//仅ip
				strcpy(IP,argv[1]);
				fputs("请输入端口:",stdout);
				fgets(Name,128,stdin);
				Port = atol(Name);
				fputs("请输入用户名:",stdout);
				fgets(Name,128,stdin);
				fputs("密码:",stdout);
				tcgetattr(fileno(stdin),&initialrsettings);
				newrsettings = initialrsettings;
				newrsettings.c_lflag &= ~ECHO;
				tcsetattr(fileno(stdin),TCSAFLUSH,&newrsettings);
				fgets(Passwd,128,stdin);
				tcsetattr(fileno(stdin),TCSANOW,&initialrsettings);
			}else
			{
				//ip:port模式
				strcpy(IP,strtok(argv[1],":"));
				Port = atol(strtok(NULL,":"));
				fputs("请输入用户名:",stdout);
				fgets(Name,128,stdin);
				fputs("密码:",stdout);
				tcgetattr(fileno(stdin),&initialrsettings);
				newrsettings = initialrsettings;
				newrsettings.c_lflag &= ~ECHO;
				tcsetattr(fileno(stdin),TCSAFLUSH,&newrsettings);
				fgets(Passwd,128,stdin);
				tcsetattr(fileno(stdin),TCSANOW,&initialrsettings);
			}
		}else
		{
			//xxxx:xxxx@1.1.1.1:1111模式
			char *tmpN;
			char *tmpI;
			tmpN= strtok(argv[1],"@");//取用户名密码
			tmpI = strtok(NULL,"@");//ip
			strcpy(Name,strtok(tmpN,":"));
			strcpy(Passwd,strtok(NULL,":"));
			strcpy(IP,strtok(tmpI,":"));
			Port = atol(strtok(NULL,":"));
		}
	}
	int modeFlag = 0;/*0:binary;1:ASCII*/
	int socket;
	printf("\nclient initializing......\n");
	//连接
	if((socket = Connect_ToServer(IP,Port))==-1)//连接socket
	{
		perror("oops:client2");
		return -1;
	}
	//构造用户信息数据包
	UserInfo User;
	strcpy(User.Name,Name);
	strcpy(User.Passwd,Passwd);//测试正确
	printf("Name:%s\n",User.Name);
	//传送用户名及密码  data放密码等
	SRMsg SendMsg , RecvMsg;

	SendMsg.Header.Type = FTP_USERAUTH;
	SendMsg.Data = &User;
	SendMsg.Header.Bytes = sizeof(UserInfo);
	if(Send_Message(socket,&SendMsg) == -1)
	{
		printf("Check userinfo Error\n");
	}//相等返回0
	Recv_Message(socket,&RecvMsg);
	printf("%s\n",RecvMsg.Data);
	if(RecvMsg.Header.Type == FTP_FALSE)
	{
		printf("Name or Password False!\n");
		close(socket);
		exit(0);
	}
	if(RecvMsg.Header.Type == FTP_TRUE)
	{
		printf("Login Successful!\n");
	}
	printf("用户名:%s\n",User.Name);
	char buf[2048],cmd[2048];
	char workpath[BUFFER] = ".";
	char workpath_backup[512]=".";
	while(1)
	{
		printf("Myftp>");
		memset(buf,'\0',BUFFER);
		fgets(cmd,2048,stdin);
		cmd[strlen(cmd)-1] = '\0';
		char tmp[BUFFER],send[BUFFER];
		memset(tmp,'\0',BUFFER);
		memset(send,'\0',BUFFER);

		if(!strncmp(cmd,"ls",2))//只能打印当前目录下的文件列表
		{
			SendMsg.Data =cmd;
			SendMsg.Header.Bytes = strlen(cmd)+1;
			SendMsg.Header.Type = FTP_LS;
			if(Send_Message(socket,&SendMsg) == -1)
			{
				printf("error\n");
			}
			while(1)
			{
				Recv_Message(socket,&RecvMsg);
				if(RecvMsg.Header.Type == FTP_FALSE)
				{
					printf("%s\n",RecvMsg.Data);
					continue;
				}else if(RecvMsg.Header.Type == FTP_TRUE)
				{
					break;
				}
			}
			continue;
		}else if(!strncmp(cmd,"lls",3))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;
			p=p+3;
			PutsDir(p,workpath);
			continue;
		}else if(!strncmp(cmd,"put",3))
		{
			SendMsg.Data =NULL;//p;
			SendMsg.Header.Bytes = 0;//strlen(p)+1;
			SendMsg.Header.Type = FTP_UPLOAD;
			if(Send_Message(socket,&SendMsg) == -1)
			{
				printf("error\n");
			}
			CSStringPut(socket,cmd,workpath);
			continue;
		}else if(!strncmp(cmd,"get",3))
		{
			SendMsg.Data =cmd;
			SendMsg.Header.Bytes = strlen(cmd)+1;
			SendMsg.Header.Type = FTP_DOWNLOAD;
			if(Send_Message(socket,&SendMsg) == -1)
			{
				printf("error\n");
			}
			CSStringGet(socket,workpath);
			continue;
		}else if(!strncmp(cmd,"quit",4)){
			
			SendMsg.Header.Type = FTP_QUIT;
			SendMsg.Data = NULL;
			SendMsg.Header.Bytes = 0;
			Send_Message(socket,&SendMsg);
			close(socket);
			printf("Quit Successful! GoodBye!\n");
			exit(0);
		}else if(!strcmp(cmd,""))
		{
			continue;
		}else if(!strncmp(cmd,"lmkdir",6))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;//去掉最后的空格
			p=p+6;
			if(strlen(p) == 0)
			{
				fputs("lmkdir: 缺少操作数\n",stdout);
			}else{
				while(1)
				{
					if(*p == ' ')
						p++;
					else
						break;
				}//去掉中间的空格
			}
			//strcpy(buf,p);
			strcpy(tmp,workpath);
			strcat(tmp,"/");
			strcat(tmp,p);
			if(!mkdir(tmp,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH))//664
			{
				chmod(tmp,0777);
				continue;
			}else{
				fputs("Unknow False,Please Try Again!\n",stdout);
				continue;
			}
		}else if(!strncmp(cmd,"mkdir",5))//一次建一个文件夹
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;//去掉最后的空格
			p=p+5;
			if(strlen(p) == 0)
			{
				fputs("mkdir: 缺少操作数\n",stdout);
			}else{
				while(1)
				{
					if(*p == ' ')
						p++;
					else
						break;
				}//去掉中间的空格
				SendMsg.Data =p;//p即命令
				SendMsg.Header.Bytes = strlen(p)+1;
				SendMsg.Header.Type = FTP_MKDIR;
				if(Send_Message(socket,&SendMsg) == -1)
				{
					printf("error\n");
				}
				Recv_Message(socket,&RecvMsg);
				if(RecvMsg.Header.Type == FTP_FALSE)
				{
					printf("Unknow False,Please Try Again!\n");
					continue;
				}else if(RecvMsg.Header.Type == FTP_TRUE)
				{
					//printf("%s\n",RecvMsg.Data);
					continue;
				}
			}
			continue;
		}else if(!strncmp(cmd,"lrmdir",6))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;//去掉最后的空格
			p=p+6;
			if(strlen(p) == 0)
			{
				fputs("lrmdir: 缺少操作数\n",stdout);
			}else{
				while(1)
				{
					if(*p == ' ')
						p++;
					else
						break;
				}//去掉中间的空格
			}
			strcpy(buf,p);
			strcpy(tmp,workpath);
			strcat(tmp,"/");
			strcat(tmp,buf);
			if(!rmdir(tmp))//664
			{
				continue;
			}else{
				printf("Unknow False,Please Try Again!\n");
				continue;
			}
		}else if(!strncmp(cmd,"rmdir",5))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;//去掉最后的空格
			p=p+5;
			if(p == '\0')
			{
				fputs("rmdir: 缺少操作数\n",stdout);
			}else{
				while(1)
				{
					if(*p == ' ')
						p++;
					else
						break;
				}//去掉中间的空格
				SendMsg.Data =p;
				SendMsg.Header.Bytes = strlen(p)+1;
				SendMsg.Header.Type = FTP_RMDIR;
				if(Send_Message(socket,&SendMsg) == -1)
				{
					printf("error\n");
				}
				Recv_Message(socket,&RecvMsg);
				if(RecvMsg.Header.Type == FTP_FALSE)
				{
					printf("Unknow False,Please Try Again!\n");
					continue;
				}else if(RecvMsg.Header.Type == FTP_TRUE)
				{
					//printf("%s\n",RecvMsg.Data);
					continue;
				}
			}
			continue;
		}else if(!strcmp(cmd,"lpwd"))
		{
			printf("%s\n",workpath);
		}else if(!strncmp(cmd,"pwd",3))
		{
			SendMsg.Header.Type = FTP_PWD;
			SendMsg.Data = NULL;
			SendMsg.Header.Bytes = 0;
			if(Send_Message(socket,&SendMsg) == -1)
			{
				printf("error\n");
			}
			Recv_Message(socket,&RecvMsg);
			if(RecvMsg.Header.Type == FTP_FALSE)
			{
				printf("%s\n",RecvMsg.Data);
				//printf("Unknow False,Please Try Again!\n");
				continue;
			}else if(RecvMsg.Header.Type == FTP_TRUE)
			{
				printf("%s\n",RecvMsg.Data);
				continue;
			}
		}else if(!strncmp(cmd,"lcd",3))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;
			p=p+3;
			while(1)
			{
				if(!strncmp(p," ",1))
					p=p+1;
				else
					break;
			}//去掉中间的空格
			if(strlen(p) == 0)//cd
			{
				memset(workpath,'\0',BUFFER);
				strcpy(workpath,workpath_backup);//执行
				continue;
			}else{
				
				if(!strcmp(p,".."))//回退一步
				{
					if(!strcmp(workpath,workpath_backup))//根
					{
						continue;
					}else{
						*strrchr(workpath,'/') = '\0';
						continue;
					}
				}else{
					if(!strncmp(p,"/",1))//跳转目录
					{
						if(strlen(p) == 1)
						{
							continue;
						}else{
							memset(tmp,'\0',BUFFER);
							strcpy(tmp,workpath_backup);//执行
							strcat(tmp,p);//构造路径
							if(!access(tmp, F_OK|R_OK|W_OK))
							{
								memset(workpath,'\0',BUFFER);
								strcpy(workpath,tmp);
								continue;
							}else{
								fputs("file not exit or refuse visit!\n",stdout);//执行	
								continue;
							}
						}
					}else{//进入目录
						memset(tmp,'\0',BUFFER);
						strcpy(tmp,workpath);//执行
						strcat(tmp,"/");
						strcat(tmp,p);
						if(!access(tmp, F_OK|R_OK|W_OK))
						{
							strcat(workpath,"/");
							strcat(workpath,p);
							continue;
						}else{
							fputs("file not exit or refuse visit!\n",stdout);//执行	
							continue;
						}
					}
				}
			}
		}else if(!strncmp(cmd,"cd",2))
		{
			while(1)
			{
				if(cmd[strlen(cmd)-1] == ' ')
					cmd[strlen(cmd)-1] = '\0';
				else
					break;
			}//去掉最后的空格
			char *p =cmd;
			p=p+2;
			if(strlen(p) == 0)
			{
				
				SendMsg.Data =NULL;
				SendMsg.Header.Bytes = 0;
				SendMsg.Header.Type = FTP_CD;
			}else{
				while(1)
				{
					if(*p == ' ')
						p=p+1;
					else
						break;
				}//去掉中间的空格
				SendMsg.Data =p;
				SendMsg.Header.Bytes = strlen(p)+1;
				SendMsg.Header.Type = FTP_CD;
			}
			if(Send_Message(socket,&SendMsg) == -1)
			{
				printf("error\n");
			}
			Recv_Message(socket,&RecvMsg);
			if(RecvMsg.Header.Type == FTP_FALSE)
			{
				printf("%s\n",RecvMsg.Data);
				continue;
			}else if(RecvMsg.Header.Type == FTP_TRUE)
			{
				continue;
			}
		}else if(!strcmp(cmd,"help"))
		{
			printf("quit		退出程序\n");
			printf("lls		列出当前目录下所有文件/文件夹\n");
			printf("ls		列出Server当前目录下所有文件/文件夹\n");
			printf("get		下载一个/多个文件\n");
			printf("lmkdir		创建文件夹\n");
			printf("mkdir		Server上创建文件夹\n");
			printf("lpwd		打印当前目录\n");
			printf("pwd		打印Server当前目录\n");
			printf("lcd		返回上一层目录\n");
			printf("cd		Server返回上一层目录\n");
			printf("lrmdir		删除文件/文件夹\n");
			printf("rmdir		Server删除文件/文件夹\n");
			printf("put		-a:ASCII模式,默认为Bin模式;上传一个/多个文件\n");
			printf("help		获取帮助\n");
		}else
		{
			printf("Can Not find input:%s\n",cmd);
		}
	}//while
}

