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
#define LOG_PREFIX "dlg_chn_select"
#include <syslog.h>
#include <camera_manager.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

/* action: 0:close, 1:open, 2:start/stop */
int dialog_channel_select(csi_camera_channel_cfg_s **selected_chn, int action)
{
	int ret_key = KEY_ESC;
	int ret;
	char str_buf[256];
	*selected_chn = NULL;

	if (cam_session == NULL) {
		LOG_E("cam_session is NULL\n");
		return KEY_ESC;
	}

	if (cam_session->camera_handle == NULL) {
		csi_camera_info_s *info =
			&(cam_session->camera_infos.info[cam_session->camera_id]);
		snprintf(str_buf, sizeof(str_buf),
			"Please open camera first\n");
		dialog_textbox_simple("Error", str_buf, 6, 30);
		return KEY_ESC;
	}

again:
	item_reset();

	if (camera_channel_query_list(cam_session) != 0) {
		LOG_E("camera_channel_query_list() failed\n");
		snprintf(str_buf, sizeof(str_buf),
			"Failed to query channel list from Camera failed!\n");
		dialog_textbox_simple("Error", str_buf, 10, 40);
		return KEY_ESC;
	}
	/* Prepare for checklist nodes */
	for (int i = CSI_CAMERA_CHANNEL_0; i < CSI_CAMERA_CHANNEL_MAX_COUNT; i++) {
		csi_camera_channel_cfg_s *chn_cfg = &(cam_session->chn_cfg[i]);
		if (chn_cfg->status == CSI_CAMERA_CHANNEL_INVALID) {
			continue;
		}

		item_make("Channel[%d]", chn_cfg->chn_id);
		item_add_str(" (%s)  --->", camera_string_chn_status(chn_cfg->status));
		item_set_data(chn_cfg);

		if (chn_cfg->status == CSI_CAMERA_CHANNEL_OPENED || chn_cfg->status == CSI_CAMERA_CHANNEL_RUNNING) {
			item_set_tag('X');
			item_set_selected(1);
		}
	}

	if (action == 0) {	// action is close
		char *button_names[] = {" Close ", " Help "};
		ret = dialog_checklist(
			"Select Channel",	/* Title */
			"Select the channel to close",
			CHECKLIST_HEIGTH_MIN + 10,
			WIN_COLS - CHECKLIST_WIDTH_MIN - 16, 8,
			button_names, sizeof(button_names) / sizeof(button_names[0]));
	} else if (action == 1) {		// action is open
		ret = dialog_checklist(
			"Select Channel",	/* Title */
			"Select the channel to config and open",
			CHECKLIST_HEIGTH_MIN + 10,
			WIN_COLS - CHECKLIST_WIDTH_MIN - 16, 8,
			NULL, 0);
	} else {		// action is start/stop
		ret = dialog_checklist(
			"Select Channel",	/* Title */
			"Select the channel to Start or Stop",
			CHECKLIST_HEIGTH_MIN + 10,
			WIN_COLS - CHECKLIST_WIDTH_MIN - 16, 8,
			NULL, 0);
	}

	int selected = item_activate_selected();
	LOG_D("ret=%d, selected=%s\n", ret, selected ? "true" : "false");
	switch (ret) {
	case 0:				// item is selected/close
		if (!selected)
			break;

		/* Show operation result */
		*selected_chn = (csi_camera_channel_cfg_s *)item_data();

		if (action == 0) {	// to close
			if ((*selected_chn)->status == CSI_CAMERA_CHANNEL_RUNNING) {
				dialog_textbox_simple("Infomation",
					"The channel is Running, \nplease stop first", 6, 40);
				goto again;
			}
		} else if (action == 1) {	// to open
			if ((*selected_chn)->status == CSI_CAMERA_CHANNEL_OPENED ||
			    (*selected_chn)->status == CSI_CAMERA_CHANNEL_RUNNING) {
				dialog_textbox_simple("Infomation",
				"The channel is Opened or Running, \nplease close first", 6, 40);
				goto again;
			}
		}
		/* Show message bar */
		snprintf(str_buf, sizeof(str_buf),
			"The Camera[%d] channel[%d] is selected\n",
			cam_session->camera_id,
			(*selected_chn)->chn_id);
		message(str_buf, 0);
		ret = 0;
		break;
	case 1:				// need help
		LOG_W("Help is not supported yet\n");
		goto again;
	case KEY_ESC:			// specific KEY
		ret = KEY_ESC;
		break;
	case -ERRDISPLAYTOOSMALL:	// error
	default:
		LOG_W("Oops? why return ret=%d\n", ret);
		ret = KEY_ESC;
		break;
	}

	LOG_D("ret=%d\n", ret);
	return ret;
}

