/**
*	Time:2015.12.05
*	Auther:ChenLi
**/
#include "MyFtp.h"

//用户验证处
#define _XOPEN_SOURCE /* See feature_test_macros(7) */

//----------------------------阻塞式发送,接收数据--------------------------
int Receive_AllUnti(int sockfd,void *buffer,int Size)
{
	int Received = 0;
	int GoBytes;
	while(1)
	{
		GoBytes = (int)recv(sockfd,buffer + Received, Size - Received,0);
        	if(GoBytes<=0){
            		return -1;
		}
		Received += GoBytes;
		if(Received == Size){
			return 0;
		}
	}//while
}
int Send_AllUnit(int sockfd,const void *buffer,int Size)
{
	int Send = 0;
	int SendBytes;

	while(1)
	{
		SendBytes = (int)send(sockfd,buffer+Send,Size-Send,0);
		if(SendBytes<=0){
			return -1;
		}
		Send += SendBytes;
		if(Send = SendBytes)
		{
			return 0;
		}
		
	}//while	
}

//-----------------------------------发送消息头---------------------------
int Send_Header(int sockfd,const Header* Header)
{
	int bufHeader[2];
	bufHeader[0] = htonl(Header->Type);
	bufHeader[1] = htonl(Header->Bytes);
	if(Send_AllUnit(sockfd,bufHeader,sizeof(int)*2)==-1)
	{
	        printf("Connection Lost at Sending packet header:FTP_SendMessage\n");
		return -1;
	}
	return 0;
}
//---------------------------------发送,接收消息体---------------------------
int Send_Message(int sockfd,const SRMsg* SRMsg)
{
	int EndMark = htonl(101);
	int SRMark;
	if(!SRMsg || sockfd == -1 || (SRMsg->Header.Bytes !=0 && !SRMsg->Data))
	{
		return -1;
	}
	if(Send_Header(sockfd,&SRMsg->Header) == -1)
	{
		return -1;
	}
//加返回验证send(sockfd,buffer+Send,Size-Send,0);
	int tmp = (int)recv(sockfd,&SRMark, sizeof(int),0);
	if(tmp <=0){
		printf("Connection Lost at Receive Packet Return: FTP_RecvMessage(%d)\n",sockfd);
		return -1;
	}
	if(SRMsg->Header.Bytes !=0 && ntohl(SRMark) == 1)
	{
		if(Send_AllUnit(sockfd,SRMsg->Data,SRMsg->Header.Bytes)== -1)
		{
			printf("Connection Lost at Sending packet message:FTP_SendMessage(%d)\n",sockfd);
			return -1;
		}
	}
	if(Send_AllUnit(sockfd,&EndMark,sizeof(int)) == -1)
	{
		printf("Connection Lost at Sending endmarker:FTP_SendMessage(%d)\n",sockfd);
		return -1;
	}
	return 0;
}

int Recv_Message(int sockfd,SRMsg* RMsg)
{
	int bufHeader[2];
	int EndMark;
	int SRMark = htonl(1);
	if(!RMsg || sockfd == -1)
	{
		return -1;
	}
	RMsg->Data = NULL;
	if(Receive_AllUnti(sockfd,bufHeader,2*sizeof(int))==-1)
	{
		printf("Connection Lost at Receiving packet:FTP_RecvMessage(%d)1\n",sockfd);
		return -1;
	}

	RMsg->Header.Type = ntohl(bufHeader[0]);
	RMsg->Header.Bytes = ntohl(bufHeader[1]);

	int tmp = (int)send(sockfd,&SRMark,sizeof(int),0);
	if(tmp<=0)
	{
		return -1;
		printf("Connection Lost at Sending Packet Mark:FTP_SendMessage(%d)\n",sockfd);
	}

	if(RMsg->Header.Bytes == 0)
	{
		RMsg->Data == NULL;
	}
	else{
		RMsg->Data = malloc(RMsg->Header.Bytes);
        	if(!RMsg->Data)
		{
			printf("Cannot malloc%dbytes for getting data at FTP_RecvMessage(%d)2\n",RMsg->Header.Bytes,sockfd);
			return -1;
		}
		if(Receive_AllUnti(sockfd,RMsg->Data,RMsg->Header.Bytes)== -1)
		{
			printf("Connection Lost at Receiving message:FTP_RecvMessage(%d)3\n",sockfd);
			free(RMsg->Data);
			return -1;
		}
	}
	if(Receive_AllUnti(sockfd,&EndMark,sizeof(int))==-1){
		printf("Connection Lost at Receiving end marker:FTP_RecvMessage(%d)\n",sockfd);
		free(RMsg->Data);//SAFE_RELEASE(pMsg->m_pData);
		return -1;
	}
	EndMark = ntohl(EndMark);
	if(EndMark != 101)
	{
		printf("Recived non-EndMarker %x:FTP_RecvMessage(%d)\n",EndMark,sockfd);
		free(RMsg->Data);
		return -1;
	}
	return 0;
}

