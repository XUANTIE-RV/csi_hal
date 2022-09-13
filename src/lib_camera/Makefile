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

TARGET := libhal_camera.a
TARGET_DYNAMIC := libhal_camera.so
OUTPUT_DIR := $(DIR_TO_ROOT)/output/hal

LIBS	+= -lcamera_action

#CFLAGS  = -Wall -g -O0
INCLUDE += -I./camera_action/include

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

ifeq ($(PLATFORM),simulator)
  LIBOPENCV_INC = $(shell pkg-config --cflags opencv)
  LIBOPENCV_LIBS = $(shell pkg-config --libs opencv)
  LIBS += -lcamera_utilities $(LIBOPENCV_LIBS)
  CFLAGS += $(LIBOPENCV_INC)
endif



all: camera_action $(TARGET) $(TARGET_DYNAMIC)

camera_action:
	make -C camera_action

$(TARGET_DYNAMIC): $(OBJS) camera_action
	@mkdir -p $(OUTPUT_DIR)/$(SO_LIB_DIR)
	@echo "Linking" $@ "..."
	$(CC) -shared -fPIC -o $(OUTPUT_DIR)/$(SO_LIB_DIR)/$@ .obj/*.o $(CFLAGS)  $(INCLUDE) \
	      -L$(DIR_TO_ROOT)/output/common/ -L$(DIR_TO_ROOT)/output/hal/ \
				-lhal_common -lcamera_action $(LIBS)

$(TARGET): $(OBJS)
	@mkdir -p $(OUTPUT_DIR)
	@echo "Linking" $@ "..."
	$(AR) -r -o $(OUTPUT_DIR)/$@ .obj/*.o

$(OBJS): %.o:%.c
	@mkdir -p .obj
	@echo "Compiling" $< "..."
ifeq ($(PLATFORM),light)
	$(CC) $(CFLAGS) $(INCLUDE) -fPIC -c -o .obj/$(notdir $@) $<
else
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<
endif

clean:
	rm -rf .obj
	rm -f $(OUTPUT_DIR)/$(TARGET)
	rm -f $(OUTPUT_DIR)/$(SO_LIB_DIR)/$(TARGET_DYNAMIC)
	make -C camera_action clean

.PHONY: clean all camera_action
