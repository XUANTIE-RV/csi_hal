##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##
DIR_TO_ROOT=../../..
include $(DIR_TO_ROOT)/build.param

TARGET_STATIC := libhal_platform.a
TARGET_DYNAMIC := libhal_platform.so
OUTPUT_DIR := $(DIR_TO_ROOT)/output/hal

#CFLAGS  = -Wall -g -O0
INCLUDE += -I./opencv/

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: drivers opencv $(TARGET_STATIC) $(TARGET_DYNAMIC) #app

opencv:
	make -C opencv

app:
	make -C app

drivers: $(TARGET_STATIC)
	make -C drivers

$(TARGET_DYNAMIC): $(OBJS) opencv
	@mkdir -p $(OUTPUT_DIR)/$(SO_LIB_DIR)
	@echo "Linking" $@ "..."
	$(CC) -shared -fPIC -o $(OUTPUT_DIR)/$(SO_LIB_DIR)/$@ .obj/*.o $(CFLAGS)  $(INCLUDE) \
	      -L$(DIR_TO_ROOT)/output/common/ -L$(DIR_TO_ROOT)/output/hal/ \
	      -lhal_common -lcamera_platform -lpthread $(LIBOPENCV_LIBS)

$(TARGET_STATIC): $(OBJS)
	@mkdir -p $(OUTPUT_DIR)
	@echo "Linking" $@ "..."
	$(AR) -r -o $(OUTPUT_DIR)/$@ .obj/*.o

$(OBJS): %.o:%.c
	@mkdir -p .obj
	@echo "Compiling" $< "..."
	$(CC) $(CFLAGS) $(INCLUDE) -c -o .obj/$(notdir $@) $<

clean:
	rm -rf .obj
	rm -f $(OUTPUT_DIR)/$(TARGET_STATIC)
	rm -f $(OUTPUT_DIR)/$(SO_LIB_DIR)/$(TARGET_DYNAMIC)
	make -C drivers clean
	make -C opencv clean

.PHONY: clean all drivers opencv app
