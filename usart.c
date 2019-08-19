//
// Created by fwc on 2018/6/20.
//

#include "usart.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <string.h>



int          tty_fd = -1;
u8           is_run = 0;
u8           is_usart_Connect  = 0;
u8           isSendDeviceInit = 0;
char         *ttyComm;
u8              isHandler;

u8           *send_temp_data;
size_t       send_temp_len;

u8           send_cmd_ack = 0;
u8           *ack_data = NULL;
size_t       ack_data_len = 0;

u8           read_thread_run = 0;
u8           cmdack_thread_run = 0;

int init_usart(int fd);
int read_thread();
int cmd_ack_thread();

void try_open_usart(void)
{
    int flag = 0;
    int fd;

    if(tty_fd > 0)
    {
        close(tty_fd);
        tty_fd = -1;
    }

    fd = open(ttyComm,O_RDWR | O_NOCTTY | O_NDELAY);

    if(fd > 0)
    {
        flag = fcntl(fd,F_GETFL,0);
        if(fcntl(fd,F_SETFL,flag | O_NONBLOCK) == -1)
        {
            perror("set usart setfl fail");

        }

        if(isatty(fd) == 0)
        {
            perror("this deivce not usart");

        }

        if(init_usart(fd) == 0)
        {
            is_usart_Connect = 1;
        }
        else
        {
            perror("try init usart fail!");
        }

    } else
    {
        if(fd == -1)
        {
            perror("open usart fail");

        }
    }
}

int  open_usart(const char *ttys)
{
    int flag = 0;
    if(NULL == ttyComm)
    {
        ttyComm = (char *)malloc(sizeof(char) * 20);
    }
    memset(ttyComm,0,sizeof(char) * 20);
    strcpy(ttyComm,ttys);

    if(tty_fd > 0)
    {
        close(tty_fd);
        tty_fd = -1;
    }

    int fd = open(ttys,O_RDWR | O_NOCTTY | O_NDELAY);

    if(fd > 0)
    {
        flag = fcntl(fd,F_GETFL,0);
        if(fcntl(fd,F_SETFL,flag | O_NONBLOCK) == -1)
        {
            perror("set usart setfl fail");

        }

        if(isatty(fd) == 0)
        {
            perror("this deivce not usart");

        }

        if(init_usart(fd) == 0)
        {

            is_usart_Connect = 1;
            is_run = 1;
            read_thread();
            cmd_ack_thread();

        } else
        {
            return -1;
        }


    } else
    {
        if(fd == -1)
        {
            perror("open usart fail");
            return -1;
        }
    }
    return 0;
}

int init_usart(int fd)
{
    struct termios oldAttr;
    struct termios newAttr = { 0 };

    if(tty_fd != -1)
    {
        close(tty_fd);
    }

    sleep(1);

    tcflush(fd, TCIFLUSH);    //清空输入缓存
    tcflush(fd, TCOFLUSH);    //清空输出缓存
    tcflush(fd, TCIOFLUSH);   //清空输入输出缓存

    if(tcgetattr(fd,&oldAttr) == -1)
    {
        perror("get usart attr fail");
        return 1;
    }

    //memset(&newAttr,0,sizeof(newAttr));
    cfsetispeed(&newAttr,B115200);
    cfsetospeed(&newAttr,B115200);

    newAttr.c_cflag |= CLOCAL | CREAD;
    newAttr.c_cflag &= ~CRTSCTS;
    newAttr.c_cflag &= ~CSIZE;
    newAttr.c_cflag |= CS8;
    newAttr.c_cflag &= ~PARENB;
    newAttr.c_iflag &= ~(INPCK | ICRNL | IGNCR);
    newAttr.c_cflag &= ~CSTOPB;
    newAttr.c_cc[VMIN] = 0;
    newAttr.c_cc[VTIME] = 0;
    newAttr.c_oflag &= ~OPOST;
    newAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    tcflush(fd,TCIFLUSH);

    if(tcsetattr(fd,TCSANOW,&newAttr) == -1)
    {
        perror("set usart attr fail");
        return 1;
    }

    tty_fd = fd;
    return 0;
}

void listener_usart_data(void (*call)(char* data,size_t len))
{

}


