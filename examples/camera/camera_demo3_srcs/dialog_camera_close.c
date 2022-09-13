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
#define LOG_PREFIX "dlg_close_camera"
#include <syslog.h>
#include <camera_manager.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_camera_close(void)
{
	int ret_key = KEY_ESC;
	int ret;

	if (cam_session == NULL) {
		LOG_E("cam_session is NULL\n");
		return -1;
	}

	if (cam_session->camera_handle == NULL) {
		LOG_I("The camera has not been opened yet\n");
		dialog_textbox_simple("Infomation", "There's No camera opened yet", 10, 40);
		return -1;
	}


	char str_buf[256];
	int cam_id = cam_session->camera_id;
	snprintf(str_buf, sizeof(str_buf),
		"The camera below is to be closed:\n"
		"\t id=%d \n\t name='%s' \n\t device='%s' \n\t bus='%s'\n",
		cam_id,
		cam_session->camera_infos.info[cam_id].camera_name,
		cam_session->camera_infos.info[cam_id].device_name,
		cam_session->camera_infos.info[cam_id].bus_info);

	ret_key = dialog_yesno("Infomation", str_buf, 10, 50);
	LOG_D("dialog_yesno() return %d\n", ret);

	if (ret_key != 0) {	// 0 is the button position of 'yes'
		LOG_D("close camera is canceled\n");
		return 0;
	}

	ret = camera_close(cam_session);
	if (ret != 0) {
		LOG_E("camera_close() failed\n");
		ret = -1;
	}

	snprintf(str_buf, sizeof(str_buf),
		"The camera close %s: {%d, %s, %s, %s}\n",
		(ret == 0) ? "OK" : "Failed",
		cam_id,
		cam_session->camera_infos.info[cam_id].camera_name,
		cam_session->camera_infos.info[cam_id].device_name,
		cam_session->camera_infos.info[cam_id].bus_info);

	message(str_buf, (ret == 0) ? 1 : 2);
	return ret;
}

