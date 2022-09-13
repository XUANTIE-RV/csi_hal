/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdlib.h>

#define LOG_LEVEL 2
#define LOG_PREFIX "camera_meta"
#include <syslog.h>
#include <csi_camera_frame.h>

int csi_camera_frame_alloc_meta(csi_camera_meta_s **meta, int meta_count, size_t *meta_data_size)
{
	return 0;
}

int csi_camera_frame_free_meta(csi_camera_meta_s *meta)
{
	return 0;
}

int csi_camera_frame_get_meta_unit(csi_camrea_meta_unit_s *meta_unit,
				   csi_camera_meta_s      *meta_data,
				   csi_camera_meta_id_e    meta_field)
{
	return 0;
}

