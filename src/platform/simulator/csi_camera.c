/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#define LOG_LEVEL 2
#define CHANNEL_NUM 3
#define CAMERA_FRAME_NUM 10
#define PROPERTY_NUM 100
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#include <csi_camera.h>

#include "simulator_camera.h"
#include <llist.h>
#include <ringbuffer.h>
#include <producer_consumer_async.h>

#define MAGIC_NUMBER 0xdeadbeaf
#define DEVICENAME0 "/dev/video0"
#define DEVICENAME1 "/dev/video1"
#define DEVICENAME2 "/dev/video2"
#define DEVICENAME3 "/dev/video3"
#define DEVICEFAKE   "/dev/fake"
#define CAMERA_HEIGHT 640
#define CAMERA_WIDTH  480


int pca_consumer_process_chn0(pca_data_info_t *data_info);
int pca_consumer_process_chn1(pca_data_info_t *data_info);
int pca_consumer_process_chn2(pca_data_info_t *data_info);

static sem_t frame_csem;
static pthread_mutex_t mutex_event = PTHREAD_MUTEX_INITIALIZER;
static pthread_t camera_ptid = -1;
static pthread_mutex_t mutex_chn[CHANNEL_NUM] = {PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER};
static bool camera_producer_flag = true;
static csi_camera_channel_id_e g_chn_id[CHANNEL_NUM] = {CSI_CAMERA_CHANNEL_0, CSI_CAMERA_CHANNEL_1, CSI_CAMERA_CHANNEL_2};
int (*pca_consumer_process_chn[CHANNEL_NUM])(pca_data_info_t *data_info) =  {pca_consumer_process_chn0, pca_consumer_process_chn1,pca_consumer_process_chn2};

static  pca_dispatcher_info_t *s_dispatcher = NULL;
static  pca_consumer_processor_info_t process_info[CHANNEL_NUM] = {
	{.data_type = 1,.process = pca_consumer_process_chn0,.name = "chn0_process"},
	{.data_type = 1,.process = pca_consumer_process_chn1,.name = "chn1_process"},
	{.data_type = 1,.process = pca_consumer_process_chn2,.name = "chn2_process"}
};

static const csi_camera_infos_s g_camera_infos = {
	3,
	{
		{"RGB_Camera", "/dev/video0", "CSI-MIPI", CSI_CAMERA_CAP_META_CAPTURE | CSI_CAMERA_CAP_VIDEO_CAPTURE },
		{"Mono_Camera", "/dev/video1", "USB", CSI_CAMERA_CAP_VIDEO_CAPTURE},
		{"fake_Camera", "/dev/fake", "USB", CSI_CAMERA_CAP_VIDEO_CAPTURE}
	}
};

static csi_camera_modes_s g_camera_modes = {
	2, {{1, "default"}, {2, "portrait"}}
};
static csi_camera_channel_cfg_s g_camera_channel_cfgs[CHANNEL_NUM] = {
	{
		CSI_CAMERA_CHANNEL_0, CSI_CAMERA_CHANNEL_CAPTURE_VIDEO | CSI_CAMERA_CHANNEL_CAPTURE_META, 4,
		{640, 480, CSI_PIX_FMT_I420}, CSI_IMG_TYPE_DMA_BUF, CSI_CAMERA_META_DEFAULT_FIELDS, CSI_CAMERA_CHANNEL_CLOSED
	},
	{
		CSI_CAMERA_CHANNEL_1, CSI_CAMERA_CHANNEL_CAPTURE_VIDEO | CSI_CAMERA_CHANNEL_CAPTURE_META, 4,
		{640, 480, CSI_PIX_FMT_I420}, CSI_IMG_TYPE_DMA_BUF, CSI_CAMERA_META_DEFAULT_FIELDS, CSI_CAMERA_CHANNEL_CLOSED
	},
	{
		CSI_CAMERA_CHANNEL_2, CSI_CAMERA_CHANNEL_CAPTURE_VIDEO | CSI_CAMERA_CHANNEL_CAPTURE_META, 4,
		{640, 480, CSI_PIX_FMT_I420}, CSI_IMG_TYPE_DMA_BUF, CSI_CAMERA_META_DEFAULT_FIELDS, CSI_CAMERA_CHANNEL_CLOSED
	},
};

