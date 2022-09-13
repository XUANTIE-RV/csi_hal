##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##
DIR_TO_ROOT=..
include $(DIR_TO_ROOT)/build.param

ALL_TARGETS := common camera frame
ifeq ($(PLATFORM),simulator)
  ALL_TARGETS += vdec venc
endif

all: $(ALL_TARGETS)

common:
	@echo $(BUILD_LOG_START)
	make -C common/
	@echo $(BUILD_LOG_END)

camera:
	@echo $(BUILD_LOG_START)
	make -C camera/
	@echo $(BUILD_LOG_END)

frame:
	@echo $(BUILD_LOG_START)
	make -C frame/
	@echo $(BUILD_LOG_END)

vdec:
	@echo $(BUILD_LOG_START)
	make -C vdec/
	@echo $(BUILD_LOG_END)

venc:
	@echo $(BUILD_LOG_START)
	make -C venc/
	@echo $(BUILD_LOG_END)

clean:
	make -C common/ clean
	make -C camera/ clean
	make -C frame/ clean
	make -C vdec/ clean
	make -C venc/ clean
	rm -rf $(DIR_TO_ROOT)/output/examples/resource

.PHONY: clean all common camera frame vdec venc
