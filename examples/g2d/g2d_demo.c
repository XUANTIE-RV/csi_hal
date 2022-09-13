#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <csi_g2d.h>
#include <csi_g2d_types.h>

typedef struct _bitmap_file {
    const char *name;
    unsigned int width;
    unsigned int height;
    unsigned int format;
} bitmap_file;

const static bitmap_file sbitmaps[] = {
    {
	.name	= "resource/image1_640x480_nv12.yuv",
	.width	= 640,
	.height	= 480,
	.format	= CSI_G2D_FMT_NV12,
    },
    {
	.name	= "resource/image2_640x480_argb.rgb",
	.width	= 640,
	.height	= 480,
	.format	= CSI_G2D_FMT_BGRA8888,
    },
    {
	.name	= "resource/image3_640x480_nv12.yuv",
	.width	= 640,
	.height	= 480,
	.format	= CSI_G2D_FMT_NV12,
    },
    {
	.name	= "resource/horse_640x480_nv12.yuv",
	.width	= 640,
	.height	= 480,
	.format	= CSI_G2D_FMT_NV12,
    },
};

static int g2d_load_bitmap(csi_g2d_surface *surface, const char *path)
{
    int i, j, bytes, lsize;
    int width[3], height[3];
    FILE *file;
    void *ptr;

    if (!surface || !surface->nplanes || surface->nplanes > 3)
	return -1;

    file = fopen(path, "r");
    if (!file)
	return -1;

    width[0]  = surface->width;
    height[0] = surface->height;
    switch (surface->format) {
    case CSI_G2D_FMT_NV12:
    case CSI_G2D_FMT_NV21:
	width[1]  = surface->width / 2;
	height[1] = surface->height / 2;
	break;
    case CSI_G2D_FMT_ARGB8888:
    case CSI_G2D_FMT_BGRA8888:
	break;
    /* TODO: add more formats support here */
    default:
	return -1;
    }

    for (i = 0; i < surface->nplanes; i++) {
	for (j = 0; j < height[i]; j++) {
	    ptr = surface->lgcaddr[i] + j * surface->stride[i];
	    lsize = width[i] * surface->cpp[i];

	    bytes = fread(ptr, 1, lsize, file);
	    if (bytes != lsize)
		return -1;
	}
    }

    return 0;
}

static int g2d_save_bitmap(csi_g2d_surface *surface, const char *path)
{
    int i, j, bytes, lsize;
    int width[3], height[3];
    FILE *file;
    void *ptr;

    if (!surface || !surface->nplanes || surface->nplanes > 3)
	return -1;

    file = fopen(path, "w");
    if (!file)
	return -1;

    width[0]  = surface->width;
    height[0] = surface->height;
    switch (surface->format) {
    case CSI_G2D_FMT_NV12:
    case CSI_G2D_FMT_NV21:
	width[1]  = surface->width / 2;
	height[1] = surface->height / 2;
	break;
    /* TODO: add more formats support here */
    case CSI_G2D_FMT_ARGB8888:
	break;
    default:
	return -1;
    }

    for (i = 0; i < surface->nplanes; i++) {
	for (j = 0; j < height[i]; j++) {
	    ptr = surface->lgcaddr[i] + j * surface->stride[i];
	    lsize = width[i] * surface->cpp[i];

	    bytes = fwrite(ptr, 1, lsize, file);
	    if (bytes != lsize)
		return -1;
	}
    }

    return 0;
}

