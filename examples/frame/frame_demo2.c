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

#define LOG_LEVEL 2
#define LOG_PREFIX "master"
#include <syslog.h>

#include <ion.h>
#include <csi_frame.h>
#include <csi_camera_frame.h>
#include "frame_demo_common.h"

const char *img_name = "../resource/yuv420_1280x720_csky2016.yuv";
csi_img_format_t img_format = {
	.width = 1280,
	.height = 720,
	.pix_fmt = CSI_PIX_FMT_I420,
};

extern int producer_process(int socket_fd);
extern int consumer_process(int socket_fd);

int main(int argc, char *argv[])
{
	int ret;
	int socket_pair[2]; 	/* sockpair() */
	pid_t producer_pid;
	pid_t consumer_pid;
	int process_status;	/* the status of child process */

	char str_sockfd[10];	/* store the string format */

	/* Create socket pare */
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0,
		       socket_pair) == -1) {	/* must be AF_LOCAL or AF_UNIX !!!!*/
		LOG_E("create socketpair error(%d), %s\n", errno, strerror(errno));
		exit(1);
	}
	LOG_I("Create socketpair OK:{%d, %d}\n", socket_pair[0], socket_pair[1]);

	/* run producer process, using socket_pair[0] */
	if ((producer_pid = fork()) == 0) {
		/* child process2: producer_process */
		LOG_D("producer process starting ...\n");
		close(socket_pair[1]); // producer only use socket_pair[0]

		producer_process(socket_pair[0]);
		LOG_D("producer process exiting ...\n");
		exit(0);
	}

	/* run consumer process, using socket_pair[1] */
	if ((consumer_pid = fork()) == 0) {
		/* child process1: consumer_process */
		LOG_D("consumer process starting...\n");
		close(socket_pair[0]); // producer only use socket_pair[1]

		consumer_process(socket_pair[1]);
		LOG_D("consumer process exiting...\n");
		exit(0);
	}

	/* wait the children process end */
	waitpid(producer_pid, &process_status, 0);
	if (WIFEXITED(process_status) == 0) {
		LOG_E("producer exit abnormal\n");
	}

	waitpid(consumer_pid, &process_status, 0);
	if (WIFEXITED(process_status) == 0) {
		LOG_E("consumer eixt abnormal\n");
	}

	/* close the socket_pair */
	ret = close(socket_pair[0]);
	LOG_D("close(socket_pair[0]) ret=%d, %s\n", errno, strerror(errno));

	ret = close(socket_pair[1]);
	LOG_D("close(socket_pair[1]) ret=%d, %s\n", errno, strerror(errno));

	LOG_D("Exit master\n");
	return 0;
}

