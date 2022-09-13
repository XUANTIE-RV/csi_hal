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
#define LOG_PREFIX "dlg_open_camera"
#include <syslog.h>
#include <camera_manager.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_camera_open(void)
{
	int ret_key = KEY_ESC;
	int ret;

	if (cam_session == NULL) {
		LOG_E("cam_session is NULL\n");
		return KEY_ESC;
	}

	if (cam_session->camera_handle != NULL) {
		csi_camera_info_s *info =
			&(cam_session->camera_infos.info[cam_session->camera_id]);
		char textbox_content[256];
		snprintf(textbox_content, sizeof(textbox_content),
			"Please close camera below first:\n"
			" camera: %s\n device:%s\n bus:%s\n",
			info->camera_name, info->device_name, info->bus_info);
		dialog_textbox("Error",
				textbox_content,
				10, //int initial_height,
				40, //int initial_width,
				(int []) {0}, //int *keys,
				NULL,
				NULL,
				NULL, //update_text_fn update_text,
				NULL /*void *data*/);
		return 0;
	}

again:
	item_reset();

	if (camera_query_list(cam_session) != 0) {
		LOG_E("camera_query_list() failed\n");
		return KEY_ESC;
	}

	/* Prepare for checklist nodes */
	csi_camera_infos_s *cam_infos = &cam_session->camera_infos;
	for (int i = 0; i < cam_infos->count; i++) {
		csi_camera_info_s *info = &(cam_infos->info[i]);
		item_make("%-16s", info->camera_name);
		item_add_str("%-12s %s",
			info->device_name,
			info->bus_info);

		item_set_data(info);	// store cam_info pointer
		if (i == cam_session->camera_id) {
			item_set_tag('X');
			item_set_selected(1);
		} else {
			item_set_tag(' ');
			item_set_selected(0);
		}
	}

	ret = dialog_checklist(
		"Select Camera to open",	/* Title */
		"Select the camera to open. "
		"Press 'h' to get capabilities details",
		CHECKLIST_HEIGTH_MIN + 10,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 16,
		8,
		NULL, 0);

	int cam_id;
	int selected = item_activate_selected();
	LOG_D("ret=%d, selected=%s\n", ret, selected ? "true" : "false");
	char str_buf[256];
	switch (ret) {
	case 0:				// item is selected
		if (!selected)
			break;

		/* Show operation result */
		cam_id = item_activate_selected_pos();
		//char str_buf[256];
		bool opened = (camera_open(cam_session, cam_id) == 0);
		snprintf(str_buf, sizeof(str_buf),
			"Open the below camera %s:\n"
			"\t id=%d \n\t name='%s' \n\t device='%s' \n\t bus='%s'\n",
			opened ? "OK" : "Failed",
			cam_id,
			cam_session->camera_infos.info[cam_id].camera_name,
			cam_session->camera_infos.info[cam_id].device_name,
			cam_session->camera_infos.info[cam_id].bus_info);

		dialog_textbox_simple("Infomation", str_buf, 10, 40);
		LOG_I("csi_camera_infos_s.info[%d] is opened %s\n",
			cam_id, opened ? "OK" : "Failed");

		/* Show message bar */
		snprintf(str_buf, sizeof(str_buf),
			"The camera open %s: {%d, %s, %s, %s}\n",
			opened ? "OK" : "Failed",
			cam_id,
			cam_session->camera_infos.info[cam_id].camera_name,
			cam_session->camera_infos.info[cam_id].device_name,
			cam_session->camera_infos.info[cam_id].bus_info);

		message(str_buf, opened ? 1 : 2);

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

	LOG_D("ret=%d\n", ret);
	return 0;
}

