#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdarg.h>
#include <dirent.h>
#define BACKLOG 20	// 在请求队列中允许的最大请求数

struct user_struct {
    char user[20];
    char pass[20];
};
struct cmd_struct {
    char* command;
    int (*cmd_handler)(int ctrlfd, char *cmd_line);
};

int cmd_user(int, char*);
int cmd_pass(int, char*);
int cmd_pwd(int, char*);
int cmd_cwd(int, char*);
int cmd_list(int, char*);
int cmd_size(int, char*);
int cmd_type(int, char*);
int cmd_port(int, char*);
int cmd_pasv(int, char*);
int cmd_retr(int, char*);
int cmd_stor(int, char*);
int cmd_quit(int, char*);
int server_port = 21;
int pasv_socket = -1;
int pasv_control_socket = -1;
int port_control_socket = -1;
char ftp_home_dir[PATH_MAX];
struct user_struct* cur_user;
int quit_flag;
char server_resps[][256] = {    // 服务器应答码和应答信息
    "150 Begin to transfer\r\n",
    "200 OK\r\n",
    "213 %d\r\n" ,
    "220 Server ready\r\n",
    "221 Goodbye\r\n",
    "226 Transfer complete\r\n",
    "227 Entering passive mode (%d,%d,%d,%d,%d,%d)\r\n",
    "230 User %s logged in successfully!\r\n",
    "250 CWD command correct\r\n",
    "257 \"%s\" is current directory\r\n",
    "331 Password required for %s\r\n",
    "500 Unsupport command %s\r\n",
    "530 Login failed! %s\r\n",
    "550 Error: %s\r\n"
};
struct user_struct users[] = {  // 用户信息
    {"anonymous", ""},
    {"Neo0103", "1234"}
};
struct cmd_struct cmds[] = {    // 匹配FTP命令和相应的执行函数
    {"USER", cmd_user},
    {"PASS", cmd_pass},
    {"PWD",  cmd_pwd},
    {"CWD",  cmd_cwd},
    {"LIST", cmd_list},
    {"TYPE", cmd_type},
    {"PORT", cmd_port},
    {"PASV", cmd_pasv},
    {"RETR", cmd_retr},
    {"STOR", cmd_stor},
    {"QUIT", cmd_quit},
};

// 通过应答码匹配相应的应答信息，在server_resps[]中找寻
char* resps_num_map(int num) {
    int i;
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", num);
    if (strlen(buf) != 3)
        return NULL;
    for (i = 0; i < (sizeof(server_resps) / sizeof(server_resps[0])); i++)
        if (strncmp(buf, server_resps[i], 3) == 0)
            return server_resps[i];
    return NULL;
}

// 发送消息
int send_msg(int fd, char *msg, int len) {
    int n, off = 0, left = len;
    while (1) {
        n = write(fd, msg + off, left);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return n;
        }
        if (n < left) {
            left -= n;
            off += n;
            continue;
        }
        return len;
    }
}

// 接收消息
int recv_msg(int fd, char buf[], int len) {
    int n;
    while (1) {
        n = read(fd, buf, len);
        if (n < 0) {
            continue;
            return n;
        }
        return n;
    }
}