static csi_camera_property_description_s
g_camera_property_descriptions[PROPERTY_NUM] = {
	{
		CSI_CAMERA_PID_HFLIP, CSI_CAMERA_PROPERTY_TYPE_BOOLEAN, "horization flip", 0, 1, 1, {.bool_value = true},
		{.bool_value = false}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_VFLIP, CSI_CAMERA_PROPERTY_TYPE_BOOLEAN, "vertically flip", 0, 1, 1, {.bool_value = true},
		{.bool_value = false}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_ROTATE, CSI_CAMERA_PROPERTY_TYPE_INTEGER, "rotate", 0, 270, 90, {.int_value = 0},
		{.int_value = 0}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_EXPOSURE_MODE, CSI_CAMERA_PROPERTY_TYPE_ENUM, "exposure mode", 0, 3, 1,
		{.enum_value = CSI_CAMERA_EXPOSURE_MODE_AUTO}, {.enum_value = CSI_CAMERA_EXPOSURE_MODE_AUTO}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_EXPOSURE_ABSOLUTE, CSI_CAMERA_PROPERTY_TYPE_INTEGER, "exposure absolute", 0,
		10000, 1, {.int_value = 1}, {.int_value = 1}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_3A_LOCK, CSI_CAMERA_PROPERTY_TYPE_BITMASK, "3A LOCK", 0, 4, 2, {.bitmask_value = CSI_CAMERA_LOCK_EXPOSURE},
		{.bitmask_value = CSI_CAMERA_LOCK_EXPOSURE}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_RED_GAIN, CSI_CAMERA_PROPERTY_TYPE_INTEGER, "red gain", 0, 2147483647, 1, {.int_value = 1},
		{.int_value = 1}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_GREEN_GAIN, CSI_CAMERA_PROPERTY_TYPE_INTEGER, "green gain", 0, 2147483647, 1, {.int_value = 1},
		{.int_value = 1}, 0, {0, 0}
	},
	{
		CSI_CAMERA_PID_BLUE_GAIN, CSI_CAMERA_PROPERTY_TYPE_INTEGER, "blue gain", 0, 2147483647, 1, {.int_value = 1},
		{.int_value = 1}, 0, {0, 0}
	},
};
static LLIST  *g_camera_mode_cfg;
static LLIST  *g_camera_subscribe_event;
static LLIST  *g_camera_event;

static frame_channel_info_s g_frame_channel_infos[CHANNEL_NUM];
static camera_frame_info_s  g_camera_frame_infos[CAMERA_FRAME_NUM];

int32_t csi_camera_get_version(csi_api_version_u *version)
{
	version->major = CSI_CAMERA_VERSION_MAJOR;
	version->minor = CSI_CAMERA_VERSION_MINOR;
	return 0;
}

int csi_camera_query_list(csi_camera_infos_s *infos)
{
	if (infos == NULL) {
		LOG_E("pass parameter error!\n");
		return -1;
	}
	memcpy(infos, &g_camera_infos, sizeof(csi_camera_infos_s));
	return 0;
}

static int getcameraindex(const char *cam_name)
{
	int index;
	if (!strcmp(cam_name, DEVICENAME0)) {
		index = 0;
	} else if (!strcmp(cam_name, DEVICENAME1)) {
		index = 1;
	} else if (!strcmp(cam_name, DEVICENAME2)) {
		index = 2;
	} else if (!strcmp(cam_name, DEVICENAME3)) {
		index = 3;
	} else if (!strcmp(cam_name, DEVICEFAKE)) {
		index = 4;
	} else {
		LOG_E("name is error!\n");
		index = -1;
	}
	return index;
}

static int32_t data_consumed_callback(struct pca_data_info* data_info)
{
	camera_frame_info_s *data = (camera_frame_info_s *)data_info->data;
	printf("data->frame_status = %d\n",data->frame_status);
	data->frame_status = CSI_FRAME_IDLE;
	return 0;
}

void *camera_producer_frame(void *arg)
{
	pca_data_info_t data_info;
	int i = 0,ret = 0;
	static int loop = 0;
	while (camera_producer_flag) {
		//LOG_D("camera_producer_frame!\n");
		loop++;
		for(i = 0;i < CAMERA_FRAME_NUM;i++){
			if(g_camera_frame_infos[i].frame_status == CSI_FRAME_IDLE){
				break;
			}
		}
		LOG_D("camera_producer_frame!  before %d\n",loop);
		if(i == CAMERA_FRAME_NUM){
			//usleep(100);
			continue;
		}
		LOG_D("camera_producer_frame! %d\n",loop);
		csi_camera_opencv_get_picture(
			CAMERA_HEIGHT,
			CAMERA_WIDTH,
			g_camera_frame_infos[i].frame_bufs);
		data_info.type = 1;
		data_info.length = 4;
		data_info.data = &g_camera_frame_infos[i];
		data_info.consumed_callback = data_consumed_callback;
		ret = pca_feed_dispatcher(s_dispatcher,&data_info);
		if(ret != 0){
			LOG_E("pca_feed_dispatcher\n");
		}
	}
	pthread_exit(NULL);
}

