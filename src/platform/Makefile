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

all: platform

platform:
	make -C $(PLATFORM)

clean:
	make -C $(PLATFORM) clean


.PHONY: clean all info platform common
