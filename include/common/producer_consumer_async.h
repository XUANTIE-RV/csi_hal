/*
 * Copyright 2018-2019 C-SKY Microsystems Co., Ltd.
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
#ifndef __PRODUCER_CONSUMER_ASYNC_H__
#define __PRODUCER_CONSUMER_ASYNC_H__

#include "common.h"

struct pca_data_info;
typedef int32_t (*pca_consumed_callback)(struct pca_data_info* data_info);

typedef struct pca_data_info {
	void* data;				/* data pointer */
	uint32_t length;			/* data length */
	uint32_t type;				/* data type */
	pca_consumed_callback consumed_callback;/* data need free by dispatcher */
} pca_data_info_t;

typedef struct pca_data_statistics {
	uint32_t total_delivered;	/* Data feeded in */
	uint32_t total_discarded;	/* Data discarded because busy */
	uint32_t total_updated;		/* New data updated in before consume */
	uint32_t total_consumed;	/* Data consumed */
} pca_data_statistics_t;

typedef int32_t (*pca_consumer_process)(pca_data_info_t* data_info);

typedef struct pca_thread_info {
	pthread_t tid;			/* thread ID */
	char name[32];			/* thread name */
	sem_t sem_wakeup;		/* semphore to wakeup thread */
	bool  is_running;

	pca_data_info_t data_info;	/* the data delivered from parent */
	bool has_new_data;		/* indicate there is new data to handle */
	bool need_unlock_data_mutex;	/* Just for mark flag when need unlock */
	pthread_mutex_t mutex_data;	/* mutex to protect handling data */
	pca_data_statistics_t stats;	/* the statistics of data */
} pca_thread_info_t;

typedef struct pca_consumer_processor_info {
	pca_consumer_process process;	/* consumer processor function */
	int data_type;			/* the data type to be consume */
	char name[16];			/* consumer name for debug */
} pca_consumer_processor_info_t;

struct pca_dispatcher_info;		/* Declare here but defined below */

typedef struct pca_consumer_list_info {
	pca_thread_info_t thread_info;		/* thread info */
	pca_consumer_processor_info_t processor;/* post processor info */
	struct list_head list;			/* to build list */
	struct pca_dispatcher_info* dispatcher;	/* dispatcher's pointer */
} pca_consumer_list_info_t;

typedef struct {
	pca_data_info_t data_info;		/* data_info */
	unsigned int reference_count;		/* data using count */
	struct list_head list;			/* to build list */
} pca_data_free_list_info_t;

typedef struct pca_dispatcher_info {
	pca_thread_info_t thread_info;		/* thread info */

	pthread_mutex_t consumer_mutex;		/* mutex to protect consumer_list */
	pca_consumer_list_info_t consumer_list;	/* consumers list */

	pthread_mutex_t data_free_mutex;	/* mutex to protect data_free_list */
	pca_data_free_list_info_t data_free_list;/* data list need to free */
} pca_dispatcher_info_t;

int32_t pca_dispatcher_create(pca_dispatcher_info_t** dispatcher, char* name);
int32_t pca_dispatcher_run(pca_dispatcher_info_t* dispatcher);
int32_t pca_dispatcher_feed(pca_dispatcher_info_t* dispatcher,
			    void *data, int32_t data_type);
int32_t pca_dispatcher_stop(pca_dispatcher_info_t* dispatcher);
int32_t pca_dispatcher_destory(pca_dispatcher_info_t* dispatcher);

int32_t pca_add_consumer_processor(pca_dispatcher_info_t* dispatcher,
				   pca_consumer_processor_info_t *processor);
int32_t pca_remove_consumer_processor(pca_dispatcher_info_t* dispatcher,
				      pca_consumer_processor_info_t *processor);
int32_t pca_feed_consumers(pca_dispatcher_info_t* dispatcher,
			   pca_data_info_t* data_info);
int32_t pca_feed_dispatcher(pca_dispatcher_info_t* dispatcher,
			    pca_data_info_t* data_info);

#endif /* __PRODUCER_CONSUMER_ASYNC_H__ */

