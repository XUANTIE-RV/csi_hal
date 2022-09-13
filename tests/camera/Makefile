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

$(info INCLUDE=$(INCLUDE))

#CFLAGS  = -Wall -g -O0
LIBS	+= -lhal_common -lhal_platform
OUTPUT_DIR := ../../output/test

TARGET_TEST1 := camera_test1
SRCS_TEST1 = camera_test1.c
OBJS_TEST1 = $(SRCS_TEST1:.c=.o)

#$(error $(OBJS))
all: $(TARGET_TEST1)


$(TARGET_TEST1): $(OBJS_TEST1)
	@mkdir -p $(OUTPUT_DIR)
	@echo ">>> Linking" $@ "..."
	$(CC) -o $(OUTPUT_DIR)/$@ .obj/*.o $(LIBS)

$(OBJS_TEST1): %.o:%.c
	@mkdir -p .obj
	@echo ">>> Compiling" $< "..."
	$(CC)  $(INCLUDE) -c -o .obj/$(notdir $@) $<

clean:
	rm -rf .obj
	rm -f $(OUTPUT_DIR)/$(TARGET_TEST1)

.PHONY: clean all
