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

#include <stdlib.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <libv4l2.h>
#include <errno.h>

#include "v4l2_uvc.hpp"
#include "v4l2_controls.hpp"

#ifndef V4L2_CTRL_ID2CLASS
#define V4L2_CTRL_ID2CLASS(id)    ((id) & 0x0fff0000UL)
#endif

/*
 * don't use xioctl for control query when using V4L2_CTRL_FLAG_NEXT_CTRL
 */
static int query_ioctl(int hdevice, int current_ctrl, struct v4l2_queryctrl *ctrl)
{
    int ret = 0;
    int tries = 4;
    do
    {
        if(ret)
            ctrl->id = current_ctrl | V4L2_CTRL_FLAG_NEXT_CTRL;
        ret = ioctl(hdevice, VIDIOC_QUERYCTRL, ctrl);
    }
    while (ret && tries-- &&
            ((errno == EIO || errno == EPIPE || errno == ETIMEDOUT)));

    return(ret);
}

bool is_special_case_control(int control_id)
{
    switch(control_id)
    {
        case V4L2_CID_PAN_RELATIVE:
        case V4L2_CID_TILT_RELATIVE:
        case V4L2_CID_PAN_RESET:
        case V4L2_CID_TILT_RESET:
            return true;
            break;
        default:
            return false;
            break;
    }
}

void print_control(Control *control, int i)
{
    int j=0;

    switch (control->control.type)
    {
        case V4L2_CTRL_TYPE_INTEGER:
            printf("control[%d]:(int) 0x%x '%s'\n",i ,control->control.id, control->control.name);
            printf ("\tmin:%d max:%d step:%d def:%d curr:%d\n",
                    control->control.minimum, control->control.maximum, control->control.step,
                    control->control.default_value, control->value);
            break;

        case V4L2_CTRL_TYPE_BOOLEAN:
            printf("control[%d]:(bool) 0x%x '%s'\n",i ,control->control.id, control->control.name);
            printf ("\tdef:%d curr:%d\n",
                    control->control.default_value, control->value);
            break;

        case V4L2_CTRL_TYPE_MENU:
            printf("control[%d]:(menu) 0x%x '%s'\n",i ,control->control.id, control->control.name);
            printf("\tmin:%d max:%d def:%d curr:%d\n",
                    control->control.minimum, control->control.maximum,
                    control->control.default_value, control->value);
            for (j = 0; control->menu[j].index <= control->control.maximum; j++)
                printf("\tmenu[%d]: [%d] -> '%s'\n", j, control->menu[j].index, control->menu[j].name);
            break;

        case V4L2_CTRL_TYPE_BUTTON:
            printf("control[%d]:(button) 0x%x '%s'\n",i ,control->control.id, control->control.name);
            break;

        default:
            printf("control[%d]:(unknown - 0x%x) 0x%x '%s'\n",i ,control->control.type,
                    control->control.id, control->control.name);
            break;
    }
}

