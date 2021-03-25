#include "Common.h"
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

int64_t get_cur_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t ts = (int64_t)tv.tv_sec*1000 + tv.tv_usec/1000;
    return ts;
}