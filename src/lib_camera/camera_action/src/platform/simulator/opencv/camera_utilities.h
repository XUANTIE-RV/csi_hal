/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: fuqian.zxr <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __CAMERA_UTILITIES_H__
#define __CAMERA_UTILITIES_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <csi_camera.h>

//wait for complete
int image_save_opencv_i420(void *mat, int height, int width);
int image_display_opencv_i420(void *mat, int height, int width);

int image_save_opencv_nv12(void *mat, int height, int width);
int image_display_opencv_nv12(void *mat, int height, int width);

int image_save_opencv_bgr(void *mat, int height, int width);
int image_display_opencv_bgr(void *mat, int height, int width);

#ifdef __cplusplus
};
#endif

#endif
