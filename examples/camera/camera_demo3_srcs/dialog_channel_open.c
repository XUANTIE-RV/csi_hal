/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <curses.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "dlg_prop_list"
#include <syslog.h>
#include <csi_camera_property.h>
#include <csi_camera_platform_spec.h>
#include <camera_manager.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

static int fill_channel_config_items(csi_camera_channel_cfg_s *channel)
{
	char str_buf[128];
	const char item_format[] = "%-14s";

	item_make(item_format, "capture_type");
	item_add_str(": (%s)  --->", camera_string_chn_capture_types(channel->capture_type, str_buf));
	item_set_data(channel);

	item_make(item_format, "Capture fields");
	item_add_str(": (%s)  --->", camera_string_chn_meta_fields(channel->meta_fields, str_buf));
	item_set_data(channel);

	item_make(item_format, "Buffer count");
	item_add_str(": (%d)  --->", channel->frm_cnt);
	item_set_data(channel);

	item_make(item_format, "Image width");
	item_add_str(": (%d)  --->", channel->img_fmt.width);
	item_set_data(channel);

	item_make(item_format, "Image height");
	item_add_str(": (%d)  --->", channel->img_fmt.height);
	item_set_data(channel);

	item_make(item_format, "Pixel format");
	item_add_str(": (%s)  --->", camera_string_pixel_format(channel->img_fmt.pix_fmt));
	item_set_data(channel);

	item_make(item_format, "Buffer type");
	item_add_str(": (%s)  --->", camera_string_img_type(channel->img_type));
	item_set_data(channel);
	return 0;
}

typedef enum channel_param {
	CHANNEL_PARAM_CAPTURE_TYPE = 0,
	CHANNEL_PARAM_META_FIELDS,
	CHANNEL_PARAM_FRM_CNT,
	CHANNEL_PARAM_IMG_WIDTH,
	CHANNEL_PARAM_IMG_HEIGIT,
	CHANNEL_PARAM_IMG_PIX_FMT,
	CHANNEL_PARAM_IMG_TYPE,
} channel_param_e;

extern char dialog_input_result[MAX_LEN + 1];
static int channel_config_integer(csi_camera_channel_cfg_s *channel, channel_param_e param)
{
	int ret_key = KEY_ESC;
	char title[64];
	char prompt[64] = "Please enter the appropriate number";
	char str_value[32];

	if (channel == NULL) {
		LOG_W("channel is NULL\n");
		return KEY_ESC;
	}

again:
	switch (param) {
	case CHANNEL_PARAM_FRM_CNT:
		snprintf(title, sizeof(title), "Config Channel[%d] %s",
			channel->chn_id, "Buffer Count");
		snprintf(str_value, sizeof(str_value), "%d", channel->frm_cnt);
		break;
	case CHANNEL_PARAM_IMG_WIDTH:
		snprintf(title, sizeof(title), "Config Channel[%d] %s",
			channel->chn_id, "Image Width");
		snprintf(str_value, sizeof(str_value), "%d", channel->img_fmt.width);
		break;
	case CHANNEL_PARAM_IMG_HEIGIT:
		snprintf(title, sizeof(title), "Config Channel[%d] %s",
			channel->chn_id, "Image Height");
		snprintf(str_value, sizeof(str_value), "%d", channel->img_fmt.height);
		break;
	default:
		break;
	}

	curs_set(1);
	ret_key = dialog_inputbox(title, prompt, 9, 40, str_value);
	curs_set(0);

	if (ret_key == KEY_ESC || ret_key == 1)
		return KEY_ESC;

	/* can't be empty */
	if (strlen(dialog_input_result) == 0) {
		goto again;
	}

	/* value check, reset if invalid */
	int value = atoi(dialog_input_result);
	if (value <= 0){
		goto again;
	}

	/* accept value, set into property */
	switch (param) {
	case CHANNEL_PARAM_FRM_CNT:
		if (value > 100){	// filt unreasonable out
			goto again;
		}
		channel->frm_cnt = value;
		break;
	case CHANNEL_PARAM_IMG_WIDTH:
		if (value > (7680*2)){	// filt unreasonable out
			goto again;
		}
		channel->img_fmt.width = value;
		break;
	case CHANNEL_PARAM_IMG_HEIGIT:
		if (value > (4320*2)){	// filt unreasonable out
			goto again;
		}
		channel->img_fmt.height = value;
		break;
	default:
		break;
	}

	return 0;
}

