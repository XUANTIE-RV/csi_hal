/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <csi_frame.h>
#include <sys/shm.h>

int csi_frame_create(csi_frame_s *frame,
		     csi_img_type_e type,
		     csi_img_s img_info)
{
	if (type == CSI_IMG_TYPE_SHM) {
	} else  {
		printf("Not supported img type:%d\n", type);
		return -1;
	}
	return 0;
}

int csi_frame_reference(csi_frame_s *frame_dest, csi_frame_s *frame_src)
{
	return 0;
}

int csi_frame_release(csi_frame_s *frame)
{
	return 0;
}

void* csi_frame_mmap(csi_frame_s *frame)
{
	return NULL;
}

int csi_frame_munmap(csi_frame_s *frame)
{
	return 0;
}

