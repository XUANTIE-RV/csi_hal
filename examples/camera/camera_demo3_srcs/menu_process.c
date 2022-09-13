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

#include <camera_manager.h>
#include <camera_manager_utils.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"
#include "menu_process.h"

void menu_camera_process(menu_camera_item_e item)
{
	int ret;
	switch (item) {
	case MENU_CAMERA_LIST:
		LOG_D("dialog_camera_list()\n");
		ret = dialog_camera_list();
		break;
	case MENU_CAMERA_OPEN:
		LOG_D("dialog_camera_open()\n");
		ret = dialog_camera_open();
		break;
	case MENU_CAMERA_SET_MODE:
		LOG_D("dialog_camera_set_mode()\n");
		ret = dialog_camera_set_mode();
		break;
	case MENU_CAMERA_SET_PROPERTY:
		LOG_D("dialog_camera_property_list()\n");
		ret = dialog_camera_property_list();
		break;
	case MENU_CAMERA_CLOSE:
		LOG_D("dialog_camera_close()\n");
		ret = dialog_camera_close();
		break;
	case -1:
		ret = -1;
		break;
	default:
		LOG_E("Unknown menu item:%d\n", item);
		ret = -1;
	}
	return;
}

void menu_channel_process(menu_camera_item_e item)
{
	int ret;
	csi_camera_channel_cfg_s *selected_chn;
	char str_buf[128];

	switch (item) {
	case MENU_CHANNEL_LIST:
		LOG_D("channel_list()\n");
		dialog_channel_list();
		break;
	case MENU_CHANNEL_OPEN:
again_channel_open:
		LOG_D("channel_open()\n");
		ret = dialog_channel_select(&selected_chn, 1);
		if (ret == KEY_ESC || ret != 0 ||	/* Not "Select" button */
		    selected_chn == NULL) {		/* No Channel be selected */
			return;
		}
		ret = dialog_channel_open(selected_chn);
		touchwin(stdscr);
		refresh();
		goto again_channel_open;
		break;
	case MENU_CHANNEL_CLOSE:
again_channel_close:
		LOG_D("channel_close()\n");
		ret = dialog_channel_select(&selected_chn, 0);
		if (ret == KEY_ESC || ret != 0 ||	/* Not "Select" button */
		    selected_chn == NULL) {		/* No Channel be selected */
			return;
		}
		ret = camera_channel_close(cam_session, selected_chn->chn_id);
		snprintf(str_buf, sizeof(str_buf),
			"Close Camera[%d]:Channel[%d] %s",
			cam_session->camera_id, selected_chn->chn_id,
			(ret == 0) ? "OK" : "failed");
		dialog_textbox_simple("Infomation", str_buf, 10, 40);
		touchwin(stdscr);
		refresh();
		goto again_channel_close;
		break;
	case -1:
		break;
	default:
		LOG_E("Unknown menu item:%d\n", item);
		break;
	}

	return;
}

void menu_event_run_process(menu_camera_item_e item)
{
	int ret;
	char str_buf[128];
	camera_event_action_union_t *event_action = NULL;
	csi_camera_channel_cfg_s *selected_chn;

	switch (item) {
	case MENU_EVENT_SUBSCRIBE_ACTION:
again_subscribe_action:
		LOG_D("dialog_event_subscribe_action_list()\n");
		ret = dialog_event_subscribe_action_list(&event_action);
		if (ret == KEY_ESC || ret != 0 || event_action == NULL)
			return;

		cam_mng_dump_event_action_union(event_action);
		if (event_action->target == MANAGE_TARGET_CAMERA) {
			ret = dialog_event_subscribe_action_camera(event_action);
			LOG_D("dialog_event_subscribe_action_camera() ret=%d\n", ret);
			goto again_subscribe_action;
		} else if (event_action->target == MANAGE_TARGET_CHANNEL) {
			ret = dialog_event_subscribe_action_channel(event_action);
			LOG_D("dialog_event_subscribe_action_channel() ret=%d\n", ret);
			goto again_subscribe_action;
		}
		break;
	case MENU_EVENT_START_STOP:
		ret = dialog_channel_run();
		break;
	case -1:
		break;
	default:
		LOG_E("Unknown menu item:%d\n", item);
		break;
	}

	return;
}