static int channel_config_enum(csi_camera_channel_cfg_s *channel, channel_param_e param)
{
	int ret_key = KEY_ESC;
	char title[64];
	char prompt[64] = "Please select the appropriate enum item";
	char str_value[32];
	const camera_spec_enums_s *enums_array;

	if (channel == NULL) {
		LOG_W("channel is NULL\n");
		return KEY_ESC;
	}

	item_reset();
	switch (param) {
	case CHANNEL_PARAM_IMG_PIX_FMT:
		snprintf(title, sizeof(title), "Config Channel[%d] %s",
			channel->chn_id, "Image Pixel Format");

		enums_array = camera_spec_get_enum_array(CAMERA_SPEC_ENUM_CHANNEL_PIX_FMT);
		if (enums_array == NULL) {
			LOG_E("No enum array for CAMERA_SPEC_ENUM_CHN_PIX_FMT(%d)\n",
				CAMERA_SPEC_ENUM_CHANNEL_PIX_FMT);
			return KEY_ESC;
		}

		for (int i = 0; i < enums_array->count; i++) {
			item_make("%d-%s", enums_array->enums[i],
				camera_string_pixel_format(enums_array->enums[i]));
			//item_add_str("");
			if (channel->img_fmt.pix_fmt == enums_array->enums[i]) {
				item_set_selected(1);
				item_set_tag('X');
			} else {
				item_set_selected(0);
				item_set_tag(' ');
			}
		}

		break;
	case CHANNEL_PARAM_IMG_TYPE:
		snprintf(title, sizeof(title), "Config Channel[%d] %s",
			channel->chn_id, "Image Buffer Type");

		enums_array = camera_spec_get_enum_array(CAMERA_SPEC_ENUM_CHANNEL_IMG_TYPE);
		if (enums_array == NULL) {
			LOG_E("No enum array for CAMERA_SPEC_ENUM_CHN_IMG_TYPE(%08x)\n",
				CAMERA_SPEC_ENUM_CHANNEL_IMG_TYPE);
			return KEY_ESC;
		}

		for (int i = 0; i < enums_array->count; i++) {
			item_make("%d-%s", i, camera_string_img_type(i));
			//item_add_str("");
			if (channel->img_type == enums_array->enums[i]) {
				item_set_selected(1);
				item_set_tag('X');
			} else {
				item_set_selected(0);
				item_set_tag(' ');
			}
		}

		break;
	default:
		return KEY_ESC;
	}

	int list_height = MIN(enums_array->count + 1, 8);
	ret_key = dialog_checklist(
		title, prompt,
		CHECKLIST_HEIGTH_MIN + list_height + 2,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 32,
		list_height,
		NULL, 0);
	LOG_D("dialog_checklist() ret_key=%d\n", ret_key);

	if (ret_key == KEY_ESC) {
		return KEY_ESC;
	} else if (ret_key == 0) {	/* select */
		if (!item_is_selected())
			return KEY_ESC;

		int enum_id = enums_array->enums[item_n()];

		switch(param) {
		case CHANNEL_PARAM_IMG_PIX_FMT:
			LOG_D("select item: pos=%d, enum=%d:%s\n",
				item_n(), enum_id, camera_string_pixel_format(enum_id));
			channel->img_fmt.pix_fmt = enum_id;
			break;
		case CHANNEL_PARAM_IMG_TYPE:
			LOG_D("select item: pos=%d, enum=%d:%s\n",
				item_n(), enum_id, camera_string_img_type(enum_id));
			channel->img_type = enum_id;
			break;
		default:
			return KEY_ESC;
		}
		return ret_key;
	} else if (ret_key == 1) {	/* help */
		LOG_W("Help does not support yet\n");
		return ret_key;
	} else {
		LOG_E("Unknown return value: %d\n", ret_key);
		return KEY_ESC;
	}

	return 0;
}

