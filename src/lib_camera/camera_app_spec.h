/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CAMERA_APP_SPEC_H__
#define __CAMERA_APP_SPEC_H__

#include <stdio.h>
#include <csi_camera.h>

typedef enum camera_app_bitmask_id {
	CAMERA_APP_BITMAKS_CAMERA_EVENT_ACTION,
	CAMERA_APP_BITMAKS_CHANNEL_EVENT_ACTION,

	CAMERA_APP_BITMASKS_MAX_COUNT = 32
} camera_app_bitmask_id_e;

typedef struct camera_app_bitmasks {
	int count;
	int bitmask[CAMERA_APP_BITMASKS_MAX_COUNT];
} camera_app_bitmasks_t;

const camera_app_bitmasks_t *camera_app_get_bitmask_array(int bitmask_id);

#endif /* __CAMERA_APP_SPEC_H__ */
