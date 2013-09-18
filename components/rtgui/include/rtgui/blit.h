#ifndef __RTGUI_BLIT_H__
#define __RTGUI_BLIT_H__

#include <rtgui/rtgui.h>

/* Assemble R-G-B values into a specified pixel format and store them */
#define PIXEL_FROM_RGBA(Pixel, fmt, r, g, b, a)                         \
{                                                                       \
    Pixel = ((r>>fmt->Rloss)<<fmt->Rshift)|                             \
        ((g>>fmt->Gloss)<<fmt->Gshift)|                                 \
        ((b>>fmt->Bloss)<<fmt->Bshift)|                                 \
        ((a>>fmt->Aloss)<<fmt->Ashift);                                 \
}
#define PIXEL_FROM_RGB(Pixel, fmt, r, g, b)                             \
{                                                                       \
    Pixel = ((r>>fmt->Rloss)<<fmt->Rshift)|                             \
        ((g>>fmt->Gloss)<<fmt->Gshift)|                                 \
        ((b>>fmt->Bloss)<<fmt->Bshift)|                                 \
        fmt->Amask;                                                     \
}
#define RGB565_FROM_RGB(Pixel, r, g, b)                                 \
{                                                                       \
    Pixel = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);                            \
}
#define RGB555_FROM_RGB(Pixel, r, g, b)                                 \
{                                                                       \
    Pixel = ((r>>3)<<10)|((g>>3)<<5)|(b>>3);                            \
}
#define RGB888_FROM_RGB(Pixel, r, g, b)                                 \
{                                                                       \
    Pixel = (r<<16)|(g<<8)|b;                                           \
}
#define ARGB8888_FROM_RGBA(Pixel, r, g, b, a)                           \
{                                                                       \
    Pixel = (a<<24)|(r<<16)|(g<<8)|b;                                   \
}
#define RGBA8888_FROM_RGBA(Pixel, r, g, b, a)                           \
{                                                                       \
    Pixel = (r<<24)|(g<<16)|(b<<8)|a;                                   \
}
#define ABGR8888_FROM_RGBA(Pixel, r, g, b, a)                           \
{                                                                       \
    Pixel = (a<<24)|(b<<16)|(g<<8)|r;                                   \
}
#define BGRA8888_FROM_RGBA(Pixel, r, g, b, a)                           \
{                                                                       \
    Pixel = (b<<24)|(g<<16)|(r<<8)|a;                                   \
}
#define ARGB2101010_FROM_RGBA(Pixel, r, g, b, a)                        \
{                                                                       \
    r = r ? ((r << 2) | 0x3) : 0;                                       \
    g = g ? ((g << 2) | 0x3) : 0;                                       \
    b = b ? ((b << 2) | 0x3) : 0;                                       \
    a = (a * 3) / 255;                                                  \
    Pixel = (a<<30)|(r<<20)|(g<<10)|b;                                  \
}

/* Load pixel of the specified format from a buffer and get its R-G-B values */
#define RGB_FROM_PIXEL(Pixel, fmt, r, g, b)                             \
{                                                                       \
    r = rtgui_blit_expand_byte[fmt->Rloss][((Pixel&fmt->Rmask)>>fmt->Rshift)]; \
    g = rtgui_blit_expand_byte[fmt->Gloss][((Pixel&fmt->Gmask)>>fmt->Gshift)]; \
    b = rtgui_blit_expand_byte[fmt->Bloss][((Pixel&fmt->Bmask)>>fmt->Bshift)]; \
}
#define RGB_FROM_RGB565(Pixel, r, g, b)                                 \
    {                                                                   \
    r = rtgui_blit_expand_byte[3][((Pixel&0xF800)>>11)];                \
    g = rtgui_blit_expand_byte[2][((Pixel&0x07E0)>>5)];                 \
    b = rtgui_blit_expand_byte[3][(Pixel&0x001F)];                      \
}
#define RGB_FROM_RGB555(Pixel, r, g, b)                                 \
{                                                                       \
    r = rtgui_blit_expand_byte[3][((Pixel&0x7C00)>>10)];                \
    g = rtgui_blit_expand_byte[3][((Pixel&0x03E0)>>5)];                 \
    b = rtgui_blit_expand_byte[3][(Pixel&0x001F)];                      \
}
#define RGB_FROM_RGB888(Pixel, r, g, b)                                 \
{                                                                       \
    r = ((Pixel&0xFF0000)>>16);                                         \
    g = ((Pixel&0xFF00)>>8);                                            \
    b = (Pixel&0xFF);                                                   \
}

#define RGBA_FROM_PIXEL(Pixel, fmt, r, g, b, a)                         \
{                                                                       \
    r = rtgui_blit_expand_byte[fmt->Rloss][((Pixel&fmt->Rmask)>>fmt->Rshift)]; \
    g = rtgui_blit_expand_byte[fmt->Gloss][((Pixel&fmt->Gmask)>>fmt->Gshift)]; \
    b = rtgui_blit_expand_byte[fmt->Bloss][((Pixel&fmt->Bmask)>>fmt->Bshift)]; \
    a = rtgui_blit_expand_byte[fmt->Aloss][((Pixel&fmt->Amask)>>fmt->Ashift)]; \
}
#define RGBA_FROM_8888(Pixel, fmt, r, g, b, a)                          \
{                                                                       \
    r = (Pixel&fmt->Rmask)>>fmt->Rshift;                                \
    g = (Pixel&fmt->Gmask)>>fmt->Gshift;                                \
    b = (Pixel&fmt->Bmask)>>fmt->Bshift;                                \
    a = (Pixel&fmt->Amask)>>fmt->Ashift;                                \
}
#define RGBA_FROM_RGBA8888(Pixel, r, g, b, a)                           \
{                                                                       \
    r = (Pixel>>24);                                                    \
    g = ((Pixel>>16)&0xFF);                                             \
    b = ((Pixel>>8)&0xFF);                                              \
    a = (Pixel&0xFF);                                                   \
}
#define RGBA_FROM_ARGB8888(Pixel, r, g, b, a)                           \
{                                                                       \
    r = ((Pixel>>16)&0xFF);                                             \
    g = ((Pixel>>8)&0xFF);                                              \
    b = (Pixel&0xFF);                                                   \
    a = (Pixel>>24);                                                    \
}
#define RGBA_FROM_ABGR8888(Pixel, r, g, b, a)                           \
{                                                                       \
    r = (Pixel&0xFF);                                                   \
    g = ((Pixel>>8)&0xFF);                                              \
    b = ((Pixel>>16)&0xFF);                                             \
    a = (Pixel>>24);                                                    \
}
#define RGBA_FROM_BGRA8888(Pixel, r, g, b, a)                           \
{                                                                       \
    r = ((Pixel>>8)&0xFF);                                              \
    g = ((Pixel>>16)&0xFF);                                             \
    b = (Pixel>>24);                                                    \
    a = (Pixel&0xFF);                                                   \
}
#define RGBA_FROM_ARGB2101010(Pixel, r, g, b, a)                        \
{                                                                       \
    r = ((Pixel>>22)&0xFF);                                             \
    g = ((Pixel>>12)&0xFF);                                             \
    b = ((Pixel>>2)&0xFF);                                              \
    a = rtgui_blit_expand_byte[6][(Pixel>>30)];                         \
}

typedef void (*rtgui_blit_line_func)(rt_uint8_t *dst, rt_uint8_t *src, int line);
rtgui_blit_line_func rtgui_blit_line_get(int dst_bpp, int src_bpp);
rtgui_blit_line_func rtgui_blit_line_get_inv(int dst_bpp, int src_bpp);

#endif
