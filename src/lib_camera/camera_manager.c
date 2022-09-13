/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#define LOG_LEVEL 3
#define LOG_PREFIX "camera_manager"
#include <syslog.h>
#include <camera_string.h>
#include <csi_camera_platform_spec.h>

#include "camera_manager.h"
#include "camera_manager_utils.h"
#include "platform_action.h"

// ref: https://developer.android.google.cn/reference/android/hardware/camera2/CameraManager#inherited-methods

#define _CHECK_SESSION_RETURN()				\
	do {						\
		if (session == NULL) {			\
			LOG_E("session is NULL\n");	\
			return -EPERM;			\
		}					\
	} while (0)

#define _CHECK_STATE_RETURN(expect)					\
	do {								\
		if ((session->state & expect) == 0) {			\
			LOG_E("current state(%d) is not expected(%d)\n",\
				session->state, expect);		\
			return -EPERM;					\
		}							\
	} while (0)
static bool camera_thread_flag = true;

static int _camera_session_reset(cams_t *session)
{
	_CHECK_SESSION_RETURN();

	memset(session, 0, sizeof(*session));
	session->state = CAMERA_STATE_CLOSED;
	session->camera_id = -1;
	session->camera_handle = NULL;

	session->event_handle = NULL;
	for (int chn = 0; chn < CSI_CAMERA_CHANNEL_MAX_COUNT; chn++) {
		session->chn_cfg[chn].chn_id = -1;
		session->chn_cfg[chn].status = CSI_CAMERA_CHANNEL_INVALID;
	}

	session->camera_action_fun = NULL;
	session->channel_action_fun = NULL;

	return 0;
}

typedef enum _info_field {
	CAMERA_INFOS_READY	= (1 << 0), 	// saved in: cams_t.camera_infos
	CAMERA_MODES_READY	= (1 << 1),	// saved in: cams_t.camera_modes
	CHANNEL_INIT_READY	= (1 << 2),	// saved in: cams_t.chn_cfg
} _info_field_e;

/* set: true means set field to be 1, false means clear the field */
static inline void _set_info_status(cams_t *session, _info_field_e field,
				    bool set)
{
	if (set)
		session->info_status |= field;
	else
		session->info_status &= ~field;
}

/* set: true means set field to be 1, false means clear the field */
static inline bool _get_info_status(cams_t *session, _info_field_e field)
{
	return ((session->info_status & field) != 0);
}

int camera_create_session(cams_t **session)
{
	if (session == NULL) {
		LOG_E("input param error\n");
		return -EPERM;
	}
	*session = malloc(sizeof(cams_t));
	if (*session == NULL) {
		LOG_E("Create manager failed, %s\n", strerror(errno));
		return -ENOMEM;
	}

	_camera_session_reset(*session);
	return 0;
}

int camera_destory_session(cams_t **session)
{
	free(*session);
	*session = NULL;
	return 0;
}

int camera_query_list(cams_t *session)
{
	if (_get_info_status(session, CAMERA_INFOS_READY)) {
		LOG_D("camera list has been got before\n");
		return 0;
	}

	int ret = csi_camera_query_list(&session->camera_infos);
	if (ret != 0) {
		LOG_E("csi_camera_query_list() failed\n");
		_set_info_status(session, CAMERA_INFOS_READY, false);
	}

	_set_info_status(session, CAMERA_INFOS_READY, true);
	return 0;
}

int camera_get_caps(cams_t *session)
{
	int ret;
	_CHECK_SESSION_RETURN();

	return 0;
}

