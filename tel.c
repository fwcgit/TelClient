//
//  tel.c
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#include "tel.h"
#include "fsocket.h"
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "logs.h"
#include <sys/select.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "msg.h"
#include <stdlib.h>
#include "crc.h"

char SERVER_ADDR[30];
int  SERVER_PORT;
int  sockFd;
char isConnect          = 0;
char isConnectRun       = 0;
char is_send_heartbeat  = 0;
char isReadRun          = 0;
pthread_t readPid;
pthread_t connectPid;
pthread_t heartbeatPid  = 0;

void handler_connect_statu(long statu)
{
    int err = 10000;
    if(statu == 0)
    {
        err = 0;
    }
    else if(statu < 0l)
    {
        err = errno;
    }
    
    switch (err) {
        case ECONNRESET:
        case EBADF:
        case EPIPE:
        case ENOTSOCK:
        case 0:
            isConnect = 0;
            is_send_heartbeat = 0;
            break;
            
        default:
            break;
    }
}

void* run_read_data(void *args)
{
    int exit_rv = 101;
    int ret;
    int datalen=0;
    ssize_t totalSize = 0;
    struct timeval tout;
    fd_set fds;
    char buff[1024];
    ssize_t recvCount;
    package msg;
    char *data = NULL;
    
    tout.tv_usec = 0;
    tout.tv_sec  = 1;
    
    while(isReadRun)
    {
        if(isConnect)
        {
            
            FD_ZERO(&fds);
            FD_SET(sockFd,&fds);
            
            ret = select(sockFd+1,&fds,NULL,NULL,&tout);
            
            if(ret == 0)
            {
                tout.tv_usec = 0;
                tout.tv_sec  = 1;
                printf("read select timout\n");
            }
            else if(ret > 0)
            {
                if(FD_ISSET(sockFd,&fds))
                {
                    memset(buff, 0, sizeof(buff));
                    recvCount = recv(sockFd, buff, FRAME_HEAD_SIZE, 0);
                    
                    handler_connect_statu(recvCount);
                    
                    
                    if(recvCount > 0)
                    {
                        datalen = 0;
                        totalSize = 0;

                        totalSize += recvCount;
                        while(totalSize < FRAME_HEAD_SIZE)
                        {
                            recvCount = recv(sockFd, buff+totalSize, 1, 0);
                            if(recvCount <= 0)
                            {
                                break;
                            } 
                           totalSize += recvCount;
                        }

                        if(recvCount == FRAME_HEAD_SIZE)
                        {
                            if(*buff == (unsigned char)0x3b)
                            {
                                uint16 crc = *(buff+6) & 0x00ff;
                                crc <<= 8;
                                crc |= *(buff+7);

                                uint16 code_crc = CRC16(buff,FRAME_HEAD_SIZE-2);
                                if(crc == code_crc)
                                {
                                    datalen =  *(buff+1) & 0x000000ff;
                                    datalen <<= 8;
                                    datalen |= *(buff+2);
                                    datalen <<= 8;
                                    datalen |= *(buff+3);
                                    datalen <<= 8;
                                    datalen |= *(buff+4);

                                    while(totalSize < datalen)
                                    {
                                        recvCount = recv(sockFd, buff+totalSize, datalen, 0);
                                        if(recvCount <= 0)
                                        {
                                            break;
                                        } 
                                       totalSize += recvCount;
                                    }

                                    if(totalSize == datalen)
                                    {
                                        uint16 crc_data = *(buff+(datalen-2)) &0x00ff;
                                        crc_data <<= 8;
                                        crc_data |=  *(buff+(datalen-2));
                                        uint16 crc_data_code = CRC16(buff+FRAME_HEAD_SIZE,datalen -2-FRAME_HEAD_SIZE);
                                        if(crc_data == crc_data_code)
                                        {
                                            printf("recv %s Len:%ld\n",data,datalen);
                                            char *data = (char *)malloc(sizeof(char) * (datalen -2-FRAME_HEAD_SIZE));
                                            memcpy(data,buff+FRAME_HEAD_SIZE,datalen-2-FRAME_HEAD_SIZE);
                                            if(strcmp(data, "reqCode") == 0)
                                            {

                                            }
                                            free(data);
                                        }
                                       
                                    }
                                }
                            }
                        }  
 
                        
                    }
                }
            }
            else
            {
                printf("read select fail %d \n",errno);
                handler_connect_statu(ret);
            }
        }
        else
        {
            sleep(3);
        }
    }
    pthread_exit((void *)&exit_rv);
    return NULL;
}

