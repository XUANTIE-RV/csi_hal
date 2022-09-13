/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
MODULE_LICENSE("Dual BSD/GPL");

extern int fun(void);
static int hello_init(void)
{
	printk(KERN_INFO " Hello World enter\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_INFO " Hello World exit\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("LuChongzhi");
MODULE_DESCRIPTION("A simple Hello World Module");
MODULE_ALIAS("a simplest module");
MODULE_LICENSE("GPL");
