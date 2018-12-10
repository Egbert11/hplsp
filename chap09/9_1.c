/*
    同时接收普通数据和带外数据
*/

#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>

#define BUFFER_SIZE 1024

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

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd > 0);
    int flag = 1, len = sizeof(int);
    int ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len);
    assert(ret != -1);
    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);
    ret = listen(listenfd, 5);
    assert(ret != -1);
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    int fd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
    if(fd < 0){
        printf("errno is:%d\n", errno);
        close(listenfd);
    }

    char buffer[BUFFER_SIZE];
    fd_set read_fds;
    fd_set exception_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&exception_fds);

    while(1){
        memset(buffer, '\0', BUFFER_SIZE);
        FD_SET(fd, &read_fds);
        FD_SET(fd, &exception_fds);
        ret = select(fd+1,&read_fds,NULL,&exception_fds,NULL);
        if(ret < 0){
            printf("select failed\n");
            break;
        }
        if(FD_ISSET(fd, &read_fds)){
            ret = recv(fd, buffer, sizeof(buffer)-1, 0);
            if(ret <= 0){
                break;
            }
            printf("get %d bytes of normal data: %s\n", ret, buffer);
        }
        else if(FD_ISSET(fd, &exception_fds)){
            ret = recv(fd, buffer, sizeof(buffer)-1, MSG_OOB);
            if(ret <= 0){
                break;
            }
            printf("get %d bytes of oob data: %s\n", ret, buffer);
        }
    }
    close(fd);
    close(listenfd);
    return 0;
}