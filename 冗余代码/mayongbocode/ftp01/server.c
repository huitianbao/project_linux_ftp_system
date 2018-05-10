#include "serverftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h> 
#include <stdint.h>
#include <pthread.h>
#include <sys/msg.h>

#define NUMBER 20

int for_server();
int find_first_null();
int init_sock();
int server_accept();
int for_client(void *arg);

int all_user_number=0;
int seize[NUMBER];
int c_sock[NUMBER];
int s_socket;
int c_socket;
struct sockaddr_in addr;
struct sockaddr_in c_addrs[NUMBER];
struct sockaddr_in c_addr;
int size=sizeof(struct sockaddr_in);
pthread_t c_thread[NUMBER];
pthread_t s_thread;
int for_client(void *arg);
int port = 11726;
char user[NUMBER+1][50];
int quit=1;
char buffers[1024]="";

pthread_mutex_t mutex;

int main(int argc,char **argv){

	int res=pthread_mutex_init(&mutex,NULL);
	if(res!=0){
		printf("初始化互斥量失败！！！\n");
		return -1;
	}	
		
	if(init_sock()==-1){
		return -1;
	}
	int i;
	for(i=0;i<NUMBER;i++){
		seize[i]=0;
	}
	res=pthread_create(&s_thread,NULL,for_server,NULL);
	if(res!=0){
		printf("创建%d号客户线程时出现错误！！！\n");
		return -1;
	}
	char paths[1024]="";
	CMD_lpwd(&paths);
	setpath(&paths);
	
	while(quit){
		c_socket=server_accept();
		if(c_socket>=0&&quit){
			send_prompt("连接成功！！！\n");
			pthread_mutex_lock(&mutex);
			int num=find_first_null();
			if(num==-1){
				send_prompt("用户已达上线！！！\n");
			}
			else{
				int res=pthread_create(&c_thread[num],NULL,for_client,(void*)num);
				if(res!=0){
					buffers[1024]="";
					sprintf(buffers,"%d",num);
					strcat(buffers,"号客户线程创建时出现错误！！！\n");
					send_prompt(buffers);
					continue;
				}
					buffers[1024]="";
					sprintf(buffers,"%d",num);
					strcat(buffers,"号客户线程创建成功！！！\n");
					send_prompt(buffers);
				all_user_number++;
				seize[num]=1;
				c_sock[num]=c_socket;
				c_socket=-1;
				c_addrs[num]=c_addr;
			}
			pthread_mutex_unlock(&mutex);
		}
	
	}
	pthread_mutex_destroy(&mutex);
	printf("程序已退出！！！\n");
	close(s_socket);
	exit(EXIT_SUCCESS);
	return 0;
}

int find_first_null(){
	int first_null=-1;
	int i;
	for(i=0;i<NUMBER&&first_null==-1;i++){
		if(seize[i]==0){
			first_null=i;
		}
	}
	return first_null;
}
int init_sock(){

	char server[20];
	strcpy(server,"127.0.0.1");
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	addr.sin_addr.s_addr = inet_addr(server);
	
	if((s_socket=socket(AF_INET,SOCK_STREAM,0))==-1){
	      printf("创建套接字有误！！！\n");
	      return -1;
	}
	int i;
	for(i=0;i<NUMBER;i++){
		seize[i]=0;
	}
	if(bind(s_socket,(struct sockaddr *)&addr,sizeof(struct sockaddr_in))==-1){
	      printf("绑定出现错误！！！\n");
	      return -1;
	}

	if((listen(s_socket,5))==-1){
	      printf("监听出错！！！\n");
	      return -1; 
	}
	int i;
	for(i=0;i<21;i++){
		user[i][50]=NULL;	
	}
	
	send_prompt("监听中。。。\n");
	return 0;
}

int server_accept(){
	int s = accept (s_socket,&c_addr,&size);
	if(s<0){
	      printf("连接失败！！！\n");
	      return -1;
	} 
	return s;

}

