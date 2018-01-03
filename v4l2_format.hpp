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

#ifndef V4L2_FORMATS_H
#define V4L2_FORMATS_H

#include <linux/videodev2.h>

/* (Patch) define all supported formats - already done in videodev2.h*/
#ifndef V4L2_PIX_FMT_MJPEG
#define V4L2_PIX_FMT_MJPEG  v4l2_fourcc('M', 'J', 'P', 'G') /*  MJPEG stream     */
#endif

#ifndef V4L2_PIX_FMT_JPEG
#define V4L2_PIX_FMT_JPEG  v4l2_fourcc('J', 'P', 'E', 'G')  /*  JPEG stream      */
#endif

#ifndef V4L2_PIX_FMT_YUYV
#define V4L2_PIX_FMT_YUYV    v4l2_fourcc('Y','U','Y','V')   /* YUV 4:2:2        */
#endif

#ifndef V4L2_PIX_FMT_YVYU
#define V4L2_PIX_FMT_YVYU    v4l2_fourcc('Y','V','Y','U')   /* YUV 4:2:2        */
#endif

#ifndef V4L2_PIX_FMT_UYVY
#define V4L2_PIX_FMT_UYVY    v4l2_fourcc('U','Y','V','Y')   /* YUV 4:2:2        */
#endif

#ifndef V4L2_PIX_FMT_YYUV
#define V4L2_PIX_FMT_YYUV    v4l2_fourcc('Y','Y','U','V')   /* YUV 4:2:2        */
#endif

#ifndef V4L2_PIX_FMT_YUV420
#define V4L2_PIX_FMT_YUV420  v4l2_fourcc('Y','U','1','2')   /* YUV 4:2:0 Planar  */
#endif

#ifndef V4L2_PIX_FMT_YVU420
#define V4L2_PIX_FMT_YVU420  v4l2_fourcc('Y','V','1','2')   /* YUV 4:2:0 Planar  */
#endif

#ifndef V4L2_PIX_FMT_NV12
#define V4L2_PIX_FMT_NV12  v4l2_fourcc('N','V','1','2')   /* YUV 4:2:0 Planar (u/v) interleaved */
#endif

#ifndef V4L2_PIX_FMT_NV21
#define V4L2_PIX_FMT_NV21  v4l2_fourcc('N','V','2','1')   /* YUV 4:2:0 Planar (v/u) interleaved */
#endif

#ifndef V4L2_PIX_FMT_NV16
#define V4L2_PIX_FMT_NV16  v4l2_fourcc('N','V','1','6')   /* YUV 4:2:2 Planar (u/v) interleaved */
#endif

#ifndef V4L2_PIX_FMT_NV61
#define V4L2_PIX_FMT_NV61  v4l2_fourcc('N','V','6','1')   /* YUV 4:2:2 Planar (v/u) interleaved */
#endif

#ifndef V4L2_PIX_FMT_Y41P
#define V4L2_PIX_FMT_Y41P  v4l2_fourcc('Y','4','1','P')    /* YUV 4:1:1          */
#endif

#ifndef V4L2_PIX_FMT_GREY
#define V4L2_PIX_FMT_GREY  v4l2_fourcc('G','R','E','Y')    /*      Y only       */
#endif

#ifndef V4L2_PIX_FMT_Y10BPACK
#define V4L2_PIX_FMT_Y10BPACK  v4l2_fourcc('Y','1','0','B')    /*      Y only (10 bit bit-packed array) */
#endif

#ifndef V4L2_PIX_FMT_Y16
#define V4L2_PIX_FMT_Y16  v4l2_fourcc('Y','1','6',' ')    /*      Y only (16 bit)      */
#endif

#ifndef V4L2_PIX_FMT_SPCA501
#define V4L2_PIX_FMT_SPCA501 v4l2_fourcc('S','5','0','1')  /* YUYV - by line     */
#endif

#ifndef V4L2_PIX_FMT_SPCA505
#define V4L2_PIX_FMT_SPCA505 v4l2_fourcc('S','5','0','5')  /* YYUV - by line     */
#endif

#ifndef V4L2_PIX_FMT_SPCA508
#define V4L2_PIX_FMT_SPCA508 v4l2_fourcc('S','5','0','8')  /* YUVY - by line     */
#endif

#ifndef V4L2_PIX_FMT_SGBRG8
#define V4L2_PIX_FMT_SGBRG8  v4l2_fourcc('G', 'B', 'R', 'G') /* GBGB.. RGRG..    */
#endif

#ifndef V4L2_PIX_FMT_SGRBG8
#define V4L2_PIX_FMT_SGRBG8  v4l2_fourcc('G', 'R', 'B', 'G') /* GRGR.. BGBG..    */
#endif

#ifndef V4L2_PIX_FMT_SBGGR8
#define V4L2_PIX_FMT_SBGGR8  v4l2_fourcc('B', 'A', '8', '1') /* BGBG.. GRGR..    */
#endif

#ifndef V4L2_PIX_FMT_SRGGB8
#define V4L2_PIX_FMT_SRGGB8  v4l2_fourcc('R', 'G', 'G', 'B') /* RGRG.. GBGB..    */
#endif

#ifndef V4L2_PIX_FMT_BGR24
#define V4L2_PIX_FMT_BGR24   v4l2_fourcc('B', 'G', 'R', '3') /* 24  BGR-8-8-8    */
#endif

#ifndef V4L2_PIX_FMT_RGB24
#define V4L2_PIX_FMT_RGB24   v4l2_fourcc('R', 'G', 'B', '3') /* 24  RGB-8-8-8    */
#endif

#ifndef V4L2_PIX_FMT_H264
#define V4L2_PIX_FMT_H264     v4l2_fourcc('H', '2', '6', '4') /* H264 with start codes */
#endif

typedef struct _SupFormats
{
    int format;          //v4l2 software supported format
    char mode[5];        //mode (fourcc - lower case)
    int hardware;        //hardware supported (0, 1 or 2 (H264 through MJPG) )
    //void *decoder;       //function to decode format into YUYV
} SupFormats;

typedef struct _VidCap
{
    int width;            //width
    int height;           //height
    int *framerate_num;   //list of numerator values - should be 1 in almost all cases
    int *framerate_denom; //list of denominator values - gives fps
    int numb_frates;      //number of frame rates (numerator and denominator lists size)
} VidCap;

typedef struct _VidFormats
{
    int format;          //v4l2 pixel format
    char fourcc[5];      //corresponding fourcc (mode)
    int numb_res;        //available number of resolutions for format (VidCap list size)
    VidCap *listVidCap;  //list of VidCap for format
} VidFormats;

typedef struct _LFormats
{
    VidFormats *listVidFormats; //list of VidFormats
    int numb_formats;           //total number of VidFormats (VidFormats list size)
    int current_format;         //index of current format in listVidFormats
} LFormats;


/* enumerate frames (formats, sizes and fps)
 * args:
 * width: current selected width
 * height: current selected height
 * fd: device file descriptor
 *
 * returns: pointer to LFormats struct containing list of available frame formats */
LFormats *enum_frame_formats( int *width, int *height, int fd);

int get_pixMode(int pixfmt, char *mode);

int get_formatIndex(LFormats *listFormats, int format);

int check_supPixFormat(int pixfmt);

void free_formats(LFormats *listFormats);

#endif
