//
//  logfile.c
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#include "logfile.h"
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

int create_log_dir(char *path)
{
    int ret;
    
    if(dir_exist(path) != 0)
    {
        if(mkdir(path, S_IRWXU | S_IRWXG | S_IROTH) == 0)
        {
            ret = 0;
        }
        else
        {
            ret = errno;
            
        }
    }
    else{
        ret = -1;
    }
    
    printf("create dir result : %d \n",ret);

    return ret;
}

int dir_exist(char *path)
{
    int ret;
    DIR *dir;

    dir = opendir(path);
    if(NULL == dir)
    {
        ret = -1;
    }
    else
    {
        closedir(dir);
        ret = 0;
    }
    
    
    return ret;
}

void del_find_histroy_file(char *path)
{
    char pathName[200];
    
    strcpy(pathName, path);
    
    if(dir_exist(pathName) == 0)
    {
        find_file(pathName);
    }
}

void find_file(char *path)
{
    DIR *dir;
    struct dirent *dt;
    struct stat fileAttr;
    time_t fileTime;
    time_t nowTime;
    struct tm *timeInfo;
    char formatDate[100];
    

    dir = opendir(path);
    if(NULL != dir)
    {
        chdir(path);
        while ((dt = readdir(dir)) != NULL)
        {
            if(strcmp(dt->d_name, ".") == 0 || strcmp(dt->d_name, "..") == 0)
            {
                continue;
            }
            
            lstat(dt->d_name, &fileAttr);
            if(S_IFDIR & fileAttr.st_mode)
            {
                printf("dir name %s \n",dt->d_name);
                find_file(dt->d_name);
            }
            else
            {
                
                fileTime = fileAttr.st_ctime;
                timeInfo = localtime(&fileTime);
                strftime(formatDate, sizeof(formatDate), "%Y-%m-%d %H:%H:%S", timeInfo);
                
                time(&nowTime);
                if((nowTime - fileAttr.st_ctime) > SAVE_FILE_TIME)
                {
                    remove(dt->d_name);
                    printf("del file name %s \n",dt->d_name);
                }
                
                printf("----------------\n");
                printf("file name %s \n",dt->d_name);
                printf("file create time %s \n",formatDate);
                printf("----------------\n");
            }
        }
        chdir("..");
        closedir(dir);
    }
    
}

void check_dir(char *path)
{
    create_log_dir(path);
    
    del_find_histroy_file(path);
}