int camera_open(cams_t *session, int cam_id)
{
	_CHECK_SESSION_RETURN();
	int ret;

	if (!_get_info_status(session, CAMERA_INFOS_READY) ||
	    session->camera_infos.count <= cam_id) {
		LOG_E("Can't open cam_id=%d\n", cam_id);
		return -1;
	}

	char *dev_name = session->camera_infos.info[cam_id].device_name;
	csi_cam_handle_t handle;
	ret = csi_camera_open(&handle, dev_name);
	if (ret != 0) {
		LOG_E("Open camera(%s) failed\n", dev_name);
		return -1;
	}

	session->camera_id = cam_id;
	session->camera_handle = handle;
	session->state = CAMERA_STATE_OPENED;

	camera_event_action_union_t *event_action;
	const camera_spec_enums_s *enum_array;

	/* Camera Event/Action init */
	ret = csi_camera_create_event(&session->event_handle, session->camera_handle);
	if (ret != 0) {
		LOG_E("csi_camera_create_event failed, ret handle=%p\n", session->event_handle);
		return -1;
	}

	for (int i = 0; i < CSI_CAMERA_EVENT_MAX_COUNT; i++) {
		event_action = &(session->camera_event_action[i]);

		event_action->target = MANAGE_TARGET_CAMERA;
		event_action->camera_id = cam_id;
		event_action->channel_id = -1;
		event_action->camera.event = 1 << i;
		//LOG_W("i=%d, event_action->camera.event=0x%08x\n", i, event_action->camera.event);
		event_action->camera.supported = false;
		event_action->camera.subscribed = false;
		event_action->camera.action = CAMERA_ACTION_NONE;
	}

	enum_array = camera_spec_get_enum_array(CAMERA_SPEC_ENUM_CAMERA_EVENT_TYPES);
	for (int i = 0; (enum_array != NULL) && (i < enum_array->count); i++) {
		for (int j = 0; j < CSI_CAMERA_EVENT_MAX_COUNT; j++) {
			event_action = &(session->camera_event_action[j]);
			if (enum_array->enums[i] == event_action->camera.event) {
				event_action->camera.supported = true;
				//cam_mng_dump_event_action_union(event_action);
			}
		}
	}

	/* Camera Channel Event/Action init */
	for (int chn = 0; chn < CSI_CAMERA_CHANNEL_MAX_COUNT; chn++) {
		for (int i = 0; i < CSI_CAMERA_CHANNEL_EVENT_MAX_COUNT; i++) {
			event_action = &(session->channel_event_action[chn][i]);

			event_action->target = MANAGE_TARGET_CHANNEL;
			event_action->camera_id = cam_id;
			event_action->channel_id = chn;
			event_action->channel.event = 1 << i;
			event_action->channel.supported = false;
			event_action->channel.subscribed = false;
			event_action->channel.action = CAMERA_CHANNEL_ACTION_NONE;
		}
	}

	enum_array = camera_spec_get_enum_array(CAMERA_SPEC_ENUM_CHANNEL_EVENT_TYPES);
	for (int i = 0; (enum_array != NULL) && (i < enum_array->count); i++) {
		for (int j = 0; j < CSI_CAMERA_CHANNEL_MAX_COUNT; j++) {
			for (int k = 0; k < CSI_CAMERA_CHANNEL_MAX_COUNT; k++) {
				event_action = &(session->channel_event_action[j][k]);
				if (enum_array->enums[i] == event_action->camera.event) {
					event_action->camera.supported = true;
					//cam_mng_dump_event_action_union(event_action);
				}
			}
		}
	}

	session->event_action_thread_id = -1;
	return 0;
}

int camera_close(cams_t *session)
{
	int ret;
	_CHECK_SESSION_RETURN();

	if (session->event_handle >= 0) {
		csi_camera_destory_event(session->event_handle);
		session->event_handle = NULL;
	}

	if (csi_camera_close(session->camera_handle) != 0) {
		LOG_E("Close camera() failed\n");
		return -1;
	}

	_camera_session_reset(session);
	session->state = CAMERA_STATE_CLOSED;
	return 0;
}

int camera_get_modes(cams_t *session)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET);

	if (_get_info_status(session, CAMERA_MODES_READY))
		return 0;

	ret = csi_camera_get_modes(session->camera_handle, &(session->camera_modes));
	if (ret != 0) {
		LOG_E("csi_camera_get_modes() failed, ret=%d\n", ret);
		_set_info_status(session, CAMERA_MODES_READY, false);
		return ret;
	}

	_set_info_status(session, CAMERA_MODES_READY, true);
	return 0;
}

