/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __DUMP_UTILS_H__
#define __DUMP_UTILS_H__

#include <stdio.h>
#include <string.h>

#include <csi_frame.h>

void csi_dump_hex(char *data, int length, char *name);
void csi_dump_img_info(csi_img_s *img);

#endif  // __DUMP_UTILS_H__
