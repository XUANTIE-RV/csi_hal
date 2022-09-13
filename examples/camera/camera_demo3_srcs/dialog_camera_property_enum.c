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

#define LOG_LEVEL 3
#define LOG_PREFIX "dlg_prop_enum"
#include <syslog.h>

#include <curses.h>
#include <csi_camera_platform_spec.h>
#include <camera_manager.h>
#include <camera_string.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;

int dialog_camera_property_enum(csi_camera_property_description_s *property)
{
	int ret_key = KEY_ESC;

	if (property == NULL) {
		LOG_W("property is NULL\n");
		return KEY_ESC;
	}

	item_reset();
	char *title = property->name;
	char *prompt = "Please set camera property";

	item_reset();

	const camera_spec_enums_s *enums_array = camera_spec_get_enum_array(property->id);
	if (enums_array == NULL)
		return KEY_ESC;

	for (int i = 0; i < enums_array->count; i++) {
		item_make("%d-%s",
			enums_array->enums[i],
			camera_string_enum_name(property->id, enums_array->enums[i]));
		item_add_str("%s",
			(enums_array->enums[i] == property->default_value.enum_value) ? " (default)" : "");

		if (enums_array->enums[i] == property->value.enum_value)	{
			item_set_selected(1);
			item_set_tag('X');
		}
		else {
			item_set_selected(0);
			item_set_tag(' ');
		}
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
	} else if (ret_key == 0) {/* select */
		if (!item_is_selected())
			return KEY_ESC;

		int enum_id = enums_array->enums[item_n()];
		LOG_D("select item: pos=%d, enum=%d-%s\n",
 			item_n(), enum_id,
			camera_string_enum_name(property->id, enum_id));
		property->value.enum_value = enum_id;
		return ret_key;
	} else if (ret_key == 1) {/* help */
		LOG_W("Help does not support yet\n");
		return ret_key;
	} else {
		LOG_E("Unknown return value: %d\n", ret_key);
		return KEY_ESC;
	}
}

