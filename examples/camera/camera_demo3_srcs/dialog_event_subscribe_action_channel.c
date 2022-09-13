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
#define LOG_PREFIX "dlg_prop_enum"
#include <syslog.h>
#include <camera_manager.h>
#include <camera_app_spec.h>
#include <csi_camera_platform_spec.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

static int add_event_actions(camera_event_action_union_t *event_action)
{
	int count = 0;
	const camera_app_bitmasks_t *bitmasks =
		camera_app_get_bitmask_array(CAMERA_APP_BITMAKS_CHANNEL_EVENT_ACTION);

	if (bitmasks == NULL) {
		LOG_E("Can't get channel's action support list\n");
		return count;
	}
	LOG_D("camera.action=0x%08x\n", event_action->channel.action);

	switch (event_action->channel.event) {
	case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY:
	case CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT:
	case CSI_CAMERA_CHANNEL_EVENT_OVERFLOW:
		for (int i = 0; i < bitmasks->count; i++) {
			csi_camera_event_id_e loop_action = bitmasks->bitmask[i];
			bool action_set = (event_action->channel.action & loop_action) != 0;
			const char *item_name = camera_string_channel_action(loop_action);

			item_make("\t%-16s(0x%08x)", item_name, loop_action);
			item_set_tag(action_set ? '*' : ' ');
			item_set_int(loop_action);
		}
		break;
	}
}


int dialog_event_subscribe_action_channel(camera_event_action_union_t *event_action)
{
	int ret_key = KEY_ESC;

	if (event_action == NULL) {
		LOG_W("event_action is NULL\n");
		return KEY_ESC;
	}

	char *title = "Set Channel Event Action(s)";
	char *prompt = "Select the channel event then config the action(s)";
	int item_pos = 0;
	int list_count = 0;

again:
	item_reset();

	item_make("%-32s","Subscribe");
	if (event_action->channel.subscribed == false) {
		item_set_tag(' ');
	} else {
		item_set_tag('*');
		list_count = add_event_actions(event_action);
	}

	int list_height = MIN(list_count + 8, 24);
	ret_key = dialog_checkbox(
		title, prompt,
		CHECKLIST_HEIGTH_MIN + list_height + 2,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 32,
		list_height, item_pos);

	//LOG_D("dialog_checkbox() ret_key=%d\n", ret_key);
	if (ret_key == KEY_ESC) {
		return KEY_ESC;
	} else if (ret_key == 0) {/* select */
		item_pos = item_n();
		LOG_D("item_pos=%d\n", item_pos);

		if (item_pos == 0) {	// 1st item, which indicate whether register actions
			if (item_tag() == ' ') {
				event_action->channel.subscribed = true;
				goto again;
			} else {
				event_action->channel.subscribed = false;
				goto again;
			}
		} else {		// other item, which indicate whether register actions
			camera_action_e cur_action = item_int();
			if (item_tag() == ' ') {
				event_action->channel.action |= cur_action;
				item_set_tag('*');
			} else {
				event_action->channel.action &= ~cur_action;
				item_set_tag(' ');
			}
			LOG_D("camera.action=0x%08x\n", event_action->channel.action);
		}

		goto again;
		return ret_key;
	} else if (ret_key == 1) {/* Return */
		return ret_key;
	} else if (ret_key == 2) {/* Help */
		LOG_W("Help does not support yet\n");
		return ret_key;
	} else {
		LOG_E("Unknown return value: %d\n", ret_key);
		return KEY_ESC;
	}
}