//--------------------------建立套接字连接请求-----------------------
int Connect_ToServer(char *IP_Server,int nPort)
{
	int socketfd;//创建套接字
	struct sockaddr_in ServerAddress;
	socketfd = socket(AF_INET,SOCK_STREAM,0);
	if((socketfd = socket(AF_INET,SOCK_STREAM,0))==-1)
	{
		printf("Failed to create socket at Connect_ToServer\n");
		return -1;
	}
	ServerAddress.sin_port=htons(nPort);
	ServerAddress.sin_addr.s_addr = inet_addr(IP_Server);
	ServerAddress.sin_family = AF_INET;
	if(connect(socketfd,(struct sockaddr *)&ServerAddress,sizeof(struct sockaddr_in))==-1)
	{
		printf("Failed to connect socket at FTP_Connect\n");
		return -1;
	}
	printf("Connect to Server Successful!!\nServerIp: %s,PORT:%d\n",inet_ntoa(ServerAddress.sin_addr),ntohs(ServerAddress.sin_port));
	return socketfd;
}

//------------------------启动服务函数-------------------------
int Run_Server(char *ip,int nPort)
{
	int nServerSocket = -1;//socket描述符
	struct sockaddr_in LocalAddress;//地址信息
	nServerSocket = socket(AF_INET,SOCK_STREAM,0);
	if(nServerSocket == -1)
	{
		printf("Fail to creat socket!\n");
		return -1;
	}
	LocalAddress.sin_port = htons(nPort);
	LocalAddress.sin_family = AF_INET;
	LocalAddress.sin_addr.s_addr = inet_addr(ip/*INADDR_ANY"127.0.0.1"*/);
	if(bind(nServerSocket,(const struct sockaddr *)&LocalAddress,sizeof(struct sockaddr_in)) == -1)//地址绑定
	{
		printf("bind() error\n");
		return -1;
	}
	if(listen(nServerSocket,SOMAXCONN) == -1)//监听
	{
		printf("linten port error\n");
		return -1;
	}
	printf("IP:%s\n",inet_ntoa(LocalAddress.sin_addr));
	printf("PORT:%d\n",ntohs(LocalAddress.sin_port));
	printf("ftp server start successful!\n");
	
	//访问记录写入
	char* count = (char *)malloc(1024);
	if(RWDataText("config/count.x","r",NULL,count) == -1)
	{
		printf("读取配置文件失败!\n");
	}
	int Start = 0;
	while(*(count+Start) != '=')
	{
		Start++;
	}
	Start++;
	CountAll = atoi(count + Start);
	free(count);
	printf("历史访问人数:%d\n",CountAll);
	return nServerSocket;
}
//-----------------------------------------------------
//-----------------记录访问总数-----------------------
int RWDataText(char* Filename,char* Model,char* WString,char* RString)//测试通过
{
	char* buffer = (char *)malloc(1024);
	FILE* file = fopen(Filename,Model);
	if(file == NULL)
	{
		//printf("访问总数写入失败\n");
		return -1;
	}

	if(strcmp(Model,"r") == 0)
	{
		while(!feof(file))
		{
			buffer = fgets(buffer,100,file);
			if(buffer != NULL)
			{
				strcpy(RString,buffer);
			}
		}
	}else if(strcmp(Model,"w") == 0)
	{
		fputs(WString,file);
	}
	free(buffer);
	fclose(file);
	return 0;
}
//-----------------------------------------------------
int PutsDir(char *p,char *workpath)//回显目录信息printdir(char *p,int mode,)
{
	char tmp[512];
	if(strlen(p)==0)//ls
	{
		if(printfdir(workpath,0) == -1)
		{
			return -1;
		}
		//fputs("ls\n",stdout);//ls -a//打印当前目录下信息,ls
		return 0;
	}else{//fputs("ls\n",stdout);
		while(1)
		{
			if(!strncmp(p," ",1))
			{
				p=p+1;
			}else{
				break;
			}
		}
		if(!strncmp(p,"-a",2))//打印全部信息
		{
			p=p+2;
			if(strlen(p)>0)
			{
				while(1)
				{
					if(!strncmp(p," ",1))
						p=p+1;
					else
						break;
				}
				if(strlen(p)==0)
				{
					if(printfdir(workpath,1) == -1)
					{
						return -1;
					}
					//fputs("ls -a\n",stdout);
					return 0;
				}else if(!strncmp(p,"/",1))//ls -a /ss
				{
					strcpy(tmp,".");
					strcat(tmp,p);
					if(printfdir(tmp,1) == -1)
					{
						return -1;
					}
					//fputs("ls -a\n",stdout);
					return 0;
				}else{//ls -a xxx
					return 0;
				}
			}else{
				if(printfdir(workpath,1) == -1)
				{
					return -1;
				}
				//fputs("ls -a\n",stdout);//ls -a
				return 0;
			}
		}else if(!strncmp(p,"/",1))//打印全部信息
		{
			p=p+1;
			if(strlen(p)==0)
			{
				if(printfdir(workpath,1) == -1)
				{
					return -1;
				}
				//fputs("ls -a\n",stdout);
				return 0;
			}else{//ls /ms
				strcpy(tmp,"./");
				strcat(tmp,p);
				if(printfdir(tmp,0) == -1)
				{
					return -1;
				}
				//fputs("ls -a xxx\n",stdout);
				return 0;
			}
		}else{
			if(printfdir(p,0) == -1)
			{
				return -1;
			}
			//fputs("ls xx\n",stdout);//ls xx
			return 0;
		}
	}
}
int printfdir(char *cmd,int mode)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	if((dp = opendir(cmd)) == NULL) {
		return -1;
	}
	
	if(mode == 0)
	{
		while ((entry = readdir(dp)) != NULL) {
			printf("%s\n",entry->d_name);
		}
		return 0;
	}else{
		char list[9];
		strcpy(list,"----------");
		while ((entry = readdir(dp)) != NULL) {
			stat(entry->d_name,&statbuf);

			if(entry->d_type == 4)
				list[0] = 'd';
			if(statbuf.st_mode&S_IRUSR)
				list[1] = 'r';
			if(S_IWUSR & statbuf.st_mode)
				list[2] = 'w';
			if(S_IXUSR & statbuf.st_mode)
				list[3] = 'x';
			if(statbuf.st_mode&S_IRGRP)
				list[4] = 'r';
			if(S_IWGRP & statbuf.st_mode)
				list[5] = 'w';
			if(S_IXGRP & statbuf.st_mode)
				list[6] = 'x';
			if(S_IROTH & statbuf.st_mode)
				list[7] = 'r';
			if(S_IWOTH & statbuf.st_mode)
				list[8] = 'w';
			if(S_IXOTH & statbuf.st_mode)
				list[9] = 'x';
			printf("%s    %s\n",list,entry->d_name);
			memset(list,'-',10);
		}
	}
	closedir(dp);
}
//------------------------------------------------------------------S
int SPutsDir(char *p,char *workpath,int sockfd)//回显目录信息printdir(char *p,int mode,)
{
	char tmp[512];
	SRMsg SendMsg;//ls .....
	while(1)
	{
		if(p[strlen(p)-1] == ' '){
			p[strlen(p)-1] = '\0';
		}else{
			break;
		}
	}//去掉最后的空格
	p = p+2;
	while(1)
	{
		if(!strncmp(p," ",1))
			p=p+1;
		else
			break;
	}
	if(strlen(p)>0)//youdongxi
	{
		if(!strncmp(p,"-a",2))//打印全部信息
		{
			p=p+2;
			while(1)
			{
				if(!strncmp(p," ",1))
					p=p+1;
				else
					break;
			}
			if(strlen(p)>0)
			{
				if(!strncmp(p,"/",1)){
					strcpy(tmp,".");
					strcat(tmp,p);
					if(Sprintfdir(tmp,1,sockfd) == -1)
					{
						return -1;
					}
					//fputs("ls -a /xxx\n",stdout);
					return 0;
				}else
				{
					SendMsg.Header.Type = FTP_TRUE;
					SendMsg.Header.Bytes = 0;
					SendMsg.Data =NULL;// entry->d_name;
					if(Send_Message(sockfd,&SendMsg) == -1)
					{
						printf("Lost Connected\n");
					}
					return 0;
				}
			}else{
				if(Sprintfdir(workpath,1,sockfd) == -1)
				{
					return -1;
				}
				//fputs("ls -a\n",stdout);//ls -a
				return 0;
			}
		}else if(!strncmp(p,"/",1)){
			strcpy(tmp,".");
			strcat(tmp,p);
			if(Sprintfdir(tmp,0,sockfd) == -1)
			{
				return -1;
			}
			//fputs("ls /xx\n",stdout);//ls xx
			return 0;
		}else{
			SendMsg.Header.Type = FTP_TRUE;
			SendMsg.Header.Bytes = 0;
			SendMsg.Data =NULL;// entry->d_name;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
			}
			return 0;
		}
	}else{
		if(Sprintfdir(workpath,0,sockfd) == -1)
		{
			return -1;
		}
		return 0;
	}
}

