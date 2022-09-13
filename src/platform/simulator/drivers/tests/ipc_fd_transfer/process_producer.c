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

#define TEST_FILENAME "test.txt"

#define LOG_LEVEL 3
#define LOG_PREFIX "producer"
#include <syslog.h>

#include "ipc_fd_transfer.h"

int main(int argc, char **argv)
{
	int ret;
	int socket_fd;
	int file_fd;

	LOG_D("Starting producer\n");

	/* Check arg count */
	if (argc != 2) {
		LOG_E("useg:<unix_domain socket>\n");
		exit(EXIT_FAILURE);
	}

	/* Check socket_fd */
	socket_fd = atoi(argv[1]);
	if (socket_fd <= 0)  {
		LOG_E("socket_fd(%d) is invalid\n", socket_fd);
		exit(1);
	}
	LOG_D("The input socket_fd=%d\n", socket_fd);

	/* open test file */
	file_fd = open(TEST_FILENAME, O_RDONLY);
	if (file_fd < 0) {
		LOG_E("Open test file:'%s' failed, errno=%d, %s",
		      TEST_FILENAME, errno, strerror(errno));
		exit(1);
	}
	LOG_D("The file_fd=%d\n", file_fd);

	/* send test file fd */
	struct msghdr msghdr_send;			// the info struct
	struct cmsghdr *pCmsghdr = NULL; 		// the pointer of control

	msghdr_send.msg_control = ctl_un.ctl;
	msghdr_send.msg_controllen = sizeof(ctl_un.ctl);

	// the first info
	pCmsghdr = CMSG_FIRSTHDR(&msghdr_send);		// the info of head
	pCmsghdr->cmsg_len = CMSG_LEN(sizeof(int));     // the msg len
	pCmsghdr->cmsg_level = SOL_SOCKET; 		// stream mode
	pCmsghdr->cmsg_type = SCM_RIGHTS;  		// to file descriptor !!!!
	*((int *)CMSG_DATA(pCmsghdr)) = file_fd;	// data: the file fd

	// these infos are nosignification
	msghdr_send.msg_name = NULL;			// the name of protocol address
	msghdr_send.msg_namelen = 0;		        // len of name


	struct iovec iov[2];				// io vector
	char iov_data0[16] = "This's iov";		// 16 byts string
	char iov_data1[25] = "This's an other iov_data";// 25 byts string
	iov[0].iov_base = iov_data0;			//
	iov[0].iov_len = sizeof(iov_data0);		// the len of data
	iov[1].iov_base = iov_data1;			//
	iov[1].iov_len = sizeof(iov_data1);		// the len of data

	msghdr_send.msg_iov = iov;			// the io/vector info
	msghdr_send.msg_iovlen = 2;			// the num of iov

	ssize_t size = sendmsg(socket_fd, &msghdr_send, 0);	// send msg now
	if (size < 0) {
		LOG_E("sendmsg() failed, errno=%d, %s\n", errno, strerror(errno));
	} else if (size == 0) {
		LOG_W("sendmsg() %zd bytes\n", size);
	} else {
		LOG_D("sendmsg() %zd bytes\n", size);
	}

	ret = close(file_fd);
	LOG_D("close(file_fd) ret=%d, %s\n", errno, strerror(errno));

	ret = close(socket_fd);
	LOG_D("close(socket_fd) ret=%d, %s\n", errno, strerror(errno));

	LOG_D("Exiting producer\n");
	return 0;
}

