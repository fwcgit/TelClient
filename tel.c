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
    ssize_t firstDataOffset = 0;
    ssize_t totalSize = 0;
    struct timeval tout;
    fd_set fds;
    char buff[128];
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
                    recvCount = recv(sockFd, buff, sizeof(buff), 0);
                    
                    handler_connect_statu(recvCount);
                    
                    if(recvCount > 0)
                    {
                        datalen = 0;
                        totalSize = 0;
                        firstDataOffset = 0;
                        
                        if(recvCount >= sizeof(msg_head))
                        {
                            printf("firset recvCount %ld %s\r\n",recvCount,buff);
                            memcpy(&msg, buff, sizeof(msg_head));
                            datalen = msg.head.len;
                            
                            if(datalen > 0)
                            {
                                data = (char *)malloc(sizeof(char) * (datalen));
                                if(NULL == data)
                                {
                                    printf("malloc mem fail \r\n");
                                }
                                memset(data, 0, datalen);
                                firstDataOffset = recvCount - (sizeof(package)-sizeof(void *));
                                memcpy(data, buff+sizeof(package)-sizeof(void *), firstDataOffset);
                            }
                            
                            while(firstDataOffset+totalSize < datalen)
                            {
                                recvCount = recv(sockFd, data+firstDataOffset+totalSize, 1024, 0);
                                printf("firset recvCount %ld %s ----\r\n",recvCount,buff);
                                if(recvCount <= 0)
                                {
                                    break;
                                }
                                else
                                {
                                    totalSize+=recvCount;
                                }
                            }
                            
                            
                            if(firstDataOffset+9 >= datalen)
                            {
                                
                                printf("clinet recv data %s firstDataOffset:%ld totalSize:%ld\n",data,firstDataOffset,totalSize);
                                free(data);
                                data = NULL;
                            }
                        }
                        
                       
                        if(strcmp(buff, "reqCode") == 0)
                        {
                            memset((char *)&msg, 0, sizeof(msg));
                            msg.head.type = MSG_TYPE_ID;
                            msg.head.len   = 10;
                            msg.head.ck = M_CK(msg.head);
                            msg.data = malloc(sizeof(char)*(M_SIZE+msg.head.len));
                            memset(msg.data, 0, M_SIZE+msg.head.len);
                            pack_data(msg.data, &msg, M_SIZE,"Print0001", msg.head.len);
                            send_data(msg.data, M_SIZE+msg.head.len);
                            free(msg.data);
                            is_send_heartbeat = 1;
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
