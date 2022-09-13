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
/* 0: Error; 1: Error|Warning; 2:Error|Warning|Info; 3:Error|Warning|Info|Debug */
#define LOG_LEVEL 0

#include "common.h"
#include "producer_consumer_async.h"

typedef enum {
	_PCA_DATA_DISPOSE_DISCARDED,
	_PCA_DATA_DISPOSE_DELIVERED,
	_PCA_DATA_DISPOSE_UPDATED,

	_PCA_DATA_DISPOSE_COUNT
} _pca_data_dispose_e;

static const char* _pca_data_dispose_string[_PCA_DATA_DISPOSE_COUNT] = {
	"discarded(-)", "delivered(+)", "updated(.)"
};
#define _PCA_DATA_DISPOSE_STRING(dispose) (_pca_data_dispose_string[dispose])

static int list_get_count(const struct list_head * head)
{
	uint32_t count = 0;
	struct list_head *pos;
	list_for_each(pos, head){
		count ++;
	}
	return count;
}

static int32_t _pca_init_thread_info(pca_thread_info_t* info)
{
	CHECK_POINTER_RETURN(info);

	memset(info, 0, sizeof(pca_thread_info_t));
	info->tid = -1;
	sem_init(&info->sem_wakeup, 0, 0);
	pthread_mutex_init(&info->mutex_data, NULL);

	return SUCCESS;
}

static inline int32_t _pca_data_free(pca_data_info_t* data_info)
{
	CHECK_POINTER_RETURN(data_info);
	if (data_info->consumed_callback != NULL) {
		data_info->consumed_callback(data_info);
	}
	return SUCCESS;
}

static void _pca_data_free_list_dump(pca_data_free_list_info_t* data_free_list)
{
#if (LOG_LEVEL >= 3)
	pca_data_free_list_info_t* item;
	struct list_head *pos;
	uint32_t count = 0;

	LOG_D("Dumpping items in list begin\n");
	list_for_each(pos, &data_free_list->list) {
		item = list_entry(pos, pca_data_free_list_info_t, list);
		count++;
		LOG_D("\t data(%p,%d,%d,%d), reference_count=%d\n",
			item->data_info.data,
			*((uint32_t*)item->data_info.data),
			item->data_info.length,
			item->data_info.type,
			item->reference_count);
	}
	LOG_D("There are %d items in list\n", count);
#endif
}

static void _pca_thread_dump_statistics(pca_data_statistics_t *stats, char *name)
{
	LOG_I("The statistics of '%s' are:\n"
		"  total_delivered = %d\n"
		"  total_discarded = %d\n"
		"  total_updated   = %d\n"
		"  total_consumed  = %d\n\n",
		(name == NULL) ? "unknown" : name,
		stats->total_delivered,
		stats->total_discarded,
		stats->total_updated,
		stats->total_consumed);
}

static inline int32_t _pca_add_data_free_item(pca_dispatcher_info_t* dispatcher,
					      pca_data_info_t* data_info)
{
	pca_data_free_list_info_t* data_free_list = &dispatcher->data_free_list;
	struct list_head *pos;
	pca_data_free_list_info_t* item;
	bool found_in_list = false;

	if (data_info->consumed_callback == NULL)
		return SUCCESS;

	LOG_D("Add data(%p,%d,%d,%d) to list\n",
		data_info->data, *((uint32_t*)data_info->data),
		data_info->length, data_info->type);

	pthread_mutex_lock(&dispatcher->data_free_mutex);
	/* If data has been found in list, reference_count++ */
	list_for_each(pos, &data_free_list->list) {
		item = list_entry(pos, pca_data_free_list_info_t, list);
		if (item->data_info.data == data_info->data) {
			found_in_list = true;
			item->reference_count++;
			break;
		}
	}

	/* If data has not been found in list, create & add into list */
	if(!found_in_list) {
		pca_data_free_list_info_t* item =
			malloc(sizeof(pca_data_free_list_info_t));
		memcpy(&item->data_info, data_info, sizeof(pca_data_info_t));
		item->reference_count = 1;

		list_add_tail(&item->list, &data_free_list->list);
	}
	_pca_data_free_list_dump(data_free_list);
	pthread_mutex_unlock(&dispatcher->data_free_mutex);

	return SUCCESS;
}

