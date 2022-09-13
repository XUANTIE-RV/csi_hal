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
#include <csi_venc.h>

int csi_venc_get_version(csi_api_version_u *version)
{
	version->major = CSI_VENC_VERSION_MAJOR;
	version->minor = CSI_VENC_VERSION_MINOR;
	return 0;
}

int csi_venc_query_list(csi_venc_infos_s *infos)
{
	infos->count = 0;
	return 0;
}

int csi_venc_open(csi_venc_dev_t *enc, const char *device_name)
{
	return 0;
}

int csi_venc_close(csi_venc_dev_t enc)
{
	return 0;
}

int csi_venc_create_channel(csi_venc_chn_t *chn, csi_venc_dev_t enc, csi_venc_chn_cfg_s *cfg)
{
	return 0;
}

int csi_venc_destory_channel(csi_venc_chn_t chn)
{
	return 0;
}

#if 0
int csi_venc_set_memory_allocator(csi_venc_chn_t chn, csi_allocator_s *allocator)
{
	return 0;
}
#endif

int csi_venc_set_ext_property(csi_venc_chn_t chn, csi_venc_chn_ext_property_s *prop)
{
	return 0;
}

int csi_venc_get_ext_property(csi_venc_chn_t chn, csi_venc_chn_ext_property_s *prop)
{
	return 0;
}

int csi_venc_start(csi_venc_chn_t chn)
{
	return 0;
}

int csi_venc_stop(csi_venc_chn_t chn)
{
	return 0;
}

int csi_venc_reset(csi_venc_chn_t chn)
{
	return 0;
}


int csi_venc_send_frame(csi_venc_chn_t chn, csi_frame_s *frame, int timeout)
{
	return 0;
}

int csi_venc_send_frame_ex(csi_venc_chn_t chn, csi_frame_s *frame, int timeout,
			   csi_venc_frame_prop_s *prop, int prop_count)
{
	return 0;
}

int csi_venc_get_stream(csi_venc_chn_t chn, csi_stream_s *stream, int timeout)
{
	return 0;
}

int csi_venc_release_stream(csi_venc_chn_t chn, csi_stream_s *stream)
{
	return 0;
}

int csi_venc_query_status(csi_venc_chn_t chn, csi_venc_chn_status_s *status)
{
	return 0;
}

int csi_venc_create_event_handle(csi_venc_event_handle_t *chn, csi_venc_dev_t event_handle)
{
	return 0;
}

int csi_venc_destory_event(csi_venc_event_handle_t event_handle)
{
	return 0;
}

int csi_venc_subscribe_event(csi_venc_event_handle_t event_handle,
			     csi_venc_event_subscription_s *subscribe)
{
	return 0;
}

int csi_venc_unsubscribe_event(csi_venc_event_handle_t event_handle,
			       csi_venc_event_subscription_s *subscribe)
{
	return 0;
}

int csi_venc_get_event(csi_venc_event_handle_t event_handle,
		       csi_venc_event_s *event, int timeout)
{
	return 0;
}
