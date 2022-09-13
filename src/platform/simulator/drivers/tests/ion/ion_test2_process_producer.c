/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <sys/socket.h>
#include <sys/un.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "producer"
#include <syslog.h>

#include <ipc_fd_transfer.h>
#include "ion.h"
/* use fdc (frame demo common) functions */
#include "../../../../../examples/frame/frame_demo_common.h"

#if 0
   When sendmsg with multiple fd, should store in dependent cmsghdr

   // https://datatracker.ietf.org/doc/html/draft-stevens-advanced-api-02
   |<--------------------------- msg_controllen -------------------------->|
   |                                                                       |
   |<----- ancillary data object ----->|<----- ancillary data object ----->|
   |<---------- CMSG_SPACE() --------->|<---------- CMSG_SPACE() --------->|
   |                                   |                                   |
   |<---------- cmsg_len ---------->|  |<--------- cmsg_len ----------->|  |
   |<--------- CMSG_LEN() --------->|  |<-------- CMSG_LEN() ---------->|  |
   |                                |  |                                |  |
   +-----+-----+-----+--+-----------+--+-----+-----+-----+--+-----------+--+
   |cmsg_|cmsg_|cmsg_|XX|           |XX|cmsg_|cmsg_|cmsg_|XX|           |XX|
   |len  |level|type |XX|cmsg_data[]|XX|len  |level|type |XX|cmsg_data[]|XX|
   +-----+-----+-----+--+-----------+--+-----+-----+-----+--+-----------+--+
#endif

static const char *TEST_FILENAME = "ion_test.txt";

int main(int argc, char **argv)
{
	int ret = -1;
	int socket_fd;

	LOG_D("Starting producer\n");

	/* Check arg count */
	if (argc != 2) {
		LOG_E("useg:<unix_domain socket>\n");
		goto LAB_EXIT;
	}

	/* Check socket_fd */
	socket_fd = atoi(argv[1]);
	if (socket_fd <= 0)  {
		LOG_E("socket_fd(%d) is invalid\n", socket_fd);
		goto LAB_EXIT;
	}
	LOG_D("The input socket_fd=%d\n", socket_fd);

	long file_size;		/* Test file size */
	int file_fd;		/* Test file file descriptor */
	int ion_fd;		/* fd of /dev/ion */
	int dmabuf_fd;		/* dma-buf fd to transfer to consumer */
	void *dmabuf_addr;	/* dma-buf mmap to user addr */

	/* alloc ion dma-buf */
	if (fdc_get_img_file_size(&file_size, TEST_FILENAME) != 0)
		goto LAB_ERR_SOCKET_FD_OPENED;

	// open ion device
	if (fdc_get_ion_device_fd(&ion_fd) != 0)
		goto LAB_ERR_SOCKET_FD_OPENED;

	// alloc dma-buf from ion device
	struct ion_allocation_data alloc_data;
	if (fdc_alloc_ion_buf(&alloc_data, ion_fd, file_size) != 0)
		goto LAB_ERR_ION_OPENED;
	dmabuf_fd = alloc_data.fd;

	// map dma-buf to user addr
	if (fdc_mmap_dmabuf_addr(&dmabuf_addr, file_size, dmabuf_fd) != 0)
		goto LAB_ERR_BUF_ALLOCED;

	// open file
	file_fd = open(TEST_FILENAME, O_RDONLY);
	if (file_fd < 0) {
		LOG_E("Open test file '%s' failed, %s\n",
		      TEST_FILENAME, strerror(errno));
		goto LAB_ERR_MMAPPED;
	}

	// store test file content into dma-buf
	if (fdc_store_img_to_dmabuf(dmabuf_addr, file_fd, file_size) != 0)
		goto LAB_ERR_TEST_FILE_OPENED;

	struct msghdr msghdr_send;			// the info struct
	struct cmsghdr *pCmsghdr = NULL; 		// the pointer of control

	/* create 2 cmsg space, must add "= {}" */
	char ctrls[CMSG_SPACE(sizeof(int)) + CMSG_SPACE(sizeof(int))] = {};
	msghdr_send.msg_control = ctrls;
	msghdr_send.msg_controllen = sizeof(ctrls);
	LOG_D("msg_controllen = %zd\n", msghdr_send.msg_controllen);

	// the first info stores dma-buf fd
	pCmsghdr = CMSG_FIRSTHDR(&msghdr_send);		// the info of head
	pCmsghdr->cmsg_len = CMSG_LEN(sizeof(int));     // the msg len
	pCmsghdr->cmsg_level = SOL_SOCKET; 		// stream mode
	pCmsghdr->cmsg_type = SCM_RIGHTS;  		// file descriptor

	int *fdptr = (int *)CMSG_DATA(pCmsghdr);
	*fdptr = dmabuf_fd;	// data: dma-buf fd !!!!!
	LOG_I("sendmsg() with dmabuf_fd=%d\n", dmabuf_fd);

	// the second info stores test file fd
	pCmsghdr = CMSG_NXTHDR(&msghdr_send, pCmsghdr);
	pCmsghdr->cmsg_len = CMSG_LEN(sizeof(int));     // the msg len
	pCmsghdr->cmsg_level = SOL_SOCKET; 		// stream mode
	pCmsghdr->cmsg_type = SCM_RIGHTS;  		// file descriptor
	lseek(file_fd, 0, SEEK_SET);			// seek to file head
	*((int *)CMSG_DATA(pCmsghdr)) = file_fd;	// data: test file fd !!!!!
	LOG_I("sendmsg() with file_fd=%d\n", file_fd);

	LOG_F("ctrls in hex mode:\n");
	LOG_F("        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	LOG_F("  -----------------------------------------------------\n");
	for (int i = 0; i < sizeof(ctrls); i++) {
		if (i % 16 == 0)
			LOG_F("  %04x: ", i);
		LOG_F("%02x ", ctrls[i]);
		if (i % 16== 15)
			printf("\n");
	}
	LOG_F("\n");

	// these infos are nosignification
	msghdr_send.msg_name = NULL;			// the name
	msghdr_send.msg_namelen = 0;		        // len of name

	struct iovec iov[1];				// io vector
	long dmabuf_len = file_size;
	iov[0].iov_base = &dmabuf_len;			// no data here
	iov[0].iov_len = sizeof(dmabuf_len);		// the len of data

	msghdr_send.msg_iov = iov;			// the io/vector info
	msghdr_send.msg_iovlen = 1;			// the num of iov

	ssize_t size = sendmsg(socket_fd, &msghdr_send, 0);	// send msg now
	if (size < 0) {
		LOG_E("sendmsg() failed, errno=%d, %s\n", errno, strerror(errno));
	} else if (size == 0) {
		LOG_W("sendmsg() %zd bytes\n", size);
	} else {
		LOG_D("sendmsg() %zd bytes\n", size);
	}

	ret = 0;

LAB_ERR_TEST_FILE_OPENED:
	ret = close(file_fd);
	LOG_D("close(file_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_MMAPPED:
	ret = munmap(dmabuf_addr, file_size);
	LOG_D("munmap() ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_BUF_ALLOCED:
	ret = close(dmabuf_fd);
	LOG_D("close(dmabuf_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_ION_OPENED:
	close(ion_fd);
	LOG_D("close(ion_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_ERR_SOCKET_FD_OPENED:
	ret = close(socket_fd);
	LOG_D("close(socket_fd) ret=%d, %s\n", ret, strerror(errno));

LAB_EXIT:
	if (ret)
		LOG_E("Exiting producer\n");
	else
		LOG_D("Exiting producer\n");
	exit(ret);
}

