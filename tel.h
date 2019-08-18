//
//  tel.h
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#ifndef tel_h
#define tel_h

#include <stdio.h>

#define MSG_LEN(msg,len) sizeof(msg)-sizeof(void *)+msg.head.len

extern char SERVER_ADDR[30];
extern int  SERVER_PORT;
extern int sockFd;

void start_tel(char *addr,int port);

void start_read_thread(void);

void stop_read_thread(void);

void stop_tel(void);

void stop_connect_thread(void);

void start_connect_thread(void);

int send_data_pack(char type,char *data,size_t len);

int send_data(char *data,int len);

void start_heartbeat_thread(void);

void stop_heartbeat_thread(void);


#endif /* tel_h */
