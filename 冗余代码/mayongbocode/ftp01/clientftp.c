#include "clientftp.h"
#include <stdio.h>
#include <dirent.h> 
#include <stdint.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


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
		printf("连接可能断开，发送消息头失败！！！\n");
		return -1;
	}
	return 0;
}
int FTP_recvHead(int sock,struct myHeader* head){
	uint32_t buff[2];
	if (recv_until_all(sock, buff, sizeof(uint32_t) * 2) == -1){
		printf("连接可能断开，接收消息头失败！！！\n");
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
		printf("连接可能断开，发送消息尾失败！！！\n");
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
		printf("连接可能断开，发送消息尾失败！！！\n");
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
			printf("连接可能断开，发送消息体失败！！！\n");
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
			printf("无法分配内存空间给信息文件！！！\n");
			return -1;
		}
		if(recv_until_all(sock,msg->path,msg->head.size)==-1){
		printf("连接可能断开，接收送消息体失败！！！\n");
		return -1;
		}
	}
	return 0;

}


int FTP_upload(int sock, FILE *fp, int mode){
	if (fp == NULL || sock == -1) {
        	printf("文件或套接字有错误！！！\n");
        	return -1;
        }
	int maxlen=1024;
	char *buf = malloc(maxlen);
    	if (buf == NULL) {
        	printf("无法分配内存空间给buf！！！\n");
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
			printf("连接可能断开，发送消息尾失败！！！\n");
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
			printf("连接可能断开，发送消息尾失败！！！\n");
			return -1;
		}
	  }
	}
	return 0;	
}

int FTP_download(int sock, FILE * fp, int mode){
	if (fp == NULL || sock == -1) {
        	printf("文件或套接字有错误！！！\n");
        	return -1;
        }
	int maxlen=1024;
	char *buf = malloc(maxlen);
    	if (buf == NULL) {
        	printf("无法分配内存空间给buf！！！\n");
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
					printf("打开文件失败，或因没有权限\n");
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


