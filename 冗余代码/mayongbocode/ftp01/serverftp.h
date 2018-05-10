#include <stdio.h>
enum FTP_cmd_type {
	CD,
	LS,
	PWD	,
	MKDIR,
	RMDIR,
	PUT,
	GET,
        ERROR,
        OK,
	FI,
	END
};
struct myHeader
{
	enum FTP_cmd_type cmd;
	int size;
};
struct myMessage{
	struct myHeader head;
	char *path;
};
struct msg_st{
	long int msg_type;
	char msg_main[1024];
};

int verify_limits(char *name,char *password);

int send_prompt(char *buffer);

int create_user(char* name,char* password);

void get_full_path(char *path);

int CMD_lcd(char *dir);

int CMD_lls(char* tmp);

int CMD_lmkdir(char *dir);

int CMD_lrmdir(char *dir);

int CMD_lpwd(char *path);

int FTP_upload(int sock, FILE* fp, int mode);

int FTP_download(int sock, FILE* fp, int mode);

int FTP_sendHead(int sock,struct myHeader* head);

int FTP_recvHead(int sock,struct myHeader* head);

int FTP_sendEnd(int sock);

int FTP_sendMessage(int sock,struct myMessage* msg);

int FTP_recvMessage(int sock,struct myMessage* msg);

