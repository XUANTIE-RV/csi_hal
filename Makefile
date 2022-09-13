##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##

include build.param

all: info common platform tests examples

info:
	@echo $(BUILD_LOG_START)
	@echo "  ========== Build Info =========="
	@echo "    Platform: "$(PLATFORM)
	@echo "    ROOT_DIR: "$(ROOT_DIR)
	@echo $(BUILD_LOG_END)

common:
	@echo $(BUILD_LOG_START)
	make -C src/common
	@echo $(BUILD_LOG_END)

platform: common
	@echo $(BUILD_LOG_START)
	make -C src/platform
	@echo $(BUILD_LOG_END)

libs: platform
	@echo $(BUILD_LOG_START)
	make -C src/lib_camera
	@echo $(BUILD_LOG_END)

tests: platform libs
	@echo $(BUILD_LOG_START)
	make -C tests
	@echo $(BUILD_LOG_END)

examples: platform libs
	@echo $(BUILD_LOG_START)
	make -C examples
	@echo $(BUILD_LOG_END)

clean:
	rm -rf .obj
	make -C src/common/ clean
	make -C src/platform/ clean
	make -C src/lib_camera clean
	make -C tests/ clean
	make -C examples/ clean

.PHONY: clean all info common platform tests examples
