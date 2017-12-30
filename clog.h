//
// Created by blackcheer on 17-12-30.
//

#ifndef LISO_CLOG_H
#define LISO_CLOG_H

#include <time.h>
#include <stdio.h>
#include <stdbool.h>

bool clog_initialize(char *file_name);
bool clog_print(char *str, int len);
void clog_error(char *str);
bool clog_print_f(char *format, ...);
bool clog_close();

#endif //LISO_CLOG_H