int camera_set_mode(cams_t *session, int mode_id)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET);

	csi_camera_mode_cfg_s mode_cfg;
	mode_cfg.mode_id = mode_id;
	mode_cfg.calibriation = NULL;	// TODO
	mode_cfg.lib3a = NULL;		// TODO

	ret = csi_camera_set_mode(session->camera_handle, &mode_cfg);
	if (ret != 0) {
		LOG_E("csi_camera_set_mode() failed, ret=%d\n", ret);
		return ret;
	}

	session->state = CAMERA_STATE_MODE_SET;
	return 0;
}

int camera_query_property(cams_t *session,
			  csi_camera_property_description_s *description)
{
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET);

	return csi_camera_query_property(session->camera_handle, description);
}

int camera_set_property(cams_t *session, csi_camera_properties_s *properties)
{
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED|CAMERA_STATE_MODE_SET|CAMERA_STATE_RUNNING);

	return csi_camera_set_property(session->camera_handle, properties);
}

int camera_channel_query_list(cams_t *session)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET |
			    CAMERA_STATE_RUNNING);

	// Init channel's ID and Status
	if (! _get_info_status(session, CHANNEL_INIT_READY)) {
		for (int i = CSI_CAMERA_CHANNEL_0; i < CSI_CAMERA_CHANNEL_MAX_COUNT; i++) {
			session->chn_cfg[i].chn_id = i;	// CSI_CAMERA_CHANNEL_x
			session->chn_cfg[i].status = CSI_CAMERA_CHANNEL_INVALID;
		}
	}

	for (int i = CSI_CAMERA_CHANNEL_0; i < CSI_CAMERA_CHANNEL_MAX_COUNT; i++) {
		if (_get_info_status(session, CHANNEL_INIT_READY)) {
			if (session->chn_cfg[i].status == CSI_CAMERA_CHANNEL_INVALID) {
				continue;
			}
		}
		ret = csi_camera_channel_query(session->camera_handle, &session->chn_cfg[i]);
		if (ret != 0) {
			LOG_E("Get %d channel configuration from camera[%d] failed\n",
			      i, session->camera_id);
			continue;
		} else {
			//LOG_D("channel[%d] status=%s\n", session->chn_cfg[i].chn_id,
			//	camera_string_chn_status(session->chn_cfg[i].status));
		}
	}

	_set_info_status(session, CHANNEL_INIT_READY, true);

	return 0;
}

int camera_channel_open(cams_t *session, csi_camera_channel_cfg_s *chn_cfg)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET |
			    CAMERA_STATE_RUNNING);

	if (! _get_info_status(session, CHANNEL_INIT_READY)) {
		camera_channel_query_list(session);
	}

	int chn_id = chn_cfg->chn_id;
	csi_camera_channel_cfg_s *channel = &(session->chn_cfg[chn_id]);
	if (channel->status != CSI_CAMERA_CHANNEL_CLOSED) {
		LOG_E("Channel[%d] status(%d) is not CLOSED\n",
		      chn_id, channel->status);
		return -1;
	}

	ret = csi_camera_channel_open(session->camera_handle, chn_cfg);
	if (ret != 0) {
		LOG_E("Open channel(%d) failed, ret=%d\n", chn_id, ret);
		return -1;
	}

	// Update session->chn_cfg
	csi_camera_channel_query(session->camera_handle,
				 &(session->chn_cfg[chn_cfg->chn_id]));

	LOG_I("Open channel[%d] OK\n", chn_id);
	return 0;
}