static Control *add_control(int hdevice, struct v4l2_queryctrl *queryctrl, Control **current, Control **first)
{
    Control *control = NULL;
    struct v4l2_querymenu *menu = NULL; //menu list

    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED)
    {
        printf("Control 0x%08x is disabled: remove it from control list\n", queryctrl->id);
        return NULL;
    }

    //check menu items if needed
    if(queryctrl->type == V4L2_CTRL_TYPE_MENU
      )
    {
        int i = 0;
        int ret = 0;
        struct v4l2_querymenu querymenu={0};

        for (querymenu.index = queryctrl->minimum;
                querymenu.index <= queryctrl->maximum;
                querymenu.index++)
        {
            querymenu.id = queryctrl->id;
            ret = xioctl (hdevice, VIDIOC_QUERYMENU, &querymenu);
            if (ret < 0)
                continue;

            if(!menu)
                menu = (struct v4l2_querymenu *)calloc(1, sizeof (struct v4l2_querymenu) * (i+1));
            else
                menu = (struct v4l2_querymenu *)realloc(menu, sizeof (struct v4l2_querymenu) * (i+1));

            memcpy(&(menu[i]), &querymenu, sizeof(struct v4l2_querymenu));
            i++;
        }

        if(!menu)
            menu = (struct v4l2_querymenu *)calloc(1, sizeof(struct v4l2_querymenu) * (i+1));
        else
            menu = (struct v4l2_querymenu *)realloc(menu, sizeof(struct v4l2_querymenu) * (i+1));

        menu[i].id = querymenu.id;
        menu[i].index = queryctrl->maximum+1;
        if(queryctrl->type == V4L2_CTRL_TYPE_MENU)
            menu[i].name[0] = 0;
    }

    // Add the control to the linked list
    control = (Control *)calloc (1, sizeof(Control));
    memcpy(&(control->control), queryctrl, sizeof(struct v4l2_queryctrl));
    control->class_ctrl = V4L2_CTRL_ID2CLASS(control->control.id);
    //add the menu adress (NULL if not a menu)
    control->menu = menu;
    control->string = NULL;

    if(*first != NULL)
    {
        (*current)->next = control;
        *current = control;
    }
    else
    {
        *first = control;
        *current = *first;
    }

    return control;
}

/*
 * returns a Control structure NULL terminated linked list
 * with all of the device controls with Read/Write permissions.
 * These are the only ones that we can store/restore.
 * Also sets num_ctrls with the controls count.
 */
Control *get_control_list(int hdevice, int *num_ctrls, int list_method)
{
    int ret=0;
    Control *first   = NULL;
    Control *current = NULL;

    int n = 0;
    struct v4l2_queryctrl queryctrl={0};

    int currentctrl = 0;
    queryctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;

    if(list_method == LIST_CTL_METHOD_NEXT_FLAG) //try the next_flag method first
    {
        while ((ret=query_ioctl(hdevice, currentctrl, &queryctrl)) == 0)
        {
            if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                n++;

            currentctrl = queryctrl.id;

            queryctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
        }
        if (queryctrl.id != V4L2_CTRL_FLAG_NEXT_CTRL)
        {
            *num_ctrls = n;
            return first; //done
        }

        if(ret)
        {
            printf("Control 0x%08x failed to query with error %i\n", queryctrl.id, ret);
        }

        printf("buggy V4L2_CTRL_FLAG_NEXT_CTRL flag implementation (workaround enabled)\n");

        //next_flag method failed loop through the ids:
        // USER CLASS Controls
        for (currentctrl = V4L2_CID_USER_BASE; currentctrl < V4L2_CID_LASTP1; currentctrl++)
        {
            queryctrl.id = currentctrl;
            if (xioctl(hdevice, VIDIOC_QUERYCTRL, &queryctrl) == 0)
            {
                if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                    n++;
            }
        }
        //CAMERA CLASS Controls
        for (currentctrl = V4L2_CID_CAMERA_CLASS_BASE; currentctrl < V4L2_CID_CAMERA_CLASS_BASE+32; currentctrl++)
        {
            queryctrl.id = currentctrl;
            if (xioctl(hdevice, VIDIOC_QUERYCTRL, &queryctrl) == 0)
            {
                if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                    n++;
            }
        }
        //PRIVATE controls (deprecated)
        for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
                xioctl(hdevice, VIDIOC_QUERYCTRL, &queryctrl) == 0; queryctrl.id++)
        {
            if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                n++;
        }
    }
    else
    {
        printf("using control id loop method for enumeration \n");
        //next_flag method failed loop through the ids:
        // USER CLASS Controls
        for (currentctrl = V4L2_CID_USER_BASE; currentctrl < V4L2_CID_LASTP1; currentctrl++)
        {
            queryctrl.id = currentctrl;
            if (xioctl(hdevice, VIDIOC_QUERYCTRL, &queryctrl) == 0)
            {
                if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                    n++;
            }
        }
        //CAMERA CLASS Controls
        for (currentctrl = V4L2_CID_CAMERA_CLASS_BASE; currentctrl < V4L2_CID_CAMERA_CLASS_BASE+32; currentctrl++)
        {
            queryctrl.id = currentctrl;
            if (xioctl(hdevice, VIDIOC_QUERYCTRL, &queryctrl) == 0)
            {
                if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                    n++;
            }
        }
        //PRIVATE controls (deprecated)
        for (queryctrl.id = V4L2_CID_PRIVATE_BASE;
                xioctl(hdevice, VIDIOC_QUERYCTRL, &queryctrl) == 0; queryctrl.id++)
        {
            if(add_control(hdevice, &queryctrl, &current, &first) != NULL)
                n++;
        }
    }

    *num_ctrls = n;
    return first;
}

