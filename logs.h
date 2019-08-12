//
//  logs.h
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#ifndef logs_h
#define logs_h

#include <stdio.h>

extern char global_log_path[100];

void write_logs(char *content);

void printf_logs(char *log,...);

#endif /* logs_h */
