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
int FTP_upload(int sock, FILE* fp, int mode);

int FTP_download(int sock, FILE* fp, int mode);

int FTP_sendHead(int sock,struct myHeader* head);

int FTP_recvHead(int sock,struct myHeader* head);

int FTP_sendEnd(int sock);

int FTP_sendMessage(int sock,struct myMessage* msg);

int FTP_recvMessage(int sock,struct myMessage* msg);


