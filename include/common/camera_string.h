/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CAMERA_STRING_H__
#define __CAMERA_STRING_H__

#include <stdio.h>

#include <string.h>
#include <csi_camera.h>
#include <camera_manager.h>

#ifdef  __cplusplus
extern "C" {
#endif

const char *camera_string_enum_name(int property_id, int enum_id);
const char *camera_string_bitmask_name(int property_id, int enum_id);
const char *camera_string_capture_type(csi_camera_channel_capture_type_e type, bool accept_unknown);
const char *camera_string_chn_capture_types(int fields, char *fields_string);
const char *camera_string_pixel_format(csi_pixel_fmt_e pix_fmt);
const char *camera_string_img_type(csi_img_type_e img_type);
const char *camera_string_meta_field(csi_camera_meta_id_e type, bool accept_unknown);
const char *camera_string_chn_meta_fields(int fields, char *fields_string);
const char *camera_string_chn_status(csi_img_type_e img_type);

const char *camera_string_camera_event_type(csi_camera_event_id_e event_id);
const char *camera_string_channel_event_type(csi_camera_channel_event_id_e event_id);
const char *camera_string_channel_action(camera_channel_action_e action);
const char *camera_string_camera_action(camera_action_e action);

#ifdef  __cplusplus
}
#endif

#endif  /* __CAMERA_STRING_H__ */
