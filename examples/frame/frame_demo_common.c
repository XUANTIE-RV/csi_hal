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
//#define LOG_PREFIX "demo_common"
#include <syslog.h>

#include <ion.h>
#include <csi_frame.h>
#include <csi_camera_frame.h>
#include "frame_demo_common.h"

#define PAGE_SIZE 4096
//#define IMG_YUV420_FILENAME "../resource/yuv420_1280x720_csky2016.yuv"
#define IMG_RESOLUTION_WIDTH  1280
#define IMG_RESOLUTION_HEIGHT 720

/* fdc = frame demo common */
int fdc_get_img_file_size(long *img_size, const char *filename)
{
	struct stat img_stat;
	if (stat(filename, &img_stat) != 0) {
		LOG_E("stat file '%s' failed, %s\n", filename, strerror(errno));
		return -1;
	}
	*img_size = img_stat.st_size;
	LOG_O("Get image '%s' size=%ld\n", filename, *img_size);
	return 0;
}

int fdc_get_ion_device_fd(int *ion_fd)
{
	*ion_fd = open(ION_DEVICE_NAME, O_RDWR);
	if (*ion_fd < 0) {
		LOG_E("Open ion device:'%s' failed, %s\n", ION_DEVICE_NAME, strerror(errno));
		return -1;
	}
	LOG_O("Open ion device OK, ion_fd=%d\n", *ion_fd);
	return 0;
}

int fdc_alloc_ion_buf(struct ion_allocation_data *alloc_data,
		  int ion_fd, unsigned long img_size)
{
	alloc_data->len = img_size;
	if (ioctl(ion_fd, ION_IOC_ALLOC, alloc_data) ||
	    alloc_data->fd < 0) {
		LOG_E("ion ioctl failed, %s\n", strerror(errno));
		return -1;
	}
	int dmabuf_fd = alloc_data->fd;
	LOG_O("Alloc %ld from ion device OK, dmabuf_fd=%d\n", img_size, dmabuf_fd);
	return 0;
}

int fdc_mmap_dmabuf_addr(void **addr, unsigned long length, int dmabuf_fd)
{
	LOG_D("Enter, length=%zd, dmabuf_fd=%d\n", length, dmabuf_fd);
	*addr = mmap(NULL, length, PROT_READ | PROT_WRITE,
			 MAP_SHARED, dmabuf_fd, 0);
	if (*addr == NULL || *addr == MAP_FAILED) {
		LOG_E("Map dmabuf failed\n");
		return -1;
	}
	LOG_O("mmap dmabuf to user addr: %p OK\n", *addr);
	return 0;
}

int fdc_store_img_to_dmabuf(void *dmabuf_addr, int img_fd, long img_size)
{
	long totle_len=0, read_len = 0;
	char *img_ptr = (char*)dmabuf_addr;
	while ((read_len = read(img_fd, (img_ptr + totle_len), PAGE_SIZE)) > 0) {
		totle_len += read_len;
	}
	if (read_len < 0) {
		LOG_E("Read from image file failed, %s\n", strerror(errno));
		return -1;
	}
	if (totle_len != img_size) {
		LOG_E("Read total len(%ld) != stat size(%ld)\n", totle_len, img_size);
		return -1;
	}
	LOG_O("Store image into dma-buf OK\n");
	return 0;
}

