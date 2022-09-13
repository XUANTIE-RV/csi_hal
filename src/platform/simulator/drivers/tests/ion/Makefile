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

#CFLAGS  += -DXX
OUTPUT_DIR := $(DIR_TO_ROOT)/output/driver
LIBS	+= -lhal_common -lhal_platform
INCLUDE += -I../../ion

TARGET_1 = ion_test1
SRCS_1 = ion_test1.c

TARGET_2 = ion_test2
SRCS_2 = ion_test2.c

TARGET_3 = ion_test2_process_producer
SRCS_3 = ion_test2_process_producer.c $(DIR_TO_ROOT)/examples/frame/frame_demo_common.c

TARGET_4 = ion_test2_process_consumer
SRCS_4 = ion_test2_process_consumer.c $(DIR_TO_ROOT)/examples/frame/frame_demo_common.c

TARGET_5 = ion_test3
SRCS_5 = ion_test3.c ion_test3_process_producer.c ion_test3_process_consumer.c
SRCS_5 += $(DIR_TO_ROOT)/examples/frame/frame_demo_common.c

all: $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4) $(TARGET_5)
	cp -f ion_test.txt $(OUTPUT_DIR)

clean:
	rm -rf .obj
	rm -f $(TARGET_1) $(OUTPUT_DIR)/$(TARGET_1)
	rm -f $(TARGET_2) $(OUTPUT_DIR)/$(TARGET_2)
	rm -f $(TARGET_3) $(OUTPUT_DIR)/$(TARGET_3)
	rm -f $(TARGET_4) $(OUTPUT_DIR)/$(TARGET_4)
	rm -f $(TARGET_5) $(OUTPUT_DIR)/$(TARGET_5)
	rm -f $(OUTPUT_DIR)/ion_test.txt

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: clean all
