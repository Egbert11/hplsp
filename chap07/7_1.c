#include<unistd.h>
#include<stdio.h>

/* 测试进程的UID和EUID的区别 */

int main(){
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf("userid is %d, effective userid is:%d\n", uid, euid);
    return 0;
}