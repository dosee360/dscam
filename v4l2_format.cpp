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
#include <cstddef>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "v4l2_uvc.hpp"
#include "v4l2_format.hpp"

#define SUP_PIX_FMT 26

// list possible formats although only support yuyv now
static SupFormats listSupFormats[SUP_PIX_FMT] =
{
    {
        .format   = V4L2_PIX_FMT_H264,
        .mode     = "h264",
        .hardware = 0
            //.decoder  = decode_jpeg
    },
    {
        .format   = V4L2_PIX_FMT_MJPEG,
        .mode     = "mjpg",
        .hardware = 0
            //.decoder  = decode_jpeg
    },
    {
        .format   = V4L2_PIX_FMT_JPEG,
        .mode     ="jpeg",
        .hardware = 0
            //.decoder  = decode_jpeg
    },
    {
        .format   = V4L2_PIX_FMT_YUYV,
        .mode     = "yuyv",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_YVYU,
        .mode     = "yvyu",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_UYVY,
        .mode     = "uyvy",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_YYUV,
        .mode     = "yyuv",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_Y41P,
        .mode     = "y41p",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_GREY,
        .mode     = "grey",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_Y10BPACK,
        .mode     = "y10b",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_Y16,
        .mode     = "y16 ",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_YUV420,
        .mode     = "yu12",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_YVU420,
        .mode     = "yv12",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_NV12,
        .mode     = "nv12",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_NV21,
        .mode     = "nv21",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_NV16,
        .mode     = "nv16",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_NV61,
        .mode     = "nv61",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SPCA501,
        .mode     = "s501",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SPCA505,
        .mode     = "s505",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SPCA508,
        .mode     = "s508",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SGBRG8,
        .mode     = "gbrg",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SGRBG8,
        .mode     = "grbg",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SBGGR8,
        .mode     = "ba81",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_SRGGB8,
        .mode     = "rggb",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_RGB24,
        .mode     = "rgb3",
        .hardware = 0
    },
    {
        .format   = V4L2_PIX_FMT_BGR24,
        .mode     = "bgr3",
        .hardware = 0
    }
};


/* set hardware flag for v4l2 pix format
 * args:
 * pixfmt: V4L2 pixel format
 * return index from supported devices list
 * or -1 if not supported                    */
int set_SupPixFormat(int pixfmt)
{
    int i=0;
    for (i=0; i<SUP_PIX_FMT; i++)
    {
        if (pixfmt == listSupFormats[i].format)
        {
            listSupFormats[i].hardware = 1; /*supported by hardware*/
            return (i);
        }
    }
    return(-1); /*not supported*/
}

/* check if format is supported by hardware
 * args:
 * pixfmt: V4L2 pixel format
 * return index from supported devices list
 * or -1 if not supported                    */
int check_supPixFormat(int pixfmt)
{
    int i=0;
    for (i=0; i<SUP_PIX_FMT; i++)
    {
        if (pixfmt == listSupFormats[i].format)
        {
            if(listSupFormats[i].hardware > 0) return (i); /*supported by hardware*/
        }
    }
    return (-1);

}

/* convert v4l2 pix format to mode (Fourcc)
 * args:
 * pixfmt: V4L2 pixel format
 * mode: fourcc string (lower case)
 * returns 1 on success
 * and -1 on failure (not supported)         */
int get_pixMode(int pixfmt, char *mode)
{
    int i=0;
    for (i=0; i<SUP_PIX_FMT; i++)
    {
        if (pixfmt == listSupFormats[i].format)
        {
            snprintf(mode, 5, "%s", listSupFormats[i].mode);
            return (1);
        }
    }
    return (-1);
}

/* get Format index from available format list
 * args:
 * listFormats: available video format list
 * format: v4l2 pix format
 *
 * returns format list index */
int get_formatIndex(LFormats *listFormats, int format)
{
    int i=0;
    for(i=0;i<listFormats->numb_formats;i++)
    {
        if(format == listFormats->listVidFormats[i].format)
            return (i);
    }
    return (-1);
}


