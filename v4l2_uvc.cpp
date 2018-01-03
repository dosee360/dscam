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
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <libv4l2.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>

#include "v4l2_uvc.hpp"
#include "globals.hpp"
#include "v4l2_format.hpp"
#include "ms_time.hpp"


/* ioctl with a number of retries in the case of failure
* args:
* fd - device descriptor
* IOCTL_X - ioctl reference
* arg - pointer to ioctl data
* returns - ioctl result
*/
int xioctl(int fd, int IOCTL_X, void *arg)
{
    int ret = 0;
    int tries= IOCTL_RETRY;
    do
    {
        //Start of e-con
        //		ret = v4l2_ioctl(fd, IOCTL_X, arg);
        ret = ioctl(fd, IOCTL_X, arg);
        //End of e-con
    }
    while (ret && tries-- &&
            ((errno == EINTR) || (errno == EAGAIN) || (errno == ETIMEDOUT)));

    if (ret && (tries <= 0)) printf("ioctl (%i) retried %i times - giving up: %s)\n", IOCTL_X, IOCTL_RETRY, strerror(errno));

    return (ret);
}

/* Query video device capabilities and supported formats
 * args:
 * vd: pointer to a VdIn struct ( must be allready allocated )
 *
 * returns: error code  (0- OK)
 */
static int check_videoIn(struct vdIn *vd, struct GLOBAL *global)
{
    int ret = 0;

    if (vd == NULL)
        return VDIN_ALLOC_ERR;

    memset(&vd->cap, 0, sizeof(struct v4l2_capability));

    if ( xioctl(vd->fd, VIDIOC_QUERYCAP, &vd->cap) < 0 )
    {
        printf("VIDIOC_QUERYCAP error");
        return VDIN_QUERYCAP_ERR;
    }

    if ( ( vd->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE ) == 0)
    {
        printf("Error opening device %s: video capture not supported.\n",
                vd->videodevice);
        return VDIN_QUERYCAP_ERR;
    }
    if (!(vd->cap.capabilities & V4L2_CAP_STREAMING))
    {
        printf("%s does not support streaming i/o\n",
                vd->videodevice);
        return VDIN_QUERYCAP_ERR;
    }

    printf("Init. %s (location: %s)\n", vd->cap.card, vd->cap.bus_info);

    vd->listFormats = enum_frame_formats( &global->width, &global->height, vd->fd);

    if(!(vd->listFormats->listVidFormats))
        printf("Couldn't detect any supported formats on your device (%i)\n", vd->listFormats->numb_formats);

    return VDIN_OK;
}

static int unmap_buff(struct vdIn *vd)
{
    int i=0;
    int ret=0;

    for (i = 0; i < NB_BUFFER; i++)
    {
        // unmap old buffer
        if((vd->mem[i] != MAP_FAILED) && vd->buff_length[i])
            if((ret=v4l2_munmap(vd->mem[i], vd->buff_length[i]))<0)
            {
                printf("couldn't unmap buff");
            }
    }
    return ret;
}

static int map_buff(struct vdIn *vd)
{
    int i = 0;
    // map new buffer
    for (i = 0; i < NB_BUFFER; i++)
    {
        vd->mem[i] = v4l2_mmap( NULL, // start anywhere
                vd->buff_length[i],
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                vd->fd,
                vd->buff_offset[i]);
        if (vd->mem[i] == MAP_FAILED)
        {
            printf("Unable to map buffer");
            return VDIN_MMAP_ERR;
        }
    }

    return (0);
}

/* Query and map buffers
 * args:
 * vd: pointer to a VdIn struct ( must be allready allocated )
 *
 * returns: error code  (0- OK)
 */
static int query_buff(struct vdIn *vd)
{
    int i=0;
    int ret=0;

    for (i = 0; i < NB_BUFFER; i++)
    {
        memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
        vd->buf.index = i;
        vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //vd->buf.flags = V4L2_BUF_FLAG_TIMECODE;
        //vd->buf.timecode = vd->timecode;
        //vd->buf.timestamp.tv_sec = 0;//get frame as soon as possible
        //vd->buf.timestamp.tv_usec = 0;
        vd->buf.memory = V4L2_MEMORY_MMAP;
        ret = xioctl(vd->fd, VIDIOC_QUERYBUF, &vd->buf);
        if (ret < 0)
        {
            printf("VIDIOC_QUERYBUF - Unable to query buffer");
            if(errno == EINVAL)
            {
                printf("trying with read method instead\n");
            }
            return VDIN_QUERYBUF_ERR;
        }
        if (vd->buf.length <= 0)
            printf("WARNING VIDIOC_QUERYBUF - buffer length is %d\n",
                    vd->buf.length);

        vd->buff_length[i] = vd->buf.length;
        vd->buff_offset[i] = vd->buf.m.offset;
    }
    // map the new buffers
    if(map_buff(vd) != 0)
        return VDIN_MMAP_ERR;
    return VDIN_OK;
}


