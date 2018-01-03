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

#ifndef GLOBALS_H
#define GLOBALS_H

#include "defs.hpp"

struct GLOBAL
{

    char *videodevice;     // video device (def. /dev/video0)
    char *mode;            //YUYV default
    int width;             //frame width
    int height;            //frame height
    int format;            //v4l2 pixel format
    int fps;               //fps denominator
    int fps_num;           //fps numerator (usually 1)
    int lctl_method;       // 0 for control id loop, 1 for next_ctrl flag method
};


int initGlobals (struct GLOBAL *global);
int closeGlobals(struct GLOBAL *global);

#endif
