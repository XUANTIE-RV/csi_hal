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

TARGET := libhal_common.a
TARGET_DYNAMIC := libhal_common.so
OUTPUT_DIR := $(DIR_TO_ROOT)/output/hal

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

all: $(TARGET) $(TARGET_DYNAMIC)

$(TARGET): $(OBJS)
	@mkdir -p $(OUTPUT_DIR)
	@echo "Linking" $@ "..."
	$(AR) -r -o $(OUTPUT_DIR)/$@ .obj/*.o

$(TARGET_DYNAMIC): $(OBJS)
	@mkdir -p $(OUTPUT_DIR)/$(SO_LIB_DIR)
	@echo "Linking" $@ "..."
	$(CC) -shared -fPIC -o $(OUTPUT_DIR)/$(SO_LIB_DIR)/$@ .obj/*.o $(CFLAGS)  $(INCLUDE)

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
	rm -rf $(OUTPUT_DIR)/$(TARGET)
	rm -rf $(OUTPUT_DIR)/$(SO_LIB_DIR)/$(TARGET_DYNAMIC)

.PHONY: clean all
