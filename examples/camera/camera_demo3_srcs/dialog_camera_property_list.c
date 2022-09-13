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
#define LOG_PREFIX "dlg_prop_list"
#include <syslog.h>

#include <curses.h>
#include <camera_manager.h>
#include <camera_string.h>
#include "param.h"
#include "app_dialogs.h"

#include <csi_camera_property.h>
#include <csi_camera_platform_spec.h>

extern cams_t *cam_session;

//#define USE_DUMMY_DESC

#ifdef USE_DUMMY_DESC
csi_camera_property_description_s property_hflip = {
	CSI_CAMERA_PID_HFLIP, CSI_CAMERA_PROPERTY_TYPE_BOOLEAN, "horizon flip",
	/*min, max , step, def, value, flag, res[2]*/
	0,     1,    1,   {.bool_value=true},   {.bool_value=false},    0,    {0, 0}
};

csi_camera_property_description_s property_rotate = {
	CSI_CAMERA_PID_ROTATE, CSI_CAMERA_PROPERTY_TYPE_INTEGER, "rotate",
	/*min, max, step, def, value, flag, res[2]*/
	0,     270,    90,   {.int_value=0},   {.int_value=90},    0,    {0, 0}
};

csi_camera_property_description_s property_exposure_mode = {
	CSI_CAMERA_PID_EXPOSURE_MODE, CSI_CAMERA_PROPERTY_TYPE_ENUM, "exposure mode",
	/*min, max, step, def, value, flag, res[2]*/
	0,     0,    0,   {.enum_value=CSI_CAMERA_EXPOSURE_MODE_AUTO},   {.enum_value=CSI_CAMERA_EXPOSURE_SHUTTER_PRIORITY},    0,    {0, 0}
};

csi_camera_property_description_s property_3a_lock = {
	CSI_CAMERA_PID_3A_LOCK, CSI_CAMERA_PROPERTY_TYPE_BITMASK, "3A LOCK",
	1, 4, 2, {.bitmask_value = 3}, {.bitmask_value = 2}, 0, {0, 0}
};
#endif

struct desc_list {
	csi_camera_property_description_s desc;
	struct desc_list *next;
};

struct desc_list *desc_head = NULL;
struct desc_list *desc_cur = NULL;
struct desc_list  desc_nil;

static void desc_list_reset(void)
{
	struct desc_list *p, *next;

	for (p = desc_head; p && (p != &desc_nil); p = next) {
        next = p->next;
		free(p);
	}
	desc_head = NULL;
	desc_cur = &desc_nil;
}

static void desc_list_add(csi_camera_property_description_s *desc)
{
	struct desc_list *p = malloc(sizeof(*p));
	memcpy(p, desc, sizeof(*desc));

	if (desc_head)
		desc_cur->next = p;
	else
		desc_head = p;
	desc_cur = p;
	p->next = &desc_nil;
}

static void init_camera_property_items(void)
{
#ifdef USE_DUMMY_DESC
	desc_list_add(&property_hflip);
	desc_list_add(&property_rotate);
	desc_list_add(&property_exposure_mode);
	desc_list_add(&property_3a_lock);
#else
	csi_camera_property_description_s desc;

	desc.id = CSI_CAMERA_PID_HFLIP;
	while (!camera_query_property(cam_session, &desc)) {
		//LOG_D("Get desc:%s\n", desc.name);

		desc_list_add(&desc);

		desc.id |= CSI_CAMERA_FLAG_NEXT_CTRL;
	}
#endif
}

static void uninit_camera_property_items(void)
{
	desc_list_reset();
}

static int fill_camera_property_items(void)
{
	struct desc_list *node;
	for (node = desc_head; node && (node != &desc_nil); node = node->next) {
		csi_camera_property_description_s *desc = &(node->desc);
		char item_name[64];
		char item_value[64];

		switch (desc->type) {
		case (CSI_CAMERA_PROPERTY_TYPE_INTEGER):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "INTEGER: ");
			snprintf(item_value, sizeof(item_value), "(%d)", desc->value.int_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_BOOLEAN):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "BOOLEAN: ");
			snprintf(item_value, sizeof(item_value), "(%s)", desc->value.bool_value ? "true" : "false");
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_ENUM):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "ENUM: ");
			snprintf(item_value, sizeof(item_value), "(%s)",
				 camera_string_enum_name(desc->id, desc->value.enum_value));
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_STRING):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "STRING: ");
			snprintf(item_value, sizeof(item_value), "(%s)", desc->value.str_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_BITMASK):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "BITMASK: ");
			snprintf(item_value, sizeof(item_value), "(%08x)", desc->value.bitmask_value);
			break;
		default:
			LOG_E("error type!\n");
			return -1;
		}

		item_make("%s", item_name);
		item_add_str("%s  --->", item_value);
		item_set_data(&(node->desc));
		//LOG_D("fill %s\n", desc->name);
	}

	return 0;
}