static inline int32_t _pca_remove_data_free_item(pca_dispatcher_info_t* dispatcher,
						 pca_data_info_t* data_info)
{
	pca_data_free_list_info_t* data_free_list = &dispatcher->data_free_list;
	struct list_head *pos;
	pca_data_free_list_info_t* item;
	bool found_in_list = false;

	if (data_info->consumed_callback == NULL)
		return SUCCESS;

	/* Search in the list */
	LOG_D("Remove data(%p,%d,%d,%d) from list\n",
		data_info->data, *((uint32_t*)data_info->data),
		data_info->length, data_info->type);

	pthread_testcancel();
	pthread_mutex_lock(&dispatcher->data_free_mutex);
	list_for_each(pos, &data_free_list->list) {
		item = list_entry(pos, pca_data_free_list_info_t, list);
		if (item->data_info.data == data_info->data) {
			found_in_list = true;
			item->reference_count--;
			/* If reference_count is 0, then remove from list */
			if (item->reference_count == 0) {
				LOG_D("Free\n");
				_pca_data_free(&item->data_info);
				list_del(&item->list);
				free(item);
			}
			break;
		}
	}
	_pca_data_free_list_dump(data_free_list);
	pthread_mutex_unlock(&dispatcher->data_free_mutex);

	if (!found_in_list) {
		LOG_W("Can't find the data(%p,%d,%d,%d) in list!\n",
			data_info->data, *((uint32_t*)data_info->data),
			data_info->length, data_info->type);
	}

	return SUCCESS;
}

static void _pca_flush_data_free_list(pca_dispatcher_info_t* dispatcher)
{
	pca_data_free_list_info_t* data_free_list = &dispatcher->data_free_list;
	struct list_head *pos;
	pca_data_free_list_info_t* item;

	pthread_mutex_lock(&dispatcher->data_free_mutex);
	list_for_each(pos, &data_free_list->list) {
		item = list_entry(pos, pca_data_free_list_info_t, list);

		list_del(&item->list);
		LOG_D("Free\n");
		_pca_data_free(&item->data_info);
		free(item);
	}
	_pca_data_free_list_dump(data_free_list);
	pthread_mutex_unlock(&dispatcher->data_free_mutex);
	LOG_I("Flush data free list finished\n");
}

static void* _pca_thread_consumer(void *arg)
{
	pca_consumer_list_info_t* consumer = (pca_consumer_list_info_t*)arg;
	pca_dispatcher_info_t* dispatcher = consumer->dispatcher;

	LOG_D("Entering\n");
	consumer->thread_info.is_running = true;

	while (1) {
		sem_wait(&consumer->thread_info.sem_wakeup);
		LOG_D("sem_wait\n");
		if (!consumer->thread_info.has_new_data)
			continue;

		/* Use mutex to protect data can't be modify while handling */
		pthread_mutex_lock(&consumer->thread_info.mutex_data);
		LOG_D("pthread_mutex_lock\n");
		pca_consumer_process process = consumer->processor.process;
		int process_data_type = consumer->processor.data_type;

		pca_data_info_t* data_info = &consumer->thread_info.data_info;
		if (process_data_type == data_info->type) {
			process(data_info);
			consumer->thread_info.stats.total_consumed++;
			LOG_D("Data(%p,%d,%d,%d) is consumed by %s.\n",
				data_info->data, *((uint32_t*)(data_info->data)),
				data_info->length, data_info->type,
				consumer->thread_info.name);
		} else {
			LOG_W("This data_type(%d) should not dispatcher to %s\n",
				data_info->type, consumer->processor.name);
		}
		_pca_remove_data_free_item(dispatcher, data_info);
		consumer->thread_info.has_new_data = false;
		pthread_mutex_unlock(&consumer->thread_info.mutex_data);

	}
	return NULL;
}

static void* _pca_thread_dispatcher(void *arg)
{
	pca_dispatcher_info_t* dispatcher = (pca_dispatcher_info_t*)arg;
	int i, ret;

	LOG_I("Entering\n");
	dispatcher->thread_info.is_running = true;

	while (1) {
		sem_wait(&dispatcher->thread_info.sem_wakeup);

		pthread_mutex_lock(&dispatcher->thread_info.mutex_data);
		ret = pca_feed_consumers(dispatcher, &dispatcher->thread_info.data_info);
		if (ret != SUCCESS) { /* No consumer accept the data */
			LOG_D("Free\n");
			_pca_data_free(&dispatcher->thread_info.data_info);
		}
		dispatcher->thread_info.has_new_data = false;
		dispatcher->thread_info.stats.total_consumed++;
		pthread_mutex_unlock(&dispatcher->thread_info.mutex_data);
	}
	return NULL;
}

