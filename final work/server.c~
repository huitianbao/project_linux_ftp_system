#include <crypt.h>
#include <dirent.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <pwd.h>
#include <semaphore.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

//================================================================================================
void ftpserver();

int checkuser(char *name,char *password);

int  createDialogue(int n_sockfd , struct sockaddr_in  new_address , char name[] , int isexit_pthread);

void *tdialogue(void *arg);

int  ftps_scmd(int sockfd , char cmd[]) ;

void ftps_list();

void ftps_quit(int client_sockfd);

char  *seprate(char str[]);
//=========================================================
void command_ls(int client_sockfd);

void command_pwd(int client_sockfd , char path[]);

char *command_cd(int sockfd , char *path , char *sdir);

void command_mkdir(int sockfd , char *filename);

void command_rmdir(int sockfd , char *filename);

void ftps_get(int sockfd , char *filename , char *ndir);

void ftps_put(int sockfd , char *filename , char *ndir);

void ftps_dir(int sockfd , char *path , char *sdir);
void ftps_help();
//==============================================================


void ftps_current();
void ftps_countall();


//==================================================================
void *manager();

void ftps_kill();

int checkchown(int sockfd,char *filename);

void init();

struct message *recv_msg(int sockfd,struct message *msg);

void send_msg(int sockfd,struct message *msg);

struct loginmsg *recv_lmsg(int sockfd,struct loginmsg *msg);


//---------------------------------------------------------------------
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
//==========================================================
pthread_t y[5];
int isexit[5];
char serverfile[100];
int serverfile_len;
void main(){
	init();
	ftpserver();
}
sem_t  sem;
struct user
{
	char name[20];
	char client_ip[50];
	int client_sockfd;
};
static struct user myuser[5];

struct loginmsg lmsg[6];
//======================================================================------------------------current_count;   
int current_count=0;
static int count_all=0;



/*
launch server
*/