static int g2d_test_filterblit_with_rotation(void)
{
    int ret;
    const bitmap_file *bitmap = &sbitmaps[0];
    csi_g2d_surface *source, *target;
    csi_g2d_region dstRect;

    printf("start to do filterblit operation with 90 rotation: ");

    source = calloc(1, sizeof(*source));
    if (!source) {
	printf("allocate source surface failed\n");
	exit(-1);
    }

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    source->width	= bitmap->width;
    source->height	= bitmap->height;
    source->format	= bitmap->format;
    ret = csi_g2d_surface_create(source);
    if (ret) {
	printf("create source surface failed\n");
	exit(-1);
    }

    target->width	= bitmap->width * 2;
    target->height	= bitmap->height * 2;
    target->format	= CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    printf("%ux%u NV12 -> %ux%u ARGB8888\n", source->width, source->height, target->width, target->height);

    ret = g2d_load_bitmap(source, bitmap->name);
    if (ret) {
	printf("load bitmap to source failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_source(source);
    if (ret) {
	printf("set source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_blit_set_rotation(CSI_G2D_ROTATION_0_DEGREE);
    if (ret) {
	printf("set rotation to 0 failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_blit_set_filter_tap(CSI_G2D_FILTER_TAP_5,
			CSI_G2D_FILTER_TAP_5);
    if (ret) {
	printf("set filter tap failed\n");
	exit(-1);
    }

    ret = csi_g2d_blit_filterblit(&dstRect, 1);
    if (ret) {
	printf("filterblit nv12 to argb8888 failed\n");
	exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
	printf("flush g2d failed\n");
	exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/image1_result_1280x960_argb8888.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(source);
    if (ret) {
	printf("destroy source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("filterblit operation finished\n");
    return 0;

}

static int g2d_test_stretchblit_with_rotation(void)
{
    int ret;
    const bitmap_file *bitmap = &sbitmaps[0];
    csi_g2d_surface *source, *target;
    csi_g2d_region dstRect;

    printf("start to do stretchblit operation with 90 rotation: ");

    source = calloc(1, sizeof(*source));
    if (!source) {
	printf("allocate source surface failed\n");
	exit(-1);
    }

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    source->width	= bitmap->width;
    source->height	= bitmap->height;
    source->format	= bitmap->format;
    ret = csi_g2d_surface_create(source);
    if (ret) {
	printf("create source surface failed\n");
	exit(-1);
    }

    target->width	= bitmap->height * 2;
    target->height	= bitmap->width * 2;
    target->format	= CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    printf("%ux%u NV12 -> %ux%u ARGB8888\n", source->width, source->height, target->width, target->height);

    ret = g2d_load_bitmap(source, bitmap->name);
    if (ret) {
	printf("load bitmap to source failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_source(source);
    if (ret) {
	printf("set source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_blit_set_rotation(CSI_G2D_ROTATION_90_DEGREE);
    if (ret) {
	printf("set rotation to 90 failed\n");
	exit(-1);
    }

    ret = csi_g2d_blit_stretchblit(&dstRect, 1);
    if (ret) {
	printf("stretchblit nv12 to argb8888 failed\n");
	exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
	printf("flush g2d failed\n");
	exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/image1_result_960x1280_argb8888.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(source);
    if (ret) {
	printf("destroy source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("stretchblit operation with 90 rotation finished\n");
    return 0;
}

static int g2d_test_bitblit_with_alpha_blending(void)
{
    int ret;
    const bitmap_file *bitmap = &sbitmaps[0];
    csi_g2d_surface *source, *target;
    csi_g2d_region srcRect, dstRect;
    csi_g2d_blend_mode blend_mode;

    printf("start to do bitblit operation with alpha blending\n");

    source = calloc(1, sizeof(*source));
    if (!source) {
	printf("allocate source surface failed\n");
	exit(-1);
    }

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    source->width	= bitmap->width;
    source->height	= bitmap->height;
    source->format	= CSI_G2D_FMT_ARGB8888;

    ret = csi_g2d_surface_create(source);
    if (ret) {
	printf("create source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(source);
    if (ret) {
        printf("set source surface failed\n");
        exit(-1);
    }

    srcRect.left   = srcRect.top = 0;
    srcRect.right  = source->width;
    srcRect.bottom = source->height;

    ret = csi_g2d_fill(&srcRect, 1, 0x0);
    if (ret) {
	printf("fill source surface failed\n");
	exit(-1);
    }

    srcRect.left  = 50;
    srcRect.top   = 60;
    srcRect.right = 250;
    srcRect.bottom = 260;

    ret = csi_g2d_fill(&srcRect, 1, 0xFF0000FF);
    if (ret) {
	printf("fill source surface failed\n");
	exit(-1);
    }

    srcRect.left   = srcRect.top = 0;
    srcRect.right  = source->width;
    srcRect.bottom = source->height;

    target->width	= bitmap->width;
    target->height	= bitmap->height;
    target->format	= CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_fill(&dstRect, 1, 0x0);
    if (ret) {
	printf("fill source surface failed\n");
	exit(-1);
    }

    dstRect.left = 20;
    dstRect.top  = 30;
    dstRect.right  = 120;
    dstRect.bottom = 130;

    ret = csi_g2d_fill(&dstRect, 1, 0xFFFF0000);
    if (ret) {
	printf("fill source surface failed\n");
	exit(-1);
     }

    ret = csi_g2d_surface_set_source(source);
    if (ret) {
        printf("set source surface failed\n");
        exit(-1);
    }

    /* set alpha modes */
    blend_mode = CSI_G2D_BLEND_MODE_XOR;

    ret = csi_g2d_surface_set_source_alpha_mode(
	CSI_G2D_ALPHA_MODE_STRAIGHT,
	blend_mode
    );
    if (ret) {
	printf("set source alpha mode failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_source_global_alpha_mode(
	CSI_G2D_GLOBAL_ALPHA_MODE_OFF,
	0xFFFFFFFF
    );
    if (ret) {
	printf("set source global alpha mode failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target_alpha_mode(
	CSI_G2D_ALPHA_MODE_STRAIGHT,
	blend_mode
    );
    if (ret) {
	printf("set target alpha mode failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target_global_alpha_mode(
	CSI_G2D_GLOBAL_ALPHA_MODE_OFF,
	0xFFFFFFFF
    );
    if (ret) {
	printf("set target global alpha mode failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_enable_disable_alpha_blend(true);
    if (ret) {
	printf("enable alpha blend failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_blit_bitblit(&dstRect, 1);
    if (ret) {
        printf("bitblit argb8888 to argb8888 with alpha blend failed\n");
        exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
        printf("flush g2d failed\n");
        exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/alpha_blend_result_640x480_argb8888.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(source);
    if (ret) {
	printf("destroy source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("bitblit operation with alpha blending finished\n");
    return 0;
}

static int g2d_test_bitblit_with_rotation(void)
{
    int ret;
    const bitmap_file *bitmap = &sbitmaps[0];
    csi_g2d_surface *source, *target;
    csi_g2d_region dstRect;

    printf("start to do bitblit operation with 90 rotation: ");

    source = calloc(1, sizeof(*source));
    if (!source) {
	printf("allocate source surface failed\n");
	exit(-1);
    }

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    source->width	= bitmap->width;
    source->height	= bitmap->height;
    source->format	= bitmap->format;
    ret = csi_g2d_surface_create(source);
    if (ret) {
	printf("create source surface failed\n");
	exit(-1);
    }

    target->width	= bitmap->height;
    target->height	= bitmap->width;
    target->format	= CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    printf("%ux%u NV12 -> %ux%u ARGB8888\n", source->width, source->height, target->width, target->height);

    ret = g2d_load_bitmap(source, bitmap->name);
    if (ret) {
	printf("load bitmap to source failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_source(source);
    if (ret) {
	printf("set source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_blit_set_rotation(CSI_G2D_ROTATION_90_DEGREE);
    if (ret) {
	printf("set rotation to 90 failed\n");
	exit(-1);
    }

    ret = csi_g2d_blit_bitblit(&dstRect, 1);
    if (ret) {
	printf("bitblit nv12 to nv12 failed\n");
	exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
	printf("flush g2d failed\n");
	exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/image1_result_480x640_argb8888.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(source);
    if (ret) {
	printf("destroy source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("bitblit operation with 90 rotation finished\n");
    return 0;

}

static int g2d_test_fill_rectangle(void)
{
    int ret;
    csi_g2d_surface *target;
    csi_g2d_region dstRect;

    printf("start to do rectangle fill operation: ");

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    target->width	= 640;
    target->height	= 480;
    target->format	= CSI_G2D_FMT_ARGB8888;

    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    printf("fill red to %ux%u rectangle\n", target->width, target->height);

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_fill(&dstRect, 1, 0xffff0000);
    if (ret) {
	printf("g2d fill operation failed\n");
	exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
	printf("flush g2d failed\n");
	exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/image1_result_640x480_argb8888_fill.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("fill rectangle operation finished\n");
    return 0;
}

static int g2d_test_draw_rectangle(void)
{
    int ret;
    const bitmap_file *bitmap = &sbitmaps[1];
    csi_g2d_surface *target;
    csi_g2d_region dstRect;
    csi_g2d_rectangle rectangle;

    printf("start to do rectangle draw operation\n");

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    target->width	= bitmap->width;
    target->height	= bitmap->height;
    target->format	= CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    ret = g2d_load_bitmap(target, bitmap->name);
    if (ret) {
	printf("load bitmap to target failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_brush_create(0xffff0000, false);
    if (ret) {
	printf("create brush failed\n");
	exit(-1);
    }

    rectangle.line[0].start.x = dstRect.left + 30;
    rectangle.line[0].start.y = dstRect.top  + 30;
    rectangle.line[0].end.x   = dstRect.right  - 30;
    rectangle.line[0].end.y   = dstRect.top  + 30;

    rectangle.line[1].start.x = dstRect.left + 30;
    rectangle.line[1].start.y = dstRect.top  + 30;
    rectangle.line[1].end.x   = dstRect.left + 30;
    rectangle.line[1].end.y   = dstRect.bottom - 30;

    rectangle.line[2].start.x = dstRect.left + 30;
    rectangle.line[2].start.y = dstRect.bottom - 30;
    rectangle.line[2].end.x   = dstRect.right  - 30;
    rectangle.line[2].end.y   = dstRect.bottom - 30;

    rectangle.line[3].start.x = dstRect.right  - 30;
    rectangle.line[3].start.y = dstRect.top + 30;
    rectangle.line[3].end.x   = dstRect.right  - 30;
    rectangle.line[3].end.y   = dstRect.bottom - 30;

    ret = csi_g2d_line_draw_rectangle(&rectangle, 1);
    if (ret) {
	printf("draw one rectangle failed\n");
	exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
	printf("flush g2d failed\n");
	exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/image1_result_640x480_argb8888_rectangle.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("draw rectangle operation finished\n");
    return 0;
}

static int g2d_test_multisrc_blit(void)
{
    int i, ret;
    csi_g2d_surface *source[4], *target;
    csi_g2d_region srcRect, dstRect;

    printf("start to do multisrc blit operation\n");

    for (i = 0; i < 4; i++) {
	source[i] = calloc(1, sizeof(csi_g2d_surface));
	if (!source[i]) {
	    printf("allocate source surface %d failed\n", i);
	    exit(-1);
	}
    }

    target = calloc(1, sizeof(*target));
    if (!target) {
        printf("allocate target surface failed\n");
        exit(-1);
    }

    for (i = 0; i < 4; i++) {
        source[i]->width  = sbitmaps[i].width;
        source[i]->height = sbitmaps[i].height;
        source[i]->format = sbitmaps[i].format;
        ret = csi_g2d_surface_create(source[i]);
        if (ret) {
            printf("create source surface failed\n");
            exit(-1);
        }

        ret = g2d_load_bitmap(source[i], sbitmaps[i].name);
        if (ret) {
            printf("load bitmap %d to source failed\n", i);
            exit(-1);
        }
    }

    target->width       = 640;
    target->height      = 480;
    target->format      = CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
        printf("create target surface failed\n");
        exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
        printf("set target surface failed\n");
        exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_fill(&dstRect, 1, 0x0);
    if (ret) {
        printf("fill target surface to 0x0 failed\n");
        exit(-1);
    }

    for (i = 0; i < 4; i++) {
        ret = csi_g2d_surface_select_source(i);
        if (ret) {
            printf("select source %d\n", i);
            exit(-1);
        }

        ret = csi_g2d_surface_set_source(source[i]);
        if (ret) {
            printf("set source %d surface failed\n", i);
            exit(-1);
        }

        switch (i % 4) {
        case 0:
            srcRect.left = 320;
            srcRect.top  = 240;
            srcRect.right = srcRect.left + 320;
            srcRect.bottom = srcRect.top + 240;
            break;
        case 1:
            srcRect.left = 0;
            srcRect.top  = 240;
            srcRect.right = srcRect.left + 320;
            srcRect.bottom = srcRect.top + 240;
            break;
        case 2:
            srcRect.left = 0;
            srcRect.top  = 0;
            srcRect.right = srcRect.left + 320;
            srcRect.bottom = srcRect.top + 240;
            break;
        case 3:
            srcRect.left = 320;
            srcRect.top  = 0;
            srcRect.right = srcRect.left + 320;
            srcRect.bottom = srcRect.top + 240;
            break;
        }

        ret = csi_g2d_surface_set_source_clipping(&srcRect);
        if (ret) {
            printf("set source %d clipping failed\n", i);
            return ret;
        }
    }

    ret = csi_g2d_blit_multisrc_blit(0xF, &dstRect, 1);
    if (ret) {
        printf("multisrc blit failed\n");
        exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
        printf("flush g2d failed\n");
        exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/multisrc_result_640x480_argb.rgb");
    if (ret) {
        printf("save result failed\n");
        exit(-1);
    }

    for (i = 0; i < 4; i++) {
	ret = csi_g2d_surface_destroy(source[i]);
	if (ret) {
	    printf("destroy source surface failed\n");
	    exit(-1);
	}
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("multisrc blit operation with 4 sources finished\n");
    return 0;
}

static int g2d_test_bitblit(void)
{
    int ret;
    const bitmap_file *bitmap = &sbitmaps[0];
    csi_g2d_surface *source, *target;
    csi_g2d_region dstRect;

    printf("start to do bitblit operation: ");

    source = calloc(1, sizeof(*source));
    if (!source) {
	printf("allocate source surface failed\n");
	exit(-1);
    }

    target = calloc(1, sizeof(*target));
    if (!target) {
	printf("allocate target surface failed\n");
	exit(-1);
    }

    source->width	= bitmap->width;
    source->height	= bitmap->height;
    source->format	= bitmap->format;
    ret = csi_g2d_surface_create(source);
    if (ret) {
	printf("create source surface failed\n");
	exit(-1);
    }

    target->width	= bitmap->width;
    target->height	= bitmap->height;
    target->format	= CSI_G2D_FMT_ARGB8888;
    ret = csi_g2d_surface_create(target);
    if (ret) {
	printf("create target surface failed\n");
	exit(-1);
    }

    printf("%ux%u NV12 -> %ux%u ARGB8888\n", source->width, source->height, target->width, target->height);

    ret = g2d_load_bitmap(source, bitmap->name);
    if (ret) {
	printf("load bitmap to source failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_source(source);
    if (ret) {
	printf("set source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_set_target(target);
    if (ret) {
	printf("set target surface failed\n");
	exit(-1);
    }

    dstRect.left   = dstRect.top = 0;
    dstRect.right  = target->width;
    dstRect.bottom = target->height;

    ret = csi_g2d_blit_bitblit(&dstRect, 1);
    if (ret) {
	printf("bitblit nv12 to nv12 failed\n");
	exit(-1);
    }

    ret = csi_g2d_flush();
    if (ret) {
	printf("flush g2d failed\n");
	exit(-1);
    }

    ret = g2d_save_bitmap(target, "result/image1_result_640x480_argb8888.rgb");
    if (ret) {
	printf("save result failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(source);
    if (ret) {
	printf("destroy source surface failed\n");
	exit(-1);
    }

    ret = csi_g2d_surface_destroy(target);
    if (ret) {
	printf("destroy target surface failed\n");
	exit(-1);
    }

    printf("bitblit operation with 90 rotation finished\n");
    return 0;
}

int main(void)
{
    int ret;

    ret = csi_g2d_open();
    if (ret) {
	printf("open csi g2d failed\n");
	exit(-1);
    }

    g2d_test_bitblit();

    g2d_test_draw_rectangle();

    g2d_test_fill_rectangle();

    g2d_test_bitblit_with_rotation();

    g2d_test_stretchblit_with_rotation();

    g2d_test_filterblit_with_rotation();

    g2d_test_bitblit_with_alpha_blending();

    g2d_test_multisrc_blit();

    ret = csi_g2d_close();
    if (ret) {
	printf("close g2d failed\n");
	exit(-1);
    }

    exit(0);
}