/*
 * called when setting controls
 */
static void update_ctrl_flags(Control *control_list, int id)
{
    switch (id)
    {
        case V4L2_CID_EXPOSURE_AUTO:
            {
                Control *ctrl_this = get_ctrl_by_id(control_list, id );
                if(ctrl_this == NULL)
                    break;

                switch (ctrl_this->value)
                {
                    case V4L2_EXPOSURE_AUTO:
                        {
                            //printf("auto\n");
                            Control *ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;

                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_RELATIVE );
                            if (ctrl_that)
                                ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_EXPOSURE_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                        }
                        break;

                    case V4L2_EXPOSURE_APERTURE_PRIORITY:
                        {
                            //printf("AP\n");
                            Control *ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_EXPOSURE_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_RELATIVE );
                            if (ctrl_that)
                                ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                        }
                        break;

                    case V4L2_EXPOSURE_SHUTTER_PRIORITY:
                        {
                            //printf("SP\n");
                            Control *ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;

                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_RELATIVE );
                            if (ctrl_that)
                                ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_EXPOSURE_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                        }
                        break;

                    default:
                        {
                            //printf("manual\n");
                            Control *ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_EXPOSURE_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_ABSOLUTE );
                            if (ctrl_that)
                                ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                            ctrl_that = get_ctrl_by_id(control_list,
                                    V4L2_CID_IRIS_RELATIVE );
                            if (ctrl_that)
                                ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                        }
                        break;
                }
            }
            break;

        case V4L2_CID_FOCUS_AUTO:
            {
                Control *ctrl_this = get_ctrl_by_id(control_list, id );
                if(ctrl_this == NULL)
                    break;
                if(ctrl_this->value > 0)
                {
                    Control *ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_FOCUS_ABSOLUTE);
                    if (ctrl_that)
                        ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;

                    ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_FOCUS_RELATIVE);
                    if (ctrl_that)
                        ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                }
                else
                {
                    Control *ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_FOCUS_ABSOLUTE);
                    if (ctrl_that)
                        ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);

                    ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_FOCUS_RELATIVE);
                    if (ctrl_that)
                        ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                }
            }
            break;

        case V4L2_CID_HUE_AUTO:
            {
                Control *ctrl_this = get_ctrl_by_id(control_list, id );
                if(ctrl_this == NULL)
                    break;
                if(ctrl_this->value > 0)
                {
                    Control *ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_HUE);
                    if (ctrl_that)
                        ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                }
                else
                {
                    Control *ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_HUE);
                    if (ctrl_that)
                        ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                }
            }
            break;

        case V4L2_CID_AUTO_WHITE_BALANCE:
            {
                Control *ctrl_this = get_ctrl_by_id(control_list, id );
                if(ctrl_this == NULL)
                    break;

                if(ctrl_this->value > 0)
                {
                    Control *ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_WHITE_BALANCE_TEMPERATURE);
                    if (ctrl_that)
                        ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                    ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_BLUE_BALANCE);
                    if (ctrl_that)
                        ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                    ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_RED_BALANCE);
                    if (ctrl_that)
                        ctrl_that->control.flags |= V4L2_CTRL_FLAG_GRABBED;
                }
                else
                {
                    Control *ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_WHITE_BALANCE_TEMPERATURE);
                    if (ctrl_that)
                        ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                    ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_BLUE_BALANCE);
                    if (ctrl_that)
                        ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                    ctrl_that = get_ctrl_by_id(control_list,
                            V4L2_CID_RED_BALANCE);
                    if (ctrl_that)
                        ctrl_that->control.flags &= !(V4L2_CTRL_FLAG_GRABBED);
                }
            }
            break;
    }
}

