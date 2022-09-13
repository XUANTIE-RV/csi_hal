##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##
DIR_TO_ROOT=../..
include $(DIR_TO_ROOT)/build.param

#CFLAGS +=
LIBS	+= -lhal_common -lhal_platform
OUTPUT_DIR := $(DIR_TO_ROOT)/output/examples/vdec
#PREPARE := lib_camera

TARGET_1 := vdec_demo1
SRCS_1 = vdec_demo1.c

TARGET_ALL := $(TARGET_1)

all: $(TARGET_ALL)

clean:
	rm -rf .obj
	rm -f $(TARGET_1) $(OUTPUT_DIR)/$(TARGET_1)

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: clean all
