##
 # Copyright (C) 2021 Alibaba Group Holding Limited
 # Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 #
 # This program is free software; you can redistribute it and/or modify
 # it under the terms of the GNU General Public License version 2 as
 # published by the Free Software Foundation.
##

KVERS = $(shell uname -r)
CURDIR = $(shell pwd)

# Kernel modules
obj-m += ion_thead.o

# Specify flags for the module compilation.
EXTRA_CFLAGS = -g -O1
#EXTRA_CFLAGS += -O0  is not supported in ubuntu

DIR_TO_ROOT=../../../../..
OUTPUT_DIR =$(DIR_TO_ROOT)/output/driver

build: kernel_modules

kernel_modules:
	@mkdir -p $(OUTPUT_DIR)
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) modules
	cp ./*.ko $(OUTPUT_DIR)

clean:
	rm -f $(OUTPUT_DIR)/ion_thead.ko
	make -C /lib/modules/$(KVERS)/build M=$(CURDIR) clean