/* Queue Buffers
 * args:
 * vd: pointer to a VdIn struct ( must be allready allocated )
 *
 * returns: error code  (0- OK)
 */
static int queue_buff(struct vdIn *vd)
{
    int i=0;
    int ret=0;

    for (i = 0; i < NB_BUFFER; ++i)
    {
        memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
        vd->buf.index = i;
        vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //vd->buf.flags = V4L2_BUF_FLAG_TIMECODE;
        //vd->buf.timecode = vd->timecode;
        //vd->buf.timestamp.tv_sec = 0;//get frame as soon as possible
        //vd->buf.timestamp.tv_usec = 0;
        vd->buf.memory = V4L2_MEMORY_MMAP;
        ret = xioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
        if (ret < 0)
        {
            printf("VIDIOC_QBUF - Unable to queue buffer");
            return VDIN_QBUF_ERR;
        }
    }
    vd->buf.index = 0; /*reset index*/

    return VDIN_OK;
}


/* Enable video stream
 * args:
 * vd: pointer to a VdIn struct ( must be allready initiated)
 *
 * returns: VIDIOC_STREAMON ioctl result (0- OK)
 */
int video_enable(struct vdIn *vd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret=0;

    ret = xioctl(vd->fd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        printf("VIDIOC_STREAMON - Unable to start capture");
        return VDIN_STREAMON_ERR;
    }

    vd->isstreaming = 1;
    return 0;
}

/* Disable video stream
 * args:
 * vd: pointer to a VdIn struct ( must be allready initiated)
 *
 * returns: VIDIOC_STREAMOFF ioctl result (0- OK)
 */
int video_disable(struct vdIn *vd)
{
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret=0;

    ret = xioctl(vd->fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0)
    {
        printf("VIDIOC_STREAMOFF - Unable to stop capture");
        if(errno == 9) vd->isstreaming = 0;/*capture as allready stoped*/
        return VDIN_STREAMOFF_ERR;
    }

    vd->isstreaming = 0;
    return 0;
}

/* Try/Set device video stream format
 * args:
 * vd: pointer to a VdIn struct ( must be allready allocated )
 *
 * returns: error code ( 0 - VDIN_OK)
 */
