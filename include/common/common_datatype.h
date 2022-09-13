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
#ifndef __COMMON_DATATYPE_H__
#define __COMMON_DATATYPE_H__

#ifndef NULL
#define	NULL  0x00
#endif

#ifndef TRUE
#define TRUE  0x01
#endif
#ifndef FALSE
#define FALSE 0x00
#endif

#ifndef true
#define true  0x01
#endif
#ifndef false
#define false 0x00
#endif


#ifndef bool
typedef unsigned char       bool;
#endif

#if 0	/* below types are defined in /usr/include/stdint.h */
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;

typedef unsigned long long  uint64_t;
#endif

#endif /* __COMMON_DATATYPE_H__ */