/* clean video formats list
 * args:
 * listFormats: struct containing list of available video formats
 *
 * returns: void  */
void free_formats(LFormats *listFormats)
{
    if(listFormats == NULL)
        return;

    if(listFormats->listVidFormats == NULL)
    {
        free(listFormats);
        return;
    }

    int i=0;
    int j=0;
    for(i=0;i<listFormats->numb_formats;i++)
    {
        if(listFormats->listVidFormats[i].listVidCap != NULL)
        {
            for(j=0;j<listFormats->listVidFormats[i].numb_res;j++)
            {
                //g_free should handle NULL but we check it anyway
                if(listFormats->listVidFormats[i].listVidCap[j].framerate_num != NULL)
                    free(listFormats->listVidFormats[i].listVidCap[j].framerate_num);

                if(listFormats->listVidFormats[i].listVidCap[j].framerate_denom != NULL)
                    free(listFormats->listVidFormats[i].listVidCap[j].framerate_denom);
            }
            free(listFormats->listVidFormats[i].listVidCap);
        }
    }
    free(listFormats->listVidFormats);
    free(listFormats);
}


/* enumerate frame intervals (fps)
 * args:
 * listVidFormats: array of VidFormats (list of video formats)
 * pixfmt: v4l2 pixel format that we want to list frame intervals for
 * width: video width that we want to list frame intervals for
 * height: video height that we want to list frame intervals for
 * fmtind: current index of format list
 * fsizeind: current index of frame size list
 * fd: device file descriptor
 *
 * returns 0 if enumeration succeded or errno otherwise               */
static int enum_frame_intervals(VidFormats *listVidFormats, __u32 pixfmt, __u32 width, __u32 height,
        int fmtind, int fsizeind, int fd)
{
    int ret=0;
    struct v4l2_frmivalenum fival;
    int list_fps=0;
    memset(&fival, 0, sizeof(fival));
    fival.index = 0;
    fival.pixel_format = pixfmt;
    fival.width = width;
    fival.height = height;
    listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num=NULL;
    listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom=NULL;

    printf("\tTime interval between frame: ");
    while ((ret = xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)) == 0)
    {
        fival.index++;
        if (fival.type == V4L2_FRMIVAL_TYPE_DISCRETE)
        {
            printf("%u/%u, ", fival.discrete.numerator, fival.discrete.denominator);

            list_fps++;
            listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num = (int *)realloc(
                    listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num, list_fps * sizeof(int));
            listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom = (int *)realloc(
                    listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom, list_fps * sizeof(int));

            listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num[list_fps-1] = fival.discrete.numerator;
            listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom[list_fps-1] = fival.discrete.denominator;
        }
        else if (fival.type == V4L2_FRMIVAL_TYPE_CONTINUOUS)
        {
            printf("{min { %u/%u } .. max { %u/%u } }, ",
                    fival.stepwise.min.numerator, fival.stepwise.min.numerator,
                    fival.stepwise.max.denominator, fival.stepwise.max.denominator);
            break;
        }
        else if (fival.type == V4L2_FRMIVAL_TYPE_STEPWISE)
        {
            printf("{min { %u/%u } .. max { %u/%u } / "
                    "stepsize { %u/%u } }, ",
                    fival.stepwise.min.numerator, fival.stepwise.min.denominator,
                    fival.stepwise.max.numerator, fival.stepwise.max.denominator,
                    fival.stepwise.step.numerator, fival.stepwise.step.denominator);
            break;
        }
    }

    if (list_fps==0)
    {
        listVidFormats[fmtind-1].listVidCap[fsizeind-1].numb_frates = 1;
        listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num = (int *)realloc(
                listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num,  sizeof(int));
        listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom = (int *)realloc(
                listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom,  sizeof(int));

        listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_num[0] = 1;
        listVidFormats[fmtind-1].listVidCap[fsizeind-1].framerate_denom[0] = 1;
    }
    else
        listVidFormats[fmtind-1].listVidCap[fsizeind-1].numb_frates = list_fps;

    printf("\n");
    if (ret != 0 && errno != EINVAL)
    {
        printf("VIDIOC_ENUM_FRAMEINTERVALS - Error enumerating frame intervals");
        return ret;
    }
    return 0;
}



