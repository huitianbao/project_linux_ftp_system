/* ftserve.h
 *
 * Rebecca Sagalyn
 * CS372, Program 1
 * 11/15/13
 *
 * Server side of TCP file transfer implementation, runs with custom client, 
 * ftclient.c. Sends list of files in current directory and files to ftclient. 
 * Requires user login.
 * 
 * Usage: 
 *    ./ftserve PORT#
 */

#ifndef FTSERVE_H
#define FTSERVE_H

#include "../common/common.h"


/***********字符串追加*************/
char * my_strcat(char *dst,char const *src) ;


/******************新建目录****************/
void ftserve_mkdir(int sock_control, int sock_data, char* filename);

/******************删除目录****************/
void ftserve_rmdir(int sock_control, int sock_data, char* filename);

/******************获取路经****************/
int ftserve_pwd(int sock_data, int sock_control);

/******************修改路径****************/
void ftserve_cd(int sock_control, int sock_data, char* filename);

/******************上传单个文件****************/
void ftserve_puts(int sock_control, int sock_data, char* filename);



/******************上传多个文件****************/
void ftserve_mput(int sock_control, int sock_data, char* filename);

/******************下载多个文件****************/
void ftserve_mget(int sock_control, int sock_data, char* filename);


/**
 * Send file specified in filename over data connection, sending
 * control message over control connection
 * Handles case of null or invalid filename
 */
void ftserve_retr(int sock_control, int sock_data, char* filename);



/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftserve_list(int sock_data, int sock_control);




/**
 * Open data connection to client 
 * Returns: socket for data connection
 * or -1 on error
 */
int ftserve_start_data_conn(int sock_control);



/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int ftserve_check_user(char*user, char*pass);



/** 
 * Log in connected client
 */
int ftserve_login(int sock_control);


/**
 * Wait for command from client and send response
 * Returns response code
 */
int ftserve_recv_cmd(int sock_control, char*cmd, char*arg);



/** 
 * Child process handles connection to client
 */
void ftserve_process(int sock_control);


#endif