int Sprintfdir(char *cmd,int mode,int sockfd)
{
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	SRMsg SendMsg;
	if((dp = opendir(cmd)) == NULL) {
		SendMsg.Header.Type = FTP_TRUE;
		SendMsg.Header.Bytes = 0;// strlen(entry->d_name)+1;
		SendMsg.Data =NULL;// entry->d_name;
		if(Send_Message(sockfd,&SendMsg) == -1)
		{
			printf("Lost Connected\n");
			return -1;
		}
	}	
	if(mode == 0)
	{
		while ((entry = readdir(dp)) != NULL) {
			//printf("     %s\n",entry->d_name);
			SendMsg.Header.Type = FTP_FALSE;
			SendMsg.Header.Bytes = strlen(entry->d_name)+1;
			SendMsg.Data = entry->d_name;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
				return -1;
			}
		}
		SendMsg.Header.Type = FTP_TRUE;
		SendMsg.Header.Bytes = 0;//strlen(entry->d_name)+1;
		SendMsg.Data = NULL;//entry->d_name;
		if(Send_Message(sockfd,&SendMsg) == -1)
		{
			printf("Lost Connected\n");
			return -1;
		}
		return 0;
	}else{
		char filename[512];
		strcpy(filename,"----------");
		while ((entry = readdir(dp)) != NULL) {
			stat(entry->d_name,&statbuf);

			if(entry->d_type == 4)
				filename[0] = 'd';
			if(statbuf.st_mode&S_IRUSR)
				filename[1] = 'r';
			if(S_IWUSR & statbuf.st_mode)
				filename[2] = 'w';
			if(S_IXUSR & statbuf.st_mode)
				filename[3] = 'x';
			if(statbuf.st_mode&S_IRGRP)
				filename[4] = 'r';
			if(S_IWGRP & statbuf.st_mode)
				filename[5] = 'w';
			if(S_IXGRP & statbuf.st_mode)
				filename[6] = 'x';
			if(S_IROTH & statbuf.st_mode)
				filename[7] = 'r';
			if(S_IWOTH & statbuf.st_mode)
				filename[8] = 'w';
			if(S_IXOTH & statbuf.st_mode)
				filename[9] = 'x';
			//printf("%s    %s\n",list,entry->d_name);
			strcat(filename,"     ");
			strcat(filename,entry->d_name);

			SendMsg.Header.Type = FTP_FALSE;
			SendMsg.Header.Bytes = strlen(filename)+1;
			SendMsg.Data = filename;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Lost Connected\n");
				return -1;
			}


			memset(filename,'\0',512);
			strcpy(filename,"----------");
		}
		SendMsg.Header.Type = FTP_TRUE;
		SendMsg.Header.Bytes = 0;//strlen(entry->d_name)+1;
		SendMsg.Data = NULL;//entry->d_name;
		if(Send_Message(sockfd,&SendMsg) == -1)
		{
			printf("Lost Connected\n");
			return -1;
		}
	}
	closedir(dp);
	return 0;
}

