/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* ref: https://my.oschina.net/u/1387955/blog/182281 */
/*************   主程序    *************/
/***gcc menu.c -lcurses -o menu***/
#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#define  ENTER 10
#define  ESCAPE 27

WINDOW *menubar, *messagebar, *temp, *temp1;
char param[10][10][13];

void init_curses()
{
	initscr();
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_WHITE);
	init_pair(3, COLOR_RED, COLOR_WHITE);
	init_pair(4, COLOR_WHITE, COLOR_RED);
	curs_set(0);
	noecho();
	keypad(stdscr, TRUE);
}

void GetSubStr(char *des, char *src, char ch, int n)
{
	int i, len;
	char *p1, *p, tmp[300];
	strcpy(tmp, src);
	*des = 0;
	p1 = tmp;
	i = 0;
	while (i < n) {
		i++;
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

void draw_menubar(WINDOW *menubar)
{
	int i;
	wbkgd(menubar, COLOR_PAIR(2));
	for (i = 0; i < atoi(param[0][0]); i++) {
		wattron(menubar, COLOR_PAIR(3));
		mvwprintw(menubar, 0, i * 14 + 2, "%1d.", i + 1);
		wattroff(menubar, COLOR_PAIR(3));
		mvwprintw(menubar, 0, i * 14 + 4, "%-12s", param[0][i + 1]);
	}
}

WINDOW **draw_menu(int menu)
{
	int i, start_col;
	WINDOW **items;
	items = (WINDOW **)malloc((atoi(param[menu][0]) + 1) * sizeof(WINDOW *));
	start_col = (menu - 1) * 14 + 2;
	items[0] = newwin(atoi(param[menu][0]) + 2, 14, 3, start_col);
	wbkgd(items[0], COLOR_PAIR(2));
	box(items[0], ACS_VLINE, ACS_HLINE);
	for (i = 1; i <= atoi(param[menu][0]); i++) {
		items[i] = subwin(items[0], 1, 12, 3 + i, start_col + 1);
		wprintw(items[i], "%s", param[menu][i]);
	}
	wbkgd(items[1], COLOR_PAIR(4));
	wrefresh(items[0]);
	return items;
}

void delete_menu(WINDOW **items, int count)
{
	int i;
	for (i = 0; i < count; i++) delwin(items[i]);
	free(items);
}

int scroll_menu(WINDOW **items, int menu)
{
	int key, count, selected = 0;
	count = atoi(param[menu][0]);
	while (1) {
		key = getch();
		if (key == KEY_DOWN || key == KEY_UP) {
			wbkgd(items[selected + 1], COLOR_PAIR(2));
			wnoutrefresh(items[selected + 1]);
			if (key == KEY_DOWN)
				selected = (selected + 1) % count;
			else
				selected = (selected + count - 1) % count;
			wbkgd(items[selected + 1], COLOR_PAIR(4));
			wnoutrefresh(items[selected + 1]);
			doupdate();
		} else if (key == KEY_LEFT || key == KEY_RIGHT) {
			delete_menu(items, count + 1);
			touchwin(stdscr);
			refresh();
			if (key == KEY_LEFT) {
				menu -= 1;
				if (menu <= 0) menu = atoi(param[0][0]);
				items = draw_menu(menu);
				return scroll_menu(items, menu);
			}
			if (key == KEY_RIGHT) {
				menu += 1;
				if (menu > atoi(param[0][0])) menu = 1;
				items = draw_menu(menu);
				return scroll_menu(items, menu);
			}
		} else if (key == ESCAPE || key == '0' || key == 'q') {
			delete_menu(items, count + 1);
			return -1;
		} else if (key == ENTER) {
			delete_menu(items, count + 1);
			return selected;
		}
	}
}

void message(char *ss)
{
	wbkgd(messagebar, COLOR_PAIR(2));
	wattron(messagebar, COLOR_PAIR(3));
	mvwprintw(messagebar, 0, 0, "%80s", " ");
	mvwprintw(messagebar, 0, (80 - strlen(ss)) / 2 - 1, "%s", ss);
	wattroff(messagebar, COLOR_PAIR(3));
	wrefresh(messagebar);
}

void copyright()
{
	char *str[] = {   "Orignal Author:htldm@bbs.chinaunix.net",
			  "Modified By:Frank.Z @wh.cn",
			  "mailto:[frankzhang02010@gmail.com]",
			  "Last Modified:2013.12.7 [GMT+8]"
		      };
	int rows, cols;
	int i;
	getmaxyx(stdscr, rows, cols);
	attron(A_UNDERLINE | COLOR_PAIR(1));
	for (i = 0; i < 4; i++)
		mvaddstr((rows - 2) / 2 + i, (cols - strlen(str[2])) / 2, str[i]);
	attroff(A_UNDERLINE | COLOR_PAIR(1));
	refresh();
}

int main(int argc, char **argv)
{
	int key;
	int selected_item;
	char ss[81];
	WINDOW **menu_items;

	if (get_param(argv[0])) {
		printf("\n打开配置文件 %s.conf 错!\n", argv[0]);
		return (-1);
	}

	init_curses();
	bkgd(COLOR_PAIR(1));
	menubar = subwin(stdscr, 1, 80, 1, 0);
	messagebar = subwin(stdscr, 1, 80, 24, 0);
	temp = subwin(stdscr, 22, 80, 2, 0);
	temp1 = subwin(stdscr, 20, 78, 3, 1);
	strcpy(ss, "General Menu Generate Program");
	mvwprintw(stdscr, 0, (80 - strlen(ss)) / 2 - 1, "%s", ss);
	draw_menubar(menubar);
	message("Use nub key to choses menu. ESC or '0'to Exit");
	box(temp, ACS_VLINE, ACS_HLINE);
	refresh();
	copyright();
	do {
		key = getch();
		if (isdigit(key) && key > '0' && key <= atoi(param[0][0]) + '0') {
			werase(messagebar);
			wrefresh(messagebar);
			menu_items = draw_menu(key - '0');
			selected_item = scroll_menu(menu_items, key - '0');
			touchwin(stdscr);
			refresh();
		}
	} while (key != ESCAPE && key != 'q' && key != '0');

	delwin(temp1);
	delwin(temp);
	delwin(menubar);
	delwin(messagebar);
	endwin();
	return (0);
}

