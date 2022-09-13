/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: fuqian.zxr <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define LOG_LEVEL 3
#define LOG_PREFIX "platform_action"
#include <syslog.h>
#include "platform_action.h"
#include "camera_utilities.h"

#include <stdio.h>

int camera_action_image_save(csi_frame_s *frame)
{
	int ret;
	if (frame == NULL) {
		LOG_E("pass error parameter!\n");
		ret = -1;
	}
	switch (frame->img.pix_format) {
	case CSI_PIX_FMT_I420:
		if (image_save_opencv_i420(frame->img.usr_addr[0], frame->img.height,
					   frame->img.width)) {
			LOG_E("save picture error!\n");
			ret = -1;
		}
		break;
	case CSI_PIX_FMT_NV12:
		if (image_save_opencv_nv12(frame->img.usr_addr[0], frame->img.height,
					   frame->img.width)) {
			LOG_E("save picture error!\n");
			ret = -1;
		}
		break;
	case CSI_PIX_FMT_BGR:
		if (image_save_opencv_bgr(frame->img.usr_addr[0], frame->img.height,
					  frame->img.width)) {
			LOG_E("save picture error!\n");
			ret = -1;
		}
		break;
	default:
		LOG_E("Uknown format!\n");
		ret = -1;
	}
	return ret;
}
int camera_action_image_display(csi_frame_s *frame)
{
	int ret;
	if (frame == NULL) {
		LOG_E("pass error parameter!\n");
		ret = -1;
	}
	switch (frame->img.pix_format) {
	case CSI_PIX_FMT_I420:
		if (image_display_opencv_i420(frame->img.usr_addr[0], frame->img.height,
					      frame->img.width)) {
			LOG_E("display picture error!\n");
			ret = -1;
		}
		break;
	case CSI_PIX_FMT_NV12:
		if (image_display_opencv_nv12(frame->img.usr_addr[0], frame->img.height,
					      frame->img.width)) {
			LOG_E("display picture error!\n");
			ret = -1;
		}
		break;
	case CSI_PIX_FMT_BGR:
		if (image_display_opencv_bgr(frame->img.usr_addr[0], frame->img.height,
					     frame->img.width)) {
			LOG_E("display picture error!\n");
			ret = -1;
		}
		break;
	default:
		LOG_E("Uknown format!\n");
		ret = -1;
	}
	return ret;
}
