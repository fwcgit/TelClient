//
//  logs.c
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#include "logs.h"
#include <time.h>
#include <string.h>

char global_log_path[100];

void write_logs(char *content)
{
    char fileName[100];
    char dirPath[100];

    time_t tv;
    struct tm *timeInfo;
    FILE *file;
    time(&tv);
    timeInfo = localtime(&tv);
    strftime(fileName, sizeof(fileName), "%Y-%m-%d-%H", timeInfo);
    
    strcpy(dirPath, global_log_path);
    strcat(fileName, ".txt");
    strcat(dirPath, fileName);
    
    printf("file path %s\n",dirPath);
    
    file = fopen(dirPath, "a");

    if(NULL != file)
    {
        fputs(content, file);
        fputs("\r\n", file);
        fclose(file);
    }
}

void printf_logs(char *log,...)
{
    
}
