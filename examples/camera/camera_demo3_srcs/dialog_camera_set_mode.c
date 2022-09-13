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
#define LOG_PREFIX "dlg_set_mode"
#include <syslog.h>
#include <camera_manager.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_camera_set_mode(void)
{
	int ret_key = KEY_ESC;
	int ret;

	LOG_D("Test dialog_checklist\n");
	if (cam_session == NULL) {
		LOG_E("cam_session is NULL\n");
		return KEY_ESC;
	}

	if (cam_session->camera_handle == NULL) {
		LOG_I("The camera has not been opened yet\n");
		dialog_textbox_simple("Infomation", "There's No camera opened yet", 10, 40);
		return -1;
	}

again:
	item_reset();

	if (camera_get_modes(cam_session) != 0) {
		LOG_E("camera_query_list() failed\n");
		return KEY_ESC;
	}

	/* Prepare for checklist nodes */
	csi_camera_modes_s *modes = &(cam_session->camera_modes);
	for (int i = 0; i < modes->count; i++) {
		char mode_desc[256];
		item_make("mode-%02d: ", modes->modes[i].mode_id);
		strncpy(mode_desc, modes->modes[i].description, sizeof(mode_desc));
		mode_desc[60] = '\0';
		item_add_str(mode_desc);
		item_set_data(&modes->modes[i]);	// store cam_info pointer

		if (cam_session->camera_mode_id == modes->modes[i].mode_id) {
			item_set_tag('X');
			item_set_selected(1);
		} else {
			item_set_tag(' ');
			item_set_selected(0);
		}
	}

	ret = dialog_checklist(
		"Set Camera mode",	/* Title */
		"Select the camera working mode."
		"Press 'h' to get details",
		CHECKLIST_HEIGTH_MIN + 10,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 16,
		8,
		NULL, 0);

	int item_id;
	int cam_id;
	int mode_id;
	int selected = item_activate_selected();
	LOG_D("ret=%d, selected=%d\n", ret, selected);
	char str_buf[256];
	switch (ret) {
	case 0:				// item is selected
		if (!selected)
			break;

		/* Set camera mode */
		item_id = item_n();	// selected item: modes->modes[item_id]
		cam_id = cam_session->camera_id;
		mode_id = modes->modes[item_id].mode_id;

		ret = camera_set_mode(cam_session, mode_id);
		if (ret != 0) {
			LOG_E("camera_set_mode() failed\n");
		}

		//char str_buf[256];
		snprintf(str_buf, sizeof(str_buf),
			"Set the below camera:\n"
			"\t id=%d \n\t name='%s' \n\t device='%s' \n\t bus='%s'\n"
			"to be working mode:%d %s",
			cam_id,
			cam_session->camera_infos.info[cam_id].camera_name,
			cam_session->camera_infos.info[cam_id].device_name,
			cam_session->camera_infos.info[cam_id].bus_info,
			mode_id,
			(ret == 0) ? "OK" : "Failed");

		dialog_textbox_simple("Infomation", str_buf, 10, 40);
		LOG_I("csi_camera_infos_s.info[%d] is opened %s\n",
			cam_id, (ret == 0) ? "OK" : "Failed");

		cam_session->camera_mode_id = mode_id;

		/* Show message bar */
		snprintf(str_buf, sizeof(str_buf),
			"Set camera(%d) to be mode: %d %s\n",
			cam_id,	mode_id, (ret == 0) ? "OK" : "Failed");

		message(str_buf, (ret == 0) ? 1 : 2);
		break;
	case 1:				// need help
		cam_id = item_n();
		//char str_buf[256];
		snprintf(str_buf, sizeof(str_buf),
			"Camera info:\n"
			"\t id=%d \n\t name='%s' \n\t device='%s' \n\t bus='%s'\n",
			cam_id,
			cam_session->camera_infos.info[cam_id].camera_name,
			cam_session->camera_infos.info[cam_id].device_name,
			cam_session->camera_infos.info[cam_id].bus_info);

		dialog_textbox_simple("Infomation", str_buf, 10, 40);
		goto again;
	case KEY_ESC:			// specific KEY
		break;
	case -ERRDISPLAYTOOSMALL:	// error
	default:
		LOG_W("Oops? why return ret=%d\n", ret);
		break;
	}

	LOG_D("dialog_checklist() ret=%d\n", ret);
	return 0;
}