void* run_connect(void *args)
{
    
    while(isConnectRun)
    {
        if(isConnect == 0)
        {
            if(connect_server() == 0)
            {
                isConnect = 1;
            }
        }
        
        sleep(10);
    }
    
    return NULL;
}

void* run_heartbeat(void *args)
{
    int rv = 102;
    char key[sizeof(package)-sizeof(void*)+ 10] = {0};
    package msg;
    msg.head.type = MSG_TYPE_HEART;
    msg.head.len = 10;
    msg.head.ck = M_CK(msg.head);
    msg.data = key;
    memset(msg.data, 0, 10);
    pack_data(msg.data, &msg, sizeof(package)-sizeof(void*), "Print0001", 9);
    
    while(isConnectRun)
    {
        if(isConnect && is_send_heartbeat)
        {
            printf("send heartbeat\r\n");
            send_data(msg.data, msg.head.len + sizeof(package)-sizeof(void*));
        }
        
        sleep(5);
    }
    
    pthread_exit((void *)&rv);
    return NULL;
}

void start_tel(char *addr,int port)
{

    strcpy(SERVER_ADDR, addr);
    SERVER_PORT = port;
    start_connect_thread();
    start_read_thread();
    start_heartbeat_thread();
}

void stop_tel(void)
{
    stop_read_thread();
    stop_connect_thread();
    close_socket();
}

void start_read_thread(void)
{
    isReadRun = 1;
    if(pthread_create(&readPid, NULL, run_read_data, (void *)100) == 0)
    {
        printf("run_read_data pthread success \n");
    }
    else
    {
        printf("run_read_data pthread fail %d \n",errno);
    }
}

void stop_read_thread(void)
{
    int rv = 0;
    isReadRun = 0;
    
    pthread_join(readPid, (void *)&rv);
    
    printf("stop_read_thread rv : %d \n",rv);
    
}

void start_connect_thread(void)
{
    isConnectRun = 1;
    
    if(pthread_create(&connectPid, NULL, run_connect, NULL) == 0)
    {
        printf("run_connect pthread success \n");
    }
    else
    {
        printf("run_connect pthread fail %d \n",errno);
    }
}

void stop_connect_thread(void)
{
    int rv = 0;
    isConnectRun = 0;

    pthread_join(connectPid, (void *)&rv);
    
    isConnect    = 0;
    
    printf("stop_connect_thread rv : %d \n",rv);

}

void start_heartbeat_thread(void)
{
    if(pthread_create(&heartbeatPid, NULL, run_heartbeat, NULL) == 0)
    {
        printf("start_heartbeat_thread success \n");
    }
    else
    {
        printf("start_heartbeat_thread fial \n");

    }
}

void stop_heartbeat_thread(void)
{
    int rv = 0;
    isConnectRun= 0;
    
    pthread_join(heartbeatPid, (void *)&rv);
    
    printf("stop_heartbeat_thread rv : %d \n",rv);
}

char *encode_msg(char type,char *data,int len)
{
    char *msg = malloc(sizeof(char) * (FRAME_HEAD_SIZE+len+2));
    int data_len = FRAME_HEAD_SIZE+len+2;
    *msg = 0x3b;
    *(msg+1) = (data_len >> 24) & 0xff;
    *(msg+2) = (data_len >> 16) & 0xff;
    *(msg+3) = (data_len >> 8) & 0xff;
    *(msg+4) = (data_len >> 0) & 0xff;
    *(msg+5) = type;
    uin
    
}

void send_data(char *data,int len)
{
    int ret;
    ssize_t sendRet;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockFd,&fds);
    
    ret = select(sockFd+1, NULL, &fds, NULL, 0);
    
    if(ret < 0)
    {
        perror("send fail \n");
        handler_connect_statu(sendRet);
    }
    else if(ret == 0)
    {
        printf("send timeout \n");
    }
    else
    {
        sendRet = write(sockFd, data, len);
        handler_connect_statu(sendRet);
    }
    
}

void pack_data(char *data,void *msg,size_t m_len,char *src,size_t s_len)
{
    if(NULL == data || NULL == msg || NULL == src) return;
    
    memcpy(data, msg, m_len);
    memcpy(data+m_len, src, s_len);
}