void CSStringPut(int sockfd,char *p,char *workpath)
{
	char *filetmp;
	while(1)//去掉末尾空格
	{
		if(p[strlen(p)-1] == ' ')
			p[strlen(p)-1] = '\0';
		else
			break;
	}
	p = p+3;//去掉put字符
	while(1){//去掉put后面的空格
		if(!strncmp(p," ",1)){
			p = p+1;
		}else{
			break;
		}
	}
	if(!strncmp(p,"-a",2))//-a 0
	{
		p = p+2;
		while(1)
		{//去掉-a后面的空格
			if(!strncmp(p," ",1))
			{
				p = p+1;
			}else{
				break;
			}
		}
		while(1)//取值   读取文件   发送文件
		{
			//stresp   返回原来的  更新传入参数
			while(filetmp = strsep(&p," "))
			{
				if(*filetmp == 0)
					continue;
				else
					break;
			}
			//读取文件  执行上传
			PutFile(sockfd,filetmp,0,workpath);
		}
		//发送结束标记  结束
		PutFile(sockfd,NULL,1,workpath);
	}else if(!strncmp(p,"-b",2)){//-b
		p = p+2;
		while(1)
		{//去掉-a后面的空格
			if(!strncmp(p," ",1))
			{
				p = p+1;
			}else{
				break;
			}
		}
		while(1)//取值   读取文件   发送文件
		{
			//stresp   返回原来的  更新传入参数
			while(filetmp = strsep(&p," "))
			{
				if(*filetmp == 0)
					continue;
				else
					break;
			}
			//读取文件  执行上传
			PutFile(sockfd,filetmp,1,workpath);
		}
		PutFile(sockfd,NULL,1,workpath);
	}else{
		while(p)//取值   读取文件   发送文件
		{
			//stresp   返回原来的  更新传入参数
			while(filetmp = strsep(&p," "))
			{
				if(*filetmp == 0)
					continue;
				else
					break;
			}
			//读取文件  执行上传
			PutFile(sockfd,filetmp,1,workpath);
		}
		//结束标记
		PutFile(sockfd,NULL,1,workpath);
	}
}
int PutFile(int sockfd,char *filename,int model,char *workpath)//通用发送一个文件 接收处理返回值
{
	SRMsg SendMsg,RecvMsg;
	char buf[8192];
	int length = 0;
	if(filename == NULL)
	{
		SendMsg.Header.Type = FTP_USERAUTH;//发送文件名
		SendMsg.Data =NULL;// buf;
		SendMsg.Header.Bytes = 0;//strlen(buf)+1;
		if(Send_Message(sockfd,&SendMsg) == -1)
		{
			printf("error\n");
		}
		return 0;
	}else{
		FILE *fp;
		strcpy(buf,workpath);
		strcat(buf,"/");
		strcat(buf,filename);
		
		if(model == 0)//-a
		{
			SendMsg.Header.Type = FTP_ASCII;//发送文件名
			SendMsg.Data = filename;
			SendMsg.Header.Bytes = strlen(filename)+1;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Can Not Put File!\n");
				return -1;
			}

			if((fp = fopen(buf,"r")) == NULL)
			{
				fputs("Open File Fail\n",stdout);
				return -1;
			}
			while(strlen(fgets(buf,8192,fp))>0)
			{
				SendMsg.Header.Type = FTP_UPLOAD;
				SendMsg.Data = buf;
				SendMsg.Header.Bytes = length;
				if(Send_Message(sockfd,&SendMsg) == -1)
				{
					printf("Put File Fail!\n");
					break;
				}
				memset(buf,'\0',8192);
			}
		}else{//-b

			SendMsg.Header.Type = FTP_BINARY;//发送文件名
			SendMsg.Data = filename;
			SendMsg.Header.Bytes = strlen(filename)+1;
			if(Send_Message(sockfd,&SendMsg) == -1)
			{
				printf("Can Not Put File!\n");
				return -1;
			}
			if((fp = fopen(buf,"r")) == NULL)
			{
				fputs("Open File Fail\n",stdout);
				return -1;
			}
			while(!feof(fp))
			{
				length = fread(buf,1,8192,fp);
				SendMsg.Header.Type = FTP_UPLOAD;
				SendMsg.Data = buf;
				SendMsg.Header.Bytes = length;
				if(Send_Message(sockfd,&SendMsg) == -1)
				{
					printf("Put File Fail!\n");
					break;
				}
				memset(buf,'\0',8192);
			}
		}
		fclose(fp);
	}
	return 0;	
}	
int GetFile(int sockfd,char *workpath)//通用发送一个文件 接收处理返回值
{
	FILE *fp;
	SRMsg SendMsg,RecvMsg;
	char buf[8192];
	char tmp[2048];
	int model = 1;
	Recv_Message(sockfd,&RecvMsg);
	if(RecvMsg.Header.Type == FTP_USERAUTH)
	{
		return -2;
	}else if(RecvMsg.Header.Type == FTP_ASCII)
	{
		model = 0;
	}else if(RecvMsg.Header.Type == FTP_BINARY)
	{
		model = 1;
	}
	strcpy(buf,workpath);
	strcat(buf,"/");
	strcat(buf,RecvMsg.Data);
	if(model == 0)
	{
		if ((fp = fopen(buf, "w")) == NULL) {
			printf("file open error\n");
			return -1;//exit(1);
		}
		RecvMsg.Data = buf;
		while(1)
		{
			if(Recv_Message(sockfd, &RecvMsg)==-1)
			{
				return -1;
			}
			if (RecvMsg.Header.Bytes < 0) {
				printf("接收错误\n");
				return -1;//exit(1);
			}
			fputs(RecvMsg.Data,fp);
			if(RecvMsg.Header.Bytes < 8192)
				break;
		}
	}else{
		if ((fp = fopen(buf, "wb")) == NULL) {
			printf("file open error\n");
			return -1;//exit(1);
		}
		RecvMsg.Data = buf;
		while(1)
		{
			if(Recv_Message(sockfd, &RecvMsg)==-1)
			{
				return -1;
			}
			if (RecvMsg.Header.Bytes < 0) {
				printf("接收错误\n");
				return -1;//exit(1);
			}
			int writelen = fwrite(RecvMsg.Data, sizeof(char), RecvMsg.Header.Bytes,fp);
			if (writelen > RecvMsg.Header.Bytes) {
				printf("file write failed\n");
				return -1;//exit(1);
			}
			if(RecvMsg.Header.Bytes < 8192)
				break;
		}
	}
	fclose(fp);
	chmod(tmp,0777);
	return 0;	
}

void CSStringGet(int sockfd,char *workpath)
{
	FILE *fp;
	SRMsg SendMsg,RecvMsg;
	char buf[2048];
	while(1)
	{
		if(GetFile(sockfd,workpath) == -2)
		{
			break;
		}
	}	
}
