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

#define LOG_LEVEL 2
#define LOG_PREFIX "test1"
#include <syslog.h>

#include <csi_frame.h>
#include <csi_camera.h>

#define TEST_DEVICE_NAME "/dev/video"
int main(int argc, char *argv[])
{
	LOG_I("This is camera_test1\n");
	return 0;
}