int camera_channel_close(cams_t *session, csi_camera_channel_id_e chn_id)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET |
			    CAMERA_STATE_RUNNING);

	csi_camera_channel_cfg_s *channel = &(session->chn_cfg[chn_id]);
	if (channel->status != CSI_CAMERA_CHANNEL_OPENED) {
		LOG_E("Channel[%d] status(%d) is not CLOSED\n",
		      chn_id, channel->status);
		return -1;
	}

	ret = csi_camera_channel_close(session->camera_handle, chn_id);
	if (ret != 0) {
		LOG_E("Close channel(%d) failed, ret=%d\n", chn_id, ret);
		return -1;
	}

	LOG_I("Close channel[%d] OK\n", chn_id);
	return 0;
}

int camera_subscribe_event(cams_t *session)
{
	int ret;
	struct csi_camera_event_subscription subscribe;
	camera_event_action_union_t *event_action;

	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET);

	for (int i = 0; i < CSI_CAMERA_EVENT_MAX_COUNT; i++) {
		event_action = &session->camera_event_action[i];
		if (!event_action->camera.supported) {
			LOG_D("Camera event(%08x) is not supported\n",
				event_action->camera.event);
			continue;
		}

		LOG_D("Camera event(%08x) is supported\n", event_action->camera.event);

		subscribe.type = CSI_CAMERA_EVENT_TYPE_CAMERA;
		subscribe.id = event_action->camera.event;
		if (event_action->camera.subscribed) {
			ret = csi_camera_subscribe_event(session->event_handle, &subscribe);
			if (ret == 0)
				LOG_D("subscribe_event(type=CAMERA, id=0x%08x(%s)) OK\n",
				      subscribe.id, camera_string_camera_event_type(subscribe.id));
			else
				LOG_E("subscribe_event(type=CAMERA, id=0x%08x(%s)) Failed\n",
				      subscribe.id, camera_string_camera_event_type(subscribe.id));
		} else {
			ret = csi_camera_unsubscribe_event(session->event_handle, &subscribe);
			if (ret == 0)
				LOG_D("unsubscribe_event(type=CAMERA, id=0x%08x(%s)) OK\n",
				      subscribe.id, camera_string_camera_event_type(subscribe.id));
			else
				LOG_E("unsubscribe_event(type=CAMERA, id=0x%08x(%s)) Failed\n",
				      subscribe.id, camera_string_camera_event_type(subscribe.id));
		}
	}

	for (int chn = 0; chn < CSI_CAMERA_CHANNEL_MAX_COUNT; chn++) {
		if (session->chn_cfg[chn].status != CSI_CAMERA_CHANNEL_OPENED) {
			LOG_D("Camera[%d]:Channel[%d] is not OPENED\n", session->camera_id, chn);
			continue;
		}
		for (int i = 0; i < CSI_CAMERA_CHANNEL_EVENT_MAX_COUNT; i++) {
			event_action = &session->channel_event_action[chn][i];
			if (!event_action->channel.supported) {
				LOG_D("channel(%d) event(%08x) is not supported\n",
					chn, event_action->channel.event);
				continue;
			}
			LOG_D("channel(%d) event(%08x) is supported\n",
				chn, event_action->channel.event);

			subscribe.type = CSI_CAMERA_EVENT_TYPE_CHANNEL0 + chn;
			subscribe.id = event_action->camera.event;
			if (event_action->camera.subscribed) {	// FIXME: event_action->camera.subscribed in dialog
				ret = csi_camera_subscribe_event(session->event_handle, &subscribe);
				if (ret == 0)
					LOG_D("subscribe_event(type=CHANNEL%d, id=0x%08x(%s)) OK\n",
					      chn, subscribe.id, camera_string_channel_event_type(subscribe.id));
				else
					LOG_E("subscribe_event(type=CHANNEL%d, id=0x%08x(%s)) Failed\n",
					      chn, subscribe.id, camera_string_channel_event_type(subscribe.id));
			} else {
				ret = csi_camera_unsubscribe_event(session->event_handle, &subscribe);
				if (ret == 0)
					LOG_D("unsubscribe_event(type=CHANNEL%d, id=0x%08x(%s)) OK\n",
					      chn, subscribe.id, camera_string_channel_event_type(subscribe.id));
				else
					LOG_E("unsubscribe_event(type=CHANNEL%d, id=0x%08x(%s)) Failed\n",
					      chn, subscribe.id, camera_string_channel_event_type(subscribe.id));
			}
		}
	}

	return 0;
}

