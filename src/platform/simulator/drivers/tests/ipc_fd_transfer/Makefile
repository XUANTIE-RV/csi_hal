##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##
DIR_TO_ROOT=../../../../../..
include $(DIR_TO_ROOT)/build.param

#CFLAGS  = -Wall -g -O0
OUTPUT_DIR := $(DIR_TO_ROOT)/output/driver/ipc_fd_transfer
LIBS	+= -lcommon
#INCLUDE += -I../../ion

TARGET_1 = ipc_fd_transfer
SRCS_1   = ipc_fd_transfer.c

TARGET_2 = producer
SRCS_2   = process_producer.c

TARGET_3 = consumer
SRCS_3   = process_consumer.c

all: $(TARGET_1) $(TARGET_2) $(TARGET_3)
	cp -f test.txt $(OUTPUT_DIR)

clean:
	rm -rf .obj
	rm -rf $(TARGET_1) $(TARGET_2) $(TARGET_3)
	rm -rf $(OUTPUT_DIR)

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: clean all
