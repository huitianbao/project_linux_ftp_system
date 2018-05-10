#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>
#include <sys/ioctl.h>

#include "message.h"
#include "filemanager.h"

#define IPADDR_RE "^(ftp://)?[0-9]{1,3}[./n][0-9]{1,3}[./n][0-9]{1,3}[./n][0-9]{1,3}$"

#define FD_SIZE 128
#define INVALID_SOCKET -1

/*
 * 传输信息规范
 */
#define COMMAND_MAX 128
#define IPADDR_LEN 32
#define LOGININFO_LEN 64
#define USERNAME_LEN 32
#define PASSWORD_LEN 32

#define SM "    [server]    "
#define CM "    [client]    "

/**
 * 客户信息
 */
typedef struct _ClientInfo{
    int c_sockfd;
    in_addr_t *clientIp;
    char *username;
} ClientInfo;

/**
 * 验证ftp地址正确性
 *
 * 参数target为要验证的ftp地址
 *
 * @param ip
 * @return 返回值为1则表示验证通过;返回值为-1表示验证不通过
 */
int
verifyIPAddrFormat(char *ip)
{
    int res_code = 0;
    regex_t compiled;
    int err_buf_size = 100;
    char *err_string;
    size_t nmatch = 10;
    regmatch_t match[nmatch];

    res_code = regcomp(&compiled,IPADDR_RE,REG_EXTENDED);

    if(res_code != 0)
    {
        regerror(res_code,&compiled,err_string,err_buf_size);
        printf("err1:%s\n",err_string);
        return -1;
    }

    res_code = regexec(&compiled,ip,nmatch,match,0);

    if(res_code == 1)
    {
        regerror(res_code,&compiled,err_string,err_buf_size);
        printf("err2:%s\n",err_string);
        return -1;
    }
    else if (res_code == 0)
    {
        //匹配成功
        regfree(&compiled);
        return 1;
    }
}