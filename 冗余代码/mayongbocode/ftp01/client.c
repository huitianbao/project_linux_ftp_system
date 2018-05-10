#include "clientftp.h"
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

int port = 11726;

int main(int argc,char **argv){

	int sock;
	struct sockaddr_in addr;
	char server[20];
	strcpy(server,"127.0.0.1");
	int f=1;

	if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("create socket error\n");
		//创建socket失败
		return -1;
	}

	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr = inet_addr(server);

	if(connect(sock,(struct sockaddr*)&addr,sizeof(struct sockaddr_in))==-1){
		printf("connect server error\n");
		return -1;
	}else{
		printf("connect server successful\n");
	}

	char name[50]="";
	char password[50]="";
	printf("输入用户名：");
	gets(name);
	printf("输入密码：");
	gets(password);
	if(send_until_all(sock, name, 50)){
		printf("连接可能断开。。。");
		f=0;
	}
	if(send_until_all(sock, password, 50)){
		printf("连接可能断开。。。");
		f=0;
	}
	struct myMessage ver;
        if(FTP_recvMessage(sock,&ver)==-1){
		printf("连接可能断开。。。");
		f=0;
  	}else{
		if(ver.head.size==0){
			printf("登录成功！！！\n");
		}else{
			printf("登陆失败！\n");
			f=0;
			
		}
	}
  while(f){

	char message[2068];
	printf("%s ftp>",name);
	gets(message);
	char cmd[20]="";
	char data1[1024]="";
	char data2[1024]="";
	int i=0,c=0,d1=0,d2=0,flag=0;
	struct myMessage sendmsg;
	struct myMessage recvmsg;
	for( ;flag<3&&i<2068&&message[i]!='\0';i++){
		if(message[i]==' '){
			flag++;
		}else{
			if(flag==0){
				cmd[c]=message[i];
				c++;
			}
			if(flag==1){
				data1[d1]=message[i];
				d1++;
		       }
			if(flag==2){
				data2[d2]=message [i];
				d2++;
			}
     		}
  	}
  
	if (!strcmp(cmd, "cd")) {
		flag=1;
		if((!strcmp(data1, ""))||(strcmp(data2, ""))){
			printf("cd path  打开路径为path的文件夹\n");
		}else{
			sendmsg.head.cmd = CD;
			sendmsg.head.size = sizeof(data1);
			sendmsg.path = data1;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				continue;
			}
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				continue;
  			}else{
	 			 if(recvmsg.head.cmd==ERROR){
					printf("打开文件夹失败\n");
	 			 }else{
	  				printf("成功打开文件\n");}
			}
   		}

 	} 
	if (!strcmp(cmd, "pwd")) {
		flag=1;
		if((strcmp(data1, ""))||(strcmp(data2, ""))){
			printf("ls path  列出路径为path的文件夹\n");
		}else{
			sendmsg.head.cmd = PWD;
			sendmsg.head.size =0;
			sendmsg.path =	NULL	;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				continue;
			}	
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				continue;
  			}else{
	 			if(recvmsg.head.cmd==ERROR){
					printf("获取目录失败\n");
		 		}else{
					printf("服务器端目录为：%s\n",recvmsg.path);
				}	
			}

		}
	}
	if (!strcmp(cmd, "ls")) {
		flag=1;
		if((strcmp(data1, ""))||(strcmp(data2, ""))){
			printf("ls path  列出路径为path的文件夹\n");
		}else{
			sendmsg.head.cmd = LS;
			sendmsg.head.size =0;
			sendmsg.path =	NULL	;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				continue;
			}	
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				continue;
  			}else{
	 			if(recvmsg.head.cmd==ERROR){
					printf("获取文件名失败\n");
		 		}else{
	  				printf("服务器端文件名为：");
					printf("%s\n",recvmsg.path);
				}	
			}

		}
	}
	if (!strcmp(cmd, "mkdir")) {
		flag=1;
		if((!strcmp(data1, ""))||(strcmp(data2, ""))){
			printf("mkdir path  创建路径为path的文件\n");
		}else{
			sendmsg.head.cmd = MKDIR;
			sendmsg.head.size = sizeof(data1);
			sendmsg.path = data1;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				continue;
			}
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				continue;
  			}else{
	 			 if(recvmsg.head.cmd==ERROR){
					printf("创建文件夹失败\n");
	 			 }else{
	  				printf("创建文件夹成功\n");}
			}
   		}
	}
  	if (!strcmp(cmd, "rmdir")) {
   		 flag=1;
		if((!strcmp(data1, ""))||(strcmp(data2, ""))){
			printf("rmdir path  删除路径为path的文件\n");
		}else{
			sendmsg.head.cmd = RMDIR;
			sendmsg.head.size = sizeof(data1);
			sendmsg.path = data1;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				continue;
			}
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				continue;
  			}else{
	 			 if(recvmsg.head.cmd==ERROR){
					printf("删除文件夹失败\n");
	 			 }else{
	  				printf("删除文件夹成功\n");}
			}
   		}
    	
 	 }
  	if (!strcmp(cmd, "put")) {
   		flag=1;
    		if(!strcmp(data1, "")||!strcmp(data2, "")){
		printf("put path1 path2  path1本地文件路径文件 path2服务器端文件路径\n");
		} else{
			sendmsg.head.cmd=PUT;
			sendmsg.head.size = sizeof(data2);
			sendmsg.path = data2;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				break;
			}
			FILE *fp = NULL;
			char tmp[1024]="";
			strcat(tmp,data1);
 			if ((fp = fopen(tmp, "rb")) == NULL) {
               			printf("打开文件失败！！！\n");
				if(FTP_senderrorEnd(sock) == -1){
					printf("连接可能断开，发送消息失败！！！\n");
				}
				printf("001\n");
			} else {
           			if(FTP_sendEnd(sock) == -1){
					printf("连接可能断开，发送消息失败！！！\n");
					continue;
				}
				struct myMessage endmsg;
          			if(FTP_recvMessage(sock,&endmsg)==-1){
					printf("接收失败！！！\n");
					continue;
	 			} 
	 			if(endmsg.head.size==-1){
	 				printf("打开文件失败或因没有权限！！！\n");
					continue;
				}else{
              				printf("正在上传文件 %s\n", tmp);
               				if (FTP_upload(sock, fp, 0) == -1) {
	                   			 printf("上传文件失败\n");
	  				 	continue;
               				}
               			}
               			
          		}
		}
	}
	if (!strcmp(cmd, "get")) {
		flag=1;
    		if(!strcmp(data1, "")||!strcmp(data2, "")){
		  printf("get path1 path2  path1服务器端文件路径 path2本地文件路径\n");
		} else{
			sendmsg.head.cmd=GET;
			sendmsg.head.size = sizeof(data1);
			sendmsg.path = data1;
			if(FTP_sendMessage(sock,&sendmsg)==-1){
				continue;
			}if(FTP_recvMessage(sock,&recvmsg)==-1){
				continue;
  			}else{
	 		   if(recvmsg.head.cmd==OK){
				FILE *fp = NULL;
				char tmp[1024]="";
				strcat(tmp,data2);
 				if ((fp = fopen(tmp, "wb")) == NULL) {
               				 printf("打开文件失败\n");
               				 if(FTP_senderrorEnd(sock) == -1){
					  printf("连接可能断开，发送消息失败！！！\n");
					  continue;
					}
           			 } else {
              				  printf("正在下载文件！！！\n");
               				 if (FTP_download(sock, fp, 0) == -1) {
                   			 printf("下载文件失败\n");
               				 }
          		  }
	 		}else{
	  			printf("服务器无法发送文件\n");}
			}

		}
	}
	if (!strcmp(cmd, "help")) {
		printf("help  帮助\n");
		printf("cd path  打开路径为path的文件夹\n");
		printf("ls path  列出路径为path的文件夹\n");
		printf("mkdir path  创建路径为path的文件\n");
		printf("rmkdir path  删除路径为path的文件\n");
		printf("put path1 path2  上传本地的path1路径文件到服务器的path2路径文件\n");
		printf("get path1 path2  上传服务器的path1路径文件到本地的path2路径文件\n");
		flag=1;
	}
	if (!strcmp(cmd, "quit")) {
		
       	sendmsg.head.cmd=END;
		sendmsg.head.size = 0;
		sendmsg.path = NULL;
		if(FTP_sendMessage(sock,&sendmsg)==-1){
			continue;
		}
		break;
	}
	if (!strcmp(cmd, "lpwd")) {
			flag=1;
		   char buf[512];
                getcwd(buf,sizeof(buf));//将当前工作目录的绝对路径复制到参数buffer所指的内存空间中
                printf("[Client] Current dir in client is:\n");
                printf("%s\n",buf);
                zeromery(buf,512);
	
	}
	if (!strcmp(cmd, "lrmdir")) {
		
	}

	if (!strcmp(cmd, "lcd")) {
	

	
	}
	if (!strcmp(cmd, "ldir")) {
		if((!strcmp(data1, ""))||(strcmp(data2, ""))){
			printf("mkdir path  创建路径为path的文件\n");
		}else{
			
			if(mkdir(data1, S_IRWXU | S_IRWXG | S_IRWXO)==-1){
				printf("创建文件失败\n");
			}
			else{
			printf("您已成功创建了+"data1);
			}
		}

	
	}
		
	
	
	
	if (flag==0) {
		printf("您的输入有误，需要帮助请输入“help”！！！\n");
	}
  }
	printf("程序已退出！！！\n");
	close(sock);
	return 0;
}


void zeromery(char *a,int len)
        {
            int i;
            len=sizeof(a);
            for(i=0;i<len;i++)
            {
                a[i]=0;
            }
        }