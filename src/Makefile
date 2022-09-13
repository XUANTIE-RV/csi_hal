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

all: common platform camera

common:
	make -C common

platform: common
	make -C platform

camera: platform
	make -C lib_camera

clean:
	make -C common clean
	make -C platform clean

.PHONY: clean all common platform

