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

#ifndef MS_TIME_H
#define MS_TIME_H

#include "defs.hpp"

#ifndef G_NSEC_PER_SEC
#define G_NSEC_PER_SEC 1000000000LL
#endif

/*time in miliseconds*/
DWORD ms_time (void);
/*time in microseconds*/
ULLONG us_time(void);
/*time in nanoseconds (real time for benchmark)*/
ULLONG ns_time (void);
/*MONOTONIC CLOCK in nano sec for time stamps*/
UINT64 ns_time_monotonic();

/*sleep for given time in ms*/
void sleep_ms(int ms_time);

/*wait on cond by sleeping for n_loops of sleep_ms ms */
/*(test (var == val) every loop)                      */
int wait_ms(bool* var, bool val, __MUTEX_TYPE *mutex, int ms_time, int n_loops);
#endif

