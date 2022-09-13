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

all: helloworld ion

helloworld:
	@echo $(BUILD_LOG_START)
	make -C helloworld
	@echo $(BUILD_LOG_END)

ion:
	@echo $(BUILD_LOG_START)
	make -C ion
	make -C tests/ion
	@echo $(BUILD_LOG_END)

clean:
	make -C helloworld clean
	make -C ion clean
	make -C tests/ion clean

.PHONY: clean all helloworld ion