int quit_connect(){
	int quit_sock;
	struct sockaddr_in quit_addr;
	char quit_server[20];
	strcpy(quit_server,"127.0.0.1");
	int f=1;

	if((quit_sock=socket(AF_INET,SOCK_STREAM,0))==-1){
		return -1;
	}

	quit_addr.sin_family=AF_INET;
	quit_addr.sin_port=htons(port);
	quit_addr.sin_addr.s_addr = inet_addr(quit_server);

	if(connect(quit_sock,(struct sockaddr*)&quit_addr,sizeof(struct sockaddr_in))==-1){
		return -1;
	}
	close(quit_sock);
	return 0;
}

int for_server(){

  while(quit){

	char message[210];
	printf("server manage>");
	gets(message);
	char cmd[20]="";
	char data1[200]="";
	int i=0,c=0,d1=0,flag=0;
	for( ;flag<2&&i<210&&message[i]!='\0';i++){
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

     		}
  	}
  	
  	if (!strcmp(cmd, "list")) {
  		int user_number=0;
  		for(int i=0;i<NUMBER;i++){
  			if(seize[i]==1){
  				user_number++;
  				printf("ID:%d    CLIENT:%s\n",i,user[i+1]);
  			}
  		}printf("共有%d个客户！！！\n",user_number);
  	}
  	if (!strcmp(cmd, "count")) {
  		printf("系统访问总个数%d！！！\n",all_user_number);
  	}
 	if (!strcmp(cmd, "kill")) {
 		if((!strcmp(data1, ""))){
			printf("kill ID\n");
		}else{
			int i=atoi(&data1);
 			seize[i]=0;
 			close(c_sock[i]);
 		}
 	}
 	if (!strcmp(cmd, "quit")) {
 		for(int i=0;i<NUMBER;i++){
			seize[i]=0;
		}
		quit=0;
		if(quit_connect()==-1){
			printf("accept退出失败！！！\n");
		}
 	}
 	if (!strcmp(cmd, "users")) {
 		char user_pwd[1024];
 		ls_user(user_pwd);
 		printf("客户：%s\n",user_pwd);
 	}
 	if (!strcmp(cmd, "create")) {
 		char name[50]="";
		char password[50]="";
		printf("输入用户名：");
		gets(name);
		printf("输入密码：");
		gets(password);
		if(create_user(&name,&password)!=-1){
			printf("创建成功！！！\n");
		}
 	}
 	
 	if (!strcmp(cmd, "delete")) {
 		if((!strcmp(data1, ""))){
			printf("kill username\n");
		}else{
			char temp[100]="/opt/users/";
			strcat(temp,data1);
 			remove(temp);
 		}
 	}
 	
  }
}