static int init_v4l2(struct vdIn *vd, struct GLOBAL *global)
{
    int ret = 0;

    // make sure we set a valid format
    printf("checking format: %c%c%c%c\n",
            (global->format) & 0xFF, ((global->format) >> 8) & 0xFF,
            ((global->format) >> 16) & 0xFF, ((global->format) >> 24) & 0xFF);

    if ((ret=check_supPixFormat(global->format)) < 0)
    {
        printf("Format unavailable: %c%c%c%c\n",
                (global->format) & 0xFF, ((global->format) >> 8) & 0xFF,
                ((global->format) >> 16) & 0xFF, ((global->format) >> 24) & 0xFF);
        return VDIN_FORMAT_ERR;
    }

    // set format
    vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->fmt.fmt.pix.width = global->width;
    vd->fmt.fmt.pix.height = global->height;
    vd->fmt.fmt.pix.pixelformat = global->format;
    vd->fmt.fmt.pix.field = V4L2_FIELD_ANY;

    ret = xioctl(vd->fd, VIDIOC_S_FMT, &vd->fmt);
    if (ret < 0)
    {
        printf("VIDIOC_S_FORMAT - Unable to set format");
        return VDIN_FORMAT_ERR;
    }
    if ((vd->fmt.fmt.pix.width != global->width) ||
            (vd->fmt.fmt.pix.height != global->height))
    {
        printf("Requested Format require width %d height %d \n",
                vd->fmt.fmt.pix.width, vd->fmt.fmt.pix.height);
        global->width = vd->fmt.fmt.pix.width;
        global->height = vd->fmt.fmt.pix.height;
    }
    vd->listFormats->current_format = get_formatIndex(vd->listFormats, global->format);
    if(vd->listFormats->current_format < 0)
    {
		return VDIN_FORMAT_ERR;
    }

    /* ----------- FPS --------------*/
    input_set_framerate(vd, &global->fps, &global->fps_num);

    // request buffers
    memset(&vd->rb, 0, sizeof(struct v4l2_requestbuffers));
    vd->rb.count = NB_BUFFER;
    vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->rb.memory = V4L2_MEMORY_MMAP;

    ret = xioctl(vd->fd, VIDIOC_REQBUFS, &vd->rb);
    if (ret < 0)
    {
        perror("VIDIOC_REQBUFS - Unable to allocate buffers");
        return VDIN_REQBUFS_ERR;
    }
    // map the buffers
    if (query_buff(vd))
    {
        //delete requested buffers
        //no need to unmap as mmap failed for sure
        memset(&vd->rb, 0, sizeof(struct v4l2_requestbuffers));
        vd->rb.count = 0;
        vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd->rb.memory = V4L2_MEMORY_MMAP;
        if(xioctl(vd->fd, VIDIOC_REQBUFS, &vd->rb)<0)
            perror("VIDIOC_REQBUFS - Unable to delete buffers");
        return VDIN_QUERYBUF_ERR;
    }
    // Queue the buffers
    if (queue_buff(vd))
    {
        //delete requested buffers
        unmap_buff(vd);
        memset(&vd->rb, 0, sizeof(struct v4l2_requestbuffers));
        vd->rb.count = 0;
        vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        vd->rb.memory = V4L2_MEMORY_MMAP;
        if(xioctl(vd->fd, VIDIOC_REQBUFS, &vd->rb)<0)
            perror("VIDIOC_REQBUFS - Unable to delete buffers");
        return VDIN_QBUF_ERR;
    }

    return VDIN_OK;
}

/* sets video device frame rate
 * args:
 * vd: pointer to a VdIn struct ( must be allready initiated)
 *
 * returns: VIDIOC_S_PARM ioctl result value
 */
int
input_set_framerate (struct vdIn * device, int *fps, int *fps_num)
{
    int fd;
    int ret=0;

    fd = device->fd;

    device->streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(fd, VIDIOC_G_PARM, &device->streamparm);
    if (ret < 0)
        return ret;

    if (!(device->streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME))
        return -ENOTSUP;

    device->streamparm.parm.capture.timeperframe.numerator = *fps_num;
    device->streamparm.parm.capture.timeperframe.denominator = *fps;

    ret = xioctl(fd,VIDIOC_S_PARM, &device->streamparm);
    if (ret < 0)
    {
        printf("Unable to set %d/%d fps\n", *fps_num, *fps);
        printf("VIDIOC_S_PARM error");
    }

    /*make sure we now have the correct fps*/
    input_get_framerate (device, fps, fps_num);

    return ret;
}

/* gets video device defined frame rate (not real - consider it a maximum value)
 * args:
 * vd: pointer to a VdIn struct ( must be allready initiated)
 *
 * returns: VIDIOC_G_PARM ioctl result value
 */
int
input_get_framerate (struct vdIn * device, int *fps, int *fps_num)
{
    int fd;
    int ret=0;

    fd = device->fd;

    device->streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(fd,VIDIOC_G_PARM, &device->streamparm);
    if (ret < 0)
    {
        printf("VIDIOC_G_PARM - Unable to get timeperframe");
    }
    else
    {
        if (device->streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME) {
            // it seems numerator is allways 1 but we don't do assumptions here :-)
            *fps = device->streamparm.parm.capture.timeperframe.denominator;
            *fps_num = device->streamparm.parm.capture.timeperframe.numerator;
        }
    }

    if(*fps == 0 )
        *fps = 1;
    if(*fps_num == 0)
        *fps_num = 1;

    return ret;
}

