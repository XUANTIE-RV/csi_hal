/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdio.h>

#define LOG_WITH_COLOR

#define LOG_COLOR_RED_YELLO_BACK "\033[1;31;43m"
#define LOG_COLOR_RED            "\033[2;31;49m"
#define LOG_COLOR_YELLOW         "\033[2;33;49m"
#define LOG_COLOR_GREEN          "\033[2;32;49m"
#define LOG_COLOR_BLUE           "\033[2;34;49m"
#define LOG_COLOR_GRAY           "\033[1;30m"
#define LOG_COLOR_WHITE          "\033[1;47;49m"
#define LOG_COLOR_RESET          "\033[0m"

#ifdef LOG_WITH_COLOR
const char *PFORMAT_D    = LOG_COLOR_GRAY   "[%sD][%s():%d] " LOG_COLOR_RESET;
const char *PFORMAT_I    = LOG_COLOR_BLUE   "[%sI][%s():%d] " LOG_COLOR_RESET;
const char *PFORMAT_W    = LOG_COLOR_YELLOW "[%sW][%s():%d] " LOG_COLOR_RESET;
const char *PFORMAT_E    = LOG_COLOR_RED    "[%sE][%s():%d] " LOG_COLOR_RESET;
const char *PFORMAT_O    = LOG_COLOR_GREEN  "[%sO][%s():%d] " LOG_COLOR_RESET;
#else
const char *PFORMAT_D    = "[%sD][%s():%d] ";
const char *PFORMAT_I    = "[%sI][%s():%d] ";
const char *PFORMAT_W    = "[%sW][%s():%d] ";
const char *PFORMAT_E    = "[%sE][%s():%d] ";
const char *PFORMAT_O    = "[%sO][%s():%d] ";
#endif