int32_t pca_dispatcher_create(pca_dispatcher_info_t** dispatcher, char* name)
{
	CHECK_POINTER_RETURN(dispatcher);
	CHECK_POINTER_RETURN(name);
	pca_dispatcher_info_t *info = malloc(sizeof(pca_dispatcher_info_t));
	CHECK_POINTER_RETURN(info);

	_pca_init_thread_info(&info->thread_info);
	strncpy(info->thread_info.name, name, sizeof(info->thread_info.name));

	pthread_mutex_init(&info->consumer_mutex, NULL);
	INIT_LIST_HEAD(&info->consumer_list.list);

	int item_count = list_get_count(&(info->consumer_list.list));
	printf("item_count = %d\n",item_count);
	

	pthread_mutex_init(&info->data_free_mutex, NULL);
	INIT_LIST_HEAD(&info->data_free_list.list);

	*dispatcher = info;

	return SUCCESS;
}

int32_t pca_dispatcher_run(pca_dispatcher_info_t* dispatcher)
{
	CHECK_POINTER_RETURN(dispatcher);

	int ret = pthread_create(&(dispatcher->thread_info.tid), NULL,
			_pca_thread_dispatcher, (void*)(dispatcher));
	CHECK_RET_RETURN(ret);

	return SUCCESS;
}

int32_t pca_dispatcher_stop(pca_dispatcher_info_t* dispatcher)
{
	CHECK_POINTER_RETURN(dispatcher);

	/* Stop consumers */
	pca_consumer_list_info_t* consumer_list = &dispatcher->consumer_list;
	struct list_head *pos;
	pca_consumer_list_info_t* consumer;

	pthread_mutex_lock(&dispatcher->consumer_mutex);
	list_for_each(pos, &consumer_list->list) {
		consumer = list_entry(pos, pca_consumer_list_info_t, list);

		/* Stop consumer's thread */
		pthread_cancel(consumer->thread_info.tid);
		pthread_join(consumer->thread_info.tid, NULL);
		consumer->thread_info.is_running = false;

		_pca_thread_dump_statistics(&consumer->thread_info.stats,
					    consumer->thread_info.name);

		/* Remove consuer from consumers' list */
		list_del(&consumer->list);

		/* Free consumer's memory */
		free(consumer);
	}
	pthread_mutex_unlock(&dispatcher->consumer_mutex);

	/* Flush "need free data list" */
	LOG_I("All consumers thread are stopped\n");
	_pca_flush_data_free_list(dispatcher);

	/* Dispatcher thread stop */
	pthread_cancel(dispatcher->thread_info.tid);
	pthread_join(dispatcher->thread_info.tid, NULL);
	dispatcher->thread_info.is_running = false;
	LOG_I("Dispatcher thread is stopped\n");

	_pca_thread_dump_statistics(&dispatcher->thread_info.stats,
				    dispatcher->thread_info.name);

	return SUCCESS;
}

int32_t pca_dispatcher_destory(pca_dispatcher_info_t* dispatcher)
{
	CHECK_POINTER_RETURN(dispatcher);

	free(dispatcher);
	return SUCCESS;
}