void *thread_exeute(void *args)
{

    int ret = 0;
    char buff[100];
    ssize_t len = 0;
    ssize_t readlen = 0;
    ssize_t dataLen = 0;
    struct timeval time;
    fd_set read_set;
    memset(buff, 0, sizeof(buff));
    time.tv_sec = 1;
    time.tv_usec = 0;

    while(is_run)
    {
        if(is_usart_Connect)
        {
            time.tv_sec = 1;
            time.tv_usec = 0;
            FD_ZERO(&read_set);
            FD_SET(tty_fd,&read_set);
            ret = select(tty_fd+1,&read_set,NULL,NULL,&time);

            if(ret < 0)
            {
                is_usart_Connect = 0;
            }
            else if(ret == 0){
            }
            else {

              if(!isHandler)
              {
                    readlen  = 0;
                    dataLen  = 0;
                    len = 0;
                    while(readlen < 2)
                    {
                        len = read(tty_fd, buff+readlen, 1);
                        if(len <= 0 )break;

                        if(*buff == 0x3b)
                        {
                            readlen+=len;
                        }
                    }

                    if(*buff == 0x3b && readlen >= 2)
                    {
                        dataLen = *(buff+1);
                        if(dataLen > 0)
                        {
                            while(readlen < dataLen)
                            {
                                if(readlen >= sizeof(buff)) break;
                                len = read(tty_fd, buff+readlen, dataLen);
                                if(len == 0)
                                {
                                    break;
                                }else if(len < 0)
                                {
                                    is_usart_Connect = 0;
                                    break;
                                }

                                readlen+=len;
                            }

                            if(readlen == dataLen)
                            {
                                isHandler = 1;

                            }
                        }
                    }

                    memset(buff, 0, sizeof(buff));
               }
            }
        } else
        {
            sleep(10);
            try_open_usart();
        }

    }

    return NULL;
}

int read_thread()
{
    pthread_t  pt;
    if(read_thread_run == 1) return 1;

    read_thread_run = 1;
    if(pthread_create(&pt,NULL,thread_exeute,(void *)NULL) == -1)
    {
        perror("start read_thread fail");
        return -1;
    }

    return 0;
}

void *thread_cmd_ack(void *args)
{
    while(is_run)
    {
#if 1
        if(send_cmd_ack)
        {
            usart_send_data(ack_data,ack_data_len);
        } else
        {
            if(NULL != ack_data)
            {
                free(ack_data);
                ack_data = NULL;
            }

        }
#endif
        usleep(800 * 1000);

        if(isSendDeviceInit == 0)
        {
            isSendDeviceInit = 1;
        }
    }

    return NULL;
}

int cmd_ack_thread()
{
    pthread_t  pt;
    if(cmdack_thread_run == 1) return 1;

    cmdack_thread_run = 1;
    if(pthread_create(&pt,NULL,thread_cmd_ack,(void *)NULL) == -1)
    {
        perror("start thread_cmd_ack fail");
        return -1;
    }

    return 0;
}

void clear_cmd_ack(void)
{
    send_cmd_ack = 0;
}

void parser_data(u8 *data,size_t len)
{
    u8 check = *(data+3);

}

void try_cmd_data(u8 *data,size_t len)
{
    usart_send_data(data,len);
    ack_data = (u8 *)malloc(sizeof(u8)*len);
    memcpy(ack_data,data,len);
    ack_data_len = len;
    send_cmd_ack = 1;
}

void usart_send_data(u8 *data,size_t len)
{
    fd_set wfds;
    send_temp_data = data;
    send_temp_len = len;
    int ret;
    struct  timeval tv;

    if(tty_fd == -1 || is_usart_Connect == 0  ) return;
    FD_ZERO(&wfds);
    FD_SET(tty_fd,&wfds);
    tv.tv_sec = 0;
    tv.tv_usec = 10* 1000;

    ret = select(tty_fd+1,NULL,&wfds,NULL,&tv);
    if(ret < 0)
    {
        is_usart_Connect = 0;

    }else if(ret == 0)
    {
    } else
    {
        ssize_t  slen = write(tty_fd,data,len);
        if(slen == len)
        {
        }else if(slen <= 0)
        {
            is_usart_Connect = 0;
            tcflush(tty_fd,TCOFLUSH);
        }


    }

}

void close_usart()
{
    is_run = 0;
    close(tty_fd);
}
