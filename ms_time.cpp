/*
 *  Copyright (c) 2018 DoSee Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>

#include "ms_time.hpp"

/*------------------------------ get time ------------------------------------*/
/*in miliseconds*/
DWORD ms_time (void)
{
    struct timeval *tod;
    tod = (struct timeval *)malloc( 1 * sizeof(struct timeval));
    gettimeofday(tod, NULL);
    DWORD mst = (DWORD) tod->tv_sec * 1000 + (DWORD) tod->tv_usec / 1000;
    free(tod);
    return (mst);
}
/*in microseconds*/
ULLONG us_time(void)
{
    struct timeval *tod;
    tod = (struct timeval *)malloc( 1 * sizeof(struct timeval));
    gettimeofday(tod, NULL);
    ULLONG ust = (DWORD) tod->tv_sec * 1000000 + (DWORD) tod->tv_usec;
    free(tod);
    return (ust);
}

/*REAL TIME CLOCK*/
/*in nanoseconds*/
ULLONG ns_time (void)
{
    static struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ((ULLONG) ts.tv_sec * G_NSEC_PER_SEC + (ULLONG) ts.tv_nsec);
}

/*MONOTONIC CLOCK*/
/*in nanosec*/
UINT64 ns_time_monotonic()
{
    static struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((UINT64) ts.tv_sec * G_NSEC_PER_SEC + (ULLONG) ts.tv_nsec);
}

//sleep for given time in ms
void sleep_ms(int ms_time)
{
    ulong sleep_us = ms_time *1000; /*convert to microseconds*/
    usleep( sleep_us );/*sleep for sleep_ms ms*/
}

/*wait on cond by sleeping for n_loops of sleep_ms ms (test var==val every loop)*/
/*return remaining number of loops (if 0 then a stall occurred)              */
int wait_ms(bool* var, bool val, __MUTEX_TYPE *mutex, int ms_time, int n_loops)
{
    int n=n_loops;
    __LOCK_MUTEX(mutex);
    while( (*var!=val) && ( n > 0 ) ) /*wait at max (n_loops*sleep_ms) ms */
    {
        __UNLOCK_MUTEX(mutex);
        n--;
        sleep_ms( ms_time );/*sleep for sleep_ms ms*/
        __LOCK_MUTEX(mutex);
    };
    __UNLOCK_MUTEX(mutex);
    return (n);
}

