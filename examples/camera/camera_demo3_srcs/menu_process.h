/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "camera_demo3"
#include <syslog.h>

#include "camera_manager.h"
#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

void menu_camera_process(menu_camera_item_e item);
void menu_channel_process(menu_camera_item_e item);
void menu_event_run_process(menu_camera_item_e item);

