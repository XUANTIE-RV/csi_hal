/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <curses.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "dlg_camera_list"
#include <syslog.h>
#include <camera_manager.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_camera_list(void)
{
	int ret_key = KEY_ESC;
	int ret;

	if (cam_session == NULL) {
		LOG_E("cam_session is NULL\n");
		return KEY_ESC;
	}

	/* Get camera list */
	if (camera_query_list(cam_session) != 0) {
		LOG_E("camera_query_list() failed\n");
		return KEY_ESC;
	}

	char str_buf[1024] = "";
	char str_item[256];

	/* Prepare for checklist nodes */
	csi_camera_infos_s *cam_infos = &cam_session->camera_infos;
	for (int i = 0; i < cam_infos->count; i++) {
		csi_camera_info_s *info = &(cam_infos->info[i]);
		snprintf(str_item, sizeof(str_item),
			"Camera[%d] info:\n"
			"\t name = %s\n"
			"\t dev  = %s\n"
			"\t bus  = %s\n",
			i,
			info->camera_name,
			info->device_name,
			info->bus_info);
		strcat(str_buf, str_item);


		strcat(str_buf, "\t caps = ");
		for (unsigned int j = 1; j < 0x80000000; j = j << 1) {
			switch (info->capabilities & j) {
			case CSI_CAMERA_CAP_VIDEO_CAPTURE:
				strcat(str_buf,"VIDEO_CAPTURE | ");
				break;
			case CSI_CAMERA_CAP_META_CAPTURE:
				strcat(str_buf, "META_CAPTURE | ");
				break;
			default:
				if (info->capabilities & j) {
					strcat(str_buf, "Unknown");
				}
				break;
			}
		}

		strcat(str_buf, "\n\n");
	}

	dialog_textbox_simple("System Camera List",
			str_buf,
			20,	//int initial_height,
			80);	//int initial_width

	return ret;
}

