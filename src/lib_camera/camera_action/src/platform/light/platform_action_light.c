/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: fuqian.zxr <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define LOG_LEVEL 3
#define LOG_PREFIX "platform_action"
#include <syslog.h>
#include "platform_action.h"

#include <stdio.h>

int camera_action_image_save(csi_frame_s *frame)
{
	//wait to complete
	return 0;
}

int camera_action_image_display(csi_frame_s *frame)
{
	//wait to complete
	return 0;
}