static int get_property_diff_count(void)
{
	int diff_count = 0;
	struct desc_list *node;
	for (node = desc_head; node && (node != &desc_nil); node = node->next) {
		//LOG_D("comparing '%s'\n", node->desc.name);
		csi_camera_property_description_s desc;
		desc.id = node->desc.id;
		if (camera_query_property(cam_session, &desc) != 0) {
			LOG_E("camera_query_property(%d), %s failed\n",
				node->desc.id, node->desc.name);
			continue;
		}
		if (memcmp(&(node->desc.value), &(desc.value), sizeof(desc.value)) == 0) {
			//LOG_D("property '%s' is same\n", desc.name);
			continue;
		}
		diff_count ++;
	}
	return diff_count;
}

/* return int: property count(rows) */
static int get_diff_string(char *text, int length, int rows)
{
	int count = 0;
	int total_len = 0;
	int add_len;
	text[0] = '\0';
	strcat(text,"-------------------------------------------\n");

	for (struct desc_list *node = desc_head;
	     node && (node != &desc_nil); node = node->next) {
		//LOG_D("comparing '%s'\n", node->desc.name);
		csi_camera_property_description_s *desc = &(node->desc);

		csi_camera_property_description_s device_desc;
		device_desc.id = node->desc.id;
		if (camera_query_property(cam_session, &device_desc) != 0) {
			LOG_E("camera_query_property(%d), %s failed\n",
				node->desc.id, node->desc.name);
			continue;
		}

		if (memcmp(&(desc->value), &(device_desc.value), sizeof(desc->value)) == 0) {
			continue;
		}

		LOG_D("property '%s' is diff\n", desc->name);

		char item_name[64];
		char item_value[64];

		switch (desc->type) {
		case (CSI_CAMERA_PROPERTY_TYPE_INTEGER):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "INTEGER: ");
			snprintf(item_value, sizeof(item_value), "(%d)", desc->value.int_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_BOOLEAN):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "BOOLEAN: ");
			snprintf(item_value, sizeof(item_value), "(%s)", desc->value.bool_value ? "true" : "false");
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_ENUM):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "ENUM: ");
			snprintf(item_value, sizeof(item_value), "(%s)",
				 camera_string_enum_name(desc->id, desc->value.enum_value));
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_STRING):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "STRING: ");
			snprintf(item_value, sizeof(item_value), "(%s)", desc->value.str_value);
			break;
		case (CSI_CAMERA_PROPERTY_TYPE_BITMASK):
			snprintf(item_name, sizeof(item_name), "%-16s:%10s", desc->name, "BITMASK: ");
			snprintf(item_value, sizeof(item_value), "(%08x)", desc->value.bitmask_value);
			break;
		default:
			LOG_E("error type!\n");
			return -1;
		}

		if (count >= rows)
			break;

		add_len = strlen(item_name) + strlen(item_value) + 1;
		if ((total_len + add_len) >= length) {
			LOG_W("text buffer overflow\n");
			break;
		}

		strcat(text, item_name);
		strcat(text, item_value);
		strcat(text, "\n");

		total_len += add_len;
		count++;
	}
	return count;
}

static int apply_property_setting(void)
{
	LOG_W("do apply camera property settings\n");
	int diff_count = get_property_diff_count();
	int ret;

	// assemble property struct
	csi_camera_properties_s properties;
	csi_camera_property_s *property = malloc(diff_count * sizeof(csi_camera_property_s));
	properties.count = diff_count;
	properties.property = property;
	int i = 0;

	for (struct desc_list *node = desc_head;
	     node && (node != &desc_nil); node = node->next) {
		//LOG_D("comparing '%s'\n", node->desc.name);
		csi_camera_property_description_s desc;
		desc.id = node->desc.id;
		if (camera_query_property(cam_session, &desc) != 0) {
			LOG_E("camera_query_property(%d), %s failed\n",
				node->desc.id, node->desc.name);
			continue;
		}
		if (memcmp(&(node->desc.value), &(desc.value), sizeof(desc.value)) == 0) {
			//LOG_D("property '%s' is same\n", desc.name);
			continue;
		}

		LOG_D("property '%s' is different\n", desc.name);
		property[i].id = node->desc.id;
		property[i].type = node->desc.type;
		memcpy(&(property[i].value), &(node->desc.value), sizeof(node->desc.value));

		i++;
	}

	ret = camera_set_property(cam_session, &properties);
	if (ret == 0)
		LOG_O("camera_set_property() ok!\n");
	else
		LOG_E("camera_set_property() failed!\n");

	free(property);
	return ret;
}

