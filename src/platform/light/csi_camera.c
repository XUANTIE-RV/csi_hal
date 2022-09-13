/*
 * Copyright (C) 2021 Alibaba Group Holding Limited
 * Author: LuChongzhi <chongzhi.lcz@alibaba-inc.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>

#include <csi_camera.h>
#include <video.h>
#include <stdlib.h>

int csi_camera_query_list(csi_camera_infos_t *infos)
{

	return 0;
}


csi_cam_handle_t csi_camera_open(const char *cam_name)
{


	return NULL;
}

int csi_camera_close(csi_cam_handle_t cam_handle)
{


	return 0;
}

int csi_camera_get_modes(csi_cam_handle_t cam_handle, csi_camera_modes_t *modes)
{

	return 0;
}

int csi_camera_set_mode(csi_cam_handle_t cam_handle, csi_camera_mode_cfg_t *cfg)
{
	return 0;
}

int csi_camera_query_property(csi_cam_handle_t cam_handle,
			      csi_camera_property_description_t *desc)
{
    return 0;
}


int csi_camera_get_property(csi_cam_handle_t cam_handle,
			    csi_camera_properties_t *properties)
{

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
			    csi_camera_properties_t *properties)
{
	return 0;
}

int csi_camera_query_frame(csi_cam_handle_t cam_handle,
			   csi_camera_channel_id_e chn_id,
			   int frm_id,
			   csi_frame_t *frm)

{
	return 0;
}

static int cam_channel_open(csi_cam_handle_t cam_handle, csi_camera_channel_id_e chn_id,
							csi_camera_channel_cfg_t *cfg)
{

    return 0;
}

int csi_camera_channel_open(csi_cam_handle_t cam_handle,
			    csi_camera_channel_cfg_t *cfg)
{

	return 0;
}

int csi_camera_channel_close(csi_cam_handle_t cam_handle,
			     csi_camera_channel_id_e chn_id)
{
	return 0;
}

int csi_camera_channel_query(csi_cam_handle_t cam_handle,
			     csi_camera_channel_cfg_t *cfg)
{

	return 0;
}


csi_cam_event_handle_t csi_camera_create_event(csi_cam_handle_t cam_handle)
{

	return 0;
}

int csi_camera_destory_event(csi_cam_event_handle_t event_handle)
{

	return 0;
}

int csi_camera_subscribe_event(csi_cam_event_handle_t event_handle,
			       csi_camera_event_subscription_t *subscribe)
{
	return 0;
}
int csi_camera_unsubscribe_event(csi_cam_event_handle_t event_handle,
				 csi_camera_event_subscription_t *subscribe)
{

	return 0;
}

int csi_camera_get_event(csi_cam_event_handle_t event_handle,
			 csi_camera_event_t *event,
			 int timeout)
{

	return 0;
}

int csi_camera_get_frame_count(csi_cam_handle_t cam_handle,
			       csi_camera_channel_id_e chn_id)
{
	return 0;
}

int csi_camera_get_frame(csi_cam_handle_t cam_handle,
			 csi_camera_channel_id_e chn_id,
			 csi_frame_t *frame,
			 int timeout)
{

	return 0;
}

int csi_camera_put_frame(csi_frame_t *frame)
{
	return 0;
}

static int cam_channel_set_stream(csi_cam_handle_t cam_handle, int chn_id, int on)
{

    return 0;
}

int csi_camera_channel_start(csi_cam_handle_t cam_handle, csi_camera_channel_id_e chn_id)
{


    return 0;
}

int csi_camera_channel_stop(csi_cam_handle_t cam_handle, csi_camera_channel_id_e chn_id)
{
    return 0;
}
