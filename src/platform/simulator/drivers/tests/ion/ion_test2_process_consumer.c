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

#include <ipc_fd_transfer.h>
#include "ion.h"

#define LOG_LEVEL 3
#define LOG_PREFIX "consumer"
#include <syslog.h>

#if 0
   Even there are multiple fd sent by sendmsg, only one cmsghdr will be received

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
   |cmsg_|cmsg_|cmsg_|XX|           |XX|
   |len  |level|type |XX|cmsg_data[]|XX|
   +-----+-----+-----+--+-----------+--+
                            ^ There are multi-fd in same cmsg_data[]
#endif

int main(int argc, char **argv)
{
	int ret;
	int socket_fd = -1;
	int dmabuf_fd = -1;
	int file_fd = -1;

	LOG_D("Starting consumer\n");
	sleep(1); 	// waiting for producer exit to watch log easily

	/* Check arg count */
	if (argc != 2) {
		LOG_E("useg:<unix_domain socket>\n");
		goto LAB_ERROR;
	}

	/* Check socket_fd */
	socket_fd = atoi(argv[1]);
	if (socket_fd <= 0)  {
		LOG_E("socket_fd(%d) is invalid\n", socket_fd);
		goto LAB_ERROR;
	}
	LOG_D("The input socket_fd=%d\n", socket_fd);

	struct msghdr 	msghdr_recv;				// the info struct
	struct iovec	iov[1];					// io vector

	struct cmsghdr *pCmsghdr = NULL;			// the pointer of control

	char ctls[CMSG_SPACE(sizeof(int))][2] = {};
	msghdr_recv.msg_control = ctls;
	msghdr_recv.msg_controllen = sizeof(ctls);

	msghdr_recv.msg_name = NULL;				// the name
	msghdr_recv.msg_namelen = 0;				// len of name

	/* transfer dmabuf_len in iov */
	long dmabuf_len;
	iov[0].iov_base = &dmabuf_len;
	iov[0].iov_len = sizeof(dmabuf_len);

	msghdr_recv.msg_iov = iov;				// the io/vector info
	msghdr_recv.msg_iovlen = 1;				// the num of iov

	ssize_t	recv_len;
	if ((recv_len = recvmsg(socket_fd, &msghdr_recv, 0)) < 0) {	// recv msg
		// the msg is recv by msghdr_recv
		LOG_E("recv error:%d, %s\n", errno, strerror(errno));
		goto LAB_ERROR;
	}
	LOG_D("recvmsg() length=%zd\n", recv_len);

	LOG_F("ctrls in hex mode:\n");
	LOG_F("        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	LOG_F("  -----------------------------------------------------\n");
	char *pctrl = &ctls[0][0];
	for (int i = 0; i < sizeof(ctls); i++) {
		if (i % 16 == 0)
			LOG_F("  %04x: ", i);
		LOG_F("%02x ", pctrl[i]);
		if (i % 16== 15)
			printf("\n");
	}
	LOG_F("\n");

	int msghdr_count = 0;
	for (pCmsghdr = CMSG_FIRSTHDR(&msghdr_recv); pCmsghdr != NULL;
	     pCmsghdr = CMSG_NXTHDR(&msghdr_recv,pCmsghdr)) {
		msghdr_count++;
	}
	LOG_D("msghdr_count=%d\n", msghdr_count);

	if ((pCmsghdr = CMSG_FIRSTHDR(&msghdr_recv)) != NULL) {
		if (pCmsghdr->cmsg_level != SOL_SOCKET) {
			LOG_E("Ctl level should be SOL_SOCKET\n");
			goto LAB_ERROR;
		}

		if (pCmsghdr->cmsg_type != SCM_RIGHTS) {
			LOG_E("Ctl type should be SCM_RIGHTS\n");
			goto LAB_ERROR;
		}

		LOG_D("pCmsghdr->cmsg_len=%zd\n", pCmsghdr->cmsg_len);
		int *fdptr = (int *) CMSG_DATA(pCmsghdr);
		dmabuf_fd = fdptr[0];
		file_fd = fdptr[1];
	} else {
		if (pCmsghdr == NULL)
			LOG_E("pCmsghdr=%p\n", pCmsghdr);
		else
			LOG_E("pCmsghdr->cmsg_len=%zu, should=%zu\n",
				pCmsghdr->cmsg_len, CMSG_LEN(sizeof(int)));

		goto LAB_ERROR;
	}
	LOG_I("recvmsg() with dmabuf_fd=%d, file_fd=%d\n", dmabuf_fd, file_fd);

	LOG_D("recvmsg() dmabuf_len=%ld\n", dmabuf_len);
	void *buf = mmap(NULL, dmabuf_len, PROT_READ | PROT_WRITE,
			 MAP_SHARED, dmabuf_fd, 0);
	if (buf != NULL && buf != MAP_FAILED) {
		char str_out[448+1];
		snprintf(str_out, sizeof(str_out), "%s", (char*)buf);
		LOG_I("Dump dma-buf content in consumer begin vvvv\n");
		LOG_F("%s", str_out);
		LOG_I("Dump dma-buf content in consumer end   ^^^^\n");
		munmap(buf, dmabuf_len);
		buf = NULL;
	} else {
		LOG_E("mmap failed, buf=%p, %s\n", buf, strerror(errno));
	}

	LOG_I("Please watching dmesg, 'dmabuf release' will appear in 3 seconds\n");
	sleep(3);

	ret = close(dmabuf_fd);
	LOG_D("close(dmabuf_fd) ret=%d, %s\n", errno, strerror(errno));

	int n_read;
	char buf_file[1024];
	LOG_I("Dump test file content in consumer begin vvvv\n");
	lseek(file_fd, 0, SEEK_SET);

	while ((n_read = read(file_fd, buf_file, 1024)) > 0) {	// read from fd
		write(1, buf_file, n_read);			// write to std_out
	}
	LOG_I("Dump test file content in consumer end   ^^^^\n");

	ret = close(file_fd);
	LOG_D("close(file_fd) ret=%d, %s\n", errno, strerror(errno));

	ret = close(socket_fd);
	LOG_D("close(socket_fd) ret=%d, %s\n", errno, strerror(errno));

	LOG_D("Exiting consumer\n");
	return 0;

LAB_ERROR:
	if (buf != NULL) {
		ret = munmap(buf, dmabuf_len);
		LOG_D("munmap() ret=%d, %s\n", ret, strerror(errno));
	}

	LOG_E("Exiting consumer with error\n");
	exit(EXIT_FAILURE);
}