/*
 * update flags of entire control list
 */
static void update_ctrl_list_flags(Control *control_list)
{
    Control *current = control_list;

    for(; current != NULL; current = current->next)
        update_ctrl_flags(control_list, current->control.id);
}

/*
 * Disables special auto-controls with higher IDs than
 * their absolute/relative counterparts
 * this is needed before restoring controls state
 */
void disable_special_auto (int hdevice, Control *control_list, int id)
{
    Control *current = get_ctrl_by_id(control_list, id);
    if(current && ((id == V4L2_CID_FOCUS_AUTO) || (id == V4L2_CID_HUE_AUTO)))
    {
        current->value = 0;
        set_ctrl(hdevice, control_list, id);
    }
}

/*
 * Returns the Control structure corresponding to control id,
 * from the control list.
 */
Control *get_ctrl_by_id(Control *control_list, int id)
{
    Control *current = control_list;
    for(; current != NULL; current = current->next)
    {
        if(current->control.id == id)
            return (current);
    }
    return(NULL);
}

/*
 * Goes through the control list and gets the controls current values
 * also updates flags and widget states
 */
void get_ctrl_values (int hdevice, Control *control_list, int num_controls, void *all_data)
{
    int ret = 0;
    struct v4l2_ext_control clist[num_controls];
    Control *current = control_list;
    int count = 0;
    int i = 0;

    for(; current != NULL; current = current->next)
    {
        if(current->control.flags & V4L2_CTRL_FLAG_WRITE_ONLY)
            continue;

        clist[count].id = current->control.id;
        count++;

        if((current->next == NULL) || (current->next->class_ctrl != current->class_ctrl))
        {
            struct v4l2_ext_controls ctrls = {0};
            ctrls.ctrl_class = current->class_ctrl;
            ctrls.count = count;
            ctrls.controls = clist;
            ret = xioctl(hdevice, VIDIOC_G_EXT_CTRLS, &ctrls);
            if(ret)
            {
                printf("VIDIOC_G_EXT_CTRLS failed\n");
                struct v4l2_control ctrl;
                //get the controls one by one
                if( current->class_ctrl == V4L2_CTRL_CLASS_USER
                  )
                {
                    printf("   using VIDIOC_G_CTRL for user class_ctrl controls\n");
                    for(i=0; i < count; i++)
                    {
                        ctrl.id = clist[i].id;
                        ctrl.value = 0;
                        ret = xioctl(hdevice, VIDIOC_G_CTRL, &ctrl);
                        if(ret)
                            continue;
                        clist[i].value = ctrl.value;
                    }
                }
                else
                {
                    printf("   using VIDIOC_G_EXT_CTRLS on single controls for class: 0x%08x\n",
                            current->class_ctrl);
                    for(i=0;i < count; i++)
                    {
                        ctrls.count = 1;
                        ctrls.controls = &clist[i];
                        ret = xioctl(hdevice, VIDIOC_G_EXT_CTRLS, &ctrls);
                        if(ret)
                            printf("control id: 0x%08x failed to get (error %i)\n",
                                    clist[i].id, ret);
                    }
                }
            }

            //fill in the values on the control list
            for(i=0; i<count; i++)
            {
                Control *ctrl = get_ctrl_by_id(control_list, clist[i].id);
                if(!ctrl)
                {
                    printf("couldn't get control for id: %i\n", clist[i].id);
                    continue;
                }
                switch(ctrl->control.type)
                {
                    default:
                        ctrl->value = clist[i].value;
                        printf("control %i [0x%08x] = %i\n",
                                i, clist[i].id, clist[i].value);
                        break;
                }
            }

            count = 0;
        }
    }

    update_ctrl_list_flags(control_list);
#if 0
    update_widget_state(control_list, all_data);
#endif

}

