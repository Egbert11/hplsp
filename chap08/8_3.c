/*
    HTTP请求的读取与分析
*/

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

#define BUFFER_SIZE 4096
enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER};
enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};
enum HTTP_CODE {NO_REQUEST, GET_REQUEST, BAD_REQUEST, 
FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};

static const char *szret[] = {"I get a correct result\n", "Something wrong\n"};

/* 从状态机，解析出一行内容 */
enum LINE_STATUS parse_line(char *buffer, int *checked_index, int *read_index){
    char temp;
    for(; *checked_index < *read_index; ++(*checked_index)){
        temp = buffer[*checked_index];
        if(temp == '\r'){
            if((*checked_index + 1) == *read_index){
                return LINE_OPEN;
            }
            else if(buffer[*checked_index + 1] == '\n'){
                buffer[(*checked_index)++] = '\0';
                buffer[(*checked_index)++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(temp == '\n'){
            if((*checked_index > 1) && buffer[*checked_index - 1] == '\r'){
                buffer[*checked_index - 1] = '\0';
                buffer[(*checked_index)++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

/* 分析请求行 */
enum HTTP_CODE parse_requestline(char* temp, enum CHECK_STATE *checkstate){
    /* strpbrk:在源字符串（s1）中找出最先含有搜索字符串（s2）中任一字符的位置并返回，若找不到则返回空指针。 */
    char *url = strpbrk(temp, " \t");
    if(!url){
        return BAD_REQUEST;
    }
    *url++ = '\0';

    char* method = temp;
    /*strcasecmp用忽略大小写比较字符串.*/
    if(strcasecmp(method, "GET") == 0){
        printf("The request method is GET\n");
    }
    else{
        return BAD_REQUEST;
    }

    /*strspn返回字符串s开头连续包含字符串accept内的字符数目*/
    url += strspn(url, " \t");
    char* version = strpbrk(url, " \t");
    if(!version){
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    if(strcasecmp(version, "HTTP/1.1") != 0){
        return BAD_REQUEST;
    } 

    /*strncasecmp用来比较参数s1和s2字符串前n个字符，比较时会自动忽略大小写的差异。*/
    if(strncasecmp(url, "http://", 7) == 0){
        url +=7;
        /* strchr:查找字符串s中首次出现字符c的位置。 */
        url = strchr(url, '/');
    }

    if(!url || url[0] != '/'){
        return BAD_REQUEST;
    }

    printf("The request URL is: %s\n", url);
    *checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

/* 分析头部字段 */
enum HTTP_CODE parse_headers(char *temp){
    if(temp[0] == '\0'){
        return GET_REQUEST;
    }
    else if(strncasecmp(temp, "Host:", 5) == 0){
        temp += 5;
        temp += strspn(temp, " \t");
        printf("the request host is:%s\n", temp);
    }
    else{
        printf("I can not handle this header\n");
    }
    return NO_REQUEST;
}

/* 分析http请求的入口函数 */
enum HTTP_CODE parse_content(char *buffer, int *checked_index, enum CHECK_STATE *checkstate,
                        int *read_index, int *start_line){
    enum LINE_STATUS linestatus = LINE_OK;
    enum HTTP_CODE retcode = NO_REQUEST;
    while((linestatus = parse_line(buffer, checked_index, read_index)) == LINE_OK){
        char *temp = buffer + *start_line;
        *start_line = *checked_index;
        // printf("buffer:%s,checked_index:%d,read_index:%d\n",buffer,*checked_index,*read_index);
        switch(*checkstate){
            case CHECK_STATE_REQUESTLINE:
            {
                retcode = parse_requestline(temp, checkstate);
                if(retcode == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                retcode = parse_headers(temp);
                if(retcode == BAD_REQUEST){
                    return BAD_REQUEST;
                }
                else if(retcode == GET_REQUEST){
                    return GET_REQUEST;
                }
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    // printf("linestatus:%d\n",linestatus);
    if(linestatus == LINE_OPEN){
        return NO_REQUEST;
    }
    else{
        return BAD_REQUEST;
    }
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
    }
    else{
        char buffer[BUFFER_SIZE];
        memset(buffer, '\0', BUFFER_SIZE);
        int data_read = 0;
        int read_index = 0;
        int checked_index = 0;
        int start_line = 0;
        enum CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        while(1){
            data_read = recv(fd, buffer + read_index, BUFFER_SIZE - read_index, 0);
            if(data_read == -1){
                printf("reading failed\n");
                break;
            }
            else if(data_read == 0){
                printf("remote client has closed the connection.\n");
                break;
            }
            read_index += data_read;
            // printf("buffer:%s,checked_index:%d,read_index:%d\n",buffer,checked_index,read_index);
            enum HTTP_CODE result = parse_content(buffer, &checked_index, &checkstate, &read_index, &start_line);
            if(result == NO_REQUEST){
                continue;
            }
            else if(result == GET_REQUEST){
                send(fd, szret[0], strlen(szret[0]), 0);
                break;
            }
            else{
                send(fd, szret[1], strlen(szret[1]), 0);
                break;
            }
        }
        close(fd);
    }
    close(listenfd);
    return 0;
}