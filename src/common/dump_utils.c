/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#define LOG_LEVEL 3
//#define LOG_PREFIX ""
#include <syslog.h>
#include <dump_utils.h>

#define DUMP_HEX_MAX_LENGHT 1024

void csi_dump_hex(char *data, int length, char *name)
{
	if (data == NULL || length <=0) {
		LOG_E("Input param invalid\n");
		return;
	}

	int i;
	int dump_len = (length <= DUMP_HEX_MAX_LENGHT) ? length : DUMP_HEX_MAX_LENGHT;

	LOG_F("%s (%d bytes) dump in hex mode:\n", (name != NULL) ? name :"", length);
	LOG_F("  addr: 00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F\n");
	LOG_F("  ------------------------------------------------------\n");
	for (i = 0; i < dump_len; i++) {
		if (i % 16 == 0)
			LOG_F("  %04x: ", i);
		LOG_F("%02x ", (unsigned char)data[i]);

		if (i % 16 == 7)
			LOG_F(" ");

		if (i % 16== 15)
			printf("\n");
	}
	if (length != dump_len)
		LOG_F("...");
	if (i % 16 != 0)
		LOG_F("\n");
	LOG_F("  ------------------------------------------------------\n");
}

void csi_dump_img_info(csi_img_s *img)
{
	// dump img info
	LOG_F("frame.img = {\n");
	LOG_F("\t type=%d size=%zu w=%u h=%u pix_format=%d num_planes=%u\n",
		img->type, img->size, img->width, img->height, img->pix_format, img->num_planes);
	LOG_F("\t fds[3] = {%d, %d, %d},\n",
		img->fds[0], img->fds[1], img->fds[2]);
	LOG_F("\t strides[3] = {%u, %u, %u},\n",
		img->strides[0], img->strides[1], img->strides[2]);
	LOG_F("\t offsets[3] = {%u, %u, %u},\n",
		img->offsets[0], img->offsets[1], img->offsets[2]);
	LOG_F("\t modifier = %lu\n", img->modifier);
	LOG_F("}\n");
}

