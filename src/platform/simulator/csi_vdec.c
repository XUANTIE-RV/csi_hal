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
#include <csi_vdec.h>

int csi_vdec_get_version(csi_api_version_u *version)
{
	version->major = CSI_VDEC_VERSION_MAJOR;
	version->minor = CSI_VDEC_VERSION_MINOR;
	return 0;
}

int csi_vdec_query_list(csi_vdec_infos_s *infos)
{
	infos->count = 0;
	return 0;
}

int csi_vdec_open(csi_vdec_dev_t *dec, const char *device_name)
{
	return 0;
}

int csi_vdec_close(csi_vdec_dev_t dec)
{
	return 0;
}

int csi_vdec_create_channel(csi_vdec_chn_t *chn, csi_vdec_dev_t dec, csi_vdec_config_s *cfg)
{
	return 0;
}

int csi_vdec_destory_channel(csi_vdec_chn_t chn)
{
	return 0;
}

#if 0
int csi_vdec_set_memory_allocator(csi_vdec_chn_t chn, csi_allocator_s *allocator)
{
	return 0;
}
#endif

int csi_vdec_set_mode(csi_vdec_chn_t chn, csi_vdec_mode_s *mode)
{
	return 0;
}

int csi_vdec_get_mode(csi_vdec_chn_t chn, csi_vdec_mode_s *mode)
{
	return 0;
}

int csi_vdec_set_chn_config(csi_vdec_chn_t chn, csi_vdec_config_s *cfg)
{
	return 0;
}

int csi_vdec_get_chn_config(csi_vdec_chn_t chn, csi_vdec_config_s *cfg)
{
	return 0;
}

int csi_vdec_set_pp_config(csi_vdec_chn_t chn, csi_vdec_pp_config_s *cfg)
{
	return 0;
}

int csi_vdec_get_pp_config(csi_vdec_chn_t chn, csi_vdec_pp_config_s *cfg)
{
	return 0;
}

int csi_vdec_start(csi_vdec_chn_t chn)
{
	return 0;
}

int csi_vdec_stop(csi_vdec_chn_t chn)
{
	return 0;
}

int csi_vdec_reset(csi_vdec_chn_t chn)
{
	return 0;
}

int csi_vdec_send_stream_buf(csi_vdec_chn_t chn, csi_vdec_stream_s *stream, int32_t timeout)
{
	return 0;
}

int csi_vdec_register_frames(csi_vdec_chn_t chn, csi_frame_s *frame[], int count)
{
	return 0;
}

int csi_vdec_put_frame(csi_vdec_chn_t chn, csi_frame_s *frame)
{
	return 0;
}

int csi_vdec_get_frame(csi_vdec_chn_t chn, csi_frame_s **frame, int32_t timeout)
{
	return 0;
}

int csi_vdec_query_status(csi_vdec_chn_t chn, csi_vdec_chn_status_s *pstStatus)
{
	return 0;
}

int csi_vdec_create_event_handle(csi_vdec_event_handle_t *handle, csi_vdec_dev_t event_handle)
{
	return 0;
}

int csi_vdec_destory_event(csi_vdec_event_handle_t event_handle)
{
	return 0;
}

int csi_vdec_subscribe_event(csi_vdec_event_handle_t event_,
			     csi_vdec_event_subscription_t *subscribe)
{
	return 0;
}

int csi_vdec_unsubscribe_event(csi_vdec_event_handle_t event_handle,
			       csi_vdec_event_subscription_t *subscribe)
{
	return 0;
}

int csi_vdec_get_event(csi_vdec_event_handle_t event_handle,
		       csi_vdec_event_s *event, int timeout)
{
	return 0;
}