void ftpserver(){
	int server_sockfd , server_len , client_sockfd ,client_len;
	struct sockaddr_in   server_address,client_address;
	//struct user myuser;
	//myuser.name="sd";
	fd_set input;
	//creation
	unlink("server_sockfd");
	server_sockfd=socket(AF_INET , SOCK_STREAM , 0);

	server_address.sin_family=AF_INET;
	server_address.sin_addr.s_addr=htonl(INADDR_ANY);
	server_address.sin_port=htons(9421);
	server_len=sizeof(server_address);
	//name
	bind(server_sockfd , (struct sockaddr*)&server_address ,server_len);
	
	listen(server_sockfd , 2);
	//manage new user
	sem_init(&sem ,0 ,1);
	pthread_attr_t  tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr , PTHREAD_CREATE_DETACHED);
	pthread_create(&y[0], &tattr, manager,NULL);
	(void)pthread_attr_destroy(&tattr);
	while(1){
		int i;
		for(i=1;i<=5;i++){
			if(isexit[i]==0){
				break;
			}
		}
		client_len=sizeof(client_address);
		client_sockfd=accept(server_sockfd , (struct sockaddr*)&client_address , 
			(socklen_t*)&client_len);




		recv_lmsg(client_sockfd , &lmsg[i]);

		if(checkuser(lmsg[i].name,lmsg[i].password)){
			createDialogue(client_sockfd , client_address , lmsg[i].name , i);
		} else{
			char temp[]="your name or password is wrong!\n";
			send(client_sockfd , temp,sizeof(temp),0);
			close(client_sockfd);
		}
	}
}
/*
check user and password 
return  0  or  1
*/
int checkuser(char *name,char *password){
	int bool;
	struct spwd *sp;
	int i,j;
	char passwd[512];
	char salt[512]={0};
	sp=(struct spwd *)malloc(sizeof(struct spwd));
	setspent();
	if((sp=getspnam(name))== NULL)
       	{
              	bool=0;
              	return bool;
      	}
      	else{
      		strcpy(passwd,sp->sp_pwdp);
             	//take the    salt,i record the index of code,j  ==$ times 
    		for(i=0,j=0;passwd[i] && j != 3;++i)
    		{
        			if(passwd[i] == '$')
            			++j;
    		}
    		strncpy(salt,passwd,i-1);	
    		if(strcmp(sp->sp_pwdp,crypt(password,salt))==0){
    			bool=1;
    		}
    		else{
    			bool=0;
    		}
        		endspent();
        		return bool;
        	}
}
struct cmessage{
		int tsockfd;
		struct sockaddr_in tsockaddr;
		int isexit_pthread;
};
int  createDialogue(int n_sockfd , struct sockaddr_in  new_address , 
	char name[] , int isexit_pthread){
	struct cmessage  *cmsg=NULL;
	cmsg=(struct cmessage *)malloc(sizeof(struct cmessage));
	cmsg->tsockfd=n_sockfd;
	cmsg->tsockaddr.sin_family=new_address.sin_family;
	cmsg->tsockaddr.sin_port=new_address.sin_port;
	cmsg->tsockaddr.sin_addr.s_addr=new_address.sin_addr.s_addr;
	cmsg->isexit_pthread=isexit_pthread;
	pthread_attr_t  tattr;
	pthread_attr_init(&tattr);
	pthread_attr_setdetachstate(&tattr , PTHREAD_CREATE_DETACHED);
	int i=0;
	while(isexit[i]!=0){
		i++;
	}
	if(i<5){
		struct message msg;
		recv_msg(n_sockfd , &msg);
		strcpy(myuser[i].client_ip , msg.body);
		myuser[i].client_sockfd=n_sockfd;
		strcpy(myuser[i].name, name);
		isexit[i]=1;
		pthread_create(&y[i], &tattr, tdialogue , (void *)cmsg);
	}else{
		char ch[25]="the server is busy";
		send(n_sockfd , ch,sizeof(ch),0);
	}
	(void)pthread_attr_destroy(&tattr);

}
void *tdialogue(void *arg){
	char sdir[100];
	strcpy(sdir , serverfile);
	int client_sockfd=((struct cmessage *)arg)->tsockfd;
	int isexit_pthread=((struct cmessage *)arg)->isexit_pthread;
	char ch[50]="login in successfully!";
	char cmd[20];
	struct message msg;
	strcpy(msg.body , ch);
	
	send(client_sockfd , &msg ,sizeof(msg), 0);
	char temp[50];
	count_all++;                                       //  count the user
	current_count++;


	printf("you are the %d user\n",count_all);//------------------------------------------------------printf the count all
	while(1){
		struct message msg;
		recv(client_sockfd, &msg, sizeof(msg) ,0);
		chdir(sdir);
		sem_wait(&sem);
		
		if(strncmp(msg.cmd, "ls" , 2) == 0){
			command_ls(client_sockfd);
		}
		
		if(strncmp(msg.cmd, "cd" , 2) == 0){
			strcpy(sdir , command_cd(client_sockfd , msg.body , sdir) );
		}
		if(strncmp(msg.cmd, "mkdir" , 5)==0){
			command_mkdir(client_sockfd,msg.body);
		}
		if(strncmp(msg.cmd,"rmdir",5)==0){
		        command_rmdir(client_sockfd,msg.body);  //============================rmdir
		}
		
		if(strncmp(msg.cmd, "get" , 3)==0){
			char ndir[serverfile_len];
			strcpy(ndir,sdir);
			sem_post(&sem);
			ftps_get(client_sockfd , msg.body , ndir);
			sem_wait(&sem);
		}
		if(strncmp(msg.cmd, "put" , 3)==0){
			char ndir[20];
			strcpy(ndir,sdir);
			sem_post(&sem);
			ftps_put(client_sockfd , msg.body , ndir);
			sem_wait(&sem);
		}
		if(strncmp(msg.cmd, "quit" , 4) == 0){
			ftps_quit(client_sockfd);
			sem_post(&sem);
			isexit[isexit_pthread]=0;
			break;
		}
		sem_post(&sem);

	}
	free((struct cmessage *)arg);
	pthread_exit("Exit");
}
void ftps_list(){
	int i;

	for(i=1;i<5;i++){
		if(isexit[i]!=0){
		printf("username   %s\n" ,  myuser[i].name);
		
		printf("user  ip  %s\n",myuser[i].client_ip);
		}
	}
}
void ftps_kill(){
	int temp=1;
	for(temp=1;temp<5;temp++){
		if(isexit[temp]!=0){
			struct message msg;
			strcpy(msg.body , "you are be killed");
			send(myuser[temp].client_sockfd , &msg , sizeof(msg) , 0);
			close(myuser[temp].client_sockfd);
			isexit[temp]=0;
			pthread_cancel(y[temp]);
		}
	}

//=======================--------------------------------------------------------------current_count --

			current_count--;
}
void *manager(){
	char cmd[20];
	while(1){
		printf("ftp-server>");
		fgets(cmd , 20 , stdin);
		if(strncmp(cmd,"list",4) == 0){
			ftps_list();
		}
		else if(strncmp(cmd,"current",7) == 0){//=========================================== current()
			ftps_current();
			
		}
		else if(strncmp(cmd,"count all",9) == 0){//============================================= count all
			ftps_countall();
			
		}
		else if(strncmp(cmd,"kill",4) == 0){
			ftps_kill();
		}
		else if(strncmp(cmd,"quit",4) == 0){
			ftps_kill();
			exit(0);
		}else if(strncmp(cmd,"help",4)==0){
                        ftps_help();

		}else{
			printf("the command input is wrong\n");
		}
	}
}

