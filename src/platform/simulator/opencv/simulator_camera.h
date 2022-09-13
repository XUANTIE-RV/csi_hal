#ifndef __SIMULATOR_CAMERA_H__
#define __SIMULATOR_CAMERA_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <csi_camera.h>

//wait for complete
typedef void *csi_cam_handle_t;
int csi_camera_opencv_get_picture_local(csi_pixel_fmt_e fmt, int height,
					int width, unsigned char *data,
					csi_camera_property_description_s *property_description);
int csi_camera_opencv_get_picture(int height, int width, unsigned char *data);
int csi_camera_opencv_format_picture(csi_pixel_fmt_e fmt, int dst_height,
				  int dst_width, unsigned char *data_dst,
				  int src_height, int src_width, unsigned char *data_src,
				  csi_camera_property_description_s *property_description);
int camera_open_opencv(int idx);
int camera_cfg_opencv(csi_cam_handle_t cam_handle, int height, int width);
int camera_property_set_opencv(csi_camera_property_s *property);
int camera_set_mode_opencv(csi_camera_mode_cfg_s *cfg);
int camera_close_opencv(csi_cam_handle_t cam_handle);
int camera_image_size_opencv(csi_frame_s *frame);

#ifdef __cplusplus
};
#endif

#endif
