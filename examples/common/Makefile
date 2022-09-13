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

#CFLAGS  +=
LIBS	+= -lhal_common
OUTPUT_DIR := $(DIR_TO_ROOT)/output/examples/common

TARGET_1 := mtrace_demo
SRCS_1 = mtrace_demo.c

all:  $(TARGET_1)

clean:
	rm -rf .obj
	rm -f $(TARGET_1) $(OUTPUT_DIR)/$(TARGET_1)

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: clean all
