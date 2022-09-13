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

#include "ion.h"

#define LOG_LEVEL 3
#define LOG_PREFIX "ion_test1"
#include <syslog.h>

#define PAGE_SIZE 4096
#define TEST_PAGE_COUNT (16*1024/PAGE_SIZE) /* To make shoure PAGE_SIZE aligned */

int main(int argc, char *argv[])
{
	int fd;
	int i,j;
	struct ion_allocation_data alloc_data;
	LOG_I("Starting\n");

	fd = open("/dev/ion", O_RDWR);
	if (fd < 0) {
		LOG_E("open /dev/ion failed, %s\n", strerror(errno));
		return -1;
	}

	alloc_data.len = TEST_PAGE_COUNT * PAGE_SIZE;
	if (ioctl(fd, ION_IOC_ALLOC, &alloc_data)) {
		LOG_E("ion ioctl failed, %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	LOG_F("ion alloc success: size = 0x%llx, dmabuf_fd = %u\n",
	       alloc_data.len, alloc_data.fd);

	void *buf = mmap(NULL, alloc_data.len, PROT_READ | PROT_WRITE,
			 MAP_SHARED, alloc_data.fd, 0);
	LOG_D("buf=%p\n", buf);
	if (buf != NULL && buf != MAP_FAILED) {
		for (i = 0; i < TEST_PAGE_COUNT; i++) {
			char *str_page = (char*)buf + PAGE_SIZE * i;
			str_page[0] = 'D';	// Try to re-write
			printf("Page header string: '%s'\n", str_page);
		}
		munmap(buf, alloc_data.len);
	} else {
		LOG_E("mmap failed, buf=%p, %s\n", buf, strerror(errno));
	}

	LOG_F("Close dma-buf fd\n");
	close(alloc_data.fd);

	close(fd);
	LOG_I("Exiting\n");
	return 0;
}

