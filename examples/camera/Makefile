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

CFLAGS  += -Wno-error=unused-result -Icamera_demo3_srcs -I./camera_demo3_srcs -I$(DIR_TO_ROOT)/src/lib_camera/
LIBS	+= -lhal_common -lhal_camera -lhal_platform -lpthread -lcamera_action
OUTPUT_DIR := $(DIR_TO_ROOT)/output/examples/camera
PREPARE := lib_camera

ifeq ($(PLATFORM),light)
  CFLAGS +=  -I$(VISYS_SYM_PATH)/usr/include/plink/
  CFLAGS +=  -I$(VISYS_SYM_PATH)/usr/include/vidmem/
  LIBS += -lstdc++ -lpthread 
  LIBS += -lvmem -lplink
 
endif

ifeq ($(PLATFORM),simulator)
  LIBS += -lcamera_platform -lapp_utilities -lcamera_utilities  $(LIBOPENCV_LIBS) -lstdc++
  CFLAGS += $(LIBOPENCV_INC) -I ./opencv
  PREPARE += opencv
endif

TARGET_1 := camera_demo1
SRCS_1 = camera_demo1.c

TARGET_2 := camera_demo2
SRCS_2 = camera_demo2.c

TARGET_3 := camera_demo3
SRCS_3 := camera_demo3.c $(wildcard camera_demo3_srcs/*.c)
LIBS_3 += -lcurses

TARGET_4 := camera_demo4
SRCS_4 = camera_demo4.c

TARGET_5 := camera_test1
SRCS_5 = camera_test1.c

TARGET_6 := cam_demo_simple
SRCS_6 = cam_demo_simple.c camera_frame_display.c

TARGET_7 := cam_demo_ir
SRCS_7 = cam_demo_ir.c camera_frame_display.c

TARGET_8 := cam_demo_multi
SRCS_8 = cam_demo_multi.c camera_frame_display.c

ifeq ($(PLATFORM),simulator)
TARGET_ALL := $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_4)
else
TARGET_ALL := $(TARGET_1) $(TARGET_2) $(TARGET_3) $(TARGET_5)  $(TARGET_6) $(TARGET_7) $(TARGET_8) 
endif

all: $(TARGET_ALL)
	cp run_camera_demo3.sh $(OUTPUT_DIR)
	cp camera_demo3.conf $(OUTPUT_DIR)

lib_camera:
	make -C $(DIR_TO_ROOT)/src/lib_camera

opencv:
	make -C opencv

clean:
	rm -rf .obj
	rm -f $(TARGET_1) $(OUTPUT_DIR)/$(TARGET_1)
	rm -f $(TARGET_2) $(OUTPUT_DIR)/$(TARGET_2)
	rm -f $(TARGET_3) $(OUTPUT_DIR)/$(TARGET_3)
	rm -f $(TARGET_3) $(OUTPUT_DIR)/run_camera_demo3.sh
	rm -f $(TARGET_3) $(OUTPUT_DIR)/camera_demo3.conf
	rm -f $(TARGET_4) $(OUTPUT_DIR)/$(TARGET_4)
	rm -f $(TARGET_5) $(OUTPUT_DIR)/$(TARGET_5)
	make -C ./opencv clean

include $(DIR_TO_ROOT)/common_target.mk

.PHONY: opencv clean all