static int channel_feeddata_thread_run(void)
{
	int ret = 0;
	camera_producer_flag = true;
	ret = pthread_create(&camera_ptid,NULL,camera_producer_frame,NULL);
	CHECK_RET_RETURN(ret);
	return 0;
}

static int channel_feeddata_thread_stop(void)
{
	int ret = 0;
	camera_producer_flag = false;
	ret = pthread_join(camera_ptid,NULL);
	CHECK_RET_RETURN(ret);
	camera_ptid = -1;
	return 0;
}

int csi_camera_open(csi_cam_handle_t *cam_handle, const char *device_name)
{
	int idx = 0,ret = 0;
	csi_cam_handle_info_t *retp;
	g_camera_mode_cfg = llist_create(sizeof(csi_camera_mode_cfg_s));
	if (g_camera_mode_cfg == NULL) {
		LOG_E("g_camera_mode_cfg llist_create() error!\n");
		return -1;
	}
	g_camera_subscribe_event = llist_create(sizeof(
			csi_camera_event_subscription_s));
	if (g_camera_subscribe_event == NULL) {
		LOG_E("g_camera_subscribe_event llist_create() error!\n");
		return -1;
	}
	g_camera_event = llist_create(sizeof(csi_camera_event_s));
	if (g_camera_event == NULL) {
		LOG_E("g_camera_event llist_create() error!\n");
		return -1;
	}
	idx = getcameraindex(device_name);
	retp = malloc(sizeof(csi_cam_handle_info_t));
	retp->idx = idx;
	if (idx != 4 && camera_open_opencv(idx)) {
		LOG_E("camera cannot open!\n");
		return -1;
	}
	for(int i = 0;i < CAMERA_FRAME_NUM;i++){
		g_camera_frame_infos[i].frame_bufs = malloc(sizeof(unsigned char) * CAMERA_HEIGHT * CAMERA_WIDTH * 3);
		g_camera_frame_infos[i].frame_status = CSI_FRAME_IDLE;
	}
	ret = pca_dispatcher_create(&s_dispatcher,"camera_picture_producer");
	*cam_handle = retp;
	return 0;
}

int csi_camera_close(csi_cam_handle_t cam_handle)
{
	int ret = 0;
	camera_close_opencv(cam_handle);
	llist_destroy(g_camera_mode_cfg);
	llist_destroy(g_camera_subscribe_event);
	llist_destroy(g_camera_event);
	if (cam_handle != NULL) {
		free(cam_handle);
	}
	for(int i = 0;i < CAMERA_FRAME_NUM;i++){
		if(g_camera_frame_infos[i].frame_bufs != NULL){
			free(g_camera_frame_infos[i].frame_bufs);
		}
	}
	ret = pca_dispatcher_destory(s_dispatcher);
	CHECK_RET_RETURN(ret);
	return 0;
}

int csi_camera_get_modes(csi_cam_handle_t cam_handle, csi_camera_modes_s *modes)
{
	if (modes == NULL) {
		LOG_E("pass paramter error!\n");
		return -1;
	}
	memcpy(modes, &g_camera_modes, sizeof(csi_camera_modes_s));
	return 0;
}

static void mode_cfg_print(const void *data)
{
	const csi_camera_mode_cfg_s *d = data;
	printf("camera mode cfg->mode_id: %d\n", d->mode_id);
}

int csi_camera_set_mode(csi_cam_handle_t cam_handle, csi_camera_mode_cfg_s *cfg)
{
	int i;
	csi_cam_handle_info_t *cam_info = cam_handle;
	if (cfg == NULL) {
		LOG_E("pass parameter error!\n");
		return -1;
	}
	if (cfg->mode_id < 1 && cfg->mode_id > g_camera_modes.count) {
		LOG_E("camera does not have the cfg modes!\n");
		return -1;
	}
	g_camera_mode_cfg->insert(g_camera_mode_cfg, cfg, LLIST_BACKWARD);
	if (cam_info->idx != 4 && camera_set_mode_opencv(cfg)) {
		LOG_E("camera_set_mode_opencv error!\n");
		return -1;
	}
	//g_camera_mode_cfg->travel(g_camera_mode_cfg,mode_cfg_print);
	return 0;
}

