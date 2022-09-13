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

#define LOG_LEVEL 3
#define LOG_PREFIX "dlg_chn_list"
#include <syslog.h>

#include <curses.h>

#include <camera_manager.h>
#include <camera_string.h>
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_channel_list(void)
{
	int ret_key = KEY_ESC;
	int ret;

	if (cam_session == NULL) {
		LOG_E("cam_session is NULL\n");
		return KEY_ESC;
	}

	/* Get camera list */
	if (camera_channel_query_list(cam_session) != 0) {
		LOG_E("camera_get_list() failed\n");
		return KEY_ESC;
	}

	char title[64];
	snprintf(title, sizeof(title), "Camera[%d] Channels List", cam_session->camera_id);

	char text[256*CSI_CAMERA_CHANNEL_MAX_COUNT] = "";
	char item[256];
	char temp[128];

	int valid_count = 0;
	for (int i = CSI_CAMERA_CHANNEL_0; i < CSI_CAMERA_CHANNEL_MAX_COUNT; i++) {
		csi_camera_channel_cfg_s *chn_cfg = &(cam_session->chn_cfg[i]);
		if (chn_cfg->status == CSI_CAMERA_CHANNEL_INVALID)
			continue;

		if (valid_count > 0)
			strcat(text, "\n");

		snprintf(item, sizeof(item), "Channel[%d] info:\n", chn_cfg->chn_id);
		strcat(text, item);

		snprintf(item, sizeof(item), "\t Status : %s\n",
			camera_string_chn_status(chn_cfg->status));
		strcat(text, item);

		snprintf(item, sizeof(item), "\t Capture: %s\n",
			camera_string_chn_capture_types(chn_cfg->capture_type, temp));
		strcat(text, item);

		snprintf(item, sizeof(item), "\t Frame  : count=%d, type=%s\n",
			chn_cfg->frm_cnt, camera_string_img_type(chn_cfg->img_type));
		strcat(text, item);

		snprintf(item, sizeof(item), "\t Image  : width=%d, height=%d, pix_fmt=%s\n",
			chn_cfg->img_fmt.width, chn_cfg->img_fmt.height,
			camera_string_pixel_format(chn_cfg->img_fmt.pix_fmt));
		strcat(text, item);

		snprintf(item, sizeof(item), "\t Meta   : %s\n",
			camera_string_chn_meta_fields(chn_cfg->meta_fields, temp));
		strcat(text, item);

		valid_count++;
	}

	int height = MIN((valid_count * 7 + 4),  (WIN_ROWS - 6));
	return dialog_textbox_simple(title, text, height, 80);
}

