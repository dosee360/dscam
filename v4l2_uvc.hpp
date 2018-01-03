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

#ifndef V4L2UVC_H
#define V4L2UVC_H

#include <linux/videodev2.h>
#include <pthread.h>
#include "defs.hpp"
#include "v4l2_format.hpp"
#include "v4l2_controls.hpp"

#define LIST_CTL_METHOD_LOOP 0
#define LIST_CTL_METHOD_NEXT_FLAG  1

#define NB_BUFFER 4

#define VDIN_DYNCTRL_OK            3
#define VDIN_SELETIMEOUT_ERR       2
#define VDIN_SELEFAIL_ERR          1
#define VDIN_OK                    0
#define VDIN_DEVICE_ERR           -1
#define VDIN_FORMAT_ERR           -2
#define VDIN_REQBUFS_ERR          -3
#define VDIN_ALLOC_ERR            -4
#define VDIN_RESOL_ERR            -5
#define VDIN_FBALLOC_ERR          -6
#define VDIN_UNKNOWN_ERR          -7
#define VDIN_DEQBUFS_ERR          -8
#define VDIN_DECODE_ERR           -9
#define VDIN_QUERYCAP_ERR        -10
#define VDIN_QUERYBUF_ERR        -11
#define VDIN_QBUF_ERR            -12
#define VDIN_MMAP_ERR            -13
#define VDIN_READ_ERR            -14
#define VDIN_STREAMON_ERR        -15
#define VDIN_STREAMOFF_ERR       -16
#define VDIN_DYNCTRL_ERR         -17

//set ioctl retries to 4 - linux uvc as increased timeout from 1000 to 3000 ms
#define IOCTL_RETRY 4

//Control direct
#define IOCTL_DIRECT_INC          1
#define IOCTL_DIRECT_DEC          -1

struct vdIn
{
    int fd;                             // device file descriptor
    char *videodevice;                  // video device string (default "/dev/video0)"

    struct v4l2_capability cap;         // v4l2 capability struct
    struct v4l2_format fmt;             // v4l2 formar struct
    struct v4l2_buffer buf;             // v4l2 buffer struct
    struct v4l2_requestbuffers rb;      // v4l2 request buffers struct
    struct v4l2_streamparm streamparm;  // v4l2 stream parameters struct
	
    void *mem[NB_BUFFER];               // memory buffers for mmap driver frames
    uint32_t buff_length[NB_BUFFER];    // memory buffers length as set by VIDIOC_QUERYBUF
    uint32_t buff_offset[NB_BUFFER];    // memory buffers offset as set by VIDIOC_QUERYBUF

    int isstreaming;                    // video stream flag (1- ON  0- OFF)
    UINT64 timestamp;                   // video frame time stamp
    int signalquit;                     // video loop exit flag
    uint64_t frame_index;               // captured frame index
    LFormats *listFormats;              // structure with frame formats list

	struct VidState *s;
	int exposure_id;
	int gain_id;
    int maxExposureTime;
    int minExposureTime;
    int currExposureTime;
    int maxGainValue;
    int minGainValue;
    int currGainValue;

};

int init_videoIn(struct vdIn *videoIn, struct GLOBAL *global);

int uvc_grab(struct vdIn *vd, struct GLOBAL *global, BYTE*& new_frame);

void close_videoIn(struct vdIn *videoIn);

int xioctl(int fd, int IOCTL_X, void *arg);

int input_set_framerate (struct vdIn * device, int *fps, int *fps_num);

int input_get_framerate (struct vdIn * device, int *fps, int *fps_num);

int exposure_control(struct vdIn *vd, int direct);

int gain_control(struct vdIn *vd, int direct);

#endif
