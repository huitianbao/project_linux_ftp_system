#include <arpa/inet.h>
#include <sys/ioctl.h>  
#include <net/if.h> 
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

//=========================================================================
void createclient();

void *sendsockfd(char *ch);

int ftp_rcmd(int sockfd , char cmd[]);

int ftp_scmd(int sockfd , char cmd[]) ;

void ftpc_pwd(int sockfd);

void ftpc_ls(int sockfd);

void ftpc_help();

void ftpc_lpwd();

void ftpc_quit();

void ftpc_lcd(char ch[]);

char  *cut(char str[]);

void ftpc_cd(int sockfd, char cha[]);

void ftpc_dir(int sockfd , char *cmd);

void send_ip(int sockfd);

void ftpc_mkdir(int sockfd , char *cmd);

void ftpc_rmdir(int sockfd , char *cmd);

void ftpc_get(int sockfd , char *cmd);

void ftpc_put(int sockfd , char *filename);

void ftpc_lrmdir(char *cmd);

void ftpc_lmkdir(char *cmd);

struct message *recv_msg(int sockfd,struct message *msg);

void send_msg(int sockfd,struct message *msg);

struct loginmsg *send_lmsg(int sockfd,struct loginmsg *msg);

//-------------------------------------------------------------------
struct message
{
        char cmd[10];
        char body[500];
        char data[2048];
        int len;
};
struct  loginmsg
{
        char name[20];
        char password[20];
};
void send_msg(int sockfd,struct message *msg);
//=========================================================================
void main(){
        createclient();
}

struct loginmsg lmsg;
void createclient(){             
                printf("name:");
                scanf("%s",lmsg.name);
                strcpy(lmsg.password,getpass("password:"));//get the password
                int sockfd;
                int len;
                int result;
                static char buf[100];
                struct sockaddr_in address;
                unlink("sockfd");//delete the former socket and use the ne 
                sockfd = socket(AF_INET,SOCK_STREAM,0);
                address.sin_family = AF_INET;
                char ip[20];
                printf("ip:");

                scanf("%s",ip);
                

                address.sin_addr.s_addr = inet_addr(ip);// to receive x.x.x.x
                address.sin_port = htons(9421);
                len = sizeof(address);
                result = connect(sockfd,(struct sockaddr *)&address,len);//connect
                if(result == -1){
                	
			printf("connect error");
                	exit(1);
                };
             //connect to the server socket 
                send_lmsg(sockfd  , &lmsg);
                send_ip(sockfd);
                struct message msg;

                recv(sockfd , &msg , sizeof(msg) , 0);//send msg to server to check
                strcpy(buf , msg.body);
                if(strncmp(buf, "login in successfully!" , 22) == 0){
                        puts(buf);
                }else{
                        puts(buf);
                        return;
                }
                fgets(buf , 100 , stdin);
                while(1){
                        printf("ftp-client>");
                        fgets(buf , 100 , stdin);
                      if(strncmp(buf, "ls" , 2) == 0){
                                ftpc_ls(sockfd);
                        }
                        else if(strncmp(buf , "help" , 2)==0){
                                ftpc_help();
                        }
                        else if(strncmp(buf, "cd", 2)==0){
                                ftpc_cd(sockfd , buf);
                        }                  
                        else if(strncmp(buf, "mkdir", 5)==0){            // mkdir
                                ftpc_mkdir(sockfd , buf);
                        }
                        else if(strncmp(buf, "rmdir", 5)==0){            // rmdir                                ftpc_rmdir(sockfd , buf);
                        	ftpc_rmdir(sockfd,buf);                      
			}
                        else if(strncmp(buf, "get", 3)==0){              //==get
                                ftpc_get(sockfd , buf);
                        }else if(strncmp(buf , "put", 3)==0){            // ===put
                                ftpc_put(sockfd , cut(buf));
                        }else if(strncmp(buf,"lpwd",4)==0){                  //=======lpwd
                                ftpc_lpwd();
			}else if(strncmp(buf,"lrmdir",6)==0){               //======lrmdir
				ftpc_lrmdir(buf);
			}else if(strncmp(buf,"lmkdir",6)==0){               //    lmkdir
				ftpc_lmkdir(buf);
			}else if(strncmp(buf,"lcd",3)==0){
				ftpc_lcd(buf);                         //-----------lcd
			}
                        else if(strncasecmp(buf , "quit", 4)==0){    ///=========quit
                                ftpc_quit(sockfd);
                                break;
                        }
                        else{
                                printf("the input command is error\n");
                        }
                }
}
void ftpc_help(){
        //command list 
    printf("\n=------------------- Command List         ----------------------=\n");
    printf("|                                                                 |\n");
    printf("| help            : Display All Command for the Client            |\n");
    printf("|                                                                 |\n");
    printf("| quit            : Quit The Sever                                |\n");
    printf("|                                                                 |\n");
    printf("| ls              : Display All file On the Ftp Server            |\n");
    printf("|                                                                 |\n");
    printf("| get <file>      : Download FIle from the Ftp Server             |\n");
    printf("|                                                                 |\n");
    printf("| put <file>      : Upload FIle to the Ftp Server                 |\n");
    printf("| lpwd            :Display the current path                       |\n");
    printf("| lcd <file>      : Open the file                                 |\n");
    printf("| lrmdir <file>   : Delete the file On the Ftp Client             |\n");
    printf("| lmkdir <file>   : Create the file On the Ftp Client             |\n");
    printf("| rmdir <file>    : Delete the file On the Ftp Server             |\n");
    printf("| mkdir <file>    : Create the file On the Ftp Server             |\n");
    printf("| mput <file> ... :   Download FIles from the Ftp Server          |\n");
    printf("| mget <file> ... :   Upload FIles from the Ftp Server            |\n");
    printf("|                                                                 |\n");
    printf("=-----------------------------------------------------------------=\n");

 
}
void ftpc_get(int sockfd , char *cmd){  
        char filename[50];
        strcpy(filename , cut(cmd));
        FILE *fp;
        fp=fopen(cut(cmd) , "r");                                       //--------------------------------------------------------------void ftpc_get(int sockfd , char *cmd)
        if(fp!=NULL){
                puts("the file is already exits\n");
                return;
        }
        struct message msg;
        strcpy(msg.cmd , "get");
        strcpy(msg.body , cut(cmd));
        send_msg(sockfd , &msg);
        recv_msg(sockfd , &msg);
        if(strncasecmp(msg.cmd, "error" , 5)==0){
                puts(msg.body);
                return;
        }else{
                puts(msg.body);
                puts(filename);
                fp=fopen(filename , "wa+");
      
                while(1){
                        recv_msg(sockfd , &msg);
                        if(msg.len==-1){
                                puts("get successfully\n");
                                break;
                         }
                         
                         fwrite(msg.data, 1, msg.len, fp);
                }
                fclose(fp);
        }
}
void ftpc_mkdir(int sockfd , char *cmd){   //-----------------------------------void ftpc_mkdir(int sockfd , char *cmd)
        struct message msg;
        strcpy(msg.cmd , "mkdir");
        strcpy(msg.body , cut(cmd));
        send(sockfd , &msg , sizeof(msg) , 0);
        recv(sockfd , &msg , sizeof(msg) , 0);
        puts(msg.body);
}

