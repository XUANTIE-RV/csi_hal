/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include <stdio.h>
#include <csi_camera.h>

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum manage_target {
	MANAGE_TARGET_INVALID = -1,
	MANAGE_TARGET_CAMERA = 0,
	MANAGE_TARGET_CHANNEL,
} manage_target_e;

typedef enum camera_state {
	CAMERA_STATE_CLOSED	= (1 << 0),
	CAMERA_STATE_OPENED	= (1 << 1),
	CAMERA_STATE_MODE_SET	= (1 << 2),
	CAMERA_STATE_RUNNING	= (1 << 3),
} camera_state_e;

typedef enum camera_action {
	CAMERA_ACTION_NONE          = 0,
	CAMERA_ACTION_LOG_PRINT     = (1 << 0),
	CAMERA_ACTION_CAPTURE_FRAME = (1 << 1),

	CAMERA_ACTION_MAX_COUNT     = 2,
} camera_action_e;

typedef struct camera_event_action {
	csi_camera_event_id_e event;
	bool                  supported;
	bool                  subscribed;
	camera_action_e       action;
} camera_event_action_t;

typedef enum camera_channel_action {
	CAMERA_CHANNEL_ACTION_NONE          = 0,
	CAMERA_CHANNEL_ACTION_LOG_PRINT     = (1 << 0),
	CAMERA_CHANNEL_ACTION_CAPTURE_FRAME = (1 << 1),
	CAMERA_CHANNEL_ACTION_DISPLAY_FRAME = (1 << 2),

	CAMERA_CHANNEL_ACTION_MAX_COUNT     = 3,
} camera_channel_action_e;

typedef struct camera_channel_event_action {
	csi_camera_channel_event_id_e event;
	bool                          supported;
	bool                          subscribed;
	camera_channel_action_e       action;
} camera_channel_event_action_t;

typedef struct camera_event_action_union {
	manage_target_e target;
	int camera_id;
	int channel_id;
	union {
		camera_event_action_t camera;
		camera_channel_event_action_t channel;
	};
} camera_event_action_union_t;

struct camera_session;
typedef int (*camera_action_fun_t)(struct camera_session *, csi_camera_event_s *);

typedef struct camera_session {
	camera_state_e state;	/* camera current state */
	uint32_t info_status;	/* bitmask of camera_info_status_e */

	int camera_id;				/* from csi_camera_infos_s.info[id] */
	csi_cam_handle_t camera_handle;			/* from csi_camera_open() */
	csi_camera_infos_s camera_infos;	/* from csi_camera_query_list() */
	csi_camera_modes_s camera_modes;	/* from csi_camera_get_modes() */
	int camera_mode_id;			/* from csi_camera_modes_s.modes[].mode_id */

	csi_camera_channel_cfg_s chn_cfg[CSI_CAMERA_CHANNEL_MAX_COUNT];
	csi_camera_channel_cfg_s chn_cfg_tmp;	/* store temp params when configuring */

	csi_cam_event_handle_t event_handle;
	camera_event_action_union_t camera_event_action[CSI_CAMERA_EVENT_MAX_COUNT];
	camera_event_action_union_t channel_event_action[CSI_CAMERA_CHANNEL_MAX_COUNT][CSI_CAMERA_CHANNEL_EVENT_MAX_COUNT];

	camera_action_fun_t camera_action_fun;
	camera_action_fun_t channel_action_fun;

	pthread_t event_action_thread_id;
} cams_t;

int camera_create_session(cams_t **session);
int camera_destory_session(cams_t **session);

int camera_query_list(cams_t *session);
int camera_get_caps(cams_t *session);
int camera_open(cams_t *session, int cam_id);
int camera_close(cams_t *session);

int camera_get_modes(cams_t *session);
int camera_set_mode(cams_t *session, int mode_id);

int camera_query_property(cams_t *session,
			  csi_camera_property_description_s *description);
int camera_set_property(cams_t *session, csi_camera_properties_s *properties);

int camera_channel_query_list(cams_t *session);
int camera_channel_open(cams_t *session, csi_camera_channel_cfg_s *chn_cfg);
int camera_channel_close(cams_t *session, csi_camera_channel_id_e chn_id);

int camera_register_event_action(cams_t *session,
			    camera_action_fun_t camera_action_fun,
			    camera_action_fun_t channel_action_fun);
int camera_subscribe_event(cams_t *session);
int camera_channel_start(cams_t *session, csi_camera_channel_id_e chn_id);
int camera_channel_stop(cams_t *session, csi_camera_channel_id_e chn_id);

#ifdef  __cplusplus
}
#endif

#endif /* __CAMERA_MANAGER_H__ */
