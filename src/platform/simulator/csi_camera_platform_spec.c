/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <csi_camera_platform_spec.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "dlg_cam_enums"
#include <syslog.h>

/******************************************************************************/
/*********** Platform Spec in Enum ********************************************/
/******************************************************************************/

/* The enums platform supports for Property ID: CSI_CAMERA_PID_EXPOSURE_MODE */
const camera_spec_enums_s camera_spec_camera_exposure_modes[] = {
	4,
	{
		CSI_CAMERA_EXPOSURE_MODE_AUTO,
		CSI_CAMERA_EXPOSURE_MANUAL,
		CSI_CAMERA_EXPOSURE_SHUTTER_PRIORITY,
		CSI_CAMERA_EXPOSURE_APERTURE_PRIORITY
	}
};

/* The enums platform supports for Channel pix_fmt */
const camera_spec_enums_s camera_spec_chn_pix_fmt[] = {
	3,
	{
		CSI_PIX_FMT_I420,
		CSI_PIX_FMT_NV12,
		CSI_PIX_FMT_BGR,
	}
};

/* The enums platform supports for Channel pix_fmt */
const camera_spec_enums_s camera_spec_chn_img_type[] = {
	2,
	{
		CSI_IMG_TYPE_DMA_BUF,
		CSI_IMG_TYPE_SYSTEM_CONTIG,
	}
};

/* The enums platform supports for camera event subscribe */
const camera_spec_enums_s camera_spec_camera_event_type[] = {
	4,
	{
		CSI_CAMERA_EVENT_WARNING,
		CSI_CAMERA_EVENT_ERROR,
		CSI_CAMERA_EVENT_SENSOR_FIRST_IMAGE_ARRIVE,
		CSI_CAMERA_EVENT_ISP_3A_ADJUST_READY,
	}
};

/* The enums platform supports for camera channel event subscribe */
const camera_spec_enums_s camera_spec_channel_event_type[] = {
	3,
	{
		CSI_CAMERA_CHANNEL_EVENT_FRAME_READY,
		CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT,
		CSI_CAMERA_CHANNEL_EVENT_OVERFLOW,
	}
};

const camera_spec_enums_s *camera_spec_get_enum_array(int property_id)
{
	switch(property_id) {
	case CAMERA_SPEC_ENUM_CAMERA_EXPOSURE_MODES:
		return camera_spec_camera_exposure_modes;
	case CAMERA_SPEC_ENUM_CAMERA_EVENT_TYPES:
		return camera_spec_camera_event_type;
	case CAMERA_SPEC_ENUM_CHANNEL_PIX_FMT:
		return camera_spec_chn_pix_fmt;
	case CAMERA_SPEC_ENUM_CHANNEL_IMG_TYPE:
		return camera_spec_chn_img_type;
	case CAMERA_SPEC_ENUM_CHANNEL_EVENT_TYPES:
		return camera_spec_channel_event_type;
	default:
		LOG_E("Unknown property_id:%d\n", property_id);
		return NULL;
	}
}


/******************************************************************************/
/*********** Platform Spec in Bitmask *****************************************/
/******************************************************************************/

/* The enums platform supports for Property ID: CSI_CAMERA_PID_EXPOSURE_MODE */
const camera_spec_bitmasks_t camera_spec_3a_lock[] = {
	3,
	{
		CSI_CAMERA_LOCK_EXPOSURE,
		CSI_CAMERA_LOCK_WHITE_BALANCE,
		CSI_CAMERA_LOCK_FOCUS,
	}
};

/* The enums platform supports for Property ID: CSI_CAMERA_PID_EXPOSURE_MODE */
const camera_spec_bitmasks_t channel_spec_support_capture_type[] = {
	2,
	{
		CSI_CAMERA_CHANNEL_CAPTURE_VIDEO,
		CSI_CAMERA_CHANNEL_CAPTURE_META,
	}
};

/* The enums platform supports for Property ID: CSI_CAMERA_PID_EXPOSURE_MODE */
const camera_spec_bitmasks_t channel_spec_support_meta_type[] = {
	5,
	{
		CSI_CAMERA_META_ID_CAMERA_NAME,
		CSI_CAMERA_META_ID_CHANNEL_ID,
		CSI_CAMERA_META_ID_FRAME_ID,
		CSI_CAMERA_META_ID_TIMESTAMP,
		CSI_CAMERA_META_ID_HDR,
	}
};

const camera_spec_bitmasks_t *camera_spec_get_bitmask_array(int property_id)
{
	switch(property_id) {
	case CAMERA_SPEC_BITMAKS_CAMERA_3A_LOCK:
		return camera_spec_3a_lock;
	case CAMERA_SPEC_BITMAKS_CHANNEL_CAPTURE_TYPE:
		return channel_spec_support_capture_type;
	case CAMERA_SPEC_BITMAKS_CHANNEL_META_TYPE:
		return channel_spec_support_meta_type;
	default:
		LOG_E("Unknown property_id:%d\n", property_id);
		return NULL;
	}
}


