/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>
#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "camera_demo3"
#include <syslog.h>

#include "camera_manager.h"
#include "param.h"
#include "app_dialogs.h"
#include "menu_process.h"

cams_t *cam_session;

WINDOW *menubar;
WINDOW *messagebar;
WINDOW *win_border;
WINDOW *win_content;

void init_curses()
{
	initscr();		/* init curses */

	FILE *f = fopen("/dev/tty", "r+");
	SCREEN *screen = newterm(NULL, f, f);
	set_term(screen);

	start_color();		/* color init, then init color pair */

	init_pair(1, COLOR_WHITE,  COLOR_BLACK);
	init_pair(2, COLOR_BLACK,  COLOR_WHITE);
	init_pair(3, COLOR_RED,    COLOR_WHITE);
	init_pair(4, COLOR_WHITE,  COLOR_BLUE);		// menu font
	init_pair(5, COLOR_YELLOW, COLOR_BLACK);
	init_pair(6, COLOR_GREEN,  COLOR_WHITE);
	init_pair(7, COLOR_YELLOW, COLOR_WHITE);

	curs_set(0);		/* Cursor mode: 0:hide/1:normal/2:hilight */
	noecho();		/* Do not display on screen */
	keypad(stdscr, TRUE);	/* allow keypad map */
}

void uninit_curses()
{
	echo();
	endwin();
}

void draw_menubar(WINDOW *menubar)
{
	wbkgd(menubar, COLOR_PAIR(2));

	for (int i = 0; i < atoi(param[0][0]); i++) {
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
	items[0] = newwin(atoi(param[menu][0]) + 2, MENU_ITEM_MAX_LEN+2, 3, start_col);
	wbkgd(items[0], COLOR_PAIR(2));
	box(items[0], ACS_VLINE, ACS_HLINE);
	for (i = 1; i <= atoi(param[menu][0]); i++) {
		items[i] = subwin(items[0], 1, MENU_ITEM_MAX_LEN, 3 + i, start_col + 1);
		wprintw(items[i], "%s", param[menu][i]);
	}
	wbkgd(items[1], COLOR_PAIR(4));
	wrefresh(items[0]);
	return items;
}

void delete_menu(WINDOW **items, int count)
{
	int i;
	for (i = 0; i < count; i++)
		delwin(items[i]);
	free(items);
}

int scroll_menu(WINDOW **items, int menu, int *menu_returned)
{
	int key, count, selected = 0;
	count = atoi(param[menu][0]);
	LOG_D("Active: menu=%d, item=%d\n", menu, selected);

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
				*menu_returned = menu;
				LOG_D("Active: menu=%d, item=%d\n", *menu_returned, selected);
				return scroll_menu(items, menu, menu_returned);
			}
			if (key == KEY_RIGHT) {
				menu += 1;
				if (menu > atoi(param[0][0])) menu = 1;
				items = draw_menu(menu);
				*menu_returned = menu;
				LOG_D("Active: menu=%d, item=%d\n", *menu_returned, selected);
				return scroll_menu(items, menu, menu_returned);
			}
		} else if (key == ESCAPE || key == '0' || key == 'q') {
			delete_menu(items, count + 1);
			return -1;
		} else if (key == ENTER) {
			delete_menu(items, count + 1);
			return selected;
		}
		LOG_D("Active: menu=%d, item=%d\n", *menu_returned, selected);
	}
}

/* ss:     display string,
 * status: 0:Normal, 1:OK, 2:Failed, 3:Warning */
void message(char *ss, int status)
{
	int color_pair;
	switch (status) {
	case 1:
		color_pair = 6; // GREEN:WHITE
		break;
	case 2:
		color_pair = 3;	// RED:WHITE
		break;
	case 3:
		color_pair = 7;	// YELLOW:WHITE
		break;
	default:
		color_pair = 2;	// BLACK:WHITE
		break;
	};
	wbkgd(messagebar, COLOR_PAIR(2));
	wattron(messagebar, COLOR_PAIR(color_pair));
	mvwprintw(messagebar, 0, 0, "%80s", " ");
	mvwprintw(messagebar, 0, 1, "%s", ss);
	wattroff(messagebar, COLOR_PAIR(color_pair));
	wrefresh(messagebar);
}