int for_client(void *arg){
	int mynum=(int)arg;
	int f=1;


	char name[50]="";
	char password[50]="";
	int limit=-1;
	if(recv_until_all(c_sock[mynum], name, 50)){
		buffers[1024]="";
		sprintf(buffers,"%d",mynum);
		strcat(buffers,"用户:连接可能断开。。。\n");
		send_prompt(buffers);
		f=0;
	}
	if(recv_until_all(c_sock[mynum], password, 50)){
		buffers[1024]="";
		sprintf(buffers,"%d",mynum);
		strcat(buffers,"用户:连接可能断开。。。\n");
		send_prompt(buffers);
		f=0;
	}
	if((limit=verify_limits(&name,&password))!=-1){
		if(FTP_sendEnd(c_sock[mynum])){
		buffers[1024]="";
		sprintf(buffers,"%d",mynum);
		strcat(buffers,"用户:连接可能断开。。。\n");
		send_prompt(buffers);
		f=0;
		}
		buffers[1024]="";
		sprintf(buffers,"%d",mynum);
		strcat(buffers,"用户:登录成功！！！\n");
		send_prompt(buffers);
	}else{
		if(FTP_senderrorEnd(c_sock[mynum])){
		buffers[1024]="";
		sprintf(buffers,"%d",mynum);
		strcat(buffers,"用户:连接可能断开。。。\n");
		send_prompt(buffers);
		}
		f=0;
	}
	char mypwd[50]="/";
	user[mynum+1][50]=NULL;
	strcpy(user[mynum+1],name);
	while(f&&seize[mynum]!=0){
		struct myMessage sendmsg;
		struct myMessage recvmsg;
		if(FTP_recvMessage(c_sock[mynum],&recvmsg)==-1||recvmsg.head.cmd==ERROR){
			continue;
		}
	switch(recvmsg.head.cmd){
		case CD:
			strcpy(mypwd,recvmsg.path);
    	
			buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>cd ");
			strcat(buffers,recvmsg.path);
			strcat(buffers,"\n");
			send_prompt(buffers);
	
			sendmsg.head.size=0;
			sendmsg.path=NULL;
        		if(CMD_lcd(recvmsg.path)==-1){
        			buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"打开文件夹失败\n");
				send_prompt(buffers);
	  			sendmsg.head.cmd=ERROR;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	 			}
			}else{
	  			sendmsg.head.cmd=OK;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	  			}
			}
		break;
		case PWD:
    			buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>cd\n");
			send_prompt(buffers);
			char path[1024];
			CMD_lcd(recvmsg.path);
			CMD_lcd(&mypwd);
       			if(CMD_lpwd(&path)==-1){
       				buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"获取目录失败\n");
				send_prompt(buffers);
	  			sendmsg.head.size=0;
	  			sendmsg.path=NULL;
	  			sendmsg.head.cmd=ERROR;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	 		 	}
			}else{
	 			sendmsg.head.size=strlen(path) + 1;
	  			sendmsg.path=path;
	 			sendmsg.head.cmd=OK;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	  			}
			}
		break;
		case LS:
    			buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>ls\n");
			send_prompt(buffers);
	 		char cwd[1024]="";
	 		CMD_lcd(&mypwd);
			CMD_lcd(recvmsg.path);
        		if(CMD_lls(&cwd)==-1){
	  			buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"获取文件名失败\n");
				send_prompt(buffers);
	  			sendmsg.head.size=0;
	 			sendmsg.path=NULL;
	  			sendmsg.head.cmd=ERROR;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
				}
			}else{
	 			sendmsg.head.size=strlen(cwd) + 1;
	 			sendmsg.path=cwd;
	  			sendmsg.head.cmd=OK;
	 			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	 				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	  			}
			}
		break;
		case MKDIR:
    	  		buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>mkdir ");
			strcat(buffers,recvmsg.path);
			strcat(buffers,"\n");
			send_prompt(buffers);
	  		CMD_lcd(&mypwd);
	  		get_full_path(recvmsg.path);
          		if(CMD_lmkdir(recvmsg.path)==-1){
          			buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"创建文件夹失败\n");
				send_prompt(buffers);
	  			sendmsg.head.size=0;
	 			sendmsg.path=NULL;
	  			sendmsg.head.cmd=ERROR;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
				}
			}else{
	  			sendmsg.head.size=strlen(cwd) + 1;
	  			sendmsg.path=cwd;
	  			sendmsg.head.cmd=OK;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	  			}
			}
		break;
		case RMDIR:
    	 		buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>rmdir ");
			strcat(buffers,recvmsg.path);
			strcat(buffers,"\n");
			send_prompt(buffers);
	 		CMD_lcd(&mypwd);
	 		get_full_path(recvmsg.path);
         		if(CMD_lrmdir(recvmsg.path)==-1){
         			buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"删除文件夹失败\n");
				send_prompt(buffers);
	  			sendmsg.head.size=0;
	  			sendmsg.path=NULL;
	  			sendmsg.head.cmd=ERROR;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	  			}
			}else{
	  			sendmsg.head.size=strlen(cwd) + 1;
	  			sendmsg.path=cwd;
	  			sendmsg.head.cmd=OK;
	  			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
	  				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
	  			}
			}
		break;
    		case PUT:
			buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>get ");
			strcat(buffers,recvmsg.path);
			strcat(buffers,"\n");
			send_prompt(buffers);
			CMD_lcd(&mypwd);
			get_full_path(recvmsg.path);
	
			FILE *fp = NULL;
			char tmp[1024]="";
        		strcat(tmp,recvmsg.path);
        		struct myMessage endmsg;
        		if(FTP_recvMessage(c_sock[mynum],&endmsg)==-1){
				continue;
			} 
			if(endmsg.head.size==-1){
				buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"打开文件失败或因没有权限！！！\n");
				send_prompt(buffers);
			if(FTP_senderrorEnd(c_sock[mynum]) == -1){
				buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"用户:");
				send_prompt(buffers);
				continue;
			}
			}else{
  				if ((fp = fopen(tmp, "wb")) == NULL) {
  					buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:打开文件 ");
					strcat(buffers,recvmsg.path);
					strcat(buffers,"失败\n");
					send_prompt(buffers);
            	  			if(FTP_senderrorEnd(c_sock[mynum]) == -1){
            	  				buffers[1024]="";
						sprintf(buffers,"%d",mynum);
						strcat(buffers,"用户:");
						send_prompt(buffers);
						continue;
					} 
      				}
            			if(FTP_sendEnd(c_sock[mynum]) == -1){
            				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:");
					send_prompt(buffers);
					continue;
				}	
            			if (FTP_download(c_sock[mynum], fp, 0) == -1) {
            				buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:下载文件 ");
					strcat(buffers,recvmsg.path);
					strcat(buffers,"失败\n");
					send_prompt(buffers);
               			}
     			}
		break;
    		case GET:
    			buffers[1024]="";
			sprintf(buffers,"%d",mynum);
			strcat(buffers,"用户:cmd>put ");
			strcat(buffers,recvmsg.path);
			strcat(buffers,"\n");
			send_prompt(buffers);
			CMD_lcd(&mypwd);
			get_full_path(recvmsg.path);
			sendmsg.head.size=0;
			sendmsg.path=NULL;
			sendmsg.head.cmd=OK;
			if(FTP_sendMessage(c_sock[mynum],&sendmsg)==-1){
				buffers[1024]="";
				sprintf(buffers,"%d",mynum);
				strcat(buffers,"用户:");
				send_prompt(buffers);
				continue;
			}else{
	  			FILE *fp = NULL;
	  			char tmp[1024]="";
          			strcat(tmp,recvmsg.path);
 	  			if ((fp = fopen(tmp, "rb")) == NULL) {
              				 buffers[1024]="";
					sprintf(buffers,"%d",mynum);
					strcat(buffers,"用户:打开文件 ");
					strcat(buffers,recvmsg.path);
					strcat(buffers,"失败\n");
					send_prompt(buffers);
					if(FTP_senderrorEnd(c_sock[mynum]) == -1){
						buffers[1024]="";
						sprintf(buffers,"%d",mynum);
						strcat(buffers,"用户:");
						send_prompt(buffers);
						continue;
					}
         			 } else {
                 			if (FTP_upload(c_sock[mynum], fp, 0) == -1) {
                        			buffers[1024]="";
						sprintf(buffers,"%d",mynum);
						strcat(buffers,"用户:上传文件 ");
						strcat(buffers,recvmsg.path);
						strcat(buffers,"失败\n");
						send_prompt(buffers);
                   			}
		  
          			}
			}
		break;
    		case END:
			f=0;
		break;
		}
	}
	buffers[1024]="";
	sprintf(buffers,"%d",mynum);
	strcat(buffers,"用户:退出程序\n");
	send_prompt(buffers);
	user[mynum+1][50]=NULL;
	pthread_mutex_lock(&mutex);
	seize[mynum]=0;
	close(c_sock[mynum]);
	pthread_mutex_unlock(&mutex);	
	pthread_exit(NULL); 	

}
