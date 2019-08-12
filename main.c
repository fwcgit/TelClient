//
//  main.c
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <unistd.h>
#include "logs.h"
#include "tel.h"
#include "logfile.h"
#include <signal.h>
#include "msg.h"

int main(int argc, const char * argv[]) {
    

    //signal(SIGPIPE, SIG_IGN);

    //strcpy(global_log_path, "/Users/fu/work/logdir/test1/");

    //check_dir("/Users/fu/work/logdir");

    //write_logs("test log-------");
#if 1
    package pk;
    pk.head.type = MSG_TYPE_DATA;
    pk.data = malloc(sizeof(char)*10);
    pk.head.len = sizeof(package) - sizeof(void *)+10;
    memcpy(pk.data, "abcdefghijk", 10);

    //printf("%d %d \r\n",sizeof(pk),sizeof(package));
    //start_tel("127.0.01",38888);
    //start_tel("103.119.2.35",38888);
    //start_tel("192.168.193.151",38888);
    start_tel("106.12.189.237",38888);
    
//    sleep(3);
    //send_data((char *)&pk, sizeof(pk.head.len));
    getchar();

#endif
#if 0
    char buff[40] = {0};
    package msg,*pk;
    memset((char *)&msg, 0, sizeof(msg));
    msg.head.type = MSG_TYPE_ID;
    msg.head.len   = 10;
    msg.head.ck = M_CK(msg.head);
    msg.data = malloc(sizeof(char)*(M_SIZE+msg.head.len));
    memset(msg.data, 0, M_SIZE+msg.head.len);
    pack_data(msg.data, &msg, M_SIZE,"123456789", msg.head.len);
    send_data(msg.data, M_SIZE+msg.head.len);
    memcpy(buff, msg.data, M_SIZE+msg.head.len);
    pk = (package*)malloc(sizeof(package));
    memset(pk, 0, sizeof(package));
    memcpy(pk, buff, sizeof(msg_head));
#endif
    return 0;
}