int csi_camera_query_property(csi_cam_handle_t cam_handle,
			      csi_camera_property_description_s *desc)
{
	int i = 0;
	if (desc == NULL) {
		LOG_E("pass parameter error\n");
		return -1;
	}
	if (desc->id & CSI_CAMERA_FLAG_NEXT_CTRL) {
		desc->id = (desc->id & ~(CSI_CAMERA_FLAG_NEXT_CTRL));
		for (i = 0; i < PROPERTY_NUM; i++) {
			if (desc->id == g_camera_property_descriptions[i].id) {
				i++;
				break;
			}
		}
	} else {
		for (i = 0; i < PROPERTY_NUM; i++) {
			if (desc->id == g_camera_property_descriptions[i].id) {
				break;
			}
		}
	}
	if (i == PROPERTY_NUM || !g_camera_property_descriptions[i].id) {
		return -1;
	}
	memcpy(desc, &g_camera_property_descriptions[i],
	       sizeof(csi_camera_property_description_s));
	return 0;
}

int csi_camera_get_property(csi_cam_handle_t cam_handle,
			    csi_camera_properties_s *properties)
{
	if (properties == NULL) {
		LOG_E("pass parameter error\n");
		return -1;
	}
	for (int i = 0; i < properties->count; i++) {
		unsigned int id;
		int j;
		for (j = 0; j < PROPERTY_NUM; j++) {
			if (properties->property[i].id == g_camera_property_descriptions[j].id) {
				break;
			}
		}
		if (j == PROPERTY_NUM || !g_camera_property_descriptions[id].id) {
			LOG_E("error configuration\n");
			return -1;
		}
		properties->property[i].type = g_camera_property_descriptions[j].type;
		memcpy(&properties->property[i].value,
		       &g_camera_property_descriptions[j].value, sizeof(csi_camera_property_data_u));
	}
	return 0;
}

const char *csi_camera_ctrl_id_name(unsigned int id)
{
	return NULL;
}

const char *csi_camera_ctrl_type_name(unsigned int type)
{

	return NULL;
}

int csi_camera_set_property(csi_cam_handle_t cam_handle,
			    csi_camera_properties_s *properties)
{
	csi_cam_handle_info_t *cam_info = cam_handle;
	if (properties == NULL) {
		LOG_E("pass parameter error\n");
		return -1;
	}
	for (int i = 0; i < properties->count; i++) {
		unsigned int id;
		int j;
		for (j = 0; j < PROPERTY_NUM; j++) {
			if (properties->property[i].id == g_camera_property_descriptions[j].id) {
				break;
			}
		}
		if (j == PROPERTY_NUM || !g_camera_property_descriptions[j].id) {
			LOG_E("error configuration\n");
			return -1;
		}
		if (cam_info->idx != 4 &&
		    camera_property_set_opencv(&properties->property[i])) {
			LOG_E("error configuration!\n");
			return -1;
		}
		memcpy(&g_camera_property_descriptions[j].value,
		       &properties->property[i].value, sizeof(csi_camera_property_data_u));
	}
	return 0;
}

int csi_camera_query_frame(csi_cam_handle_t cam_handle,
			   csi_camera_channel_id_e chn_id,
			   int frm_id,
			   csi_frame_s *frm)

{
	return 0;
}

int csi_camera_channel_open(csi_cam_handle_t cam_handle,
			    csi_camera_channel_cfg_s *cfg)
{
	int ret = 0;
	if (cfg == NULL) {
		LOG_E("pass parameter error\n");
		return -1;
	}
	if (cfg->chn_id < CSI_CAMERA_CHANNEL_0 || cfg->chn_id > CSI_CAMERA_CHANNEL_2) {
		LOG_E("select error channel!\n");
		return -1;
	}
	if(camera_ptid != -1){
		channel_feeddata_thread_stop();
		ret = pca_dispatcher_stop(s_dispatcher);
		CHECK_RET_RETURN(ret);
	}
	for (int i = 0; i < CHANNEL_NUM; i++) {
		for (int j = 0; j < MAX_FRAME_COUNT; j++) {
			g_frame_channel_infos[i].frame_bufs[j] = NULL;
			g_frame_channel_infos[i].frame_status[j] = CSI_FRAME_IDLE;
			g_frame_channel_infos[i].refcount[j] = 0;
		}
	}
	memcpy(&g_camera_channel_cfgs[cfg->chn_id], cfg,
	       sizeof(csi_camera_channel_cfg_s));
	g_camera_channel_cfgs[cfg->chn_id].status = CSI_CAMERA_CHANNEL_OPENED;
	return 0;
}

