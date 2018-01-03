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

#include "cstddef"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "v4l2_uvc.hpp"
#include "globals.hpp"
#include "ms_time.hpp"
#include <termios.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;

struct GLOBAL *global = NULL;
struct vdIn *videoIn = NULL;

void consume_frame(BYTE* frame, int width, int height) {
	Mat yuv(height, width, CV_8UC2, frame);
	Mat rgb;
	cvtColor(yuv, rgb, CV_YUV2BGR_YUYV);
	imshow("preview", rgb);
	waitKey(10);
}

void *camera_loop(void *arg)
{
    bool signalquit = false;
	BYTE* frame;

	namedWindow("preview", CV_WINDOW_NORMAL);

    while (!signalquit)
    {
        signalquit = videoIn->signalquit;

        /*-------------------------- Grab Frame ----------------------------------*/
        if (uvc_grab(videoIn, global, frame) < 0)
        {
            printf("Error grabbing image \n");
            continue;
        }
        else
        {
			// if consume codes take too much time, consider putting following codes to another thread
			consume_frame(frame, global->width, global->height);
			free(frame);
        }

    }

    return ((void *) 0);
}

void
clean_struct () 
{
    if(global) closeGlobals(global);
    global = NULL;

    if(videoIn) close_videoIn(videoIn);
	videoIn = NULL;

    printf("cleaned allocations - 100%%\n");
}

void
init_struct () 
{
    int ret = 0;

    global = (GLOBAL *)calloc(1, sizeof(struct GLOBAL));
    videoIn = (vdIn *)calloc(1, sizeof(struct vdIn));

	// setting params here
    initGlobals(global);

    if ( ( ret=init_videoIn (videoIn, global) ) != 0)
    {
        printf("Init video returned %i\n",ret);
        switch (ret)
        {
            case VDIN_DEVICE_ERR://can't open device
                printf("Error: Unable to open device\n");
                break;

            case VDIN_DYNCTRL_ERR: //uvc extension controls error - EACCES (needs root user)
                printf("Error: UVC Extension controls\n");
                break;

            case VDIN_FORMAT_ERR:
            case VDIN_RESOL_ERR:
				printf("Error: Format or resolution failed\n");
                break;

            case VDIN_QUERYCAP_ERR:
                printf("Error: Couldn't query device capabilities\n");
                break;
            case VDIN_READ_ERR:
                printf("Error: Read method error\n");
                break;

            case VDIN_REQBUFS_ERR:/*unable to allocate dequeue buffers or mem*/
            case VDIN_ALLOC_ERR:
            case VDIN_FBALLOC_ERR:
                printf("Error: Unable to allocate Buffers\n");
				break;

            default:
				printf("Error: Unknow error\n");
                break;
        }
		clean_struct();
		exit(0);
    }
}

static struct termios stored_settings;

static void set_keypress()
{
	struct termios new_settings;
	tcgetattr(0, &stored_settings);
	new_settings = stored_settings;

	new_settings.c_lflag &= (~ICANON);
	new_settings.c_lflag &= (~ECHO);
	new_settings.c_cc[VTIME] = 0;
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);
	return;
}

static void reset_keypress()                                
{
	tcsetattr(0, TCSANOW, &stored_settings);
	return;
}


int main(int argc, char *argv[])
{
    init_struct();

	__THREAD_TYPE video_thread;
    if( __THREAD_CREATE(&video_thread, camera_loop, NULL))
    {
        printf("Video thread creation failed\n");
		return -1;
    }

	set_keypress();
	while (1) {
		char c = getchar();
		switch(c) {
		case 'q':
			videoIn->signalquit = 1;			
			__THREAD_JOIN(video_thread);
			clean_struct();
			reset_keypress();
			return 0;

		case 'j':
			exposure_control(videoIn, IOCTL_DIRECT_DEC);
			break;

		case 'u':
			exposure_control(videoIn, IOCTL_DIRECT_INC);
			break;

		case 'k':
			gain_control(videoIn, IOCTL_DIRECT_DEC);
			break;

		case 'i':
			gain_control(videoIn, IOCTL_DIRECT_INC);
			break;
		}
	}

	return 0;
}

