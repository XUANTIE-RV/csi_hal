/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __FRAME_DEMO_COMMON_H__
#define __FRAME_DEMO_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <sys/un.h>

#include <ion.h>
#include <csi_frame.h>
#include <csi_camera_frame.h>

#define ION_DEVICE_NAME "/dev/ion"

extern const int PAGE_SIZE;
extern const char *IMG_YUV420_FILENAME;
extern const int IMG_RESOLUTION_WIDTH;
extern const int IMG_RESOLUTION_HEIGHT;;

int fdc_get_img_file_size(long *img_size, const char *filename);
int fdc_get_ion_device_fd(int *ion_fd);
int fdc_alloc_ion_buf(struct ion_allocation_data *alloc_data,
		      int ion_fd, unsigned long img_size);
int fdc_mmap_dmabuf_addr(void **addr, unsigned long length, int dmabuf_fd);
int fdc_store_img_to_dmabuf(void *dmabuf_addr, int img_fd, long img_size);
int fdc_create_csi_frame(csi_frame_s *camera_frame,
			 csi_img_format_t *img_format, csi_camera_meta_s **camera_meta,
			 long img_size, int dmabuf_fd, void *dmabuf_addr);
void fdc_dump_camera_meta(csi_camera_meta_s *camera_meta);
void fdc_destory_csi_frame(csi_frame_s *camera_frame);
void fdc_dump_msghdr(struct msghdr *msg);

#endif /* __FRAME_DEMO_COMMON_H__ */
