/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: zhuxinran <fuqian.zxr@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "llist.h"

int llist_insert(LLIST *, const void *data, int mode);
void *llist_find(LLIST *, const void *data, llist_cmp *cmp);
int llist_delete(LLIST *, const void *data, llist_cmp *cmp);
int llist_fetch(LLIST *, const void *data, llist_cmp *cmp, void *rdata);
void llist_travel(LLIST *, llist_op *op);

LLIST *llist_create(int size)
{
	LLIST *me;

	me = malloc(sizeof(*me));
	if (me == NULL)
		return NULL;

	me->size = size;
	me->head.prev = &me->head;
	me->head.next = &me->head;

	me->insert = llist_insert;
	me->find = llist_find;
	me->delete = llist_delete;
	me->fetch = llist_fetch;
	me->travel = llist_travel;

	return me;
}


int llist_insert(LLIST *me, const void *data, int mode)
{
	struct llist_node_st *newnode;

	newnode = malloc(sizeof(*newnode) + me->size);
	if (newnode == NULL)
		return -1;

	memcpy(newnode->data, data, me->size);

	if (mode == LLIST_FORWARD) {
		newnode->prev = &me->head;
		newnode->next = me->head.next;
	} else if (mode == LLIST_BACKWARD) {
		newnode->prev = me->head.prev;
		newnode->next = &me->head;
	} else	// error
		return -3;

	newnode->prev->next = newnode;
	newnode->next->prev = newnode;

	return 0;
}

static struct llist_node_st *find_(LLIST *me, const void *data, llist_cmp *cmp)
{
	struct llist_node_st *cur;

	for (cur = me->head.next ;  cur != &me->head ; cur = cur->next) {
		if (cmp(cur->data, data) == 0)
			break;
	}

	return cur;
}

void *llist_find(LLIST *me, const void *data, llist_cmp *cmp)
{
	struct llist_node_st *cur;

	cur = find_(me, data, cmp);
	if (cur == &me->head)
		return NULL;
	return cur->data;
}

int llist_delete(LLIST *me, const void *data, llist_cmp *cmp)
{
	struct llist_node_st *node;

	node = find_(me, data, cmp);
	if (node == &me->head)
		return -1;

	node->prev->next = node->next;
	node->next->prev = node->prev;
	free(node);

	return 0;
}

int llist_fetch(LLIST *me, const void *data, llist_cmp *cmp, void *rdata)
{
	struct llist_node_st *node;

	node = find_(me, data, cmp);
	if (node == &me->head)
		return -1;

	if (rdata != NULL)
		memcpy(rdata, node->data, me->size);

	node->prev->next = node->next;
	node->next->prev = node->prev;
	free(node);

	return 0;
}

void llist_travel(LLIST *me, llist_op *op)
{
	struct llist_node_st *cur;

	for (cur = me->head.next ;  cur != &me->head ; cur = cur->next) {
		op(cur->data);
	}
}

void llist_destroy(LLIST *me)
{
	struct llist_node_st *cur, *next;

	for (cur = me->head.next ;  cur != &me->head ; cur = next) {
		next = cur->next;
		free(cur);
	}

	free(me);
	return ;
}
