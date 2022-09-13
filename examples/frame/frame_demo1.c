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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>


#define LOG_LEVEL 2
#define LOG_PREFIX "camera_demo1"
#include <syslog.h>

#include <ion.h>
#include <csi_frame.h>
#include <csi_camera_frame.h>
#include "frame_demo_common.h"

static const char *img_name = "../resource/yuv420_1280x720_csky2016.yuv";
static csi_img_format_t img_format = {
	.width = 1280,
	.height = 720,
	.pix_fmt = CSI_PIX_FMT_I420,
};

int main(int argc, char *argv[])
{
	int ret = -1;
	int i;

	int img_fd;
	long img_size;
	int ion_fd;
	int dmabuf_fd;
	void *dmabuf_addr;
	csi_frame_s camera_frame;
	csi_camera_meta_s *camera_meta;

	// get image file size
	if (fdc_get_img_file_size(&img_size, img_name) != 0)
		goto LAB_ERR;

	// open ion device
	if (fdc_get_ion_device_fd(&ion_fd) != 0)
		goto LAB_ERR;


	// alloc dma-buf from ion device
	struct ion_allocation_data alloc_data;
	if (fdc_alloc_ion_buf(&alloc_data, ion_fd, img_size) != 0)
		goto LAB_ERR_ION_OPENED;
	dmabuf_fd = alloc_data.fd;

	// map dma-buf to user addr
	if (fdc_mmap_dmabuf_addr(&dmabuf_addr, img_size, dmabuf_fd) != 0)
		goto LAB_ERR_BUF_ALLOCED;

	// open image file
	img_fd = open(img_name, O_RDONLY);
	if (img_fd < 0) {
		LOG_E("Open image file '%s' failed, %s\n",
		      img_name, strerror(errno));
		goto LAB_ERR_MMAPPED;
	}

	// store image into dma-buf
	if (fdc_store_img_to_dmabuf(dmabuf_addr, img_fd, img_size) != 0)
		goto LAB_ERR_IMG_OPENED;

	// create csi_frame
	if (fdc_create_csi_frame(&camera_frame, &img_format, &camera_meta, img_size, dmabuf_fd,
				 dmabuf_addr) != 0)
		goto LAB_ERR_IMG_OPENED;

	// dump camera meta from csi frame
	fdc_dump_camera_meta((csi_camera_meta_s *)camera_frame.meta.data);

	// destory csi_frame
	fdc_destory_csi_frame(&camera_frame);
	ret = 0;

LAB_ERR_IMG_OPENED:
	close(img_fd);

LAB_ERR_MMAPPED:
	munmap(dmabuf_addr, img_size);

LAB_ERR_BUF_ALLOCED:
	close(dmabuf_fd);

LAB_ERR_ION_OPENED:
	close(ion_fd);

LAB_ERR:
	return ret;
}

