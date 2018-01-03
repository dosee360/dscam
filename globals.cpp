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

#include <linux/videodev2.h>
#include <cstddef>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "globals.hpp"
#include "v4l2_uvc.hpp"


int initGlobals (struct GLOBAL *global)
{
    global->videodevice = strdup("/dev/video0");
    global->fps = DEFAULT_FPS;
    global->fps_num = DEFAULT_FPS_NUM;
    global->width = DEFAULT_WIDTH;
    global->height = DEFAULT_HEIGHT;
    global->format = V4L2_PIX_FMT_YUYV; // only handle yuyv now
    global->lctl_method = LIST_CTL_METHOD_NEXT_FLAG;

    return (0);
}

int closeGlobals(struct GLOBAL *global)
{
    free(global->videodevice);
    free(global);
    global=NULL;

    return (0);
}
