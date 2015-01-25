#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/*
Convenience error+exit function
*/
void error(const char* s)
{
    fprintf(stderr, "ERROR: %s\n", s);
    exit(1);
}

/*
SIGCHLD handler to reap zombified child processes
*/
void sigchld_handler(int s)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/*
A special set-up function that sets a SIGCHLD handler to reap zombified
child processes
*/
void reap_children(void)
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
        error("sigaction");
}

/*
Given a time structure, convert it into a pretty timestamp format and store
it in buf
*/
void to_timestamp(time_t t, char* buf)
{
    const char* DAY[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* MONTH[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
                           "Aug", "Sep", "Oct", "Nov", "Dec"};
    struct tm *utm = gmtime(&t);
    sprintf(buf, "%s, %d %s %d %02d:%02d:%02d GMT",
            DAY[utm->tm_wday], utm->tm_mday, MONTH[utm->tm_mon],
            utm->tm_year+1900, utm->tm_hour, utm->tm_min,
            utm->tm_sec);
}

/*
Calls to_timestamp with the current time
*/
void now(char* buf)
{
    to_timestamp(time(NULL), buf);
}