int dialog_camera_property_list(void)
{
	int ret_key = KEY_ESC;
	int ret = 0;

	csi_camera_property_description_s *init_item = NULL;
	csi_camera_property_description_s *selected_item;
	init_camera_property_items();

again:
	item_reset();
	if (fill_camera_property_items() != 0) {
		LOG_E("fill_camera_property_items() failed\n");
		ret = KEY_ESC;
		goto exit;
	}

	int s_scroll = 0;
	ret_key = dialog_menu("Camera Property Settings",
		"Select the property, press Enter to change the property setting",
		WIN_ROWS-2, WIN_COLS, init_item, -1, &s_scroll,	NULL, 0);
	LOG_D("dialog_menu() ret_key=%d, s_scroll=%d\n", ret_key, s_scroll);

	if (ret_key == KEY_ESC) {
		ret = KEY_ESC;
		goto exit;
	} else if (ret_key == 1) {	/* Apply button */
		char str_buf[1024];
		int count = get_diff_string(str_buf, sizeof(str_buf), 32);
		ret_key = dialog_yesno("Apply camera with new propertis below?", str_buf, 6 + count, 50);
		if (ret_key != 0) {	// 0 is the button position of 'yes'
			LOG_D("Apply camera property is canceled\n");
			goto again;
		}
		ret = apply_property_setting();
		LOG_I("apply_property_setting() %s\n", (ret==0) ? "OK" : "Failed");

		snprintf(str_buf, sizeof(str_buf),
			"Apply camera properties below %s:\n",
			(ret == 0) ? "OK" : "Failed");

		dialog_textbox_simple("Infomation", str_buf, 10, 40);

		goto exit;
	} else if (ret_key == 2) {	/* Cancel button */
		// do nothing
		ret = KEY_ESC;
		goto exit;
	}

	/* else, Select button */
	selected_item = (csi_camera_property_description_s *)item_data();

	if (item_tag() == '-') {
		init_item = NULL;
		goto again;
	}

	csi_camera_property_description_s *item_property = item_data();
	if (item_property == NULL) {
		LOG_W("No item selected\n");
		ret = KEY_ESC;
		goto exit;
	} else {
		LOG_D("item '%s' selected\n", item_property->name);
	}

	switch (item_property->type) {
	case CSI_CAMERA_PROPERTY_TYPE_INTEGER:
		ret_key = dialog_camera_property_integer(selected_item);
		if (ret != KEY_ESC) {
			LOG_D("set property->value.int_value=%d\n",
				selected_item->value.int_value);
		}
		break;
	case CSI_CAMERA_PROPERTY_TYPE_BOOLEAN:
		ret_key = dialog_camera_property_boolean(selected_item);
		if (ret != KEY_ESC) {
			LOG_D("set property.value.bool_value='%s'\n",
				(selected_item->value.bool_value) ? "true" : "false");
		}
		break;
	case CSI_CAMERA_PROPERTY_TYPE_ENUM:
		ret_key = dialog_camera_property_enum(selected_item);
		if (ret != KEY_ESC) {
			LOG_D("set property.value.enum_value='%s'(%d)\n",
				camera_string_enum_name(selected_item->id,
						selected_item->value.enum_value),
				selected_item->value.enum_value);
		}
		break;
	case CSI_CAMERA_PROPERTY_TYPE_STRING:
		break;
	case CSI_CAMERA_PROPERTY_TYPE_BITMASK:
		ret_key = dialog_camera_property_bitmask(selected_item);
		if (ret != KEY_ESC) {
			LOG_D("set property.value.enum_value=(%08x)\n",
				selected_item->value.enum_value);
		}
		break;;
	default:
		LOG_E("Not supported property_hflip type\n");
	}

	init_item = item_property;
	goto again;

exit:
	uninit_camera_property_items();
	return ret;
}

