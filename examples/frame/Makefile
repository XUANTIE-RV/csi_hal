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

CFLAGS  += -I$(DIR_TO_ROOT)/src/platform/simulator/drivers/ion/
LIBS	+= -lhal_common -lhal_platform
OUTPUT_DIR := $(DIR_TO_ROOT)/output/examples/frame

TARGET_1 := frame_demo1
SRCS_1 = frame_demo1.c frame_demo_common.c

TARGET_2 := frame_demo2
SRCS_2 = frame_demo2.c frame_demo2_producer.c frame_demo2_consumer.c frame_demo_common.c

all:  $(TARGET_1) $(TARGET_2)
	@mkdir -p $(OUTPUT_DIR)/../resource
	cp -f $(DIR_TO_ROOT)/examples/resource/yuv420_1280x720_csky2016.yuv $(OUTPUT_DIR)/../resource

clean:
	rm -rf .obj
	rm -f $(TARGET_1) $(OUTPUT_DIR)/$(TARGET_1)
	rm -f $(TARGET_2) $(OUTPUT_DIR)/$(TARGET_2)

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: clean all
