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
	size_t malloc_size;
	if (meta_count <= 0) {
		LOG_E("meta_count=%d\n", meta_count);
		*meta_data_size = 0;
		return -1;
	}

	malloc_size = sizeof(csi_camera_meta_s) + sizeof(csi_camrea_meta_unit_s) * meta_count;
	LOG_D("malloc_size=(%zd+%zd)=%zd\n", sizeof(csi_camera_meta_s), sizeof(csi_camrea_meta_unit_s) * meta_count, malloc_size);

	*meta = malloc(malloc_size);
	if (*meta == NULL) {
 		LOG_E("malloc *meta(%zu) failed\n", malloc_size);
		return -1;
	}

	memset(*meta, 0, malloc_size);
	(*meta)->count = meta_count;
	(*meta)->size = sizeof(csi_camrea_meta_unit_s) * meta_count;
	(*meta)->units = (csi_camrea_meta_unit_s *)((char*)(*meta) + sizeof(csi_camera_meta_s));

	LOG_D("*meta=%p, (*meta)->units=%p\n", *meta, (*meta)->units);

	*meta_data_size = malloc_size;

	return 0;
}

int csi_camera_frame_free_meta(csi_camera_meta_s *meta)
{
	if (meta == NULL) {
		LOG_E("[%s:%d] meta = NULL\n", __func__, __LINE__);
		return -1;
	}

	free(meta);
	return 0;
}

int csi_camera_frame_get_meta_unit(csi_camrea_meta_unit_s *meta_unit,
				   csi_camera_meta_s      *meta_data,
				   csi_camera_meta_id_e    meta_field)
{
	int i;

	LOG_D("meta_data->count=%d\n", meta_data->count);

	for (i = 0; i < meta_data->count; i++) {
		if (meta_data->units[i].id == meta_field) {
			*meta_unit = meta_data->units[i];
			return 0;
		}
	}
	return -1;
}

