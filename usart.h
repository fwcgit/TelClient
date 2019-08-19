//
// Created by fwc on 2018/6/20.
//

#ifndef usart_h
#define usart_h
#include <stdio.h>

typedef unsigned char u8;

int     open_usart(const char *ttys);
void    usart_send_data(u8 *data,size_t len);
void    usart_parser_data(u8 *data,size_t len);
void    close_usart();

#endif 
