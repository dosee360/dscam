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

#ifndef V4L2_CONTROLS_H
#define V4L2_CONTROLS_H

#include <linux/videodev2.h>
#include <inttypes.h>
#include "defs.hpp"

typedef struct _Control
{
    struct v4l2_queryctrl control;
    struct v4l2_querymenu *menu;
    int32_t class_ctrl;
    int32_t value; //also used for string max size
    int64_t value64;
    char *string;
    //next control in the list
    struct _Control *next;
} Control;

struct VidState
{
    Control *control_list;

    int num_controls;
    int width_req;
    int height_req;
};

/*
 * returns a Control structure NULL terminated linked list
 * with all of the device controls with Read/Write permissions.
 * These are the only ones that we can store/restore.
 * Also sets num_ctrls with the controls count.
 */
Control *get_control_list(int hdevice, int *num_ctrls, int list_method);

/*
 * Returns the Control structure corresponding to control id,
 * from the control list.
 */
Control *get_ctrl_by_id(Control *control_list, int id);

/*
 * Goes through the control list and gets the controls current values
 */
void get_ctrl_values (int hdevice, Control *control_list, int num_controls, void *all_data);

/*
 * Gets the value for control id
 * and updates control flags and widgets
 */
int get_ctrl(int hdevice, Control *control_list, int id);

/*
 * Disables special auto-controls with higher IDs than
 * their absolute/relative counterparts
 * this is needed before restoring controls state
 */
void disable_special_auto (int hdevice, Control *control_list, int id);

/*
 * Goes through the control list and tries to set the controls values
 */
void set_ctrl_values (int hdevice, Control *control_list, int num_controls);

/*
 * sets all controls to default values
 */
void set_default_values(int hdevice, Control *control_list, int num_controls, void *all_data);

/*
 * sets the value for control id
 */
int set_ctrl(int hdevice, Control *control_list, int id);

/*
 * frees the control list allocations
 */
void free_control_list (Control *control_list);

void print_control(Control *control, int i);

#ifndef V4L2_CID_IRIS_ABSOLUTE
#define V4L2_CID_IRIS_ABSOLUTE		(V4L2_CID_CAMERA_CLASS_BASE +17)
#endif
#ifndef V4L2_CID_IRIS_RELATIVE
#define V4L2_CID_IRIS_RELATIVE		(V4L2_CID_CAMERA_CLASS_BASE +18)
#endif


#endif
