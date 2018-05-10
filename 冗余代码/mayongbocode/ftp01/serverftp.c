#include "serverftp.h"
#include <stdio.h>
#include <dirent.h> 
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/msg.h>

char userpath[1024];
char buffers[1024];
int setpath(char *path){
	strcpy(userpath,path);
	strcat(userpath,"/users/");
}
int verify_limits(char *name,char *password){
	char path[500];
	strcpy(path,userpath);
	strcat(path,name);
	FILE *fp = NULL;
	if ((fp = fopen(path, "rb")) == NULL) {
      		return -1;
	}
	char buf[1024];
	fread(&buf, sizeof(char), 1024, fp);
	char pd[50]="";
	int bf=0,i=0,pw=0,limit=0;
	for(;bf!=4;i++){
		if(buf[i]==':'||buf[i]=='\n'){bf++;i++;}
		if(bf==1){
			pd[pw]=buf[i];pw++;
		}
		if(bf==3){limit=buf[i];limit-=48;}
	}
	if(strcmp(password,pd)==0){return limit;}
	else{return -1;}
}

int send_prompt(char *buffer){
	int msg_id;
	
	struct msg_st prompt;
	msg_id=msgget((key_t)0002,0666|IPC_CREAT);
	if(msg_id==-1){
		printf("发送提示信息失败！！！\n");
		return -1;
	}
	
	strcpy(prompt.msg_main,buffer);
	if(msgsnd(msg_id,(void *)&prompt,1024,0)==-1){
		return -1;
	}
	return 0;
}

int ls_user(char* tmp){
	DIR *dp;
	struct dirent* entry;
	struct stat statbuf;
	if((dp = opendir(userpath))==NULL){
		printf("打开文件目录有错误！！！\n");
		return -1;
	}
	while((entry = readdir(dp)) !=NULL){
		lstat(entry->d_name,&statbuf);
		strcat(tmp,entry->d_name);
		strcat(tmp,"    ");
		}
	closedir(dp);
	return 0; 
}
int create_user(char* name,char* password){
	char path[500]="";	
	strcat(path, userpath);
	strcat(path, name);
	char buf[100]="PW:";
	char lim[20]="\nLM:1";
	strcat(buf,password);
	strcat(buf,lim);
	if((access(path,F_OK))!=0){
		FILE *fpu = NULL;
		if ((fpu = fopen(path, "wb")) == NULL) {
             		 printf("打开文件失败\n");
             		 return -1;
           	}
           	fputs(buf,fpu);
           	fclose(fpu);
	}else{
		return -1;
	}
	return 0;
}

void get_full_path(char *path)
{
	if (path[0] == '/') {
		return;
	}
	char tmp[1000];
	getcwd(tmp, 1000);
	strcat(tmp, "/");
	strcat(tmp, path);
	strcpy(path, tmp);

}

int CMD_lcd(char *dir){
	return chdir(dir);
}
int CMD_lpwd(char *path){
	if(getcwd(path,1024)==-1){
		return -1;
	}
	return 0;
}
int CMD_lls(char* tmp){
	char cwd[1024];
	if(getcwd(&cwd,1024)==-1){
		return -1;
	}
	DIR *dp;
	struct dirent* entry;
	struct stat statbuf;
	if((dp = opendir(cwd))==NULL){
		buffers[1024]="";
		strcat(buffers,"打开文件目录有错误！！！\n");
		send_prompt(buffers);
		return -1;
	}
	while((entry = readdir(dp)) !=NULL){
		lstat(entry->d_name,&statbuf);
		strcat(tmp,entry->d_name);
		strcat(tmp,"    ");
		}
	closedir(dp);
	return 0; 
}

int CMD_lmkdir(char *dir){
	return mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
}

int CMD_lrmdir(char *dir){
	return rmdir(dir);
}

