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
#include "usart.h"

char *SELF_ID = "S0003";
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

void printOldData(char *data,ssize_t len)
{
#if 1
    int i = 0;
    if(NULL != data && len > 0){
        for(i = 0; i < len; i++)
        {
                printf("%02X------",*(data+i));
        }
    }
    printf("--------------------\r\n");
 #endif
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
            printf("read\n");
            FD_ZERO(&fds);
            FD_SET(sockFd,&fds);
            
            ret = select(sockFd+1,&fds,NULL,NULL,&tout);
            
            if(ret == 0)
            {
                tout.tv_usec = 0;
                tout.tv_sec  = 1;
                //printf("read select timout\n");
            }
            else if(ret > 0)
            {
                if(FD_ISSET(sockFd,&fds))
                {
                    memset(buff, 0, sizeof(buff));
                    recvCount = recv(sockFd, buff, FRAME_HEAD_SIZE, 0);
                    
                    handler_connect_statu(recvCount);
                    
                    printf("recvCount %ld -----\r\n",recvCount);
                    if(recvCount > 0)
                    {
                        datalen = 0;
                        totalSize = 0;

                        totalSize += recvCount;
                        while(totalSize < FRAME_HEAD_SIZE)
                        {
                            recvCount = recv(sockFd, buff+totalSize, 1, 0);
                            printf("FRAME_HEAD_SIZE \r\n");
                            if(recvCount <= 0)
                            {
                                break;
                            } 
                           totalSize += recvCount;
                        }
                    printf("recvCount %ld ++++++\r\n",recvCount);
                        if(recvCount == FRAME_HEAD_SIZE)
                        {
                            printOldData(buff,recvCount);
                            if(*buff == 0x3b)
                            {
                                printf("0x3b \r\n");
                                unsigned short crc = *(buff+6) & 0x00ff;
                                crc <<= 8;
                                crc |= *(buff+7)& 0x00ff;;
                                unsigned short code_crc = CRC16((unsigned char *)buff,FRAME_HEAD_SIZE-2);
                                printf("crc%d  code_crc %d\r\n",crc,code_crc);
                                if(crc == code_crc)
                                {
                                    datalen =  *(buff+1) & 0x000000ff;
                                    datalen <<= 8;
                                    datalen |= *(buff+2)& 0x000000ff;;
                                    datalen <<= 8;
                                    datalen |= *(buff+3)& 0x000000ff;;
                                    datalen <<= 8;
                                    datalen |= *(buff+4)& 0x000000ff;;

                                    printf("datalen%d \r\n",datalen);
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
                                        printf("totalSize%ld \r\n",totalSize);
                                        printOldData(buff,datalen);
                                        char s0 = *(buff+(datalen-2));
                                        char s1 = *(buff+(datalen-1));
                                        printf("%X %X \r\n",s0,s1);
                                        unsigned short data_crc = 0;
                                        data_crc = *(buff+(datalen-2)) & 0x00ff;
                                        data_crc <<= 8;
                                        data_crc |= *(buff+(datalen-1))& 0x00ff;
                                        unsigned short crc_data_code = CRC16((unsigned char *)(buff+FRAME_HEAD_SIZE),datalen -2-FRAME_HEAD_SIZE);
                                        printf("crc_data%d  crc_data_code %d\r\n",data_crc,crc_data_code);

                                        if(data_crc == crc_data_code)
                                        {
                                            char *data = (char *)malloc(sizeof(char) * (datalen -2-FRAME_HEAD_SIZE));
                                            memcpy(data,buff+FRAME_HEAD_SIZE,datalen-2-FRAME_HEAD_SIZE);
                                            printf("recv %s Len:%d\n",data,datalen-2-FRAME_HEAD_SIZE);
                                            char type = *(buff+5);
                                            if(type == MSG_TYPE_ID)
                                            {
                                                 if(strcmp(data, "reqCode") == 0)
                                                {
                                                    send_data_pack(MSG_TYPE_ID,"", 0);
                                                    is_send_heartbeat = 1;
                                                }
                                            }
                                            else if(type == MSG_TYPE_CMD)
                                            {

                                            }
                                            else if(type == MSG_TYPE_DATA)
                                            {
                                                printOldData(data,datalen -2-FRAME_HEAD_SIZE);
                                                if(*data == (char)0xA0)
                                                {
                                                    printf("send ir data \r\n");
                                                    u8 cmd[4];
                                                    cmd[0] = 0x3B;
                                                    cmd[1] = 0x0F;
                                                    cmd[3] = 0x0D;
                                                    if(*(data+1) == 0x10)
                                                    {
                                                         cmd[2] = 0xA0;
                                                       
                                                    }
                                                    else if(*(data+1) == 0x11)
                                                    {
                                                        cmd[2] = 0xA1;
                                                    }
                                                    usart_send_data(cmd,4);
                                                }
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
  
    while(isConnectRun)
    {
        if(isConnect && is_send_heartbeat)
        {
            printf("send heartbeat\r\n");
            send_data_pack(MSG_TYPE_HEART,"", 0);
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


int send_data_pack(char type,char *data,size_t len)
{
        int data_len = FRAME_HEAD_SIZE+2+5+len;
        char *user_data = (char *)malloc(sizeof(char) * (5+data_len));
        *user_data = 0x3b;
        *(user_data+1) = (char)(data_len >> 24);
        *(user_data+2) = (char)(data_len >> 16);
        *(user_data+3) = (char)(data_len >> 8);
        *(user_data+4) = (char)(data_len >> 0);
        *(user_data+5) = type;

        unsigned short crc_code = CRC16((unsigned char *)user_data,6);
        *(user_data+6) = (char)(crc_code>>8);
        *(user_data+7) = (char)(crc_code>>0);

        if(len > 0)
        {
            memcpy(user_data+FRAME_HEAD_SIZE,SELF_ID,5);
            memcpy(user_data+FRAME_HEAD_SIZE+5,data,len);
        }
        else
        {
             memcpy(user_data+FRAME_HEAD_SIZE,SELF_ID,5);
        }
        
        unsigned short crc_data = CRC16((unsigned char *)user_data+FRAME_HEAD_SIZE,len+5);
        *(user_data+(data_len-2)) = (char)(crc_data>>8);
        *(user_data+(data_len-1)) = (char)(crc_data>>0);
        
        int s_len = send_data(user_data,data_len);
        return s_len;
}

int send_data(char *data,int len)
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

   return ret; 
}