int csi_camera_channel_close(csi_cam_handle_t cam_handle,
			     csi_camera_channel_id_e chn_id)
{
	g_camera_channel_cfgs[chn_id].status = CSI_CAMERA_CHANNEL_CLOSED;
	return 0;
}

int csi_camera_channel_query(csi_cam_handle_t cam_handle,
			     csi_camera_channel_cfg_s *cfg)
{
	if (cfg == NULL) {
		LOG_E("pass parameter error\n");
		return -1;
	}
	if (cfg->chn_id < CSI_CAMERA_CHANNEL_0 || cfg->chn_id > CSI_CAMERA_CHANNEL_2) {
		LOG_E("select error channel!\n");
		return -1;
	}
	memcpy(cfg, &g_camera_channel_cfgs[cfg->chn_id],
	       sizeof(csi_camera_channel_cfg_s));
	return 0;
}

int csi_camera_create_event(csi_cam_event_handle_t *event_handle,
			    csi_cam_handle_t cam_handle)
{
	return 0;
}

int csi_camera_destory_event(csi_cam_event_handle_t event_handle)
{
	return 0;
}

static int type_cmp(const void *data, const void *findtype)
{
	const csi_camera_event_subscription_s *d = data;
	const  csi_camera_event_type_e *type = findtype;
	return (d->type - *type);
}

static void print_subscribe(const void *data)
{
	const csi_camera_event_subscription_s *d = data;
	printf("subscribe type: %d id: %d\n", d->type, d->id);
}

int csi_camera_subscribe_event(csi_cam_event_handle_t event_handle,
			       csi_camera_event_subscription_s *subscribe)
{
	int i;
	if (subscribe == NULL) {
		LOG_E("pass parameter error!\n");
		return -1;
	}
	csi_camera_event_subscription_s *retp;
	retp = g_camera_subscribe_event->find(g_camera_subscribe_event,
					      &subscribe->type, type_cmp);
	if (retp == NULL) {
		g_camera_subscribe_event->insert(g_camera_subscribe_event, subscribe,
						 LLIST_BACKWARD);
	} else {
		retp->id |= subscribe->id;
	}
	//for printf test
	//g_camera_subscribe_event->travel(g_camera_subscribe_event,print_subscribe);
	return 0;
}

int csi_camera_unsubscribe_event(csi_cam_event_handle_t  event_handle,
				 csi_camera_event_subscription_s *subscribe)
{
	int i = 0;
	csi_camera_event_subscription_s *retp;
	if (subscribe == NULL) {
		LOG_E("pass parameter error!\n");
		return -1;
	}
	retp = g_camera_subscribe_event->find(g_camera_subscribe_event,
					      &subscribe->type, type_cmp);
	if (retp == NULL) {
		LOG_E("can't find subscribe event!\n");
		return -1;
	}
	if ((retp->id &= ~(subscribe->id)) == 0) {
		g_camera_subscribe_event->delete (g_camera_subscribe_event, &subscribe->type,
						  type_cmp);
	}
	return 0;
}

static int camera_id_cmp(const void *data, const void *findid)
{
	return 0;
}

static void event_print(const void *data)
{
	const csi_camera_event_s *d = data;
	printf("camera event type: %d id: %d\n", d->type, d->id);
}

int csi_camera_get_event(csi_cam_event_handle_t event_handle,
			 csi_camera_event_s *event,
			 int timeout)
{
	struct timeval time1;
	int ret;
	int semvalue = -1;
	int findid = -1;
	if (timeout == -1) {
		sem_wait(&frame_csem);
	} else {
		gettimeofday(&time1, NULL);
		event->timestamp.tv_sec = time1.tv_sec + timeout;
		semvalue = sem_timedwait(&frame_csem, &event->timestamp);
		if (semvalue == -1) {
			LOG_E("do not get camera event!\n");
			return -1;
		}
	}
	pthread_mutex_lock(&mutex_event);
	//print for test
	//g_camera_event->travel(g_camera_event,event_print);
	ret = g_camera_event->fetch(g_camera_event, NULL, camera_id_cmp, event);
	if (ret == -1) {
		LOG_E("do not get camera event!\n");
		pthread_mutex_unlock(&mutex_event);
		return -1;
	}
	LOG_D("event->type = %d event->id = %d\n", event->type, event->id);
	pthread_mutex_unlock(&mutex_event);
	return 0;
}

