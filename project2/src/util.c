#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>
#include <time.h>
#include <sys/time.h>

void error(const char* s)
{
    fprintf(stderr, "ERROR: %s\n", s);
    exit(1);
}

int select_wait(int fd, int* msecs)
{
    fd_set active;
    FD_ZERO(&active);
    FD_SET(fd, &active);
    struct timeval timeout = {*msecs/1000, *msecs%1000 * 1000};
    int res = select(fd+1, &active, NULL, NULL, &timeout);
    if (res < 0)
        error("select");
    *msecs = timeout.tv_sec * 1000 + timeout.tv_usec / 1000;
    return res;
}

char* now()
{
    static char buf[20] = {0};

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t t = time(NULL);
    struct tm *utm = gmtime(&t);
    //sprintf(buf, "%02d:%02d:%02d.%06ld", (utm->tm_hour + 16) % 24,
    //        utm->tm_min, utm->tm_sec, tv.tv_usec);
    sprintf(buf, "%02d.%06ld", utm->tm_sec, tv.tv_usec);

    return buf;
}