/* enumerate frame sizes
 * args:
 * listVidFormats: array of VidFormats (list of video formats)
 * pixfmt: v4l2 pixel format that we want to list frame sizes for
 * fmtind: format list current index
 * width: pointer to integer containing the selected video width
 * height: pointer to integer containing the selected video height
 * fd: device file descriptor
 *
 * returns 0 if enumeration succeded or errno otherwise               */
static int enum_frame_sizes(VidFormats *listVidFormats, __u32 pixfmt, int fmtind, int *width, int *height, int fd)
{
    int ret=0;
    int fsizeind=0; /*index for supported sizes*/
    listVidFormats[fmtind-1].listVidCap = NULL;
    struct v4l2_frmsizeenum fsize;

    memset(&fsize, 0, sizeof(fsize));
    fsize.index = 0;
    fsize.pixel_format = pixfmt;
    while ((ret = xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &fsize)) == 0)
    {
        fsize.index++;
        if (fsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
        {
            printf("{ discrete: width = %u, height = %u }\n",
                    fsize.discrete.width, fsize.discrete.height);

            fsizeind++;
            listVidFormats[fmtind-1].listVidCap = (VidCap *)realloc(
                    listVidFormats[fmtind-1].listVidCap,
                    fsizeind * sizeof(VidCap));

            listVidFormats[fmtind-1].listVidCap[fsizeind-1].width = fsize.discrete.width;
            listVidFormats[fmtind-1].listVidCap[fsizeind-1].height = fsize.discrete.height;

            ret = enum_frame_intervals(listVidFormats,
                    pixfmt,
                    fsize.discrete.width,
                    fsize.discrete.height,
                    fmtind,
                    fsizeind,
                    fd);

            if (ret != 0) printf("  Unable to enumerate frame sizes");
        }
        else if (fsize.type == V4L2_FRMSIZE_TYPE_CONTINUOUS)
        {
            printf("{ continuous: min { width = %u, height = %u } .. "
                    "max { width = %u, height = %u } }\n",
                    fsize.stepwise.min_width, fsize.stepwise.min_height,
                    fsize.stepwise.max_width, fsize.stepwise.max_height);
            printf("  will not enumerate frame intervals.\n");
        }
        else if (fsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
        {
            printf("{ stepwise: min { width = %u, height = %u } .. "
                    "max { width = %u, height = %u } / "
                    "stepsize { width = %u, height = %u } }\n",
                    fsize.stepwise.min_width, fsize.stepwise.min_height,
                    fsize.stepwise.max_width, fsize.stepwise.max_height,
                    fsize.stepwise.step_width, fsize.stepwise.step_height);
            printf("  will not enumerate frame intervals.\n");
        }
        else
        {
            printf("  fsize.type not supported: %d\n", fsize.type);
            printf("     (Discrete: %d   Continuous: %d  Stepwise: %d)\n",
                    V4L2_FRMSIZE_TYPE_DISCRETE,
                    V4L2_FRMSIZE_TYPE_CONTINUOUS,
                    V4L2_FRMSIZE_TYPE_STEPWISE);
        }
    }
    if (ret != 0 && errno != EINVAL)
    {
        printf("VIDIOC_ENUM_FRAMESIZES - Error enumerating frame sizes");
        return ret;
    }
    else if ((ret != 0) && (fsizeind == 0))
    {
        /* ------ some drivers don't enumerate frame sizes ------ */
        /*         negotiate with VIDIOC_TRY_FMT instead          */

        fsizeind++;
        struct v4l2_format fmt;
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = *width;
        fmt.fmt.pix.height = *height;
        fmt.fmt.pix.pixelformat = pixfmt;
        fmt.fmt.pix.field = V4L2_FIELD_ANY;
        ret = xioctl(fd, VIDIOC_TRY_FMT, &fmt);
        /*use the returned values*/
        *width = fmt.fmt.pix.width;
        *height = fmt.fmt.pix.height;
        printf("{ VIDIOC_TRY_FMT : width = %u, height = %u }\n", *width, *height);
        printf("fmtind:%i fsizeind: %i\n",fmtind,fsizeind);
        if(listVidFormats[fmtind-1].listVidCap == NULL)
        {
            listVidFormats[fmtind-1].listVidCap = (VidCap *)realloc(
                    listVidFormats[fmtind-1].listVidCap,
                    fsizeind * sizeof(VidCap));
            listVidFormats[fmtind-1].listVidCap[0].framerate_num = NULL;
            listVidFormats[fmtind-1].listVidCap[0].framerate_num = (int *)realloc(
                    listVidFormats[fmtind-1].listVidCap[0].framerate_num,
                    sizeof(int));
            listVidFormats[fmtind-1].listVidCap[0].framerate_denom = NULL;
            listVidFormats[fmtind-1].listVidCap[0].framerate_denom = (int *)realloc(
                    listVidFormats[fmtind-1].listVidCap[0].framerate_denom,
                    sizeof(int));
        }
        else
        {
            printf("assert failed: listVidCap not Null\n");
            return (-2);
        }
        listVidFormats[fmtind-1].listVidCap[0].width = *width;
        listVidFormats[fmtind-1].listVidCap[0].height = *height;
        listVidFormats[fmtind-1].listVidCap[0].framerate_num[0] = 1;
        listVidFormats[fmtind-1].listVidCap[0].framerate_denom[0] = 25;
        listVidFormats[fmtind-1].listVidCap[0].numb_frates = 1;
    }

    listVidFormats[fmtind-1].numb_res=fsizeind;
    return 0;
}

/* enumerate frames (formats, sizes and fps)
 * args:
 * width: current selected width
 * height: current selected height
 * fd: device file descriptor
 *
 * returns: pointer to LFormats struct containing list of available frame formats */
LFormats *enum_frame_formats(int *width, int *height, int fd)
{
    int ret=0;
    int fmtind=0;
    struct v4l2_fmtdesc fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.index = 0;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    LFormats *listFormats = NULL;
    listFormats = (LFormats *) malloc (sizeof(LFormats));
    listFormats->listVidFormats = NULL;

    while ((ret = xioctl(fd, VIDIOC_ENUM_FMT, &fmt)) == 0)
    {
        fmt.index++;
        printf("{ pixelformat = '%c%c%c%c', description = '%s' }\n",
                fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF,
                (fmt.pixelformat >> 16) & 0xFF, (fmt.pixelformat >> 24) & 0xFF,
                fmt.description);
        /* set hardware flag and allocate on device list */
        if((ret=set_SupPixFormat(fmt.pixelformat)) >= 0)
        {
            fmtind++;
            listFormats->listVidFormats = (VidFormats *)realloc(listFormats->listVidFormats, fmtind * sizeof(VidFormats));
            listFormats->listVidFormats[fmtind-1].format=fmt.pixelformat;
            snprintf(listFormats->listVidFormats[fmtind-1].fourcc,5,"%c%c%c%c",
                    fmt.pixelformat & 0xFF, (fmt.pixelformat >> 8) & 0xFF,
                    (fmt.pixelformat >> 16) & 0xFF, (fmt.pixelformat >> 24) & 0xFF);
            //enumerate frame sizes
            ret = enum_frame_sizes(listFormats->listVidFormats, fmt.pixelformat, fmtind, width, height, fd);
            if (ret != 0)
                printf("  Unable to enumerate frame sizes.\n");
        }
        else
        {
            printf("   { not supported - request format(%i) support}\n",
                    fmt.pixelformat);
        }
    }

    listFormats->numb_formats = fmtind;
    return (listFormats);
}

