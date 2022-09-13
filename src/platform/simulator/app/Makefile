##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##
DIR_TO_ROOT=../../../..
include $(DIR_TO_ROOT)/build.param

#CFLAGS  = -Wall -g -O0
OUTPUT_DIR := $(DIR_TO_ROOT)/output/driver/ipc_fd_transfer
LIBS	+= -lcommon -lcurses
#INCLUDE += -I../../ion

TARGET_1 = menu
SRCS_1   = menu.c

TARGET_2 = 7_input_function
SRCS_2   = 7_input_function.c


all: $(TARGET_1) $(TARGET_2)

clean:
	rm -rf .obj
	rm -rf $(TARGET_1) $(OUTPUT_DIR)/$(TARGET_1)
	rm -rf $(TARGET_2) $(OUTPUT_DIR)/$(TARGET_2)

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: clean all
