/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: zhuxinran <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LLIST_H__
#define __LLIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#define LLIST_FORWARD		1
#define LLIST_BACKWARD		2

typedef void llist_op(const void *);
typedef int llist_cmp(const void *, const void *);

struct llist_node_st {
	struct llist_node_st *prev;
	struct llist_node_st *next;
	char data[1];
};

typedef struct llist_head_st {
	int size;
	struct llist_node_st head;
	int (*insert)(struct llist_head_st *, const void *data, int mode);
	void *(*find)(struct llist_head_st *, const void *data, llist_cmp *cmp);
	int (*delete)(struct llist_head_st *, const void *data, llist_cmp *cmp);
	int (*fetch)(struct llist_head_st *, const void *data, llist_cmp *cmp,
		     void *rdata);
	void (*travel)(struct llist_head_st *, llist_op *op);
} LLIST;

LLIST *llist_create(int size);

void llist_destroy(LLIST *);

#ifdef __cplusplus
}
#endif

#endif