void init_controls (struct vdIn *vd, struct GLOBAL *global)
{
    struct VidState *s = NULL;
    Control *current;
    int i = 0;

	s = (VidState *)calloc(1, sizeof(struct VidState));
	if (s)
	{
		vd->s = s;
		printf("%s: Success to create vid state\n", vd->videodevice);
	}

    if (s->control_list)
    {
        free_control_list(s->control_list);
    }

    s->num_controls = 0;
    //get the control list
    s->control_list = get_control_list(vd->fd, &(s->num_controls), global->lctl_method);

    if(!s->control_list)
    {
        printf("Error: empty control list\n");
        return;
    }

    get_ctrl_values (vd->fd, s->control_list, s->num_controls, NULL);

    for(current = s->control_list; current != NULL; current = current->next)
    {
        get_ctrl(vd->fd, s->control_list, current->control.id);
        if (!strcmp((const char *)current->control.name, "Exposure (Absolute)"))
        {
            vd->exposure_id = current->control.id;

        }
        if (!strcmp((const char *)current->control.name, "Gain"))
        {
            vd->gain_id = current->control.id;
        }

        print_control(current, i);
        i++;
    }

    if (!vd->exposure_id) 
    {
        vd->exposure_id = current[3].control.id;
    }
    if (!vd->gain_id)
    {
        vd->gain_id = current[1].control.id;
    }
}

void close_controls(struct vdIn *vd)
{
    struct VidState *s = vd->s;

    if (s->control_list)
    {
        free_control_list (s->control_list);
        s->control_list = NULL;
    }
    free(s);
    s = NULL;
    vd->s = NULL;
}


void clear_v4l2(struct vdIn *vd)
{
    v4l2_close(vd->fd);
    vd->fd = 0;
    free(vd->videodevice);
    vd->videodevice = NULL;
}

/* Init VdIn struct with default and/or global values
 * args:
 * vd: pointer to a VdIn struct ( must be allready allocated )
 * global: pointer to a GLOBAL struct ( must be allready initiated )
 *
 * returns: error code ( 0 - VDIN_OK)
 */
int init_videoIn(struct vdIn *vd, struct GLOBAL *global)
{
    int ret = VDIN_OK;
    char *device = global->videodevice;

    if (vd == NULL || device == NULL)
        return VDIN_ALLOC_ERR;
    if (global->width == 0 || global->height == 0)
        return VDIN_RESOL_ERR;

    vd->videodevice = NULL;
    vd->videodevice = strdup(device);
    printf("video device: %s \n", vd->videodevice);

	//open device
	if (vd->fd <=0 )
    {
        if ((vd->fd = v4l2_open(vd->videodevice, O_RDWR | O_NONBLOCK, 0)) < 0)
        {
            printf("ERROR opening V4L interface:%s\n", vd->videodevice);
            ret = VDIN_DEVICE_ERR;
            clear_v4l2(vd);
            return (ret);
        }
    }

	// query device info
    memset(&vd->fmt, 0, sizeof(struct v4l2_format));
    if((ret = check_videoIn(vd, global)) != VDIN_OK)
    {
        clear_v4l2(vd);
        return (ret);
    }

	// setting
    if ((ret=init_v4l2(vd, global)) < 0)
    {
        printf("Init v4L2 failed !! \n");
        clear_v4l2(vd);
        return (ret);
    }
    printf("fps is set to %i/%i\n", global->fps_num, global->fps);

    // init controls
    init_controls(vd, global);

    return (ret);
}

static int close_v4l2_buffers (struct vdIn *vd)
{
    //delete requested buffers
    unmap_buff(vd);
    memset(&vd->rb, 0, sizeof(struct v4l2_requestbuffers));
    vd->rb.count = 0;
    vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->rb.memory = V4L2_MEMORY_MMAP;
    if(xioctl(vd->fd, VIDIOC_REQBUFS, &vd->rb)<0)
    {
        printf("VIDIOC_REQBUFS - Failed to delete buffers: %s (errno %d)\n", strerror(errno), errno);
        return(VDIN_REQBUFS_ERR);
    }

    return (VDIN_OK);
}


/* cleans VdIn struct and allocations
 * args:
 * pointer to initiated vdIn struct
 *
 * returns: void
 */
void close_videoIn(struct vdIn *vd)
{
    if (vd->isstreaming) video_disable(vd);

    if(vd->videodevice) free(vd->videodevice);
    // free format allocations
    if(vd->listFormats) free_formats(vd->listFormats);
    close_v4l2_buffers(vd);

    // close controls
    close_controls(vd);

    vd->videodevice = NULL;
    // close device descriptorF
    if(vd->fd) v4l2_close(vd->fd);
    // free struct allocation
    if(vd) free(vd);
    vd = NULL;
}