// 发送应答码和相应应答信息
int send_resp(int fd, int num, ...) {
    char *cp = resps_num_map(num);
    va_list ap;
    char buf[BUFSIZ];
    if (!cp) {
        printf("resps_num_map(%d) failed\n", num);
        return -1;
    }
    va_start(ap, num);
    vsnprintf(buf, sizeof(buf), cp, ap);
    va_end(ap);
    printf("Server response code:%s\n", buf);
    if (send_msg(fd, buf, strlen(buf)) != strlen(buf)) {
        printf("send_msg() failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

// 获取控制连接的socket
int get_control_sock(void) {
    int fd;
    if (pasv_socket >= 0) {
        fd = accept(pasv_socket, NULL, NULL);
        if (fd >= 0) {
            close(pasv_socket);
            pasv_socket = -1;
            pasv_control_socket = fd;
            return fd;
        }
        else
            printf("accept() failed:%s\n", strerror(errno));
    }
    else if (port_control_socket >= 0)
        return port_control_socket;
    return (-1);
}

// 关闭所有socket
int close_all_fd(void) {
    if (pasv_socket >= 0) {
        close(pasv_socket);
        pasv_socket = -1;
    }
    if (pasv_control_socket >= 0) {
        close(pasv_control_socket);
        pasv_control_socket = -1;
    }
    if (port_control_socket >= 0) {
        close(port_control_socket);
        port_control_socket = -1;
    }
    return 0;
}

// 服务器上的用户名
int cmd_user(int ctrlfd, char* cmdline) {
    char* cp = strchr(cmdline, ' ');
    if (cp) {
        int i;
        for (i = 0; i < (sizeof(users) / sizeof(users[0])); i++)
            if (strcmp(cp + 1, users[i].user) == 0) {
                printf("user(%s) is found\n", cp + 1);
                cur_user = &users[i];       // 设置当前用户
                break;
            }
        if (!cur_user)
            printf("user (%s) not found\n", cp + 1);
        else
            return send_resp(ctrlfd, 331, cp + 1);      // 用户名存在，请求密码
    }
    return send_resp(ctrlfd, 550, "user not found!");
}

// 服务器上的密码
int cmd_pass(int ctrlfd, char *cmdline) {
    char* space = strchr(cmdline, ' ');
    if (cur_user && space) {
        printf("papapssss%s\n",space+1);
        if (strlen(cur_user->pass) == 0 || strcmp(space + 1, cur_user->pass) == 0) {
            printf("Password for user %s is correct.\n", cur_user->user);
            return send_resp(ctrlfd, 230, cur_user->user);      // 密码正确
        }
        printf("password for user %s is wrong.\n", cur_user->user);
    }
    cur_user = NULL;
    return send_resp(ctrlfd, 530, "");
}

// 返回服务器当前工作路径
int cmd_pwd(int ctrlfd, char *cmdline) {
    char curdir[PATH_MAX];
    char* cp;
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");
    getcwd(curdir, sizeof(curdir));
    cp = &curdir[strlen(ftp_home_dir)];
    return send_resp(ctrlfd, 257, (*cp == '\0') ? "/" : cp);
}

// 改变服务器当前工作路径
int cmd_cwd(int ctrlfd, char *cmdline) {
    char* space = strchr(cmdline, ' ');
    char curdir[PATH_MAX];
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");
    if (!space)
        return send_resp(ctrlfd, 550, "CWD command wrong!");
    getcwd(curdir, sizeof(curdir));
    if (strcmp(curdir, ftp_home_dir) == 0 && space[1] == '.' && space[2] == '.')
        return send_resp(ctrlfd, 550, "no permission to access!");
    // 绝对路径
    if (space[1] == '/') {
        if (chdir(ftp_home_dir) == 0) {
            if (space[2] == '\0' || chdir(space+2) == 0)
                return send_resp(ctrlfd, 250);
        }
        chdir(curdir);
        return send_resp(ctrlfd, 550, "CWD command wrong!");
    }
    // 相对路径
    if (chdir(space+1) == 0)
        return send_resp(ctrlfd, 250);
    chdir(curdir);
    return send_resp(ctrlfd, 550, "CWD command wrong!");
}

// 使用命令'ls -l'获取路径下的文件列表
int get_list(char buf[], int len) {
    int temp, n;
    system("ls -l>.temp");
    temp = open("./.temp", O_RDONLY);
    if (temp < 0)
        puts ("OPEN .temp ERROR");
    else
        n = read(temp,buf,len);
    system("rm -f ./.temp");
    return n;
}

// 返回路径下的文件列表
int cmd_list(int ctrlfd, char *cmdline) {
    char buf[BUFSIZ];
    int n;
    int fd;
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong!");
    if ((fd = get_control_sock()) < 0) {
        printf("LIST cmd: no available fd%s", "\n");
        close_all_fd();
        return send_resp(ctrlfd, 550, "LIST command wrong!");
    }
    send_resp(ctrlfd, 150);
    // 发送命令'ls -l'的结果给客户端
    n = get_list(buf, sizeof(buf));
    if (n >= 0) {
        if (send_msg(fd, buf, n) != n) {
            printf("send_msg() failed: %s\n", strerror(errno));
            close_all_fd();
            return send_resp(ctrlfd, 550, "sendmsg failed");
        }
    }
    else {
        printf("get_list() failed %s", "\n");
        close_all_fd();
        return send_resp(ctrlfd, 550, "get list failed");
    }
    close_all_fd();
    return send_resp(ctrlfd, 226);      // 数据传输成功，关闭数据连接

}

// 指定文件类型: 缺省为 A, 表示 ASCII
int cmd_type(int ctrlfd, char *cmdline) {
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");
    return send_resp(ctrlfd, 200);
}

// 以网络字节顺序设置IP地址和端口
int set_ip_port_for_PORT(char *cmdline, unsigned int *ip, unsigned short *port) {
    char* cp = strchr(cmdline, ' ');
    int i;
    unsigned char buf[6];
    if (!cp) return -1;
    for (cp++, i = 0; i < (sizeof(buf) / sizeof(buf[0])); i++) {
        buf[i] = atoi(cp);
        cp = strchr(cp, ',');
        if (!cp && i < (sizeof(buf) / sizeof(buf[0])) - 1)
            return -1;
        cp++;
    }
    if (ip)
        *ip = *(unsigned int*)&buf[0];
    if (port)
        *port = *(unsigned short*)&buf[4];
    return 0;
}

/*
PORT <host-port>
<host-port>为(n1,n2,n3,n4,n5,n6), 客户端IP地址(n1.n2.n3.n4), 端口(n5*256+n6)
*/
int cmd_port(int ctrlfd, char *cmdline) {
    unsigned int ip;
    unsigned short port;
    struct sockaddr_in sin;
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");
    if (set_ip_port_for_PORT(cmdline, &ip, &port) != 0) {
        printf("set_ip_port_for_PORT() failed%s", "\n");
        if (port_control_socket >= 0) {
            close(port_control_socket);
            port_control_socket = -1;
        }
        return send_resp(ctrlfd, 550, "set ip port for PORT failed");
    }
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ip;
    sin.sin_port = port;
    printf("PORT cmd:%s:%d\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
    if (port_control_socket >= 0) {
        close(port_control_socket);
        port_control_socket = -1;
    }
    port_control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (port_control_socket < 0) {
        printf("socket() failed:%s\n", strerror(errno));
        if (port_control_socket >= 0) {
            close(port_control_socket);
            port_control_socket = -1;
        }
        return send_resp(ctrlfd, 550, "socket failed");
    }
    if (connect(port_control_socket, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        printf("bind() failed:%s\n", strerror(errno));
        if (port_control_socket >= 0) {
            close(port_control_socket);
            port_control_socket = -1;
        }
        return send_resp(ctrlfd, 550, "bind failed");
    }
    printf("PORT mode connect OK%s", "\n");
    return send_resp(ctrlfd, 200);
}

// 进入被动模式
int cmd_pasv(int ctrlfd, char *cmdline) {
    struct sockaddr_in pasvaddr;
    int len;
    unsigned int ip;
    unsigned short port;
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");
    if (pasv_socket >= 0) {
        close(pasv_socket);
        pasv_socket = -1;
    }
    pasv_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (pasv_socket < 0) {
        printf("socket() failed: %s\n", strerror(errno));
        return send_resp(ctrlfd, 550, "socket failed");
    }
    len = sizeof(pasvaddr);
    getsockname(ctrlfd, (struct sockaddr*)&pasvaddr, &len);
    pasvaddr.sin_port = 0;
    if (bind(pasv_socket, (struct sockaddr*)&pasvaddr, sizeof(pasvaddr)) < 0) {
        printf("bind() failed: %s\n", strerror(errno));
        close(pasv_socket);
        pasv_socket = -1;
        return send_resp(ctrlfd, 550, "bind failed");
    }
    if (listen(pasv_socket, BACKLOG) < 0) {
        printf("listen() failed: %s\n", strerror(errno));
        close(pasv_socket);
        pasv_socket = -1;
        return send_resp(ctrlfd, 550, "listen failed");
    }
    len = sizeof(pasvaddr);
    getsockname(pasv_socket, (struct sockaddr*)&pasvaddr, &len);
    ip = ntohl(pasvaddr.sin_addr.s_addr);
    port = ntohs(pasvaddr.sin_port);
    printf("local bind: %s:%d\n", inet_ntoa(pasvaddr.sin_addr), port);
    // 将服务器本地IP和端口通过应答码和应答信息发送给客户端
    return send_resp(ctrlfd, 227, (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, (port>>8)&0xff, port&0xff);
}

// 从服务器下载一个文件
int cmd_retr(int ctrlfd, char *cmdline) {
    char buf[BUFSIZ];
    char *space = strchr(cmdline, ' ');
    struct stat st;
    int fd = -1, n;
    int connfd;

    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");

    if (!space || lstat(space + 1, &st) < 0) {
        printf("RETR cmd error: %s\n", cmdline);
        if (fd >= 0)
            close(fd);
        close_all_fd();
        return send_resp(ctrlfd, 550, "no such file");
    }
    if ((connfd = get_control_sock()) < 0) {
        printf("get_control_sock() failed%s", "\n");
        if (fd >= 0)
            close(fd);
        close_all_fd();
        return send_resp(ctrlfd, 550, "no such file");
    }
    send_resp(ctrlfd, 150);
    // 开始读文件数据并将内容写到数据连接socket中
    if ((fd = open(space + 1, O_RDONLY)) < 0) {
        printf("open() failed: %s\n", strerror(errno));
        if (fd >= 0)
            close(fd);
        close_all_fd();
        return send_resp(ctrlfd, 550);
    }
    while (1) {
        if ((n = read(fd, buf, sizeof(buf))) < 0) {
            if (errno == EINTR)
                continue;
            printf("read() failed: %s\n", strerror(errno));
            if (fd >= 0)
                close(fd);
            close_all_fd();
            return send_resp(ctrlfd, 550, "read failed");
        }
        if (n == 0) break;
        if (send_msg(connfd, buf, n) != n) {
            printf("send_msg() failed: %s\n", strerror(errno));
            if (fd >= 0)
                close(fd);
            close_all_fd();
            return send_resp(ctrlfd, 550, "sendmsg failed");
        }
    }
    printf("RETR(%s) OK\n", space + 1);
    if (fd >= 0)
        close(fd);
    close_all_fd();
    return send_resp(ctrlfd, 226);
}

// 向服务器上传一个文件
int cmd_stor(int ctrlfd, char *cmdline) {
    char buf[BUFSIZ];
    char* space = strchr(cmdline, ' ');
    struct stat st;
    int fd = -1, n;
    int left, off;
    int connfd;
    if (!cur_user)
        send_resp(ctrlfd, 530, "username or password is wrong");
    if (!space || lstat(space + 1, &st) == 0) {
        printf("STOR cmd err: %s\n", cmdline);
        goto err_label;
    }
    if ((connfd = get_control_sock()) < 0) {
        printf("get_control_sock() failed%s", "\n");
        goto err_label;
    }
    send_resp(ctrlfd, 150);
    if ((fd = open(space + 1, O_WRONLY|O_CREAT|O_TRUNC, 0600)) < 0) {
        printf("open() failed: %s\n", strerror(errno));
        goto err_label;
    }
    // 开始从socket中读入数据并将内容写入本地磁盘中
    while (1) {
        if ((n = recv_msg(connfd, buf, sizeof(buf))) < 0) {
            printf("recv_msg() failed: %s\n", strerror(errno));
            goto err_label;
        }
        if (n == 0) break;
        left = n;
        off = 0;
        while (left > 0) {
            int nwrite;
            if ((nwrite = write(fd, buf + off, left)) < 0) {
                if (errno == EINTR)
                    continue;
                printf("write() failed: %s\n", strerror(errno));
                goto err_label;
            }
            off += nwrite;
            left -= nwrite;
        }
    }
    printf("STOR(%s) OK\n", space+1);
    if (fd >= 0)
        close(fd);
    close_all_fd();
    sync();
    return send_resp(ctrlfd, 226);

    err_label:
    if (fd >= 0) {
        close(fd);
        unlink(space+1);
    }
    close_all_fd();
    return send_resp(ctrlfd, 550);
}

// 退出
int cmd_quit(int ctrlfd, char *cmdline) {
    send_resp(ctrlfd, 221);
    quit_flag = 1;
    return 0;
}

// 客户端发起命令请求
int cmd_request(int ctrlfd, char buf[]) {
    char* end = &buf[strlen(buf) - 1];
    char* space = strchr(buf, ' ');
    int i;
    char save;
    int err;
    char* cp2;
    if (*end == '\n' || *end == '\r') {
        if (buf && strlen(buf) > 0) {
            cp2 = &buf[strlen(buf) - 1];
            while (*cp2 == '\r' || *cp2 == '\n')
                if (--cp2 < buf)
                    break;
            cp2[1] = '\0';
        }
        if (!space)
            space = &buf[strlen(buf)];
        save = *space;
        *space = '\0';
        // 解析并匹配命令，并调用执行相应命令的函数
        for (i = 0; cmds[i].command; i++) {
            if (strcmp(buf, cmds[i].command) == 0) {
                *space = save;
                printf("received a valid cmd: %s\n", buf);
                return cmds[i].cmd_handler(ctrlfd, buf);
            }
        }

        // 不识别的命令
        *space = save;
        printf("received a unsupported cmd: %s\n", buf);
        *space = '\0';
        err = send_resp(ctrlfd, 500, buf);
        *space = save;
        return err;
    }
    printf("received a invalid cmd: %s\n", buf);
    return send_resp(ctrlfd, 550, "received a invalid cmd");
}

// 控制连接处理
int ctrl_conn_handler(int connfd) {
    char buf[BUFSIZ];
    int buflen;
    int err = 0;
    // 建立控制连接
    if (send_resp(connfd, 220) != 0) {
        close(connfd);
        printf("Close the ctrl connection OK %s", "\n");
        return -1;
    }
    // 接收命令
    while (1) {
        buflen = recv_msg(connfd, buf, sizeof(buf));
        if (buflen < 0) {
            printf("recv_msg() failed: %s\n", strerror(errno));
            err =-1;
            break;
        }
        if (buflen == 0)
            break;
        buf[buflen] = '\0';
        cmd_request(connfd, buf);       // 解析命令
        if (quit_flag)
            break;
    }
    close(connfd);
    printf("Close the control connection OK %s", "\n");
    return err;
}

// 创建服务器socket并监听
int create_server() {
    int sockfd;
    int bReuseaddr = 1;
    struct sockaddr_in srvaddr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Create socket() failed!\n");
        return sockfd;
    }
    // 设置调用closesocket()后, 仍可继续重用该socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &bReuseaddr, sizeof(bReuseaddr)) < 0) {
        printf("setsockopt() failed!\n");
        close(sockfd);
        return -1;
    }
    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(server_port);
    srvaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    // 绑定端口和地址
    if (bind(sockfd, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) < 0) {
        printf("bind() failed!\n");
        close(sockfd);
        return -1;
    }
    // 监听
    if (listen(sockfd, BACKLOG) < 0) {
        printf("listen() failed: %s\n", strerror(errno));
        close(sockfd);
        return -1;
    }
    int len = sizeof(srvaddr);
    getsockname(sockfd, (struct sockaddr*)&srvaddr, &len);
    printf("Create server listen socket successfully: %s:%d\n", inet_ntoa(srvaddr.sin_addr), ntohs(srvaddr.sin_port));
    return sockfd;
}

// 等待客户端运行连接
int wait_client(int listenfd) {
    int connfd;
    int pid;
    while (1) {
        printf("Server ready, wait client's connection...%s", "\n");
        connfd = accept(listenfd, NULL, NULL);      // 处理连接端口的服务请求
        if (connfd < 0) {
            printf("accept() failed: %s\n", strerror(errno));
            exit(-1);
        }
        struct sockaddr_in client_addr;
        int len = sizeof(client_addr);
        getpeername(connfd, (struct sockaddr*)&client_addr, &len);
        printf("accept a connection from %s:%d\n",
            inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        switch (pid = fork()) {
            case -1:
                perror("The fork failed!");
                break;
            case 0:
                close(listenfd);
                if (ctrl_conn_handler(connfd) != 0)     // 转到控制连接处理函数
                    exit(-1);
                exit(0);
                break;
            default:
                close(connfd);
                continue;
        }
        exit(0);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 2)
        server_port = atoi(argv[1]);
    else {
        printf("Usage: %s <port>\n", argv[0]);
        exit(-1);
    }
    printf("The server port is %d\n", server_port);
    getcwd(ftp_home_dir, sizeof(ftp_home_dir));
    int listenfd = create_server();
    wait_client(listenfd);
}
