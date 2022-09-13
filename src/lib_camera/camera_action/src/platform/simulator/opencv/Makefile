##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##
DIR_TO_ROOT=../../../../../../..

include $(DIR_TO_ROOT)/build.param
LIBOPENCV_INC  = $(shell pkg-config --cflags opencv) -I./
LIBOPENCV_LIBS = $(shell pkg-config --libs opencv) 

TARGET := libcamera_utilities.a
OUTPUT_DIR := $(DIR_TO_ROOT)/output/hal/

#CFLAGS  = -Wall -g -O0

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET) 

$(TARGET): $(OBJS)
	@mkdir -p $(OUTPUT_DIR)
	@echo "Linking" $@ "..."
	$(AR) -r -o $(OUTPUT_DIR)/$@ .obj/*.o

$(OBJS): %.o:%.cpp
	@echo $(BUILD_LOG_START)
	@mkdir -p .obj
	@echo "Compiling" $< "..."
	$(CXX) $(CFLAGS) $(INCLUDE) $(LIBOPENCV_INC) $(LIBOPENCV_LIBS) -c -o .obj/$(notdir $@) $<  
	@echo $(BUILD_LOG_END)

clean:
	rm -rf .obj
	rm -f $(OUTPUT_DIR)/$(TARGET)

.PHONY: clean all 
