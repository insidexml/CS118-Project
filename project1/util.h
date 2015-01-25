#ifndef UTIL_H
#define UTIL_H

#include <time.h>

void error(const char* s);
void reap_children(void);
void to_timestamp(time_t t, char* buf);
void now(char* buf);

#endif