void ftpc_rmdir(int sockfd , char *cmd){  //-----------------------------------void ftpc_rmdir(int sockfd , char *cmd)
        struct message msg;
        strcpy(msg.cmd , "rmdir");
        strcpy(msg.body , cut(cmd));
        send(sockfd , &msg , sizeof(msg) , 0);
        recv(sockfd , &msg , sizeof(msg) , 0);
        puts(msg.body);
}
void ftpc_ls(int sockfd){
        struct message msg;
        strcpy(msg.cmd , "ls");
        send(sockfd , &msg, sizeof(msg) ,0);
        //char path[500];
        recv(sockfd , &msg,sizeof(msg) , 0);
        puts(msg.body);   
}
void ftpc_quit(int sockfd){                              //---------------------------void ftpc_quit(int sockfd)
        struct message msg;
        strcpy(msg.cmd , "quit");
        send(sockfd , &msg , sizeof(msg) ,0);
}
char  *cut(char str[]){                        /// ------------------------------------char  *cut(char str[])
        static char buf[100];
        strcpy(buf,str);
        int i,j,b=0,temp;
        for(i = 0, j = 0; buf[i] != '\0'; i++){  
                if(buf[i] != '\n'){        
                        buf[j] = buf[i]; 
                        j++;
                }  
        }  
        buf[j] = '\0';
        for (i=0;i<strlen(buf);i++){
                if (buf[i]==' ' && buf[i+1]!=' ') {
                        temp=i+1;
                        b=1;
                        break;
                }
        }
        if(b){
                for (i=0;i<strlen(buf);i++){
                        buf[i]=buf[i+temp];
                }
        }
        else{
                printf("error in fun(cut)");
        }
        return buf;
}

void ftpc_cd(int sockfd , char *cmd){   //            ----------------------------------------void ftpc_cd(int sockfd , char *cmd)
        struct message msg;
        strcpy(msg.cmd , "cd");
        strcpy(msg.body , cut(cmd));
        send(sockfd , &msg , sizeof(msg) , 0);
        recv(sockfd , &msg ,sizeof(msg) ,0);
        puts(msg.body);
}


