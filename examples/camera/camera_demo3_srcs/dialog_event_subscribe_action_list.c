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
#define LOG_PREFIX "dlg_prop_list"
#include <syslog.h>

#include <curses.h>
#include <camera_manager.h>
#include <camera_manager_utils.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"

#include <csi_camera_property.h>
#include <csi_camera_platform_spec.h>

extern cams_t *cam_session;

static int fill_items(void)
{
	char item_name[64];
	char item_value[64];
	camera_event_action_union_t *event_action;

	/* Fill camera event and action */
	item_make("*** Camera[%d] Event/Action ***", cam_session->camera_id);
	item_set_tag(' ');
	item_set_data(NULL);

	for (int i = 0; i < CSI_CAMERA_EVENT_MAX_COUNT; i++) {
		event_action = &(cam_session->camera_event_action[i]);
		if (!event_action->camera.supported)
			continue;

		item_make("    %-18s", camera_string_camera_event_type(event_action->camera.event));
		item_add_str("(%s)  --->", event_action->camera.subscribed
					   ? "Subscribed" : "Un-subscribed");
		item_set_tag('M');
		item_set_data(event_action);
		//cam_mng_dump_event_action_union(event_action);
	}
	for(int i = 0;i < CSI_CAMERA_CHANNEL_MAX_COUNT;i++){
		if((cam_session->chn_cfg[i].status == CSI_CAMERA_CHANNEL_OPENED) || 
				(cam_session->chn_cfg[i].status == CSI_CAMERA_CHANNEL_RUNNING)){
			item_make("    *** Channel[%d] Event/Action ***",
					cam_session->chn_cfg[i].chn_id);
			item_set_tag(' ');
			item_set_data(NULL);

			for (int j = 0; j < CSI_CAMERA_CHANNEL_EVENT_MAX_COUNT; j++) {
				event_action = &(cam_session->channel_event_action[i][j]);
				if (!event_action->channel.supported)
					continue;

				item_make("        %-18s", camera_string_channel_event_type(event_action->channel.event));
				item_add_str("(%s)  --->", event_action->channel.subscribed
						? "Subscribed" : "Un-subscribed");
				item_set_tag('N');			// Type: Channel
				item_set_data(event_action);
				//cam_mng_dump_event_action_union(event_action); 

			}
		}
	}
	return 0;
}

int dialog_event_subscribe_action_list(camera_event_action_union_t **event_action)
{
	int ret_key = KEY_ESC;
	int ret = 0;

	char selected_tag;
	camera_event_action_union_t *selected_data;
	*event_action = NULL;
	char str_buf[128];

again:
	item_reset();
	if (fill_items() != 0) {
		LOG_E("fill_items() failed\n");
		ret = KEY_ESC;
		goto exit;
	}

	int s_scroll = 0;
	ret_key = dialog_menu("Camera Event Subscribe & Action Setting",
		"Select the Camera/Channel, press Enter to set the event subscribe and event action",
		WIN_ROWS-2, WIN_COLS, NULL, -1, &s_scroll, NULL, 0);
	//LOG_D("dialog_menu() ret_key=%d, s_scroll=%d\n", ret_key, s_scroll);

	switch (ret_key) {
	case 0:		/* Select button */
		selected_tag = item_tag();
		selected_data = (camera_event_action_union_t *)item_data();
		if (selected_data != NULL) {
			*event_action = selected_data;
			ret = 0;
			break;
		} else {
			goto again;
		}
		break;
	case 1:		/* Apply button */
		ret = camera_subscribe_event(cam_session);
		snprintf(str_buf, sizeof(str_buf),
			"Apply Camera[%d] and Channels event register %s.",
			cam_session->camera_id, (ret == 0) ? "OK" : "failed");
		dialog_textbox_simple("Infomation", str_buf, 6, 56);
		goto again;
	case 2:		/* Cancel button */
	case KEY_ESC:	/* Escape */
	default:
		ret = KEY_ESC;
		goto exit;
	}

exit:
	return ret;
}

