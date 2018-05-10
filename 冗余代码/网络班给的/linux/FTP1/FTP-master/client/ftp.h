#ifndef __FTP_H__
#define __FTP_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>
#define BUF_SIZE 8*1024




void ftp_init_from_file(char* path, char* ip, char* port);
int socket_server(char* ip, char* port);

int recv_n(int fd_recv, char* recv_buf, int len );
int read_n(int fd_read, char* read_buf, int len );
int send_n(int fd_send, char* send_buf, int len );
int write_n(int fd_write, char* write_buf, int len );
int socket_client(char* ip, char* port);
void delete_space(char* src);
int upload(int fd_up, char* file_name);
int download(int fd_down, char* file_name);
#endif


