/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PARAM_H__
#define __PARAM_H__

#include <curses.h>

#define MENU_ITEM_MAX_LEN 16
extern char param[10][10][MENU_ITEM_MAX_LEN+1];
int get_param(char *name);

#define MENU_CAMERA 1
typedef enum {
	MENU_CAMERA_LIST = 0,
	MENU_CAMERA_OPEN,
	MENU_CAMERA_SET_MODE,
	MENU_CAMERA_SET_PROPERTY,
	MENU_CAMERA_CLOSE,
} menu_camera_item_e;

#define MENU_CHANNEL 2
typedef enum {
	MENU_CHANNEL_LIST = 0,
	MENU_CHANNEL_OPEN,
	MENU_CHANNEL_CLOSE,
} menu_channel_item_e;

#define MENU_EVENT_RUN 3
typedef enum {
	MENU_EVENT_SUBSCRIBE_ACTION = 0,
	MENU_EVENT_START_STOP,
} menu_event_run_item_e;


#endif /* __PARAM_H__ */