///===========================================================================================ftp help

void ftps_help(){
        //command list 
    printf("\n=------------------- Command List  Server ----------------------=\n");
    printf("|                                                                 |\n");
    printf("| help            : Display All Command for the Server            |\n");
    printf("|                                                                 |\n");
    printf("| quit            : Quit The Sever                                |\n");
    printf("| list            : List the user who connected                   |\n");
    printf("| current         : Display the current user                      |\n");
    printf("| count all       : Display the count of all users                |\n");
    printf("| kill <name>     : Kill the user by name                         |\n");
    printf("|                                                                 |\n");
    printf("|                                                                 |\n");
    printf("|                                                                 |\n");
    printf("=-----------------------------------------------------------------=\n");

 
}

//==================================================================
void ftps_get(int sockfd , char *filename , char *ndir){
	struct message msg;

		chdir(ndir);
		FILE *fp;
		fp=fopen(filename , "r");
		if(fp==NULL){
			strcpy(msg.body , "this file do not exits in this server");
			strcpy(msg.cmd , "error");
			send_msg(sockfd , &msg);
		}
		else{
			if(checkchown(sockfd , filename)==0){
				strcpy(msg.body , "ERROR Permission denied");
				strcpy(msg.cmd , "error");
				send_msg(sockfd , &msg);
			}else{
				strcpy(msg.body , "ok");
				strcpy(msg.cmd , "corrent");
				send_msg(sockfd , &msg);
				char buf[1024];
				int count;                   //=====================================1024
				while ((count = fread(buf, 1, 1024, fp)) > 0) {
					memcpy(msg.data , buf , count);
					msg.len=count;
					send_msg(sockfd , &msg);
				}
				msg.len=-1;
				strcpy(msg.cmd , "the command get is excute over");
				send_msg(sockfd , &msg);
				fclose(fp);
			}
		}
		
}
void ftps_put(int sockfd , char *filename , char *ndir){
	struct message msg;
	
		chdir(ndir);
		FILE *fp;
		fp=fopen(filename , "r");
		if(fp!=NULL){
			strcpy(msg.body , "server file name repetition");
			strcpy(msg.cmd , "error");
			send_msg(sockfd , &msg);
		}else{
			fp=fopen(filename , "a+");
               			
			strcpy(msg.body , "corrent");
			strcpy(msg.cmd , "ok");
			send_msg(sockfd , &msg);
			char buf[1024];
			while(1){
                        			recv_msg(sockfd , &msg);
                        			if(msg.len==-1){
                        				
                                			break;
                         			}
                         			memcpy(buf , msg.data , msg.len);
                         			fwrite(buf, 1, msg.len, fp);
               	 		}
               	 		fclose(fp);
               	 		uid_t uid;
			int i=open(filename,O_RDONLY);
			struct passwd *pwd;
			int x;
			for(x=0;x<5;x++){
				if(myuser[x].client_sockfd==sockfd){
				pwd=getpwnam(myuser[x].name);
				uid=pwd->pw_uid;
				fchown(i,uid,-1);
				}
			}
		}
		
}
void command_mkdir(int sockfd , char *filename){        //---------------------------------------------------void command_mkdir(int sockfd , char *filename)
	int status;
	status=mkdir(filename , S_IRUSR | S_IWUSR | S_IXUSR);
	struct message msg;
	uid_t uid;
	int i=open(filename,O_RDONLY);
	struct passwd *pwd;
	int x;
	for(x=0;x<5;x++){
		if(myuser[x].client_sockfd==sockfd){
			pwd=getpwnam(myuser[x].name);
			uid=pwd->pw_uid;
			fchown(i,uid,-1);
		}
	}
	if(status == 0){
		strcpy(msg.body , "success");
	}else{
		strcpy(msg.body , "fail");
	}
	send(sockfd , &msg, sizeof(msg) , 0);
}


void command_rmdir(int sockfd , char *filename){   //----------------------------------------------------------void command_rmdir(int sockfd , char *filename)
struct message msg;   //-------------------------------------------------------------------------------------------------bug  1
char path0[512]; //  this is the cash for getcwd()
char* path1="/";
char* path2;

if(getcwd(path0,512)==NULL){
perror("fail to get pwd");

exit(1);
}

path1=strcat(path0,path1);
path2=strcat(path1,filename);   //--------------------------------------------------------------------------------------------------------------

if(rmdir(path2)==-1){
strcpy(msg.body , "fail");
}else{
strcpy(msg.body , "success");
}

send(sockfd , &msg, sizeof(msg) , 0);

}