int csi_camera_get_frame_count(csi_cam_handle_t cam_handle,
			       csi_camera_channel_id_e chn_id)
{
	int i;
	pthread_mutex_lock(&mutex_chn[chn_id]);
	for (i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
		if (g_frame_channel_infos[chn_id].frame_status[i] == CSI_FRAME_READY) {
			i++;
		}
	}
	pthread_mutex_unlock(&mutex_chn[chn_id]);
	return i;
}

int csi_camera_get_frame(csi_cam_handle_t cam_handle,
			 csi_camera_channel_id_e chn_id,
			 csi_frame_s *frame,
			 int timeout)
{
	int ret = -1;
	int i = 0, j = 0;

	if (frame == NULL) {
		LOG_E("pass parameter error!\n");
		return -1;
	}
	if (chn_id < CSI_CAMERA_CHANNEL_0 || chn_id > CSI_CAMERA_CHANNEL_2) {
		LOG_E("select error ch_id!\n");
		return -1;
	}
	pthread_mutex_lock(&mutex_chn[chn_id]);
	frame->img.type = CSI_IMG_TYPE_UMALLOC;
	frame->img.width = g_camera_channel_cfgs[chn_id].img_fmt.width;
	frame->img.height = g_camera_channel_cfgs[chn_id].img_fmt.height;
	frame->img.pix_format = g_camera_channel_cfgs[chn_id].img_fmt.pix_fmt;
	frame->img.size = camera_image_size_opencv(frame);
	frame->img.num_planes = CSI_IMAGE_I420_PLANES;
	frame->img.strides[0] = frame->img.width;
	frame->img.strides[1] = frame->img.width;

	frame->img.offsets[0] = 0;
	frame->img.offsets[1] = frame->img.width * frame->img.height;
	if (frame->img.type == CSI_IMG_TYPE_UMALLOC) {
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			if (g_frame_channel_infos[chn_id].frame_status[i] == CSI_FRAME_READY) {
				frame->img.usr_addr[0] = g_frame_channel_infos[chn_id].frame_bufs[i];
				frame->img.usr_addr[1] = NULL;
				g_frame_channel_infos[chn_id].refcount[i] = 0;
				ret = 0;
				break;
			}
		}
	}
	pthread_mutex_unlock(&mutex_chn[chn_id]);
	return ret;
}

int csi_camera_put_frame(csi_frame_s *frame)
{
	int ret = 0;
	switch (frame->img.type) {
	case CSI_IMG_TYPE_DMA_BUF:
		ret = -1;
		break;
	case CSI_IMG_TYPE_SYSTEM_CONTIG:
		ret = -1;
		break;
	case CSI_IMG_TYPE_CARVEOUT:
		ret = -1;
		break;
	case CSI_IMG_TYPE_UMALLOC:
		for (int i = 0; i < CHANNEL_NUM; i++) {
			pthread_mutex_lock(&mutex_chn[i]);
			for (int j = 0; j < g_camera_channel_cfgs[i].frm_cnt; j++) {
				//LOG_D("frame->img.usr_addr[0] = %p g_frame_channel_infos[%d].frame_bufs[%d] = %p\n",
				//    frame->img.usr_addr[0], i, j, g_frame_channel_infos[i].frame_bufs[j]);
				if (frame->img.usr_addr[0] == g_frame_channel_infos[i].frame_bufs[j]) {
					g_frame_channel_infos[i].frame_status[j] = CSI_FRAME_IDLE;
				}
			}
			pthread_mutex_unlock(&mutex_chn[i]);
		}
		break;
	case CSI_IMG_TYPE_SHM:
		ret = -1;
		break;
	default:
		LOG_E("Uknown format\n");
		ret = -1;
		break;
	}
	return ret;
}


