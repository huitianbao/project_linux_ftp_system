#include <stdio.h>
#include <netinet/in.h>
#include "ftp.h"
#include "command.h"

int main(int argc, char **argv)
{
    int sockfd;
    int port = 95271;
    struct sockaddr_in addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("server socket creation error\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_aton("127.0.0.1", &addr.sin_addr);

    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))
            == -1) {
        printf("server binding error\n");
        return -1;
    }

    if (listen(sockfd, 5) == -1) {
        printf("server listening error\n");
        return -1;
    }

    printf("server is running, ip: %s, port: %d\n",
            inet_ntoa(addr.sin_addr), port);

    int len;
    int cli_sockfd;
    struct sockaddr_in cli_addr;

    cli_sockfd = accept(sockfd, &cli_addr, &len);
    if (cli_sockfd < 0) {
        printf("server acception error\n");
        return -1;
    }

    struct FTP_msg send_msg;
    struct FTP_msg recv_msg;
    char tmp[1000];
    FILE *fp = NULL;

    while (1) {
        if (FTP_recv_msg(cli_sockfd, &recv_msg) == -1) {
            printf("server: receiving data error\n");
            return -1;
        }

        switch (recv_msg.head.type) {
            case LOG:
                //
                break;
            case CD:
                printf("command is CD\n");
                if (cmd_lcd(recv_msg.data) == -1) {
                    send_msg.head.type = ERROR;
                } else {
                    send_msg.head.type = OK;
                }
                send_msg.head.len = 0;
                send_msg.data = NULL;
                if(FTP_send_msg(cli_sockfd, &send_msg)==-1){
                    printf("sending CD answer error\n");
                }
                break;
            case PWD:
                printf("command is PWD\n");
                if (cmd_lpwd(tmp) == -1) {
                    send_msg.head.type = ERROR;
                    send_msg.head.len = 0;
                    send_msg.data = NULL;
                } else {
                    send_msg.head.type = OK;
                    send_msg.data = tmp;
                    send_msg.head.len = strlen(tmp) + 1;
                }
                if(FTP_send_msg(cli_sockfd, &send_msg)==-1){
                    printf("sending CD answer error\n");
                }
                break;
            case LS:
                printf("command is LS\n");
                if (cmd_dir(recv_msg.data, tmp) == -1) {
                    send_msg.head.type = ERROR;
                    send_msg.data = NULL;
                    send_msg.head.len = 0;
                } else {
                    send_msg.head.type = OK;
                    send_msg.data = tmp;
                    send_msg.head.len = strlen(tmp) + 1;
                }
                if(FTP_send_msg(cli_sockfd, &send_msg)==-1){
                    printf("sending CD answer error\n");
                }
                break;
            case MKDIR:
                printf("command is MKDIR\n");
                if (cmd_lmkdir(recv_msg.data) == -1) {
                    send_msg.head.type = ERROR;
                } else {
                    send_msg.head.type = OK;
                }
                send_msg.head.len = 0;
                send_msg.data = NULL;
                if(FTP_send_msg(cli_sockfd, &send_msg)==-1){
                    printf("sending CD answer error\n");
                }
                break;
            case UPLOAD:
                printf("command is UPLOAD\n");
                getcwd(tmp, 1000);
                strcat(tmp, "/");
                strcat(tmp, recv_msg.data);
                printf("upload file %s\n", tmp);
                if ((fp = fopen(tmp, "wa+")) == NULL) {
                    printf("open file %s error\n", tmp);
                    send_msg.head.type = ERROR;
                    send_msg.head.len = 0;
                    send_msg.data = NULL;
                    if (FTP_send_msg(cli_sockfd, &send_msg) == -1) {
                        printf("UPLOAD: sending error error\n");
                    }
                } else {
                    send_msg.head.type = OK;
                    send_msg.head.len = 0;
                    send_msg.data = NULL;
                    if (FTP_send_msg(cli_sockfd, &send_msg) == -1) {
                        printf("sending ok answer error\n");
                        continue;
                    }
                    if (FTP_download(cli_sockfd, fp, 0) == -1) {
                        printf("write file %s error\n", tmp);
                    }
                }

                break;
            case DOWNLOAD:
                printf("command is DOWNLOAD\n");
                getcwd(tmp, 1000);
                strcat(tmp, "/");
                strcat(tmp, recv_msg.data);
                printf("get file %s\n", tmp);
                if ((fp = fopen(tmp, "r")) == NULL) {
                    printf("open file %s error\n", tmp);
                    send_msg.head.type = ERROR;
                    send_msg.head.len = 0;
                    send_msg.data = NULL;
                    if (FTP_send_msg(cli_sockfd, &send_msg) == -1) {
                        printf("DOWNLOAD: sending error error\n");
                    }
                } else {
                    send_msg.head.type = OK;
                    send_msg.head.len = 0;
                    send_msg.data = NULL;
                    if (FTP_send_msg(cli_sockfd, &send_msg) == -1) {
                        printf("sending ok answer error\n");
                        continue;
                    }
                    if (FTP_upload(cli_sockfd, fp, 0) == -1) {
                        printf("reading file %s error\n", tmp);
                    }
                }
                break;
            case END:
                printf("command is END\n");
                break;
        }

    }

    printf("server exit\n");

}
