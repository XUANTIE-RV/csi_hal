/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <csi_camera_property.h>
#include <camera_string.h>

const char *camera_string_enum_name(int property_id, int enum_id)
{
	switch(property_id) {
	case CSI_CAMERA_PID_EXPOSURE_MODE:
		switch (enum_id) {
		case CSI_CAMERA_EXPOSURE_MODE_AUTO:
			return "Auto";
		case CSI_CAMERA_EXPOSURE_MANUAL:
			return "Manual";
		case CSI_CAMERA_EXPOSURE_SHUTTER_PRIORITY:
			return "Shutter Priority";
		case CSI_CAMERA_EXPOSURE_APERTURE_PRIORITY:
			return "Aperture Priority";
		default:
			return "Unknown";
		}
	default:
		return  "Unknown";
	}
}

const char *camera_string_bitmask_name(int property_id, int bitmask)
{
	switch(property_id) {
	case CSI_CAMERA_PID_3A_LOCK:
		switch (bitmask) {
		case CSI_CAMERA_LOCK_EXPOSURE:
			return "Lock Exposure";
		case CSI_CAMERA_LOCK_WHITE_BALANCE:
			return "Lock White Balance";
		case CSI_CAMERA_LOCK_FOCUS:
			return "Lock Focus";
		default:
			return "Unknown";
		}
	default:
		return  "Unknown";
	}
}

const char *camera_string_capture_type(csi_camera_channel_capture_type_e type, bool accept_unknown)
{
	switch (type) {
	case CSI_CAMERA_CHANNEL_CAPTURE_VIDEO:
		return "Video";
	case CSI_CAMERA_CHANNEL_CAPTURE_META:
		return "Meta";
	default:
		if (accept_unknown)
			return "Unknown";
		else
			return "";
	}
}

const char *camera_string_chn_capture_types(int fields, char *fields_string)
{
	fields_string[0] = '\0';
	bool has_item = false;

	for (uint32_t i = 1; i < 0x80000000; i = i << 1) {
		const char *item_string = camera_string_capture_type(i & fields, false);
		if (strlen(item_string) > 0) {
			if (has_item)
				strcat(fields_string, "|");
			strcat(fields_string, item_string);

			has_item = true;
		}
	}
	return fields_string;
}

const char *camera_string_pixel_format(csi_pixel_fmt_e pix_fmt)
{
	switch (pix_fmt) {
	case CSI_PIX_FMT_I420:
		return "I420";
	case CSI_PIX_FMT_NV12:
		return "NV12";
	case CSI_PIX_FMT_BGR:
		return "BGR";
	default:
		return "Unknown";
	}
}

const char *camera_string_img_type(csi_img_type_e img_type)
{
	switch (img_type) {
	case CSI_IMG_TYPE_DMA_BUF:
		return "dma-buf";
	case CSI_IMG_TYPE_SYSTEM_CONTIG:
		return "system-contig";
	case CSI_IMG_TYPE_CARVEOUT:
		return "carveout";
	case CSI_IMG_TYPE_UMALLOC:
		return "user-malloc";
	case CSI_IMG_TYPE_SHM:
		return "shared-memory";
	default:
		return "Unknown";
	}
}

const char *camera_string_meta_field(csi_camera_meta_id_e type, bool accept_unknown)
{
	switch (type) {
	case CSI_CAMERA_META_ID_CAMERA_NAME:
		return "Cam_Name";
	case CSI_CAMERA_META_ID_CHANNEL_ID:
		return "Chn_ID";
	case CSI_CAMERA_META_ID_FRAME_ID:
		return "Frm_ID";
	case CSI_CAMERA_META_ID_TIMESTAMP:
		return "Timestamp";
	case CSI_CAMERA_META_ID_HDR:
		return "Is_HDR";
	default:
		if (accept_unknown)
			return "Unknown";
		else
			return "";
	}
}

const char *camera_string_chn_meta_fields(int fields, char *fields_string)
{
	fields_string[0] = '\0';
	bool has_item = false;

	for (uint32_t i = 1; i < 0x80000000; i = i << 1) {
		const char *item_string = camera_string_meta_field(i & fields, false);
		if (strlen(item_string) > 0) {
			if (has_item)
				strcat(fields_string, "|");
			strcat(fields_string, item_string);

			has_item = true;
		}
	}
	return fields_string;
}

const char *camera_string_chn_status(csi_img_type_e img_type)
{
	switch (img_type) {
	case CSI_CAMERA_CHANNEL_INVALID:
		return "invalid";
	case CSI_CAMERA_CHANNEL_CLOSED:
		return "closed";
	case CSI_CAMERA_CHANNEL_OPENED:
		return "opened";
	case CSI_CAMERA_CHANNEL_RUNNING:
		return "running";
	default:
		return "Unknown";
	}
}

const char *camera_string_camera_event_type(csi_camera_event_id_e event_id)
{
	switch (event_id) {
	case CSI_CAMERA_EVENT_WARNING:
		return "Event Warning";
	case CSI_CAMERA_EVENT_ERROR:
		return "Event Error";
	case CSI_CAMERA_EVENT_SENSOR_FIRST_IMAGE_ARRIVE:
		return "Sensor 1st Frame";
	case CSI_CAMERA_EVENT_ISP_3A_ADJUST_READY:
		return "3A Adjust Ready";
	default:
		return "Unknown";
	}
}

const char *camera_string_channel_event_type(csi_camera_channel_event_id_e event_id)
{
	switch (event_id) {
	case CSI_CAMERA_CHANNEL_EVENT_FRAME_READY:
		return "Frame Ready";
	case CSI_CAMERA_CHANNEL_EVENT_FRAME_PUT:
		return "Frame put back";
	case CSI_CAMERA_CHANNEL_EVENT_OVERFLOW:
		return "Frame overflow";
	default:
		return "Unknown";
	}
}

const char *camera_string_camera_action(camera_action_e action)
{
	switch (action) {
	case CAMERA_ACTION_NONE:
		return "Do nothing";
	case CAMERA_ACTION_LOG_PRINT:
		return "Log print";
	case CAMERA_ACTION_CAPTURE_FRAME:
		return "Capture Frame";
	default:
		return "Unknown";
	}
}
const char *camera_string_channel_action(camera_channel_action_e action)
{
	switch (action) {
	case CAMERA_CHANNEL_ACTION_NONE:
		return "Do nothing";
	case CAMERA_CHANNEL_ACTION_LOG_PRINT:
		return "Log print";
	case CAMERA_CHANNEL_ACTION_CAPTURE_FRAME:
		return "Capture Frame";
	case CAMERA_CHANNEL_ACTION_DISPLAY_FRAME:
		return "Display Frame";
	default:
		return "Unknown";
	}
}