static int channel_config_bitmsk_capture_type(csi_camera_channel_cfg_s *channel)
{
	int ret_key = KEY_ESC;
	if (channel == NULL) {
		LOG_W("channel is NULL\n");
		return KEY_ESC;
	}

	char title[64];
	char prompt[64] = "Please select the appropriate enum item";
	int item_pos = 0;
	snprintf(title, sizeof(title), "Config Channel[%d] Capture Types", channel->chn_id);
	const camera_spec_bitmasks_t *bitmasks_array =
		camera_spec_get_bitmask_array(CAMERA_SPEC_BITMAKS_CHANNEL_CAPTURE_TYPE);
	if (bitmasks_array == NULL)
			return KEY_ESC;

again:
	item_reset();
	for (int i = 0; i < bitmasks_array->count; i++) {
		item_make("bit-%02d: %s", bitmasks_array->bitmask[i],
			camera_string_capture_type(bitmasks_array->bitmask[i], true));
		//item_add_str("");

		if (bitmasks_array->bitmask[i] & channel->capture_type)	{
			item_set_selected(1);
			item_set_tag('*');
		}
		else {
			item_set_selected(0);
			item_set_tag(' ');
		}
	}

	int list_height = MIN(bitmasks_array->count + 1, 8);
	ret_key = dialog_checkbox(
		title, prompt,
		CHECKLIST_HEIGTH_MIN + list_height + 2,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 32,
		list_height, item_pos);

	LOG_D("dialog_checkbox() ret_key=%d\n", ret_key);
	if (ret_key == KEY_ESC) {
		return KEY_ESC;
	} else if (ret_key == 0) {/* select */
		item_pos = item_n();
		int bitmask_value = bitmasks_array->bitmask[item_n()];
		if (item_is_selected()) {
			channel->capture_type |= bitmask_value;
		} else {
			channel->capture_type &= ~bitmask_value;
		}

		int enum_id = bitmasks_array->bitmask[item_n()];
		LOG_D("%s item: pos=%d, enum=%d-%s, bitmask_value=%08x\n",
			item_is_selected() ? "Select" : "Unselect",
 			item_n(), enum_id,
			camera_string_capture_type(enum_id, true),
			channel->capture_type);
		goto again;
		return ret_key;
	} else if (ret_key == 1) {/* help */
		LOG_W("Help does not support yet\n");
		return ret_key;
	} else {
		LOG_E("Unknown return value: %d\n", ret_key);
		return KEY_ESC;
	}
}

static int channel_config_bitmsk_meta_type(csi_camera_channel_cfg_s *channel)
{
	int ret_key = KEY_ESC;
	if (channel == NULL) {
		LOG_W("channel is NULL\n");
		return KEY_ESC;
	}

	char title[64];
	char prompt[64] = "Please select the appropriate enum item";
	int item_pos = 0;
	snprintf(title, sizeof(title), "Config Channel[%d] Meta Types", channel->chn_id);
	const camera_spec_bitmasks_t *bitmasks_array =
		camera_spec_get_bitmask_array(CAMERA_SPEC_BITMAKS_CHANNEL_META_TYPE);
	if (bitmasks_array == NULL)
			return KEY_ESC;

again:
	item_reset();
	for (int i = 0; i < bitmasks_array->count; i++) {
		item_make("bit-%02d: %s", bitmasks_array->bitmask[i],
			camera_string_meta_field(bitmasks_array->bitmask[i], true));
		//item_add_str("");

		if (bitmasks_array->bitmask[i] & channel->meta_fields)	{
			item_set_selected(1);
			item_set_tag('*');
		}
		else {
			item_set_selected(0);
			item_set_tag(' ');
		}
	}

	int list_height = MIN(bitmasks_array->count + 1, 8);
	ret_key = dialog_checkbox(
		title, prompt,
		CHECKLIST_HEIGTH_MIN + list_height + 2,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 32,
		list_height, item_pos);

	LOG_D("dialog_checkbox() ret_key=%d\n", ret_key);
	if (ret_key == KEY_ESC) {
		return KEY_ESC;
	} else if (ret_key == 0) {/* select */
		item_pos = item_n();
		int bitmask_value = bitmasks_array->bitmask[item_n()];
		if (item_is_selected()) {
			channel->meta_fields |= bitmask_value;
		} else {
			channel->meta_fields &= ~bitmask_value;
		}

		int enum_id = bitmasks_array->bitmask[item_n()];
		LOG_D("%s item: pos=%d, enum=%d-%s, bitmask_value=%08x\n",
			item_is_selected() ? "Select" : "Unselect",
 			item_n(), enum_id,
			camera_string_meta_field(enum_id, true),
			channel->meta_fields);
		goto again;
		return ret_key;
	} else if (ret_key == 1) {/* help */
		LOG_W("Help does not support yet\n");
		return ret_key;
	} else {
		LOG_E("Unknown return value: %d\n", ret_key);
		return KEY_ESC;
	}
}

