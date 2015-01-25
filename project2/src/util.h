#ifndef UTIL_H
#define UTIL_H

#define tprintf(format, args...) \
    printf("%s: ", now()); \
    printf(format, ## args)

void error(const char* s);
int select_wait(int fd, int* msecs);
char* now();

#endif
