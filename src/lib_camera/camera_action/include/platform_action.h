/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: fuqian.zxr <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PLATFORM_ACTION_H__
#define __PLATFORM_ACTION_H__

#include <csi_camera.h>
int camera_action_image_save(csi_frame_s *frame);
int camera_action_image_display(csi_frame_s *frame);

#endif