int pca_consumer_process_chn0(pca_data_info_t *data_info)
{

	LOG_D("pca_consumer_process_chn0\n");
	csi_camera_event_subscription_s tmp,*retp;
	camera_frame_info_s *data = (camera_frame_info_s *)data_info->data;
	tmp.type = CSI_CAMERA_EVENT_TYPE_CHANNEL0;
	retp = g_camera_subscribe_event->find(g_camera_subscribe_event,&tmp.type,type_cmp);
	if(retp == NULL){
		return -1;
	}
	pthread_mutex_lock(&mutex_chn[0]);
	for (int i = 0; i < g_camera_channel_cfgs[0].frm_cnt; i++) {
		if (g_frame_channel_infos[0].frame_status[i] == CSI_FRAME_IDLE) {
			csi_camera_opencv_format_picture(
				g_camera_channel_cfgs[0].img_fmt.pix_fmt,
				g_camera_channel_cfgs[0].img_fmt.height,
				g_camera_channel_cfgs[0].img_fmt.width,
				g_frame_channel_infos[0].frame_bufs[i],
				CAMERA_HEIGHT,
				CAMERA_WIDTH,
				data->frame_bufs,
				g_camera_property_descriptions);
			g_frame_channel_infos[0].frame_status[i] = CSI_FRAME_READY;
			tmp.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
		}
	}
	pthread_mutex_lock(&mutex_event);
	g_camera_event->insert(g_camera_event,&tmp,LLIST_FORWARD);
	pthread_mutex_unlock(&mutex_event);
	data->frame_status = CSI_FRAME_READY;
	pthread_mutex_unlock(&mutex_chn[0]);
	sem_post(&frame_csem);
	return 0;
}

int pca_consumer_process_chn1(pca_data_info_t *data_info)
{

	LOG_D("pca_consumer_process_chn1\n");
	csi_camera_event_subscription_s tmp,*retp;
	camera_frame_info_s *data = (camera_frame_info_s *)data_info->data;
	tmp.type = CSI_CAMERA_EVENT_TYPE_CHANNEL1;
	retp = g_camera_subscribe_event->find(g_camera_subscribe_event,&tmp.type,type_cmp);
	if(retp == NULL){
		return -1;
	}
	pthread_mutex_lock(&mutex_chn[1]);
	for (int i = 0; i < g_camera_channel_cfgs[0].frm_cnt; i++) {
		if (g_frame_channel_infos[1].frame_status[i] == CSI_FRAME_IDLE) {
			csi_camera_opencv_format_picture(
				g_camera_channel_cfgs[1].img_fmt.pix_fmt,
				g_camera_channel_cfgs[1].img_fmt.height,
				g_camera_channel_cfgs[1].img_fmt.width,
				g_frame_channel_infos[1].frame_bufs[i],
				CAMERA_HEIGHT,
				CAMERA_WIDTH,
				data->frame_bufs,
				g_camera_property_descriptions);
			g_frame_channel_infos[1].frame_status[i] = CSI_FRAME_READY;
			tmp.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
		}
	}
	pthread_mutex_lock(&mutex_event);
	g_camera_event->insert(g_camera_event,&tmp,LLIST_FORWARD);
	pthread_mutex_unlock(&mutex_event);
	//g_camera_event->travel(g_camera_event,event_print);
	data->frame_status = CSI_FRAME_READY;
	pthread_mutex_unlock(&mutex_chn[1]);
	sem_post(&frame_csem);
	return 0;
}

int pca_consumer_process_chn2(pca_data_info_t *data_info)
{
	LOG_D("pca_consumer_process_chn2\n");
	csi_camera_event_subscription_s tmp,*retp;
	camera_frame_info_s *data = (camera_frame_info_s *)data_info->data;
	tmp.type = CSI_CAMERA_EVENT_TYPE_CHANNEL2;
	retp = g_camera_subscribe_event->find(g_camera_subscribe_event,&tmp.type,type_cmp);
	if(retp == NULL){
		return -1;
	}
	pthread_mutex_lock(&mutex_chn[2]);
	for (int i = 0; i < g_camera_channel_cfgs[2].frm_cnt; i++) {
		if (g_frame_channel_infos[2].frame_status[i] == CSI_FRAME_IDLE) {
			csi_camera_opencv_format_picture(
				g_camera_channel_cfgs[2].img_fmt.pix_fmt,
				g_camera_channel_cfgs[2].img_fmt.height,
				g_camera_channel_cfgs[2].img_fmt.width,
				g_frame_channel_infos[2].frame_bufs[i],
				CAMERA_HEIGHT,
				CAMERA_WIDTH,
				data->frame_bufs,
				g_camera_property_descriptions);
			g_frame_channel_infos[2].frame_status[i] = CSI_FRAME_READY;
			tmp.id = CSI_CAMERA_CHANNEL_EVENT_FRAME_READY;
		}
	}
	pthread_mutex_lock(&mutex_event);
	g_camera_event->insert(g_camera_event,&tmp,LLIST_FORWARD);
	pthread_mutex_unlock(&mutex_event);
	data->frame_status = CSI_FRAME_READY;
	pthread_mutex_unlock(&mutex_chn[2]);
	sem_post(&frame_csem);
	return 0;
}

