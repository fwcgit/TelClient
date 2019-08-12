//
//  fsocket.c
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#include "fsocket.h"
#include <sys/socket.h>
#include <sys/types.h>
#include "tel.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>


int create_socket(void)
{
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    
    return fd;
}

int connect_server(void)
{
    int newSockfd;
    int errorCode;
    struct sockaddr_in addr_in;
    
    errorCode = 0;
    newSockfd = create_socket();
    if(newSockfd != -1)
    {
        sockFd = newSockfd;
        addr_in.sin_family = AF_INET;
        addr_in.sin_addr.s_addr = inet_addr(SERVER_ADDR);
        addr_in.sin_port = htons(SERVER_PORT);
        
        if(connect(newSockfd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr)) == -1)
        {
            errorCode = -1;
            printf("connect server fail %d \n",errno);
            close(newSockfd);
        }
        else
        {
            printf("connect server success\n");
        }
    }
    else
    {
        errorCode = -1;
    }
    return errorCode;
}

void close_socket(void)
{
    close(sockFd);
}
