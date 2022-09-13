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
#define LOG_PREFIX "dlg_prop_enum"
#include <syslog.h>
#include <camera_manager.h>
#include <csi_camera_platform_spec.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_camera_property_bitmask(csi_camera_property_description_s *property)
{
	int ret_key = KEY_ESC;

	if (property == NULL) {
		LOG_W("property is NULL\n");
		return KEY_ESC;
	}

	char *title = property->name;
	char *prompt = "Please set camera property";
	int item_pos = 0;

again:
	item_reset();

	const camera_spec_bitmasks_t *bitmasks_array = camera_spec_get_bitmask_array(property->id);
	if (bitmasks_array == NULL)
		return KEY_ESC;

	for (int i = 0; i < bitmasks_array->count; i++) {
		item_make("bit-%02d: %s", bitmasks_array->bitmask[i],
			camera_string_bitmask_name(property->id, bitmasks_array->bitmask[i]));
		item_add_str("%s",
			(bitmasks_array->bitmask[i] & property->default_value.enum_value) ? " (default)" : "");

		if (bitmasks_array->bitmask[i] & property->value.enum_value)	{
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
			property->value.bitmask_value |= bitmask_value;
		} else {
			property->value.bitmask_value &= ~bitmask_value;
		}

		int enum_id = bitmasks_array->bitmask[item_n()];
		LOG_D("%s item: pos=%d, enum=%d-%s, bitmask_value=%08x\n",
			item_is_selected() ? "Select" : "Unselect",
 			item_n(), enum_id,
			camera_string_bitmask_name(property->id, enum_id),
			property->value.bitmask_value);
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

