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
#define LOG_PREFIX "master"
#include <syslog.h>

#include "ipc_fd_transfer.h"


int main(int argc, char *argv[])
{
	int ret;
	int socket_pair[2]; 	/* sockpair() */
	char str_sockfd[10];	/* store the string format */
	pid_t producer_pid;
	pid_t consumer_pid;
	int process_status;	// the status of child process

	/* Create socket pare */
	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_pair) == -1) {	/* must be AF_LOCAL or AF_UNIX !!!!*/
		LOG_E("create socketpair error(%d), %s\n", errno, strerror(errno));
		exit(1);
	}
	LOG_I("Create socketpair OK:{%d, %d}\n", socket_pair[0], socket_pair[1]);

	/* run producer process, using socket_pair[0] */
	if ((producer_pid = fork()) == 0) {

		LOG_D("launch producer process\n");
		close(socket_pair[1]); // producer only use socket_pair[0]
		snprintf(str_sockfd, sizeof(str_sockfd), "%d", socket_pair[0]);

		execl("./producer", "producer", str_sockfd, (char *)NULL);
		exit(0);
	}

	/* run consumer process, using socket_pair[1] */
	if ((consumer_pid = fork()) == 0) {

		LOG_D("launch consumer process\n");
		close(socket_pair[0]); // consumer only use socket_pair[1]
		snprintf(str_sockfd, sizeof(str_sockfd), "%d", socket_pair[1]);

		execl("./consumer", "consumer", str_sockfd, (char *)NULL);
		exit(0);
	}

	// wait the child process
	waitpid(producer_pid, &process_status, 0);
	if (WIFEXITED(process_status) == 0) {
		LOG_E("producer eixt abnormal\n");
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

	LOG_I("Exit master\n");
	return 0;
}