/* return int: property rows */
static int get_diff_string(char *text, csi_camera_channel_cfg_s *channel)
{
	int count = 0;
	int total_len = 0;
	int add_len;
	text[0] = '\0';

	strcat(text,"-------------------------------------------\n");

	char item_name[64];
	char item_value[256];
	char str_buf1[128];
	char str_buf2[128];

	if(cam_session->chn_cfg_tmp.capture_type != channel->capture_type) {
		snprintf(item_name, sizeof(item_name), "%-14s", "capture type");
		snprintf(item_value, sizeof(item_value), ": (%s) => (%s)\n",
			camera_string_chn_capture_types(channel->capture_type, str_buf1),
			camera_string_chn_capture_types(cam_session->chn_cfg_tmp.capture_type, str_buf2));
		strcat(text, item_name);
		strcat(text, item_value);
		count++;
	}

	if(cam_session->chn_cfg_tmp.meta_fields != channel->meta_fields) {
		snprintf(item_name, sizeof(item_name), "%-14s", "Meta fields");
		snprintf(item_value, sizeof(item_value), ": (%s) =>\n\t\t    (%s)\n",
			camera_string_chn_meta_fields(channel->meta_fields, str_buf1),
			camera_string_chn_meta_fields(cam_session->chn_cfg_tmp.meta_fields, str_buf2));
		strcat(text, item_name);
		strcat(text, item_value);
		count+=2;
	}

	if(cam_session->chn_cfg_tmp.frm_cnt != channel->frm_cnt) {
		snprintf(item_name, sizeof(item_name), "%-14s", "buffer count");
		snprintf(item_value, sizeof(item_value), ": (%d) => (%d)\n",
			channel->frm_cnt, cam_session->chn_cfg_tmp.frm_cnt);
		strcat(text, item_name);
		strcat(text, item_value);
		count++;
	}

	if(cam_session->chn_cfg_tmp.img_fmt.width != channel->img_fmt.width) {
		snprintf(item_name, sizeof(item_name), "%-14s", "Image width");
		snprintf(item_value, sizeof(item_value), ": (%d) => (%d)\n",
			channel->img_fmt.width, cam_session->chn_cfg_tmp.img_fmt.width);
		strcat(text, item_name);
		strcat(text, item_value);
		count++;
	}

	if(cam_session->chn_cfg_tmp.img_fmt.height != channel->img_fmt.height) {
		snprintf(item_name, sizeof(item_name), "%-14s", "Image height");
		snprintf(item_value, sizeof(item_value), ": (%d) => (%d)\n",
			channel->img_fmt.height, cam_session->chn_cfg_tmp.img_fmt.height);
		strcat(text, item_name);
		strcat(text, item_value);
		count++;
	}

	if(cam_session->chn_cfg_tmp.img_fmt.pix_fmt != channel->img_fmt.pix_fmt) {
		snprintf(item_name, sizeof(item_name), "%-14s", "Pixel format");
		snprintf(item_value, sizeof(item_value), ": (%s) => (%s)\n",
			camera_string_pixel_format(channel->img_fmt.pix_fmt),
			camera_string_pixel_format(cam_session->chn_cfg_tmp.img_fmt.pix_fmt));
		strcat(text, item_name);
		strcat(text, item_value);
		count++;
	}

	if(cam_session->chn_cfg_tmp.img_type != channel->img_type) {
		snprintf(item_name, sizeof(item_name), "%-14s", "Buffer type");
		snprintf(item_value, sizeof(item_value), ": (%s) => (%s)\n",
			camera_string_img_type(channel->img_type),
			camera_string_img_type(cam_session->chn_cfg_tmp.img_type));
		strcat(text, item_name);
		strcat(text, item_value);
		count++;
	}

	return count;
}