int32_t pca_add_consumer_processor(pca_dispatcher_info_t* dispatcher,
				   pca_consumer_processor_info_t *processor)
{
	CHECK_POINTER_RETURN(dispatcher);
	CHECK_POINTER_RETURN(processor);
	CHECK_POINTER_RETURN(processor->process);
	int32_t ret;

	/* Get dispatcher's consumer list head */
	pca_consumer_list_info_t* consumer_list = &dispatcher->consumer_list;

	/* malloc a new consumer item */
	pca_consumer_list_info_t* consumer;
	consumer = (pca_consumer_list_info_t*)(malloc(sizeof(pca_consumer_list_info_t)));
	CHECK_POINTER_RETURN(consumer);

	/* init the item members */
	ret = _pca_init_thread_info(&consumer->thread_info);
	CHECK_RET_RETURN(ret);
	memcpy(&consumer->processor, processor,
		sizeof(pca_consumer_processor_info_t));
	snprintf(consumer->thread_info.name,
		 sizeof(consumer->thread_info.name),
		 "consumer_%s", processor->name);
	consumer->dispatcher = dispatcher;

	/* Add consumer item to dispatcher's list */
	pthread_mutex_lock(&dispatcher->consumer_mutex);
	list_add_tail(&consumer->list, &consumer_list->list);

	int item_count = list_get_count(&consumer_list->list);
	pthread_mutex_unlock(&dispatcher->consumer_mutex);
	LOG_I("There are %d consumer processor now\n", item_count);

	/* Create consumer thread */
	ret = pthread_create(&(consumer->thread_info.tid), NULL,
			    _pca_thread_consumer, (void*)(consumer));
	CHECK_RET_RETURN(ret);

	return SUCCESS;
}

int32_t pca_remove_consumer_processor(pca_dispatcher_info_t* dispatcher,
				      pca_consumer_processor_info_t *processor)
{
	LOG_E("Not support yet, but stop can remove all automatically\n");
	pthread_mutex_lock(&dispatcher->consumer_mutex);
	pthread_mutex_unlock(&dispatcher->consumer_mutex);
	return FAILURE;
}

int32_t pca_feed_dispatcher(pca_dispatcher_info_t* dispatcher,
			    pca_data_info_t* data_info)
{
	CHECK_POINTER_RETURN(dispatcher);
	CHECK_POINTER_RETURN(data_info);
	CHECK_POINTER_RETURN(data_info->data);

	while (dispatcher->thread_info.is_running == false) {
		LOG_D("Waiting for dispatcher thread run...\n");
		usleep(10*1000);
	}

	_pca_data_dispose_e dispose;

	LOG_D("Feed Data(%p,%d,%d,%d) to dispatcher.\n",
			data_info->data, *((uint32_t*)(data_info->data)),
			data_info->length, data_info->type);

	if (pthread_mutex_trylock(&dispatcher->thread_info.mutex_data) == 0) {
		dispatcher->thread_info.stats.total_delivered++;
		if (dispatcher->thread_info.has_new_data) {
			/* Release old data */
			pca_data_info_t old_data_info;
			memcpy(&old_data_info, &dispatcher->thread_info.data_info,
				sizeof(pca_data_info_t));
			LOG_D("Free\n");
			_pca_data_free(&old_data_info);

			/* Copy new data */
			memcpy(&dispatcher->thread_info.data_info, data_info,
				sizeof(pca_data_info_t));
			dispatcher->thread_info.stats.total_updated ++;

			dispose = _PCA_DATA_DISPOSE_UPDATED;
		} else {
			/* Copy new data */
			memcpy(&dispatcher->thread_info.data_info, data_info,
				sizeof(pca_data_info_t));

			int sem_value;
			if (sem_getvalue(&dispatcher->thread_info.sem_wakeup, &sem_value) == 0) {
				if (sem_value == 0) {
					sem_post(&dispatcher->thread_info.sem_wakeup);
				}
			}
			dispose = _PCA_DATA_DISPOSE_DELIVERED;
		}
		dispatcher->thread_info.has_new_data = true;

		LOG_D("Feed data(%p,%d,%d,%d) to dispatcher is %s\n\n",
			data_info->data, *((uint32_t*)data_info->data),
			data_info->length, data_info->type,
			_PCA_DATA_DISPOSE_STRING(dispose));
		pthread_mutex_unlock(&dispatcher->thread_info.mutex_data);
	} else {
		dispose = _PCA_DATA_DISPOSE_DISCARDED;
		LOG_D("Feed data(%p,%d,%d,%d) to dispatcher is %s\n\n",
			data_info->data, *((uint32_t*)data_info->data),
			data_info->length, data_info->type,
			_PCA_DATA_DISPOSE_STRING(dispose));
		LOG_D("Free\n");
		_pca_data_free(data_info);
		dispatcher->thread_info.stats.total_discarded++;
	}

	return SUCCESS;
}