/*
 * Gets the value for control id
 * and updates control flags and widgets
 */
int get_ctrl(int hdevice, Control *control_list, int id)
{
    Control *control = get_ctrl_by_id(control_list, id );
    int ret = 0;

    if(!control)
        return (-1);
    if(control->control.flags & V4L2_CTRL_FLAG_WRITE_ONLY)
        return (-1);

    if( control->class_ctrl == V4L2_CTRL_CLASS_USER
      )
    {
        struct v4l2_control ctrl;
        //printf("   using VIDIOC_G_CTRL for user class controls\n");
        ctrl.id = control->control.id;
        ctrl.value = 0;
        ret = xioctl(hdevice, VIDIOC_G_CTRL, &ctrl);
        if(ret)
            printf("control id: 0x%08x failed to get value (error %i)\n",
                    ctrl.id, ret);
        else
            control->value = ctrl.value;
    }
    else
    {
        //printf("   using VIDIOC_G_EXT_CTRLS on single controls for class: 0x%08x\n",
        //    current->class_ctrl);
        struct v4l2_ext_controls ctrls = {0};
        struct v4l2_ext_control ctrl = {0};
        ctrl.id = control->control.id;
        ctrls.ctrl_class = control->class_ctrl;
        ctrls.count = 1;
        ctrls.controls = &ctrl;
        ret = xioctl(hdevice, VIDIOC_G_EXT_CTRLS, &ctrls);
        if(ret)
            printf("control id: 0x%08x failed to get value (error %i)\n",
                    ctrl.id, ret);
        else
        {
            switch(control->control.type)
            {
                default:
                    control->value = ctrl.value;
                    //printf("control %i [0x%08x] = %i\n",
                    //    i, clist[i].id, clist[i].value);
                    break;
            }
        }
    }

    update_ctrl_flags(control_list, id);
#if 0
    update_widget_state(control_list, all_data);
#endif

    return (ret);
}

/*
 * Goes through the control list and tries to set the controls values
 */
void set_ctrl_values (int hdevice, Control *control_list, int num_controls)
{
    int ret = 0;
    struct v4l2_ext_control clist[num_controls];
    Control *current = control_list;
    int count = 0;
    int i = 0;

    for(; current != NULL; current = current->next)
    {
        if(current->control.flags & V4L2_CTRL_FLAG_READ_ONLY)
            continue;

        clist[count].id = current->control.id;
        switch (current->control.type)
        {
            default:
                clist[count].value = current->value;
                break;
        }
        count++;

        if((current->next == NULL) || (current->next->class_ctrl != current->class_ctrl))
        {
            struct v4l2_ext_controls ctrls = {0};
            ctrls.ctrl_class = current->class_ctrl;
            ctrls.count = count;
            ctrls.controls = clist;
            ret = xioctl(hdevice, VIDIOC_S_EXT_CTRLS, &ctrls);
            if(ret)
            {
                printf("VIDIOC_S_EXT_CTRLS for multiple controls failed (error %i)\n", ret);
                struct v4l2_control ctrl;
                //set the controls one by one
                if( current->class_ctrl == V4L2_CTRL_CLASS_USER
                  )
                {
                    printf("   using VIDIOC_S_CTRL for user class controls\n");
                    for(i=0;i < count; i++)
                    {
                        ctrl.id = clist[i].id;
                        ctrl.value = clist[i].value;
                        ret = xioctl(hdevice, VIDIOC_S_CTRL, &ctrl);
                        if(ret)
                        {
                            Control *ctrl = get_ctrl_by_id(control_list, clist[i].id);
                            if(ctrl)
                                printf("control(0x%08x) \"%s\" failed to set (error %i)\n",
                                        clist[i].id, ctrl->control.name, ret);
                            else
                                printf("control(0x%08x) failed to set (error %i)\n",
                                        clist[i].id, ret);
                        }
                    }
                }
                else
                {
                    printf("   using VIDIOC_S_EXT_CTRLS on single controls for class: 0x%08x\n",
                            current->class_ctrl);
                    for(i=0;i < count; i++)
                    {
                        ctrls.count = 1;
                        ctrls.controls = &clist[i];
                        ret = xioctl(hdevice, VIDIOC_S_EXT_CTRLS, &ctrls);

                        Control *ctrl = get_ctrl_by_id(control_list, clist[i].id);

                        if(ret)
                        {
                            if(ctrl)
                                printf("control(0x%08x) \"%s\" failed to set (error %i)\n",
                                        clist[i].id, ctrl->control.name, ret);
                            else
                                printf("control(0x%08x) failed to set (error %i)\n",
                                        clist[i].id, ret);
                        }
                    }
                }
            }
            count = 0;
        }
    }

    //update list with real values
    //get_ctrl_values (hdevice, control_list, num_controls);
}