static int check_frame_available(struct vdIn *vd)
{
    int ret = VDIN_OK;
    fd_set rdset;
    struct timeval timeout;
    //make sure streaming is on
    if (!vd->isstreaming)
        if (video_enable(vd))
        {
            vd->signalquit = true;
            return VDIN_STREAMON_ERR;
        }

    FD_ZERO(&rdset);
    FD_SET(vd->fd, &rdset);
    timeout.tv_sec = 6; // 1 sec timeout
    timeout.tv_usec = 0;
    // select - wait for data or timeout
    ret = select(vd->fd + 1, &rdset, NULL, NULL, &timeout);
    if (ret < 0)
    {
        printf(" Could not grab image (select error)");
        vd->timestamp = 0;
        return VDIN_SELEFAIL_ERR;
    }
    else if (ret == 0)
    {
        printf(" Could not grab image (select timeout)");
        vd->timestamp = 0;
        return VDIN_SELETIMEOUT_ERR;
    }
    else if ((ret > 0) && (FD_ISSET(vd->fd, &rdset)))
        return VDIN_OK;
    else
        return VDIN_UNKNOWN_ERR;

}

/* Grabs video frame and store it in new_frame(free it by yourself) 
 *
 * returns: error code ( 0 - VDIN_OK)
 */
int uvc_grab(struct vdIn *vd, struct GLOBAL *global, BYTE*& new_frame)
{
    int ret = check_frame_available(vd);
	int frame_size = global->width * global->height * 2;

    if (ret < 0)
        return ret;

    // dequeue the buffer
    memset(&vd->buf, 0, sizeof(struct v4l2_buffer));
    vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->buf.memory = V4L2_MEMORY_MMAP;

    ret = xioctl(vd->fd, VIDIOC_DQBUF, &vd->buf);
    if (ret < 0)
    {
        printf("VIDIOC_DQBUF - Unable to dequeue buffer ");
        ret = VDIN_DEQBUFS_ERR;
        return ret;
    }

	// store
	new_frame = (BYTE*)malloc(frame_size);
	memcpy(new_frame, vd->mem[vd->buf.index], frame_size);

	// queue the buffer
    ret = xioctl(vd->fd, VIDIOC_QBUF, &vd->buf);
    if (ret < 0)
    {
        printf("VIDIOC_QBUF - Unable to queue buffer");
        ret = VDIN_QBUF_ERR;
        return ret;
    }

    vd->frame_index++;

    return VDIN_OK;
}

int exposure_control(struct vdIn *vd, int direct)
{
    Control *ctrl = NULL;
    ctrl = get_ctrl_by_id(vd->s->control_list, vd->exposure_id);
    if (!ctrl)
    {
        printf("%s failed for NULL!!\n", __func__);
        return -1;
    }

    if (direct == IOCTL_DIRECT_INC)
    {
        ctrl->value++;
        if (ctrl->value > ctrl->control.maximum)
            ctrl->value = ctrl->control.maximum;
        set_ctrl(vd->fd, vd->s->control_list, vd->exposure_id);
    }
    else if (direct == IOCTL_DIRECT_DEC)
    {
        ctrl->value--;
        if (ctrl->value < ctrl->control.minimum)
            ctrl->value = ctrl->control.minimum;
        set_ctrl(vd->fd, vd->s->control_list, vd->exposure_id);
    }
    else
    {
        printf("%s, error for direct!!\n", __func__);
        return -1;
    }

    vd->currExposureTime = ctrl->value;

    print_control(ctrl, 0);
    return 0;
}


int gain_control(struct vdIn *vd, int direct)
{
    Control *ctrl = NULL;
    ctrl = get_ctrl_by_id(vd->s->control_list, vd->gain_id);
    if (!ctrl)
    {
        printf("%s failed for NULL\n", __func__);
        return -1;
    }
    if (direct == IOCTL_DIRECT_INC)
    {
        ctrl->value++;
        if (ctrl->value > ctrl->control.maximum)
            ctrl->value = ctrl->control.maximum;
        set_ctrl(vd->fd, vd->s->control_list, vd->gain_id);
    }
    else if(direct == IOCTL_DIRECT_DEC)
    {
        ctrl->value--;
        if (ctrl->value < ctrl->control.minimum)
            ctrl->value = ctrl->control.minimum;
        set_ctrl(vd->fd, vd->s->control_list, vd->gain_id);
    }
    else
    {
        printf("%s, error for direct!!\n", __func__);
        return -1;
    }

    vd->currGainValue = ctrl->value;

    print_control(ctrl, 0);
    return 0;
}