void ftpc_lcd(char cmd[100]){                                 //--------------------------------void ftpc_lcd(char ch[100]);
	char path0[512]; //  the cash for the getcwd()
	char* path1="/";
	char* path2;

	if(getcwd(path0,512)==NULL){
		perror("fail to get pwd");
		exit(1);
	}

	path1=strcat(path0,path1);
	path2=strcat(path1,cmd+4);
	if(chdir(path2)==-1){

		printf("fail to open the directory: %s",cmd+3);
		exit(1);
	}

}
void send_ip(int sockfd){
        struct sockaddr_in *sin;
        struct ifreq        ifr;
        strcpy(ifr.ifr_name,"eth0");
        char pAddr[256];
       
        ioctl(sockfd,SIOCGIFADDR,&ifr);
        sin = (struct sockaddr_in *)&(ifr.ifr_addr);
        strcpy(pAddr, inet_ntoa(sin->sin_addr));
      
        struct message msg;
        strcpy(msg.body , pAddr);
        send_msg(sockfd , &msg);
}
voi
d ftpc_put(int sockfd , char *filename){
        FILE *fp;
        fp=(FILE *)malloc(sizeof(FILE));           //                            void ftpc_put(int sockfd , char *filename)
        fp=fopen(filename, "r");
        if(fp==NULL){
                puts("no this file in local");
                return;
        }
        struct message msg;
        strcpy(msg.cmd , "put");
        strcpy(msg.body , filename);
        send_msg(sockfd , &msg);
        recv_msg(sockfd , &msg);
        if(strncasecmp(msg.cmd, "error" , 5)==0){
                puts(msg.body);
                return;
        }else{
                char buf[2048];
                int count;
                while ((count = fread(buf, 1, 2048, fp)) > 0) {
                    //strcpy(msg.cmd,"send");
                    memcpy(msg.data , buf , count);
                    msg.len=count;
                    send_msg(sockfd , &msg);
                }
                strcpy(msg.cmd , "over");
                msg.len= -1;
                send_msg(sockfd , &msg);
                puts("put successfully!\n");
                fclose(fp);
        }
}



//================================================================================================  void ftpc_lpwd();
void ftpc_lpwd(){

	char buf[512]; //  the cash for the getcwd()

	if(getcwd(buf,512)==NULL){
		perror("fail to get pwd");
		exit(1);
	}
	printf("    \n %s\n\n",buf);


}



void ftpc_lrmdir(char *cmd){      ///------------------------------------------------------------------void ftpc_lrmdir(char *cmd)   

	char path0[512]; //  the cash for the getcwd()
	char* path1="/";
	char* path2;

	if(getcwd(path0,512)==NULL){
		perror("fail to get pwd");
		exit(1);
}

	path1=strcat(path0,path1);
	path2=strcat(path1,cmd+7);

	if(rmdir(path2)==-1){
	perror("fail to rm");
	exit(1);
	}else{
	printf("lrmdir successfully!\n");
	}

}


void ftpc_lmkdir(char *cmd){    //==--------------------------------------------------------------------void ftpc_lmkdir(char *cmd)  
	char path0[512]; //  //  the cash for the getcwd()
	char* path1="/";
	char* path2;

	if(getcwd(path0,512)==NULL){
	perror("fail to get pwd");
	exit(1);
}

	path1=strcat(path0,path1);
	path2=strcat(path1,cmd+7);

//printf("path2 is %s\n",path2);

	if(mkdir(path2,S_IRUSR|S_IWUSR|S_IXUSR)==-1){
		perror("fail to mkdir");
		exit(1);
	}else{
		printf("lmkdir successfully!\n");
	}

}












struct message *recv_msg(int sockfd,struct message *msg){
    //printf("Waiting recv\n");
    void* byte=(void*)msg;
    int length=sizeof(struct message);
    int tmp=0;
    while(length!=tmp)
    {
        tmp+=recv(sockfd,byte,length-tmp,0);
    }
    msg=(struct message*)byte;
    //printf("recv msg success\n");
    return msg;
}
void send_msg(int sockfd,struct message *msg)
{
    void* byte=(void*)msg;
    int length=sizeof(struct message);
    int tmp=0;
    while(length!=tmp)
    {
        tmp=tmp+send(sockfd,byte,length-tmp,0);
    }
}
struct loginmsg *send_lmsg(int sockfd,struct loginmsg *msg)
{
    void* byte=(void*)msg;
    int length=sizeof(struct loginmsg);
    int tmp=0;
    while(length!=tmp)
    {
        tmp=tmp+send(sockfd,byte,length-tmp,0);
    }
    return msg;
}