void copyright()
{
	char *str[] = { "Copyright (C) 2021 Alibaba Group Holding Limited",
			"Author: LuChongzhi, T-Head / IOT / OS Team",
			"mailto:[chongzhi.lcz@alibaba-inc.com]"};

	attron(A_UNDERLINE | COLOR_PAIR(1));
	for (int i = 0; i < sizeof(str) / sizeof(char *); i++)
		mvaddstr((WIN_ROWS - 2) / 2 + i, (WIN_COLS - strlen(str[0])) / 2, str[i]);

	attroff(A_UNDERLINE | COLOR_PAIR(1));
	refresh();
}

int main(int argc, char **argv)
{
	int ret = -1;
	int key;
	WINDOW **menu_items;

	if (get_param(argv[0])) {
		LOG_E("\n Open %s.conf failed!\n", argv[0]);
		goto LAB_EXIT;
	}

	if (camera_create_session(&cam_session) != 0 || cam_session == NULL) {
		LOG_E("camera_create_session() failed\n");
		goto LAB_EXIT;
	}

	init_curses();

	if (init_dialog(NULL)) {
		fprintf(stderr, N_("Your display is too small to run Menuconfig!\n"));
		fprintf(stderr, N_("It must be at least 19 lines by 80 columns.\n"));
		goto LAB_EXIT_CURSES_INITED;
	}

	bkgd(COLOR_PAIR(1));	/* COLOR_WHITE, COLOR_BLACK */

	/* set windows postion: (win, lines, cols, begin_y, begin_x) */
	menubar = subwin(stdscr, 1, WIN_COLS, 1, 0);			/* Menu bar line */
	messagebar = subwin(stdscr, 1, WIN_COLS, WIN_ROWS - 1, 0);	/* Bottom line */
	win_border = subwin(stdscr, WIN_ROWS - 3, WIN_COLS, 2, 0);	/* Content win with border */
	win_content = subwin(stdscr, WIN_ROWS - 5, WIN_COLS - 2, 3, 1);	/* Content win without border */

	/* Set window title */
	char str_title[81];
	strcpy(str_title, "<<< CSI Camera Test Tool >>>");
	wattron(stdscr, COLOR_PAIR(5));
	mvwprintw(stdscr, 0, (80 - strlen(str_title)) / 2 - 1, "%s", str_title);
	wattroff(stdscr, COLOR_PAIR(5));

	/* Draw menu */
	draw_menubar(menubar);
	message("Use number key to choses menu. 'q' or '0' to Exit", 0);

	/* Draw box border */
	box(win_border, ACS_VLINE, ACS_HLINE);

	/* Show copyright */
	copyright();

	refresh();

	/* menu operate loop */
	do {
		key = wgetch(stdscr);
		if (isdigit(key) && key > '0' && key <= atoi(param[0][0]) + '0') {
			werase(messagebar);
			wrefresh(messagebar);

			int menu_init = key - '0';	// first press menu id
			int menu_final = menu_init;	// return from scroll_menu
			menu_items = draw_menu(menu_init);
			int item_selected = scroll_menu(menu_items, menu_init, &menu_final);
			LOG_D("menu_final=%d, selected_item=%d\n",
				menu_final, item_selected);

			switch(menu_final) {
			case MENU_CAMERA:
				menu_camera_process(item_selected);
				break;
			case MENU_CHANNEL:
				menu_channel_process(item_selected);
				break;
			case MENU_EVENT_RUN:
				menu_event_run_process(item_selected);
				break;
			default:
				LOG_W("Not supported menu: %d\n", menu_final);
				break;
			}

			if (item_selected == 13) {
				LOG_D("Test dialog_inputbox\n");
				dialog_inputbox("inputbox", "prompt", 16, 80,
					"const char init");
			}

			touchwin(stdscr);
			refresh();
		} else if (key == KEY_RESIZE) {
			box(win_border, ACS_VLINE, ACS_HLINE);
			wrefresh(messagebar);
		}
	} while (key != 'q' && key != '0');

	ret = 0;

LAB_EXIT_WIN_CREATED:
	delwin(win_content);
	delwin(win_border);
	delwin(menubar);
	delwin(messagebar);

LAB_EXIT_CURSES_INITED:
	uninit_curses();

LAB_EXIT_CAM_MGR_CREATED:
	camera_destory_session(&cam_session);

LAB_EXIT:
	return ret;
}

