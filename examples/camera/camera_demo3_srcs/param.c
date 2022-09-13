/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <string.h>
#include <stdlib.h>

#include "param.h"

char param[10][10][MENU_ITEM_MAX_LEN+1];

static void GetSubStr(char *des, char *src, char ch, int n)
{
	int i, len;
	char *p1, *p, tmp[300];
	strcpy(tmp, src);
	*des = 0;
	p1 = tmp;

	for (int i = 0; i < n; i++) {
		p = (char *)strchr(p1, ch);
		if (p != NULL) {
			*p++ = 0;
			p1 = p;
		}
	}

	p = (char *)strchr(p1, ch);
	if (p != NULL) {
		*p = 0;
		strcpy(des, p1);
	}
}

int get_param(char *name)
{
	FILE *fp;
	char ss[201], xm[3], gs[3];
	int i, j;
	sprintf(ss, "%s.conf", name);
	if ((fp = fopen(ss, "r")) == NULL) return (-1);
	for (j = 0; j < 10; j++)
		for (i = 0; i < 10; i++)
			memset(param[j][i], 0, 13);
	while (1) {
		memset(ss, 0, 201);
		fgets(ss, 200, fp);
		if (feof(fp)) break;
		if (ss[0] == '#') continue;
		GetSubStr(xm, ss, '|', 0);
		GetSubStr(gs, ss, '|', 1);
		j = atoi(xm);
		for (i = 1; i <= atoi(gs); i++) {
			sprintf(param[j][0], "%s", gs);
			GetSubStr(param[j][i], ss, '|', i + 1);
		}
	}
	fclose(fp);
	return (0);
}

