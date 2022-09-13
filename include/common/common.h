/*
 * Copyright 2018 C-SKY Microsystems Co., Ltd.
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <mcheck.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common_errno.h"
#include "common_datatype.h"
#include "syslog.h"
#include "list.h"


#define CHECK_RET_RETURN(ret) \
	if (ret != 0) { \
		LOG_E("failed, ret=%d\n", ret); \
		return ret; \
	}

#define CHECK_RET_EXIT(ret) \
	if (ret != 0) { \
		LOG_E("failed, ret=%d\n", ret); \
		exit(ret); \
	}

#define CHECK_RET_WARNING(ret) \
		if (ret != 0) { \
			LOG_W("Warning, ret=%d\n", ret); \
		}

#define CHECK_POINTER_RETURN(pointer) \
	if (pointer == NULL) { \
		LOG_E("pointer is NULL\n"); \
		return ERRNO_INVALID_PARAMETER; \
	}

#endif /* __COMMON_H__ */

