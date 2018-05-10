#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/msg.h>

struct msg_st{
	long int msg_type;
	char msg_main[1024];
};
int main(){
	int f= 1;
	int msg_id;
	struct msg_st prompt;
	long int msg_rece =0;
	msg_id=msgget((key_t)0002,0666|IPC_CREAT);
	if(msg_id==-1){
		printf("创建消息窗口失败！！！\n");
		exit(EXIT_FAILURE);
	}
	while(f){
		if(msgrcv(msg_id,(void *)&prompt,1024,msg_rece,0)==-1){
			printf("接收提示信息失败！！！\n");
			exit(EXIT_FAILURE);
		}
		printf("HINT:%s",prompt.msg_main);
		if(strcmp(prompt.msg_main,"end")==0){
			f=0;
		}
	}
	if(msgctl(msg_id,IPC_RMID,0)==-1){
		fprintf(stderr,"msgctl failed\n");
		exit(EXIT_FAILURE);
	}
	exit(EXIT_SUCCESS);
}