static int channel_open_internal(char *feedback)
{
	int ret = camera_channel_open(cam_session, &cam_session->chn_cfg_tmp);
	if (ret != 0) {
		LOG_E("camera_channel_open() failed, ret=%d\n", ret);
	}

	if (feedback) {
		if (ret)
			sprintf(feedback, "camera_channel_open() failed, ret=%d\n", ret);
		else
			sprintf(feedback, "camera_channel_open() OK\n");
	}
	return ret;
}

int dialog_channel_open(csi_camera_channel_cfg_s *channel)
{
	int ret_key = KEY_ESC;
	int ret = 0;
	char str_buf[1024];
	int item_pos = 0;
	csi_camera_channel_cfg_s *chn_tmp = &(cam_session->chn_cfg_tmp);
	memcpy(chn_tmp, channel, sizeof(*chn_tmp));

again:
	item_reset();
	if (fill_channel_config_items(chn_tmp) != 0) {
		LOG_E("fill_channel_config_items() failed\n");
		ret = KEY_ESC;
		goto exit;
	}

	char *button_names[] = {"Config", " Open ", "Cancel"};

	int s_scroll = 0;
	snprintf(str_buf, sizeof(str_buf), "Config Camera[%d] - Channel[%d]",
		cam_session->camera_id, chn_tmp->chn_id);
	ret_key = dialog_menu(str_buf, "Select item, press Enter to config",
		WIN_ROWS-2, WIN_COLS, NULL, item_pos, &s_scroll, button_names, 3);
	item_pos = item_activate_selected_pos();
	LOG_D("dialog_menu() ret_key=%d, item_pos=%d\n", ret_key, item_pos);

	if (ret_key == 0) {		/* Select button */
		if (item_pos == CHANNEL_PARAM_FRM_CNT ||
		    item_pos == CHANNEL_PARAM_IMG_WIDTH ||
		    item_pos == CHANNEL_PARAM_IMG_HEIGIT) {
			channel_config_integer(chn_tmp, item_pos);
		} else if (item_pos == CHANNEL_PARAM_IMG_PIX_FMT ||
			   item_pos == CHANNEL_PARAM_IMG_TYPE) {
			channel_config_enum(chn_tmp, item_pos);
		} else if (item_pos == CHANNEL_PARAM_CAPTURE_TYPE) {
			channel_config_bitmsk_capture_type(chn_tmp);
		} else if (item_pos == CHANNEL_PARAM_META_FIELDS){
			channel_config_bitmsk_meta_type(chn_tmp);
		}
		goto again;
	} else if (ret_key == 1) {	/* Open button */
		ret = channel_open_internal(str_buf);
		LOG_I("%s\n", str_buf);
		dialog_textbox_simple("Infomation", str_buf, 10, 40);

		if (ret != 0) {
			goto again;
		}

		goto exit;
	} else if (ret_key == 2) {	/* Cancel button */
		// do nothing
		ret = KEY_ESC;
		goto exit;
	} else {
		ret = KEY_ESC;
		goto exit;
	}

	/* else, Select button */
	goto again;

exit:
	return ret;
}

