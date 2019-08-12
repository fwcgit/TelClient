//
//  logfile.h
//  FLogs
//
//  Created by fwc on 2018/7/24.
//  Copyright © 2018年 fwc. All rights reserved.
//

#ifndef logfile_h
#define logfile_h

#include <stdio.h>

#define SAVE_FILE_TIME 7 * 24 * 60 * 60

int create_log_dir(char *path);

int dir_exist(char *path);

void check_dir(char *path);

void del_find_histroy_file(char *path);

void find_file(char *path);

#endif /* logfile_h */
