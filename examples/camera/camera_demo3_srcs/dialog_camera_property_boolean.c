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
#define LOG_PREFIX "dlg_prop_boolean"
#include <syslog.h>

#include "param.h"
#include "app_dialogs.h"
#include <camera_manager.h>

extern cams_t *cam_session;

int dialog_camera_property_boolean(csi_camera_property_description_s *property)
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

	item_make("true");
	if (property->default_value.bool_value)
		item_add_str("(default)");
	if (property->value.bool_value) {
		item_set_tag('X');
		item_set_selected(1);
	} else {
		item_set_tag(' ');
		item_set_selected(0);
	}

	item_make("false");
	if (!property->default_value.bool_value)
		item_add_str(" (default)");
	if (!property->value.bool_value) {
		item_set_tag('X');
		item_set_selected(1);
	} else {
		item_set_tag(' ');
		item_set_selected(0);
	}

	ret_key = dialog_checklist(
		title, prompt,
		CHECKLIST_HEIGTH_MIN + 4,
		WIN_COLS - CHECKLIST_WIDTH_MIN - 32,
		2,
		NULL, 0);

	LOG_D("dialog_checklist() ret_key=%d\n", ret_key);
	if (ret_key == KEY_ESC) {
		return KEY_ESC;
	} else if (ret_key == 0) {/* select */
		LOG_D("select item: %d, means '%s'\n", item_n(),
			(item_n() == 0) ? "true" : "false");
		property->value.bool_value = (item_n() == 0) ? true : false;
		return ret_key;
	} else if (ret_key == 1) {/* help */
		LOG_W("Help does not support yet\n");
		return ret_key;
	} else {
		LOG_E("Unknown return value: %d\n", ret_key);
		return KEY_ESC;
	}
}

