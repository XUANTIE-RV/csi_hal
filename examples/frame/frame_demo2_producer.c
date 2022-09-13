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

#include <sys/wait.h>
#include <sys/socket.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "producer"
#include <syslog.h>
#include <dump_utils.h>

#include <ion.h>
#include <csi_frame.h>
#include <csi_camera_frame.h>
#include "frame_demo_common.h"

extern const char *img_name;
extern csi_img_format_t img_format;

int producer_process(int socket_fd)
{
	int ret = -1;

	int img_fd;
	long img_size;
	int ion_fd;
	int dmabuf_fd;
	void *dmabuf_addr;
	csi_frame_s camera_frame;
	csi_camera_meta_s *camera_meta;

	LOG_D("Starting producer\n");
	if (socket_fd <= 0)  {
		LOG_E("socket_fd(%d) is invalid\n", socket_fd);
		goto LAB_EXIT;
	}
	LOG_D("The input socket_fd=%d\n", socket_fd);

	// get image file size
	if (fdc_get_img_file_size(&img_size, img_name) != 0)
		goto LAB_EXIT;

	// open ion device
	if (fdc_get_ion_device_fd(&ion_fd) != 0)
		goto LAB_EXIT;

	// alloc dma-buf from ion device
	struct ion_allocation_data alloc_data;
	if (fdc_alloc_ion_buf(&alloc_data, ion_fd, img_size) != 0)
		goto LAB_ERR_ION_OPENED;
	dmabuf_fd = alloc_data.fd;

	// map dma-buf to user addr
	if (fdc_mmap_dmabuf_addr(&dmabuf_addr, img_size, dmabuf_fd) != 0)	// should do unmap!!!!!!!!!!!!!!!!!
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
	if (fdc_create_csi_frame(&camera_frame, &img_format, &camera_meta,
				 img_size, dmabuf_fd, dmabuf_addr) != 0)
		goto LAB_ERR_IMG_OPENED;

	// dump camera meta from csi frame

	fdc_dump_camera_meta((csi_camera_meta_s *)camera_frame.meta.data);
	csi_dump_hex((char*)camera_meta, camera_frame.meta.size, "producer: meta.data");
	csi_dump_img_info(&camera_frame.img);

	////////////////////////////////////////////////////////////////////////
	// store csi_frame into socket msg and send begin
	struct msghdr msghdr_send = { 			// init data
		.msg_name = NULL,			// the socket address
		.msg_namelen = 0,			// len of socket name
	};

	int transfer_fd_count = 1;			// it's YUV420P, has only 2 planes
	char ctrls[CMSG_SPACE(sizeof(int)) * CSI_IMAGE_MAX_PLANES] = {}; // store dma-buf fd
	msghdr_send.msg_control = ctrls;
	msghdr_send.msg_controllen = CMSG_SPACE(sizeof(int)) * transfer_fd_count;
	LOG_D("msg_controllen = %zd\n", msghdr_send.msg_controllen);

	struct cmsghdr *pCmsghdr;			// the pointer of control
	int *fdptr = (int *) CMSG_DATA(pCmsghdr);

	// the 1st info stores dma-buf fd[0]
	pCmsghdr = CMSG_FIRSTHDR(&msghdr_send);		// the info of head
	pCmsghdr->cmsg_len = CMSG_LEN(sizeof(int));     // the msg len
	pCmsghdr->cmsg_level = SOL_SOCKET; 		// stream mode
	pCmsghdr->cmsg_type = SCM_RIGHTS;  		// file descriptor
	fdptr = (int *) CMSG_DATA(pCmsghdr);
	*fdptr = camera_frame.img.fds[0];

	struct iovec iov[3];				// io vector strores csi_frame
	iov[0].iov_base = &camera_frame.img;		// iov[0] stores csi_frame.img
	iov[0].iov_len = sizeof(camera_frame.img);
	iov[1].iov_base = &camera_frame.meta;		// iov[1] stores csi_frame.meta info
	iov[1].iov_len = sizeof(camera_frame.meta);
	iov[2].iov_base = camera_frame.meta.data;	// iov[2] stores csi_frame.meta data
	iov[2].iov_len = sizeof(camera_frame.meta) + camera_frame.meta.size;

	msghdr_send.msg_iov = iov;			// the io/vector info
	msghdr_send.msg_iovlen = 3;			// the num of iov

	ssize_t size = sendmsg(socket_fd, &msghdr_send, 0);// send msg now
	if (size < 0) {
		LOG_E("sendmsg() failed, errno=%d, %s\n", errno, strerror(errno));
	} else if (size == 0) {
		LOG_W("sendmsg() failed, %zd bytes\n", size);
	} else {
		LOG_O("sendmsg() OK, %zd bytes\n", size);
	}

	// destory csi_frame
	fdc_destory_csi_frame(&camera_frame);
	ret = 0;

LAB_ERR_IMG_OPENED:
	ret = close(img_fd);
	LOG_D("close(img_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_MMAPPED:
	ret = munmap(dmabuf_addr, img_size);
	LOG_D("munmap() ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_BUF_ALLOCED:
	ret = close(dmabuf_fd);
	LOG_D("close(dmabuf_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_ION_OPENED:
	close(ion_fd);
	LOG_D("close(ion_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_EXIT:
	ret = close(socket_fd);
	LOG_D("close(socket_fd) ret=%d, %s\n", ret, strerror(errno));

	if (ret)
		LOG_E("Exiting producer\n");
	else
		LOG_D("Exiting producer\n");
	return ret;
}

