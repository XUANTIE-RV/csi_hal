/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <stdio.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "app_spec"
#include <syslog.h>
#include <csi_camera.h>
#include <camera_manager.h>

#include "camera_app_spec.h"

const camera_app_bitmasks_t camera_app_support_camera_event_action[] = {
	2,
	{
		CAMERA_ACTION_LOG_PRINT,
		CAMERA_ACTION_CAPTURE_FRAME,
	}
};

const camera_app_bitmasks_t camera_app_support_channel_event_action[] = {
	3,
	{
		CAMERA_CHANNEL_ACTION_LOG_PRINT,
		CAMERA_CHANNEL_ACTION_CAPTURE_FRAME,
		CAMERA_CHANNEL_ACTION_DISPLAY_FRAME,
	}
};

const camera_app_bitmasks_t *camera_app_get_bitmask_array(int bitmask_id)
{
	switch(bitmask_id) {
	case CAMERA_APP_BITMAKS_CAMERA_EVENT_ACTION:
		return camera_app_support_camera_event_action;
	case CAMERA_APP_BITMAKS_CHANNEL_EVENT_ACTION:
		return camera_app_support_channel_event_action;
	default:
		LOG_E("Unknown bitmask_id:%d\n", bitmask_id);
		return NULL;
	}
}

