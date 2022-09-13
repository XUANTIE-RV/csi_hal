/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "cam_mng_utils"
#include <syslog.h>
#include <camera_string.h>
#include "camera_manager_utils.h"

void cam_mng_dump_event_action_union(camera_event_action_union_t *event_action)
{
	if (event_action == NULL) {
		LOG_E("event_action is NULL\n");
	}

	LOG_D("Dumping event_action:\n");
	LOG_F("event_action = {\n");
	if (event_action->target == MANAGE_TARGET_CAMERA) {
		LOG_F("\t .target=CAMERA, .camera_id=%d\n",
			event_action->camera_id);
		LOG_F("\t .camera = {.event=%d, .support=%d, .subscribed=%d, .action=0x%08x}\n",
			event_action->camera.event, event_action->camera.supported,
			event_action->camera.subscribed, event_action->camera.action);
	} else if (event_action->target == MANAGE_TARGET_CHANNEL) {
		LOG_F("\t .target=CHANNEL, .camera_id=%d, .channel_id = %d\n",
			event_action->camera_id, event_action->channel_id);
		LOG_F("\t .channel = {.event=%d, .support=%d, .subscribed=%d, .action=0x%08x}\n",
			event_action->channel.event, event_action->channel.supported,
			event_action->channel.subscribed, event_action->channel.action);
	} else {
		LOG_F("\t .target=Unknown(%d) !!\n", event_action->target);
	}
	LOG_F("}\n");
}

