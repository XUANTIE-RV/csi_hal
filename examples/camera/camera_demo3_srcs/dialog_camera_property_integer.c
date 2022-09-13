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
#define LOG_PREFIX "dlg_prop_int"
#include <syslog.h>
#include <camera_manager.h>

#include "param.h"
#include "app_dialogs.h"

extern cams_t *cam_session;
extern char dialog_input_result[MAX_LEN + 1];

int dialog_camera_property_integer(csi_camera_property_description_s *property)
{
	int ret_key = KEY_ESC;

	if (property == NULL) {
		LOG_W("property is NULL\n");
		return KEY_ESC;
	}

	char title[64];
	snprintf(title, sizeof(title), "Camera %s setting", property->name);

	char prompt[64];
	snprintf(prompt, sizeof(prompt), "value range should be: [%d, %d], step %d",
		property->minimum, property->maximum, property->step);

	char str_value[32];
	snprintf(str_value, sizeof(str_value), "%d", property->value.int_value);

again:
	curs_set(1);
	ret_key = dialog_inputbox(title, prompt, 10, 50, str_value);
	curs_set(0);

	if (ret_key == KEY_ESC || ret_key == 1)
		return KEY_ESC;

	/* can't be empty */
	if (strlen(dialog_input_result) == 0) {
		goto again;
	}

	/* value check, reset if invalid */
	int value = atoi(dialog_input_result);
	if (value < property->minimum || value > property->maximum ||
	    ((value - property->minimum) % property->step != 0)) {
		LOG_W("%s\n", prompt);
		goto again;
	}

	/* accept value, set into property */
	property->value.int_value = value;
	return 0;
}

