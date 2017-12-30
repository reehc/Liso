#include "clog.h"

#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

int clog_file;

bool clog_initialize(char *file_name) {
    clog_file = open(file_name, O_APPEND | O_CREAT);
    if (clog_file > 0) return true;
    return false;
}

bool clog_info() {
    // TODO
    // [TIME][ADDR:PORT][REQ-METHOD][REQ-URI][REQ-VERSION][SIZE]
}

bool clog_print_f(char *format, ...) {
    char str[8192] = {0};
    va_list args;
    va_start(args, format);
    vsprintf(str, format, args);
    write(clog_file, str, strlen(str));
    va_end(args);
    return true;
}

bool clog_close() {
    if (close(clog_file) == -1) return false;
    return true;
}