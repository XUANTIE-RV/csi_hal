/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __APP_DIALOGS_H__
#define __APP_DIALOGS_H__

#include <curses.h>
#include <csi_camera_property.h>

#include "dialog.h"
#include "param.h"
#include "camera_manager.h"

extern cams_t *cam_session;	/* camera manager */

/* curses window */
#define WIN_COLS 100
#define WIN_ROWS 40

/* key defination */
#define  ENTER 10
#define  ESCAPE 27

/* windoes created by main */
extern WINDOW *menubar;
extern WINDOW *messagebar;
extern WINDOW *win_border;
extern WINDOW *win_content;

void message(char *ss, int status);

int dialog_camera_list(void);
int dialog_camera_open(void);
int dialog_camera_set_mode(void);
int dialog_camera_property_list(void);
int dialog_camera_close(void);

int dialog_camera_property_boolean(csi_camera_property_description_s *property);
int dialog_camera_property_integer(csi_camera_property_description_s *property);
int dialog_camera_property_enum(csi_camera_property_description_s *property);
int dialog_camera_property_bitmask(csi_camera_property_description_s *property);

int dialog_channel_list(void);
int dialog_channel_select(csi_camera_channel_cfg_s **selected_chn, int action);
int dialog_channel_open(csi_camera_channel_cfg_s *channel);

int dialog_event_subscribe_action_list(camera_event_action_union_t **event_action);
int dialog_event_subscribe_action_camera(camera_event_action_union_t *event_action);
int dialog_event_subscribe_action_channel(camera_event_action_union_t *event_action);

int dialog_channel_run(void);

#endif /* __APP_DIALOGS_H__ */
