#include <stdio.h>
#include <netinet/in.h>
#include "ftp.h"

int main(int argc, char **argv)
{
    char server[20];
    strcpy(server, "127.0.0.1");
    int port = 95271;

    struct sockaddr_in addr;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation error\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(server);

    if (connect
            (sockfd, (struct sockaddr *) &addr,
             sizeof(struct sockaddr_in)) == -1) {
        printf("connectiont error\n");
        return -1;
    }
    //下面开始输入命令
    FILE *fp = NULL;
    char cmd[1000];
    char tmp[1000];
    struct FTP_msg send_msg;
    struct FTP_msg recv_msg;

    while (1) {
        printf("myftp>");
        gets(cmd);

        if (!strcmp(cmd, "")) {

        } else if (!strncmp(cmd, "put", 3)) {
            send_msg.data = cmd + 4;
            send_msg.head.len = strlen(cmd + 4) + 1;
            send_msg.head.type = UPLOAD;
            if (FTP_send_msg(sockfd, &send_msg) == -1) {
                printf("put command error\n");
            }
            if (FTP_recv_msg(sockfd, &recv_msg) == -1
                    || recv_msg.head.type == ERROR) {
                printf("put error\n");
                continue;
            }
            getcwd(tmp, 1000);
            strcat(tmp, "/");
            strcat(tmp, cmd + 4);
            if ((fp = fopen(tmp, "r")) == NULL) {
                printf("opening file error\n");
                continue;
            } else {
                printf("is putting file %s\n", tmp);
                if (FTP_upload(sockfd, fp, 0) == -1) {
                    printf("putting file error\n");
                }
            }
        } else if (!strncmp(cmd, "get", 3)) {
            send_msg.data = cmd + 4;
            send_msg.head.len = strlen(cmd + 4) + 1;
            send_msg.head.type = DOWNLOAD;
            if (FTP_send_msg(sockfd, &send_msg) == -1) {
                printf("get command error\n");
            }
            if (FTP_recv_msg(sockfd, &recv_msg) == -1
                    || recv_msg.head.type == ERROR) {
                printf("get error\n");
                continue;
            }
            getcwd(tmp, 1000);
            strcat(tmp, "/");
            strcat(tmp, cmd + 4);
            if ((fp = fopen(tmp, "wa+")) == NULL) {
                printf("opening file error\n");
                send_msg.head.type = ERROR;
                send_msg.head.len = 0;
                send_msg.data = NULL;
                if (FTP_send_msg(sockfd, &send_msg) == -1) {
                    printf("get sending error error\n");
                }
            } else {
                send_msg.head.type = OK;
                send_msg.head.len = 0;
                send_msg.data = NULL;
                if (FTP_send_msg(sockfd, &send_msg) == -1) {
                    printf("get sending ok error\n");
                    continue;
                }
                if (FTP_download(sockfd, fp, 0) == -1) {
                    printf("getting file %s error\n", cmd + 4);
                }
            }
        } else if (!strncmp(cmd, "cd", 2)) {
            send_msg.data = cmd + 3;
            send_msg.head.len = strlen(cmd + 3) + 1;
            send_msg.head.type = CD;
            if (FTP_send_msg(sockfd, &send_msg) == -1) {
                printf("cd command error\n");
                continue;
            }
            if (FTP_recv_msg(sockfd, &recv_msg) == -1
                    || recv_msg.head.type == ERROR) {
                printf("cd error\n");
                continue;
            }
        } else if (!strncmp(cmd, "pwd", 3)) {
            send_msg.data = NULL;
            send_msg.head.len = 0;
            send_msg.head.type = PWD;
            if (FTP_send_msg(sockfd, &send_msg) == -1) {
                printf("pwd command error\n");
                continue;
            }
            if (FTP_recv_msg(sockfd, &recv_msg) == -1
                    || recv_msg.head.type == ERROR) {
                printf("pwd error\n");
                continue;
            }
            printf("%s\n", recv_msg.data);
        } else if (!strncmp(cmd, "mkdir", 5)) {
            send_msg.data = cmd + 6;
            send_msg.head.len = strlen(cmd + 6);
            send_msg.head.type = MKDIR;
            if (FTP_send_msg(sockfd, &send_msg) == -1) {
                printf("mkdir command error\n");
                continue;
            }
            if (FTP_recv_msg(sockfd, &recv_msg) == -1
                    || recv_msg.head.type == ERROR) {
                printf("mkdir error\n");
                continue;
            }
        } else if (!strncmp(cmd, "ls", 2)) {
            if (strlen(cmd) == 2) {
                strcpy(tmp, ".");
            } else {
                strcpy(tmp, cmd + 3);
            }
            send_msg.data = tmp;
            send_msg.head.len = strlen(tmp) + 1;
            send_msg.head.type = LS;
            if (FTP_send_msg(sockfd, &send_msg) == -1) {
                printf("ls command error\n");
                continue;
            }
            if (FTP_recv_msg(sockfd, &recv_msg) == -1
                    || recv_msg.head.type == ERROR) {
                printf("ls error\n");
                continue;
            }
            printf("%s\n", recv_msg.data);
        } else if (!strncmp(cmd, "lcd", 3)) {
            if (cmd_lcd(cmd + 4) == -1) {
                printf("lcd error\n");
            }
        } else if (!strncmp(cmd, "lpwd", 4)) {
            if (cmd_lpwd(tmp) == -1) {
                printf("lpwd error\n");
            }
        } else if (!strncmp(cmd, "lmkdir", 6)) {
            if (cmd_lmkdir(cmd + 7) == -1) {
                printf("lmkdir error\n");
            }
        } else if (!strncmp(cmd, "dir", 3)) {
            if (strlen(cmd) == 3) {
                getcwd(tmp, 1000);
            } else {
                strcpy(tmp, cmd + 4);
            }
            strcpy(cmd, tmp);
            if (cmd_dir(cmd, tmp) == -1) {
                printf("dir error\n");
            }
        } else if (!strcmp(cmd, "q") || !strcmp(cmd, "quit")) {
            break;
        } else {
            printf("unkown command\n");
        }
    }

    close(sockfd);
    return 0;
}