int camera_register_event_action(cams_t *session,
				 camera_action_fun_t camera_action_fun,
				 camera_action_fun_t channel_action_fun)
{
	_CHECK_SESSION_RETURN();
	ENTER_VOID();

	session->camera_action_fun = camera_action_fun;
	session->channel_action_fun = channel_action_fun;
	return 0;
}

static void *camera_event_action_thread(void *arg)
{
	int ret;
	cams_t *session = (cams_t *)arg;
	LOG_D("pthread start\n");

	if (session == NULL) {
		LOG_E("session is NULL\n");
		return NULL;
	}

	csi_camera_event_s event;
	int timeout = -1; // unit: ms, -1 means wait forever, or until error occurs
	int loop_count = 0;
	while (camera_thread_flag) {
		LOG_D("while(true)...loog_count=%d\n", ++loop_count);
		ret = csi_camera_get_event(session->event_handle, &event, timeout);
		if (ret != 0) {
			LOG_E("csi_camera_get_event() failed, ret=%d\n", ret);
			continue;
		}

		LOG_D("event{.type=%d, .id=%d}\n", event.type, event.id);
		if (event.type == CSI_CAMERA_EVENT_TYPE_CAMERA) {
			if (session->camera_action_fun != NULL) {
				session->camera_action_fun(session, &event);
			} else {
				LOG_D("session->camera_action_fun is NULL\n");
			}
		} else if (event.type >= CSI_CAMERA_EVENT_TYPE_CHANNEL0 &&
			   event.type <= CSI_CAMERA_EVENT_TYPE_CHANNEL7) {
			if (session->channel_action_fun != NULL) {
				session->channel_action_fun(session, &event);
			} else {
				LOG_D("session->channel_action_fun is NULL\n");
			}
		} else {
			LOG_E("Unknown event type:%d\n", event.type);
		}
	}
	pthread_exit(NULL);
}

int camera_channel_start(cams_t *session, csi_camera_channel_id_e chn_id)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_OPENED | CAMERA_STATE_MODE_SET);

	LOG_D("camera_channel_start = %d\n",chn_id);
	ret = csi_camera_channel_start(session->camera_handle, chn_id);
	if (ret != 0) {
		LOG_E("csi_camera_channel_start() failed, ret=%d", ret);
		return -1;
	}

	// start event action thread
	camera_thread_flag = true;
	ret = pthread_create(&session->event_action_thread_id, NULL,
			     (void *)camera_event_action_thread, session);
	if (ret != 0) {
		LOG_E("pthread_create() failed, ret=%d", ret);
		return -1;
	}

	session->state = CAMERA_STATE_RUNNING;
	LOG_D("camera_channel_start(chn_id=%d) OK\n", chn_id);
	return ret;
}

int camera_channel_stop(cams_t *session, csi_camera_channel_id_e chn_id)
{
	int ret;
	_CHECK_SESSION_RETURN();
	_CHECK_STATE_RETURN(CAMERA_STATE_RUNNING);
	int i  = 0;

	camera_thread_flag = false;
	if (pthread_join(session->event_action_thread_id, NULL)) {
		LOG_E("pthread_join() failed, %s\n", strerror(errno));
		return -2;
	}
	session->event_action_thread_id = -1;
	LOG_D("pthread canceled\n");

	ret = csi_camera_channel_stop(session->camera_handle, chn_id);
	if (ret != 0) {
		LOG_E("csi_camera_channel_stop() faild, ret=%d\n", ret);
		return -1;
	}

	LOG_D("camera_channel_stop ok\n");
	session->state = CAMERA_STATE_OPENED;
	return ret;
}

