/*
    用SIGURG检测带外数据是否到达
*/
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<stdbool.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<pthread.h>

#define BUF_SIZE 1024
static int connfd;

int setnonblocking(int fd){
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd){
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

/* 信号处理函数 */
void sig_urg(int sig){
    int save_errno = errno;
    printf("recv signal:%d\n", sig);
    char buffer[BUF_SIZE];
    memset(buffer, '\0', BUF_SIZE);
    int ret = recv(connfd, buffer, BUF_SIZE-1, MSG_OOB);  /* 带外数据 */
    printf("got %d bytes of oob data:'%s'\n", ret, buffer);
    errno = save_errno;
}

/* 设置信号的处理函数 */
void addsig(int sig, void (*sig_handler)(int)){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char* argv[]){
    if(argc <= 2){
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    /* 创建TCP socket，并将其绑定到端口port上 */
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    int ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
    if(connfd < 0){
        printf("errno is:%d\n",errno);
    }
    else{
        addsig(SIGURG, sig_urg);
        /*使用SIGURG信号之前，我们必须设置socket的宿主进程或进程组*/
        fcntl(connfd, F_SETOWN, getpid());

        char buffer[BUF_SIZE];
        while(1){
            memset(buffer, '\0', BUF_SIZE);
            ret = recv(connfd, buffer, BUF_SIZE-1, 0);
            if(ret <= 0){
                break;
            }
            printf("got %d bytes of normal data '%s'\n", ret, buffer);
        }
    }

    close(connfd);
    return 0;
}