int send_until_all(int sock, char *data, int len){
	int sentlen = 0;
	int sentbytes = 0;
	for(;;) {
		sentbytes = send(sock, data+sentlen,len-sentlen,0);
		if(sentbytes<=0){
			return -1;
		}
		sentlen+=sentbytes;
		if(sentlen==len){
			return 0;
		}   
	}
}
int recv_until_all(int sock, char *data, int len){
	int recvlen = 0;
	int recvbytes = 0;
	for(;;) {
		recvbytes = recv(sock, data+recvlen,len-recvlen,0);
		if(recvbytes<=0){			
			return -1;
		}
		recvlen+=recvbytes;
		if(recvlen==len){			
			return 0;
		}  
	}
}
int FTP_sendHead(int sock,struct myHeader* head){
	uint32_t buff[2];
	buff[0] = htonl(head->cmd);
	buff[1] = htonl(head->size);
	if (send_until_all(sock, buff, sizeof(uint32_t) * 2) == -1){
		buffers[1024]="";
		strcat(buffers,"连接可能断开，接收消息头失败！！！\n");
		send_prompt(buffers);
		return -1;
	}
	return 0;
}
int FTP_recvHead(int sock,struct myHeader* head){
	uint32_t buff[2];
	if (recv_until_all(sock, buff, sizeof(uint32_t) * 2) == -1){
		buffers[1024]="";
		strcat(buffers,"连接可能断开，接收消息头失败！！！\n");
		send_prompt(buffers);
		return -1;
	}
	head->cmd = ntohl(buff[0]);
	head->size = ntohl(buff[1]);
	return 0;
}
int FTP_sendEnd(int sock){
	struct myMessage msg;
    msg.head.cmd=END;
	msg.head.size=0;
	msg.path=NULL;
	if (FTP_sendHead(sock, &msg.head)==-1){
		buffers[1024]="";
		strcat(buffers,"连接可能断开，消息尾失败！！！\n");
		send_prompt(buffers);
		return -1;
	}
	return 0;
}
int FTP_senderrorEnd(int sock){
	struct myMessage msg;
        msg.head.cmd=END;
	msg.head.size=-1;
	msg.path=NULL;
	if (FTP_sendHead(sock, &msg.head)==-1){
		buffers[1024]="";
		strcat(buffers,"连接可能断开，消息尾失败！！！\n");
		send_prompt(buffers);
		return -1;
	}
	return 0;
}
int FTP_sendMessage(int sock,struct myMessage* msg){
	if (!msg || sock == -1 || (msg->head.size != 0&&!msg->path)){
		return -1;
	}
	if (FTP_sendHead(sock, &msg->head)==-1){
		return -1;
	}
	if (msg->head.size != 0){
		if (send_until_all(sock, msg->path, msg->head.size)==-1){
			buffers[1024]="";
			strcat(buffers,"连接可能断开，消息体失败！！！\n");
			send_prompt(buffers);
			return -1;
              	}
	}
	return 0;
}
int FTP_recvMessage(int sock,struct myMessage* msg){
	if(!msg||sock==-1){
		return -1;
	}
	msg->path=NULL;
	struct myHeader header;
	if(FTP_recvHead(sock,&header)==-1){
		return -1;
	}
        msg->head=header;
	if(msg->head.size<=0){
		msg->path=NULL;
	}else{
		msg->path=malloc(msg->head.size);
		if(!msg->path){
			buffers[1024]="";
			strcat(buffers,"接收消息时，分配空间失败\n");
			send_prompt(buffers);
			return -1;
		}
		if(recv_until_all(sock,msg->path,msg->head.size)==-1){
			buffers[1024]="";
			strcat(buffers,"连接可能断开，消息体失败！！！\n");
			send_prompt(buffers);
			return -1;
		}
	}
	return 0;

}


int FTP_upload(int sock, FILE *fp, int mode){
	if (fp == NULL || sock == -1) {
        	buffers[1024]="";
		strcat(buffers,"文件或套接字有错误！！！\n");
		send_prompt(buffers);
        	return -1;
        }
	int maxlen=1024;
	char *buf = malloc(maxlen);
    	if (buf == NULL) {
        	buffers[1024]="";
		strcat(buffers,"分配空间失败\n");
		send_prompt(buffers);
       		return -1;
    	} 
	int rc;
	if(mode==0){
		while((rc=fread(buf, sizeof(char), maxlen, fp))!=0){
			struct myMessage msg;
			msg.head.cmd = 	FI	;
                	msg.head.size = rc;
			msg.path = buf;
                	if(FTP_sendMessage(sock, &msg)==-1){
				return -1;	
			}
		}fclose(fp);
		if(FTP_sendEnd(sock) == -1){
			return -1;
		}
	}else{
	  if(mode==1){
		while((strlen(fgets(buf,1024,fp)))>0){
			struct myMessage msg;
			msg.head.cmd = 	FI	;
                	msg.head.size = 1024;
			msg.path = buf;
                	if(FTP_sendMessage(sock, &msg)==-1){
				return -1;	
			}
		}fclose(fp);
		if(FTP_sendEnd(sock) == -1){
			return -1;
		}
	  }
	}
	return 0;	
}

int FTP_download(int sock, FILE * fp, int mode){
	if (fp == NULL || sock == -1) {
        	buffers[1024]="";
		strcat(buffers,"文件或套接字有错误！！！\n");
		send_prompt(buffers);
        	return -1;
        }
	int maxlen=1024;
	char *buf = malloc(maxlen);
    	if (buf == NULL) {
        	buffers[1024]="";
		strcat(buffers,"分配空间失败\n");
		send_prompt(buffers);
       		return -1;
    	} 
	if(mode==0){
		int rc;	
		for(;;){
        	        struct myMessage recvmsg;
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				return -1;
			}
			if(recvmsg.head.cmd==END){
				if(recvmsg.head.size==-1){
					buffers[1024]="";
					strcat(buffers,"打开文件失败，或因没有权限\n");
					send_prompt(buffers);
				}
				fclose(fp);
				return 0;
			}
			rc=recvmsg.head.size;
			buf=recvmsg.path;
			fwrite(buf,sizeof(char),rc,fp);
		}
	}else{if(mode==1){
		for(;;){
        	        struct myMessage recvmsg;
			if(FTP_recvMessage(sock,&recvmsg)==-1){
				return -1;
			}	
			if(recvmsg.head.cmd==END){
				return 0;
			}
			buf=recvmsg.path;
			fputs(buf,fp);fclose(fp);
		}
	   }
	}
	return 0;
}

