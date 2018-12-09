#include<unistd.h>
#include<stdbool.h>
#include<stdio.h>

/* 切换用户 */
static bool switch_to_user(uid_t user_id, gid_t gp_id){
    /* 先确保目标用户不是root */
    if((user_id == 0) && (gp_id == 0)){
        return false;
    }
    gid_t gid = getgid();
    uid_t uid = getuid();
    
    /* 确保当前用户是合法用户： root或者目标用户 */
    if(((gid != 0) || (uid != 0)) && ((gid != gp_id) || (uid != user_id))){
        return false;
    }

    /* 如果不是root，则已经是目标用户 */
    if(uid != 0){
        return true;
    }

    /* 切换到目标用户 */
    if((setgid(gp_id) < 0) || (setuid(user_id) < 0)){
        return false;
    }
}

int main(){
    uid_t user_id = getuid();
    gid_t gp_id = getgid();
    printf("uid:%d, gid:%d\n", user_id, gp_id);
    bool result = switch_to_user(user_id, gp_id);
    if(result == true){
        printf("switch_to_user success.\n");
    }else{
        printf("switch_to_user fail.\n");
    }
    return 0;
}