int csi_camera_channel_start(csi_cam_handle_t cam_handle,
			     csi_camera_channel_id_e chn_id)
{
	//wait for complete
	int ret = 0;
	csi_cam_handle_info_t *cam_info = cam_handle;
	csi_camera_channel_id_e p_chn_id = chn_id;
	csi_camera_mode_cfg_s *cfg;
	int framesize = g_camera_channel_cfgs[chn_id].img_fmt.height *
			g_camera_channel_cfgs[chn_id].img_fmt.width;
	LOG_D("chn_id = %d\n", chn_id);
	sem_init(&frame_csem, 0, 0);
	for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
		g_frame_channel_infos[chn_id].frame_status[i] = CSI_FRAME_IDLE;
		g_frame_channel_infos[chn_id].refcount[i] = 0;
	}
	switch (g_camera_channel_cfgs[chn_id].img_fmt.pix_fmt) {
	case CSI_PIX_FMT_I420:
		framesize = framesize * 3 / 2;
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			g_frame_channel_infos[chn_id].frame_bufs[i] = malloc(sizeof(
						unsigned char) * framesize);
		}
		break;
	case CSI_PIX_FMT_NV12:
		framesize = framesize * 3 / 2;
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			g_frame_channel_infos[chn_id].frame_bufs[i] = malloc(sizeof(
						unsigned char) * framesize);
		}
		break;
	case CSI_PIX_FMT_BGR:
		framesize = framesize * 3;
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			g_frame_channel_infos[chn_id].frame_bufs[i] = malloc(sizeof(
						unsigned char) * framesize);
		}
		break;
	default:
		LOG_E("Uknown format\n");
		ret = -1;
		break;
	}
	if(camera_ptid != -1){
		channel_feeddata_thread_stop();
	}

	if (cam_info->idx != 4) {
		ret = pca_add_consumer_processor(s_dispatcher,&process_info[chn_id]);
		CHECK_RET_RETURN(ret);
	} else {

	}
	ret = pca_dispatcher_run(s_dispatcher);
	CHECK_RET_RETURN(ret);
	channel_feeddata_thread_run();
	g_camera_channel_cfgs[chn_id].status = CSI_CAMERA_CHANNEL_RUNNING;
	return ret;
}

int csi_camera_channel_stop(csi_cam_handle_t cam_handle,
			    csi_camera_channel_id_e chn_id)
{
	int ret = 0;
	csi_cam_handle_info_t *cam_info = cam_handle;
	LOG_D("csi_camera_channel_stop\n");
	for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
		g_frame_channel_infos[chn_id].frame_status[i] = CSI_FRAME_IDLE;
		g_frame_channel_infos[chn_id].refcount[i] = 0;
	}
	switch (g_camera_channel_cfgs[chn_id].img_fmt.pix_fmt) {
	case CSI_PIX_FMT_I420:
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			free(g_frame_channel_infos[chn_id].frame_bufs[i]);
		}
		break;
	case CSI_PIX_FMT_NV12:
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			free(g_frame_channel_infos[chn_id].frame_bufs[i]);
		}
		break;
	case CSI_PIX_FMT_BGR:
		for (int i = 0; i < g_camera_channel_cfgs[chn_id].frm_cnt; i++) {
			free(g_frame_channel_infos[chn_id].frame_bufs[i]);
		}
		break;
	default:
		LOG_E("Uknown format\n");
		ret = -1;
		break;
	}
	if(camera_ptid != -1){
		channel_feeddata_thread_stop();
		ret = pca_dispatcher_stop(s_dispatcher);
		CHECK_RET_RETURN(ret);
	}
	g_camera_channel_cfgs[chn_id].status = CSI_CAMERA_CHANNEL_OPENED;
	LOG_D("csi_camera_channel_stop ok\n");
	return ret;
}