int32_t pca_feed_consumers(pca_dispatcher_info_t* dispatcher,
			   pca_data_info_t* data_info)
{
	CHECK_POINTER_RETURN(dispatcher);
	CHECK_POINTER_RETURN(data_info);

	pca_consumer_list_info_t* consumer_list = &dispatcher->consumer_list;

	struct list_head *pos;
	pca_consumer_list_info_t* consumer;

	pthread_mutex_lock(&dispatcher->consumer_mutex);
	bool data_feed = false;

	LOG_D("Feed Data(%p,%d,%d,%d) to consumers.\n",
		data_info->data, *((uint32_t*)(data_info->data)),
		data_info->length, data_info->type);

	int i = 0;
	list_for_each(pos, &consumer_list->list) {
		consumer = list_entry(pos, pca_consumer_list_info_t, list);
		_pca_data_dispose_e dispose;

		/* If consumer's data_type != data's type, DO NOT dispatch */
		if (data_info->type != consumer->processor.data_type) {
			continue;
		}

		consumer->thread_info.stats.total_delivered++;
		if (pthread_mutex_trylock(&consumer->thread_info.mutex_data) == 0) {
			consumer->thread_info.need_unlock_data_mutex = true;
			if (consumer->thread_info.has_new_data) {
				/* Release old data */
				pca_data_info_t old_data_info;
				memcpy(&old_data_info, &consumer->thread_info.data_info,
					sizeof(pca_data_info_t));
				LOG_D("Remove\n");
				_pca_remove_data_free_item(dispatcher, &old_data_info);

				/* Copy new data */
				memcpy(&consumer->thread_info.data_info, data_info,
					sizeof(pca_data_info_t));
				_pca_add_data_free_item(dispatcher, &consumer->thread_info.data_info);
				consumer->thread_info.stats.total_updated ++;

				dispose = _PCA_DATA_DISPOSE_UPDATED;
			} else {
				/* Copy new data */
				memcpy(&consumer->thread_info.data_info, data_info,
					sizeof(pca_data_info_t));
				_pca_add_data_free_item(dispatcher, &consumer->thread_info.data_info);

				dispose = _PCA_DATA_DISPOSE_DELIVERED;
			}
			consumer->thread_info.has_new_data = true;
			data_feed = true;
			LOG_D("Feed Data(%p,%d,%d,%d) to %s is %s (%d)\n",
				data_info->data, *((uint32_t*)(data_info->data)),
				data_info->length, data_info->type,
				consumer->thread_info.name,
				_PCA_DATA_DISPOSE_STRING(dispose), i);
		} else {
			dispose = _PCA_DATA_DISPOSE_DISCARDED;
			LOG_D("Feed Data(%p,%d,%d,%d) to %s is %s (%d)\n",
				data_info->data, *((uint32_t*)(data_info->data)),
				data_info->length, data_info->type,
				consumer->thread_info.name,
				_PCA_DATA_DISPOSE_STRING(dispose), i);
			consumer->thread_info.stats.total_discarded++;
		}
		i++;
	}

	list_for_each(pos, &consumer_list->list) {
		consumer = list_entry(pos, pca_consumer_list_info_t, list);
		if (consumer->thread_info.need_unlock_data_mutex) {
			consumer->thread_info.need_unlock_data_mutex = false;
			pthread_mutex_unlock(&consumer->thread_info.mutex_data);
		}

		if (consumer->thread_info.has_new_data) {
			int sem_value;
			if (sem_getvalue(&consumer->thread_info.sem_wakeup, &sem_value) == 0) {
				if (sem_value == 0) {
					LOG_D("sem_post to %s\n", consumer->thread_info.name);
					sem_post(&consumer->thread_info.sem_wakeup);
				}
			}
		}
	}

	pthread_mutex_unlock(&dispatcher->consumer_mutex);
	dispatcher->thread_info.has_new_data = false;

	if (!data_feed) {
		LOG_D("Data(%p,%d,%d,%d) no consumer accepted.\n",
			data_info->data, *((uint32_t*)(data_info->data)),
			data_info->length, data_info->type);
		return FAILURE;
	} else {
		LOG_D("Data(%p,%d,%d,%d) feed finished.\n",
			data_info->data, *((uint32_t*)(data_info->data)),
			data_info->length, data_info->type);
		return SUCCESS;
	}
}

