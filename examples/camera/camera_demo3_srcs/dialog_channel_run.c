/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <curses.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "dlg_chn_run"
#include <syslog.h>
#include <camera_manager.h>
#include <camera_string.h>

#include "param.h"
#include "platform_action.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

#define _CHECK_SESSION_RETURN()				\
	do {						\
		if (session == NULL) {			\
			LOG_E("session is NULL\n");	\
			return -EPERM;			\
		}					\
	} while (0)

static int do_camera_event_action(cams_t *session, csi_camera_event_s *event)
{
	_CHECK_SESSION_RETURN();
	int ret = 0;

	for (uint32_t i = 0; i < CSI_CAMERA_EVENT_MAX_COUNT; i++) {
		camera_event_action_t *event_action =
			&(session->camera_event_action[i].camera);
		if (event_action->event != event->id)
			continue;

		LOG_D("Found camera action:%s\n",
		      camera_string_camera_action(event_action->action));

		if(event_action->action == CAMERA_ACTION_NONE) {
			LOG_D("CHANNEL CAMERA_ACTION_NONE!\n");
			continue;
		}
		if(event_action->action > (CAMERA_ACTION_LOG_PRINT|CAMERA_ACTION_CAPTURE_FRAME)) {
			LOG_E("Unkonw camera event action: %d\n", event_action->action);
			ret = -1;
		}
		for(uint32_t j = 0; j < CAMERA_ACTION_MAX_COUNT; j++) {
			switch (event_action->action & 1 << j) {
			case CAMERA_ACTION_LOG_PRINT:
				LOG_D("CAMERA CAMERA_ACTION_LOG_PRINT\n");
				if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_WARNING) {
					LOG_D("LOG_PRINT CSI_CAMERA_EVENT_WARNING\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_ERROR) {
					LOG_D("LOG PRINT CSI_CAMERA_EVENT_ERROR\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_SENSOR_FIRST_IMAGE_ARRIVE) {
					LOG_D("LOG PRINT CSI_CAMERA_EVENT_SENSOR_FIRST_IMAGE_ARRIVE\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_ISP_3A_ADJUST_READY) {
					LOG_D("LOG PRINT CSI_CAMERA_EVENT_ISP_3A_ADJUST_READY\n");
				} else {
					ret = -1;
					LOG_E("CAMERA CAMERA_ACTION_LOG_ERROR\n");
				}
				break;
			case CAMERA_ACTION_CAPTURE_FRAME:
				LOG_D("CAMERA CAMERA_ACTION_CAPTURE_FRAME\n");
				if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_WARNING) {
					LOG_D("CAPTURE FRAME CSI_CAMERA_EVENT_WARNING\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_ERROR) {
					LOG_D("CAPTURE FRAME CSI_CAMERA_EVENT_ERROR\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_SENSOR_FIRST_IMAGE_ARRIVE) {
					LOG_D("CAPTURE FRAME CSI_CAMERA_EVENT_SENSOR_FIRST_IMAGE_ARRIVE\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_EVENT_ISP_3A_ADJUST_READY) {
					LOG_D("CAPTURE FRAME CSI_CAMERA_EVENT_ISP_3A_ADJUST_READY\n");
				} else {
					ret = -1;
					LOG_D("CAMERA CAMERA_ACTION_CAPTURE_FRAME_ERROR\n");
				}
				break;
			default:
				break;

			}
		}
	}
	return 0;
}

static int do_channel_event_action(cams_t *session, csi_camera_event_s *event)
{
	_CHECK_SESSION_RETURN();
	csi_frame_s frame;
	int ret = 0;

	camera_event_action_union_t *event_action_u =
		session->channel_event_action[event->type - CSI_CAMERA_EVENT_TYPE_CHANNEL0];

	LOG_D("do_channel_event_action()!\n");
	for (uint32_t i = 0; i < CSI_CAMERA_CHANNEL_EVENT_MAX_COUNT; i++) {
		camera_channel_event_action_t *event_action = &(event_action_u[i].channel);
		if (event_action->event != event->id)
			continue;
		if (event_action->action == 0) {
			printf("event_action->action == 0\n");
			break;
		}

		LOG_D("Found channel action:%s\n",
		      camera_string_channel_action(event_action->action));

		if(event_action->action == CAMERA_CHANNEL_ACTION_NONE) {
			LOG_D("CHANNEL CAMERA_ACTION_NONE!\n");
			continue;
		}
		if(event_action->action > (CAMERA_CHANNEL_ACTION_LOG_PRINT|CAMERA_CHANNEL_ACTION_CAPTURE_FRAME|CAMERA_CHANNEL_ACTION_DISPLAY_FRAME)) {
			ret = -1;
			LOG_E("Unkonw channel event action: %d\n", event_action->action);
		}

		for(uint32_t j = 0; j < CAMERA_CHANNEL_ACTION_MAX_COUNT; j++) {
			switch (event_action->action & 1 << j) {
			case CAMERA_CHANNEL_ACTION_LOG_PRINT:
				LOG_D("CHANNEL CAMERA_ACTION_LOG_PRINT\n");
				if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_FRAME_READY) {
					LOG_D("LOG_PRINT CSI_CAMERA_CHANNEL_EVENT_FRAME_READY\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT) {
					LOG_D("LOG_PRINT CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_OVERFLOW) {
					LOG_D("LOG_PRINT CSI_CAMERA_CHANNEL_EVENT_OVERFLOW\n");
				} else {
					ret = -1;
					LOG_E("CHANNEL CAMERA_ACTION_LOG_ERROR\n");
				}
				break;
			case CAMERA_CHANNEL_ACTION_CAPTURE_FRAME:
				LOG_D("CHANNEL CAMERA_ACTION_CAPTURE_FRAME\n");

				/* To show the image can do below steps:
				 * 1. save capture img to be /tmp/capture.jpg
				 * 2. img2txt -W 80 -H 60 -f ansi /tmp/capture.jpg > /tmp/capture.txt
				 * 3. malloc a img_buf and store /tmp/capture.txt into it
				 * 4. printf(img_buf) to show on log console screen
				 */
				if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_FRAME_READY) {
					int timeout = -1;
					int read_frame_count = csi_camera_get_frame_count(NULL, event->type - CSI_CAMERA_EVENT_TYPE_CHANNEL0);
					LOG_D("read_frame_count = %d\n",read_frame_count);
					for (int i = 0; i < read_frame_count; i++) {
						csi_camera_get_frame(NULL,event->type - CSI_CAMERA_EVENT_TYPE_CHANNEL0, &frame, timeout);
						if(frame.img.usr_addr[0] != NULL) {
							camera_action_image_save(&frame);
							csi_camera_put_frame(&frame);
						}
					}
				} else if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT) {
					LOG_D("CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_OVERFLOW) {
					LOG_D("CSI_CAMERA_CHANNEL_EVENT_OVERFLOW\n");
				} else {
					ret = -1;
					LOG_E("CHANNEL CAMERA_ACTION_CAPTURE_FRAME_ERROR\n");
				}
				break;
			case CAMERA_CHANNEL_ACTION_DISPLAY_FRAME:
				LOG_D("CHANNEL CAMERA_ACTION_DISPLAY_FRAME");
				if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_FRAME_READY) {
					int timeout = -1;
					int read_frame_count = csi_camera_get_frame_count(NULL, event->type - CSI_CAMERA_EVENT_TYPE_CHANNEL0);
					LOG_D("read_frame_count = %d\n",read_frame_count);
					//for (int i = 0; i < read_frame_count; i++) {
						csi_camera_get_frame(NULL,event->type - CSI_CAMERA_EVENT_TYPE_CHANNEL0, &frame, timeout);
						if(frame.img.usr_addr[0] != NULL) {
							camera_action_image_display(&frame);
							LOG_D("frame->usr_addr = %p\n",frame.img.usr_addr[0]);
							csi_camera_put_frame(&frame);
						}
					//}
				} else if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT) {
					LOG_D("CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT\n");
				} else if((event_action->event & 1 << i) == CSI_CAMERA_CHANNEL_EVENT_OVERFLOW) {
					LOG_D("CSI_CAMERA_CHANNEL_EVENT_OVERFLOW\n");
				} else {
					ret = -1;
					LOG_E("CHANNEL CAMERA_ACTION_DISPLAY_FRAME_ERROR\n");
				}
				break;
			default:
				break;
			}
		}
	}
	return ret;
}

/* action: 0:close, 1:open, 2:start/stop */
int dialog_channel_run(void)
{
	int ret_key = KEY_ESC;
	int ret;
	char str_buf[256];
	csi_camera_channel_cfg_s *selected_chn = NULL;

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
		csi_camera_channel_cfg_s *channel = &(cam_session->chn_cfg[i]);
		if (channel->status == CSI_CAMERA_CHANNEL_INVALID) {
			continue;
		}

		item_make("Channel[%d]", channel->chn_id);
		item_add_str(" (%s)", camera_string_chn_status(channel->status));
		item_set_data(channel);

		if (channel->status == CSI_CAMERA_CHANNEL_RUNNING || channel->status == CSI_CAMERA_CHANNEL_OPENED) {
			item_set_tag('X');
			item_set_selected(1);
		}
	}

	char *button_names[] = {"Start", "Stop", "Cancel"};
	ret = dialog_checklist(
		"Start/Stop Channel",	/* Title */
		"Select the channel to start or stop",
		CHECKLIST_HEIGTH_MIN + 10,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 16, 8,
		button_names, ARRAY_SIZE(button_names));

	int selected = item_activate_selected();
	LOG_D("ret=%d, selected=%s\n", ret, selected ? "true" : "false");
	switch (ret) {
	case 0:				// button is: start
		if (!selected || item_data() == NULL)
			break;

		/* Show operation result */
		selected_chn = (csi_camera_channel_cfg_s *)item_data();
		if (selected_chn->status != CSI_CAMERA_CHANNEL_OPENED) {
			/* Show message bar */
			snprintf(str_buf, sizeof(str_buf),
				"The Camera[%d] channel[%d] can't start, current status is '%s'\n",
				cam_session->camera_id, selected_chn->chn_id,
				camera_string_chn_status(selected_chn->status));
			message(str_buf, 2);

			goto again;
		}
		camera_register_event_action(cam_session,
					do_camera_event_action,
					do_channel_event_action);

		LOG_D("camera_channel_start selected_chn->chn_id = %x\n",selected_chn->chn_id);
		ret = camera_channel_start(cam_session, selected_chn->chn_id);

		/* Show message bar */
		snprintf(str_buf, sizeof(str_buf),
			"Start Camera[%d] channel[%d] %s\n",
			cam_session->camera_id, selected_chn->chn_id,
			(ret == 0) ? "OK" : "failed");
		message(str_buf, (ret == 0) ? 1 : 2);
		goto again;
		break;
	case 1:				// button is: stop
		if (!selected || item_data() == NULL)
			break;

		/* Show operation result */
		selected_chn = (csi_camera_channel_cfg_s *)item_data();
		if (selected_chn->status != CSI_CAMERA_CHANNEL_RUNNING) {
			/* Show message bar */
			snprintf(str_buf, sizeof(str_buf),
				"The Camera[%d] channel[%d] can't stop, current status is '%s'\n",
				cam_session->camera_id, selected_chn->chn_id,
				camera_string_chn_status(selected_chn->status));
			message(str_buf, 2);

			goto again;
		}

		ret = camera_channel_stop(cam_session, selected_chn->chn_id);
		/* Show message bar */
		snprintf(str_buf, sizeof(str_buf),
			"Stop Camera[%d] channel[%d] %s\n",
			cam_session->camera_id, selected_chn->chn_id,
			(ret == 0) ? "OK" : "failed");
		message(str_buf, (ret == 0) ? 1 : 2);
		goto again;
		break;
	case 2: 			// button is: cancel
	case KEY_ESC:			// specific KEY
		ret = KEY_ESC;
		break;
	default:
		LOG_W("Oops? why return ret=%d\n", ret);
		ret = KEY_ESC;
		break;
	}

	LOG_D("ret=%d\n", ret);
	return ret;
}