/*
 * sets all controls to default values
 */
void set_default_values(int hdevice, Control *control_list, int num_controls, void *all_data)
{
    Control *current = control_list;

    for(; current != NULL; current = current->next)
    {
        if(current->control.flags & V4L2_CTRL_FLAG_READ_ONLY)
            continue;
        //printf("setting 0x%08X to %i\n",current->control.id, current->control.default_value);
        switch (current->control.type)
        {
            default:
                //if its one of the special auto controls disable it first
                disable_special_auto (hdevice, control_list, current->control.id);
                current->value = current->control.default_value;
                break;
        }
    }

    set_ctrl_values (hdevice, control_list, num_controls);
    get_ctrl_values (hdevice, control_list, num_controls, all_data);
}

/*
 * sets the value for control id
 */
int set_ctrl(int hdevice, Control *control_list, int id)
{
    Control *control = get_ctrl_by_id(control_list, id );
    int ret = 0;

    if(!control)
        return (-1);
    if(control->control.flags & V4L2_CTRL_FLAG_READ_ONLY)
        return (-1);

    if( control->class_ctrl == V4L2_CTRL_CLASS_USER)
    {
        //using VIDIOC_G_CTRL for user class controls
        struct v4l2_control ctrl;
        ctrl.id = control->control.id;
        ctrl.value = control->value;
        ret = xioctl(hdevice, VIDIOC_S_CTRL, &ctrl);
    }
    else
    {
        //using VIDIOC_G_EXT_CTRLS on single controls
        struct v4l2_ext_controls ctrls = {0};
        struct v4l2_ext_control ctrl = {0};
        ctrl.id = control->control.id;
        switch (control->control.type)
        {
            default:
                ctrl.value = control->value;
                break;
        }
        ctrls.ctrl_class = control->class_ctrl;
        ctrls.count = 1;
        ctrls.controls = &ctrl;
        ret = xioctl(hdevice, VIDIOC_S_EXT_CTRLS, &ctrls);
        if(ret)
            printf("control id: 0x%08x failed to set (error %i)\n",
                    ctrl.id, ret);
    }

    //update real value
    get_ctrl(hdevice, control_list, id);

    return (ret);
}

/*
 * frees the control list allocations
 */
void free_control_list (Control *control_list)
{
    Control *first = control_list;
    Control *next = first->next;
    while (next != NULL)
    {
        if(first->string) free(first->string);
        if(first->menu) free(first->menu);
        free(first);
        first = next;
        next = first->next;
    }
    //clean the last one
    if(first->string) free(first->string);
    if(first) free(first);
    control_list = NULL;
}