int fdc_create_csi_frame(csi_frame_s *camera_frame,
	csi_img_format_t *img_format, csi_camera_meta_s **camera_meta,
	long img_size, int dmabuf_fd, void *dmabuf_addr)
{
	// fill csi_frame's img info
	camera_frame->img.type = CSI_IMG_TYPE_DMA_BUF;
	camera_frame->img.size = img_size;
	camera_frame->img.width = img_format->width;
	camera_frame->img.height = img_format->height;
	camera_frame->img.pix_format = img_format->pix_fmt;
	camera_frame->img.num_planes = 1;	// Now use only 1 plane data
	camera_frame->img.fds[0] = dmabuf_fd;
	camera_frame->img.strides[0] = img_format->width;
	camera_frame->img.offsets[0] = 0;

	camera_frame->img.fds[1] = -1;
	camera_frame->img.strides[1] = 0;
	camera_frame->img.offsets[1] = 0;

	camera_frame->img.fds[2] = -1;
	camera_frame->img.strides[2] = 0;
	camera_frame->img.offsets[2] = 0;
	camera_frame->img.modifier = 0;

	// prepare csi_camera_frame meta info
	int meta_unit_count = 5;
	size_t meta_data_size;
	csi_camera_frame_alloc_meta(camera_meta, meta_unit_count, &meta_data_size);

	(*camera_meta)->units[0].id = CSI_CAMERA_META_ID_CAMERA_NAME;
	(*camera_meta)->units[0].type = CSI_META_VALUE_TYPE_STR;
	strncpy((*camera_meta)->units[0].str_value, "/dev/video0",
		sizeof((*camera_meta)->units[0].str_value));

	(*camera_meta)->units[1].id = CSI_CAMERA_META_ID_CHANNEL_ID;
	(*camera_meta)->units[1].type = CSI_META_VALUE_TYPE_UINT;
	(*camera_meta)->units[1].uint_value = 1;

	(*camera_meta)->units[2].id = CSI_CAMERA_META_ID_FRAME_ID;
	(*camera_meta)->units[2].type = CSI_META_VALUE_TYPE_INT;
	(*camera_meta)->units[2].uint_value = 2;

	(*camera_meta)->units[3].id = CSI_CAMERA_META_ID_TIMESTAMP;
	(*camera_meta)->units[3].type = CSI_META_VALUE_TYPE_TIMEVAL;
	gettimeofday(&(*camera_meta)->units[3].time_value, NULL);

	(*camera_meta)->units[4].id = CSI_CAMERA_META_ID_HDR;
	(*camera_meta)->units[4].type = CSI_META_VALUE_TYPE_TIMEVAL;
	(*camera_meta)->units[4].bool_value = false;

	camera_frame->meta.type = CSI_META_TYPE_CAMERA;
	camera_frame->meta.size = meta_data_size;
	camera_frame->meta.data = *camera_meta;

	LOG_D("*camera_meta=camera_frame->meta.data=%p\n", camera_frame->meta.data);

	return 0;
}

void fdc_dump_camera_meta(csi_camera_meta_s *camera_meta)
{
	int ret;
	csi_camrea_meta_unit_s meta_unit;
	ret = csi_camera_frame_get_meta_unit(&meta_unit, camera_meta, CSI_CAMERA_META_ID_TIMESTAMP);
	if (ret == 0) {
		struct timeval tv = meta_unit.time_value;
		struct tm *t = localtime(&tv.tv_sec);
		if (t != NULL) {
			LOG_O("Get timestamp from csi frame:%d-%d-%d %d:%d:%d.%ld\n",
				1900+t->tm_year, 1+t->tm_mon, t->tm_mday,
				t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec);
			return;
		} else {
			LOG_E("timestamp is invalid\n");
			return;
		}
	}
	return;
}

void fdc_destory_csi_frame(csi_frame_s *camera_frame)
{
	if (camera_frame->meta.data != NULL)
		csi_camera_frame_free_meta((csi_camera_meta_s*)camera_frame->meta.data);
}

void fdc_dump_msghdr(struct msghdr *msg)
{
	int i;
	if (msg == NULL) {
		LOG_E("mgs = NULL\n");
		return;
	}

	LOG_D("struct msghdr *msg(%p) = {\n", msg);
	LOG_D("\t .msg_iov=%p\n", msg->msg_iov);
	LOG_D("\t .msg_iovlen=%zd\n", msg->msg_iovlen);
	for(i = 0; i < msg->msg_iovlen; i++) {
		struct iovec *iovc = &(msg->msg_iov[i]);
		LOG_D("\t   .msg_iov[%d]={.iov_base=%p, .iov_len=%zd}\n",
			i, msg->msg_iov[i].iov_base, msg->msg_iov[i].iov_len);
	}
	LOG_D("\t .msg_control=%p\n", msg->msg_control);
	LOG_D("\t .msg_control=%zd\n", msg->msg_controllen);
	LOG_D("\t .name=%p\n", msg->msg_name);
	LOG_D("\t .msg_namelen=%d\n", msg->msg_namelen);
	LOG_D("\t .msg_flags=%d\n", msg->msg_flags);
	LOG_D("}\n");
}

