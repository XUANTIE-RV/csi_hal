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
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "consumer"
#include <syslog.h>

#include "ipc_fd_transfer.h"

int main(int argc, char **argv)
{
	int ret;
	int socket_fd;
	int file_fd;

	LOG_D("Starting consumer\n");

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

	struct cmsghdr *pCmsghdr = NULL;			// the pointer of control
	msghdr_recv.msg_control = ctl_un.ctl;
	msghdr_recv.msg_controllen = sizeof(ctl_un.ctl);

	// these infos are no signification
	msghdr_recv.msg_name = NULL;				// the name of protocol address
	msghdr_recv.msg_namelen = 0;				// len of name

	struct iovec iov[1];					// io vector
	char iov_data[128];
	iov[0].iov_base = &iov_data;
	iov[0].iov_len = sizeof(iov_data);

	msghdr_recv.msg_iov = iov;				// the io/vector info
	msghdr_recv.msg_iovlen = 1;				// the num of iov

	ssize_t	recv_len;
	if ((recv_len = recvmsg(socket_fd, &msghdr_recv, 0)) < 0) {	// recv msg
		// the msg is recv by msghdr_recv
		LOG_E("recv error:%d, %s\n", errno, strerror(errno));
		goto LAB_ERROR;
	}
	LOG_D("recvmsg() length=%zd, dump below:\n", recv_len);
	if (recv_len > 0) {
		int i;
		LOG_F("msg_iov in string mode:\n");
		LOG_F("        0123456789ABCDEF\n");
		LOG_F("  ----------------------\n");
		for (i = 0; i < recv_len; i++) {
			if (i % 16 == 0)
				LOG_F("  %04x: ", i);
			LOG_F("%c", iov_data[i]);
			if (i % 16 == 15)
				LOG_F("\n");
		}
		LOG_F("\n");

		LOG_F("msg_iov in hex mode:\n");
		LOG_F("        00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
		LOG_F("  -----------------------------------------------------\n");
		for (i = 0; i < recv_len; i++) {
			if (i % 16 == 0)
				LOG_F("  %04x: ", i);
			LOG_F("%02x ", iov_data[i]);
			if (i % 16== 15)
				printf("\n");
		}
		LOG_F("\n");
	}

	if ((pCmsghdr = CMSG_FIRSTHDR(&msghdr_recv)) != NULL &&	// now we need only one,
	     pCmsghdr->cmsg_len == CMSG_LEN(sizeof(int))) {	// we should use 'for' when
		// there are many fds
		if (pCmsghdr->cmsg_level != SOL_SOCKET) {
			LOG_E("Ctl level should be SOL_SOCKET\n");
			goto LAB_ERROR;
		}

		if (pCmsghdr->cmsg_type != SCM_RIGHTS) {
			LOG_E("Ctl type should be SCM_RIGHTS\n");
			goto LAB_ERROR;
		}

		file_fd = *((int *)CMSG_DATA(pCmsghdr));	// get the data : the file des*
	} else {
		if (pCmsghdr == NULL)
			LOG_E("pCmsghdr=%p\n", pCmsghdr);
		else
			LOG_E("pCmsghdr->cmsg_len=%zu\n", pCmsghdr->cmsg_len);

		file_fd = -1;
		goto LAB_ERROR;
	}
	LOG_D("receive file_fd=%d\n", file_fd);

	int n_read;
	char buf[1024];
	LOG_I("Dump test file content in consumer begin vvvv\n");
	while ((n_read = read(file_fd, buf, 1024)) > 0) {	// read from fd
		write(1, buf, n_read);				// write to std_out
	}
	LOG_I("Dump test file content in consumer end   ^^^^\n");

	ret = close(file_fd);
	LOG_D("close(file_fd) ret=%d, %s\n", errno, strerror(errno));

	ret = close(socket_fd);
	LOG_D("close(socket_fd) ret=%d, %s\n", errno, strerror(errno));

	LOG_D("Exiting consumer\n");
	return 0;

LAB_ERROR:
	LOG_E("Exiting consumer with error\n");
	exit(EXIT_FAILURE);
}