char *command_cd(int sockfd , char *path , char *sdir){
	char ndir[100];
	chdir(path);
	struct message msg;
	getcwd(ndir,sizeof(ndir));
	if(strncasecmp(ndir, sdir , serverfile_len) == 0){
		strcpy(sdir , ndir);
		strcpy(msg.body , "success" );
	}else{
		strcpy(msg.body ,  "fail to cd" );
	}
	send(sockfd ,  &msg , sizeof(msg),0);
	return sdir;
}
void command_ls(int client_sockfd){
	char dir[100];
	getcwd(dir,sizeof(dir));	
	DIR *dp;
	struct dirent *entry; //file msg
	struct stat statbuf;  //  dir msg
	
	if((dp=opendir(dir))==NULL){
		
		printf("fail to open the dir\n");
		return;
	}
	chdir(dir);
	char ch[500]="";
	while((entry=readdir(dp))!=NULL){
		lstat(entry->d_name,&statbuf);
		if(S_ISDIR(statbuf.st_mode)){
			if(strcmp(".",entry->d_name)==0||strcmp("..",entry->d_name)==0){
				continue;
			}
			strcat(strcat(ch,"   "),entry->d_name );
			strcat(ch,"/");
		}
		else {
			strcat(strcat(ch,"   "),entry->d_name );
		}
	}
	struct message msg;
	strcpy(msg.body , ch);
	send(client_sockfd , &msg , sizeof(msg) , 0);
}

void ftps_quit(int client_sockfd){
	close(client_sockfd);
}


void ftps_current(){

printf("the current user is %d\n",current_count);

}
void ftps_countall(){

printf("the total user is %d\n",count_all);
}
char *seprate(char str[]){
        static char buff[100];
        strcpy(buff,str);
        int i,j,b=0,temp;
        // delete the "\n"
        for(i = 0, j = 0; buff[i] != '\0'; i++){  
                if(buff[i] != '\n'){        
                        buff[j] = buff[i]; 
                        j++;
                }    
        }  
        buff[j] = '\0';
        for (i=0;i<strlen(buff);i++){
                if (buff[i]==' ' && buff[i+1]!=' ') {
                        temp=i+1;
                        b=1;
                        break;
                }
        }
        if(b){
                for (i=0;i<strlen(buff);i++){
                        buff[i]=buff[i+temp];
                }
        }
        else{
                printf("fun seprate() error\ndo not find the path\n");
        }
        return buff;
}
int checkchown(int sockfd,char *filename){
	int temp=1;
	uid_t uid;
	struct passwd *pwd;
	struct stat *buf;
	buf= (struct stat*)malloc(sizeof(struct stat));
	int x;
	for(x=0;x<5;x++){
		if(myuser[x].client_sockfd==sockfd){
			pwd=getpwnam(myuser[x].name);
			uid=pwd->pw_uid;
		}
	}
	stat(filename,buf);
	if(buf->st_uid==uid)
		temp=1;
	else
		temp=0;
	free(buf);
	return temp;
}
void init(){
	int i;
	for(i=0;i<5;i++){
		isexit[i]=0;
	}
	isexit[0]=1;
	FILE *fp;
	fp=fopen("serverfile" , "r");
	if(fp==NULL){
		if(mkdir("serverfile" , S_IRUSR | S_IWUSR | S_IXUSR | S_IROTH | S_IWOTH | S_IXOTH)!=0){
			puts("the function creat_directory is errro\n");
		}
		system("chmod 777 serverfile");
	}
	chdir("serverfile");
	getcwd(serverfile , sizeof(serverfile));
	serverfile_len=strlen(serverfile);
}

void send_msg(int sockfd,struct message *msg){
	void* byte=(void*)msg;
	int length=sizeof(struct message);
	int tmp=0;
	while(length!=tmp)
	{
		tmp=tmp+send(sockfd,byte,length-tmp,0);
	}
}
struct message *recv_msg(int sockfd,struct message *msg){
	
	void* byte=(void*)msg;
	int length=sizeof(struct message);
	int tmp=0;
	while(length!=tmp)
	{
		tmp+=recv(sockfd,byte,length-tmp,0);
	}
	msg=(struct message*)byte;
	
	return msg;
}
struct loginmsg *recv_lmsg(int sockfd,struct loginmsg *msg){
	
	void* byte=(void*)msg;
	int length=sizeof(struct loginmsg);
	int tmp=0;
	while(length!=tmp)
	{
		tmp+=recv(sockfd,byte,length-tmp,0);
	}
	msg=(struct loginmsg*)byte;
	return msg;
}
