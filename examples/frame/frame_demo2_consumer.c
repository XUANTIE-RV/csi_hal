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
#define LOG_PREFIX "consumer"
#include <syslog.h>
#include <dump_utils.h>

#include <ion.h>
#include <csi_frame.h>
#include <csi_camera_frame.h>
#include "frame_demo_common.h"

extern const char *img_name;
extern csi_img_format_t img_format;

int consumer_process(int socket_fd)
{
	int ret = -1;

	LOG_D("Starting consumer\n");
	sleep(1); 	// waiting for producer exit to watch log easily
	if (socket_fd <= 0)  {
		LOG_E("socket_fd(%d) is invalid\n", socket_fd);
		goto LAB_ERR;
	}
	LOG_D("The input socket_fd=%d\n", socket_fd);

	struct msghdr msghdr_recv = { 		// init data
		.msg_name = NULL,			// the socket address
		.msg_namelen = 0,			// len of socket name
	};

	char ctrls[CMSG_SPACE(sizeof(int)*3)] = {};
	memset(ctrls, 0, sizeof(ctrls));
	msghdr_recv.msg_control = ctrls;
	msghdr_recv.msg_controllen = sizeof(ctrls);

	csi_frame_s camera_frame;
	csi_camera_meta_s camera_meta;
	char meta_data[CSI_CAMERA_META_MAX_LEN];	//CSI_CAMERA_META_MAX_LEN

	struct iovec iov[3];
	iov[0].iov_base = &camera_frame.img;		// iov[0] stores csi_frame.img info
	iov[0].iov_len = sizeof(camera_frame.img);
	iov[1].iov_base = &camera_frame.meta;		// iov[1] stores csi_frame.meta info
	iov[1].iov_len = sizeof(camera_frame.meta);
	iov[2].iov_base = meta_data;			// iov[2] stores csi_frame.meta data
	iov[2].iov_len = sizeof(meta_data);

	msghdr_recv.msg_iov = iov;			// the io/vector info
	msghdr_recv.msg_iovlen = CSI_IMAGE_MAX_PLANES;	// the num of iov

	ssize_t	recv_len;
	if ((recv_len = recvmsg(socket_fd, &msghdr_recv, 0)) < 0) {
		// the msg is recv by msghdr_recv
		LOG_E("recv error:%d, %s\n", errno, strerror(errno));
		goto LAB_ERR;
	}
	LOG_D("recvmsg() length=%zd\n", recv_len);
	fdc_dump_msghdr(&msghdr_recv);

	int msghdr_count = 0;
	struct cmsghdr *pCmsghdr = NULL;		// the pointer of control
	for (pCmsghdr = CMSG_FIRSTHDR(&msghdr_recv); pCmsghdr != NULL;
	     pCmsghdr = CMSG_NXTHDR(&msghdr_recv,pCmsghdr)) {
		msghdr_count++;
	}
	LOG_D("msghdr_count=%d\n", msghdr_count);

	// now, only 1 plane even in I420 format (CSI_IMAGE_I420_PLANES)
	if ((pCmsghdr = CMSG_FIRSTHDR(&msghdr_recv)) != NULL &&
	    pCmsghdr->cmsg_len == CMSG_LEN(sizeof(int) * 1)) {
		if (pCmsghdr->cmsg_level != SOL_SOCKET) {
			LOG_E("Ctl level should be SOL_SOCKET\n");
			goto LAB_ERR;
		}

		if (pCmsghdr->cmsg_type != SCM_RIGHTS) {
			LOG_E("Ctl type should be SCM_RIGHTS\n");
			goto LAB_ERR;
		}

		int fd_count = (pCmsghdr->cmsg_len - CMSG_SPACE(0)) / sizeof(int);
		LOG_D("pCmsghdr->cmsg_len=%zd, fd_count=%d\n", pCmsghdr->cmsg_len, fd_count);
		int *fdptr = (int *) CMSG_DATA(pCmsghdr);

		for (int i = 0; i < CSI_IMAGE_MAX_PLANES; i ++) {
			if (i < fd_count)
				camera_frame.img.fds[i] = fdptr[i];
			else
				camera_frame.img.fds[i] = -1;
		}
	} else {
		if (pCmsghdr == NULL)
			LOG_E("pCmsghdr=%p\n", pCmsghdr);
		else
			LOG_E("pCmsghdr->cmsg_len=%zu, should=%zu\n",
				pCmsghdr->cmsg_len,
				CMSG_LEN(sizeof(int) * CSI_IMAGE_MAX_PLANES));

		goto LAB_ERR;
	}
	int dma_buf_fd = camera_frame.img.fds[0];
	LOG_D("recvmsg() with dmabuf_fds={%d, %d, %d}\n",
	      camera_frame.img.fds[0], camera_frame.img.fds[1], camera_frame.img.fds[2]);

	// fill camera frame img fds
	camera_frame.meta.data = meta_data;
	csi_dump_hex((char *)(&camera_frame.meta), sizeof(camera_frame.meta), "meta");
	csi_dump_hex(camera_frame.meta.data, camera_frame.meta.size, "consumer: meta.data");
	csi_dump_img_info(&camera_frame.img);

	csi_camera_meta_s *camera_meta_data = (csi_camera_meta_s *)camera_frame.meta.data;
	camera_meta_data->units = (csi_camrea_meta_unit_s *)((char *)camera_meta_data + sizeof(csi_camera_meta_s));
	fdc_dump_camera_meta(camera_meta_data);

	LOG_D("csi frame meta = {type=%d, size=%zd, data=%p}\n",
		camera_frame.meta.type, camera_frame.meta.size, camera_frame.meta.data);
	LOG_D("csi camera meta = {.count=%d, .size=%zd, .units=%p}\n",
		camera_meta_data->count, camera_meta_data->size, camera_meta_data->units);

	// save image file
	const char *save_file_name = "consumer_dump_file.yuv";
	remove(save_file_name);
	FILE *dump_file = fopen(save_file_name, "wb");
	if (dump_file) {
		void *addr = NULL;
		int length = 0;

		length = camera_frame.img.size; //(img_format.height * img_format.width);

		ret = fdc_mmap_dmabuf_addr(&addr, length, camera_frame.img.fds[0]);
		LOG_D("ret=%d, addr=%p, lenght=%d\n", ret, addr, length);
		if (ret == 0 || addr != NULL) {
			size_t write_len = fwrite(addr, length, 1, dump_file);
			LOG_O("dump dma-buf into '%s', write_len=%zd\n",
				save_file_name, write_len);
		}

		fclose(dump_file);
	} else {
		LOG_E("fopen failed, %s\n", strerror(errno));
	}
	//fclose(dump_file);


	for (int i = 0; i < CSI_IMAGE_MAX_PLANES; i++) {
		if (camera_frame.img.fds[i] > 0) {
			ret = close(camera_frame.img.fds[i]);
			LOG_D("close(camera_frame.img.fds[%d]) ret=%d, %s\n",
				i, errno, strerror(errno));
		}
	}

	ret = close(socket_fd);
	LOG_D("close(socket_fd) ret=%d, %s\n", errno, strerror(errno));
	ret = 0;

LAB_ERR:
	if (ret)
		LOG_E("Exiting consumer\n");
	else
		LOG_D("Exiting consumer\n");
	return ret;
}

