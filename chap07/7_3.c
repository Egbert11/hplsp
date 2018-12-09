#include<unistd.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

/* 将服务器程序以守护进程的方式运行 */

bool daemonize(){
    pid_t pid = fork();
    if (pid < 0){
        return false;
    }
    else if(pid > 0){
        exit(0);
    }
    umask(0);

    pid_t sid = setsid();
    if(sid < 0){
        return false;
    }

    if(chdir("/") < 0){
        return false;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
    return true;
}

int main(){
    bool result = daemonize();
    printf("daemonize %s\n", result ? "true" : "false");
    return 0;
}