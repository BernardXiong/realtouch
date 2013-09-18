/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include <rtgui/rtgui.h>
#include <rtgui/dc_draw.h>
#include <string.h>

#define SDL_Unsupported()   rt_kprintf("unsupported operation.\n")
#define SDL_memset			memset

static void
_dc_draw_line1(struct rtgui_dc * dst, int x1, int y1, int x2, int y2, rtgui_color_t color,
              rt_bool_t draw_end)
{
    if (y1 == y2) {
        int length;
        int pitch = (dst->pitch / dst->format->BytesPerPixel);
        rt_uint8_t *pixel;
        if (x1 <= x2) {
            pixel = (rt_uint8_t *)dst->pixels + y1 * pitch + x1;
            length = draw_end ? (x2-x1+1) : (x2-x1);
        } else {
            pixel = (rt_uint8_t *)dst->pixels + y1 * pitch + x2;
            if (!draw_end) {
                ++pixel;
            }
            length = draw_end ? (x1-x2+1) : (x1-x2);
        }
        SDL_memset(pixel, color, length);
    } else if (x1 == x2) {
        VLINE(rt_uint8_t, DRAW_FASTSETPIXEL1, draw_end);
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        DLINE(rt_uint8_t, DRAW_FASTSETPIXEL1, draw_end);
    } else {
        BLINE(x1, y1, x2, y2, DRAW_FASTSETPIXELXY1, draw_end);
    }
}

static void
_dc_draw_line2(struct rtgui_dc * dst, int x1, int y1, int x2, int y2, rtgui_color_t color,
              rt_bool_t draw_end)
{
    if (y1 == y2) {
        HLINE(rt_uint16_t, DRAW_FASTSETPIXEL2, draw_end);
    } else if (x1 == x2) {
        VLINE(rt_uint16_t, DRAW_FASTSETPIXEL2, draw_end);
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        DLINE(rt_uint16_t, DRAW_FASTSETPIXEL2, draw_end);
    } else {
        rt_uint8_t _r, _g, _b, _a;
        const SDL_PixelFormat * fmt = dst->format;
        SDL_GetRGBA(color, fmt, &_r, &_g, &_b, &_a);
        if (fmt->Rmask == 0x7C00) {
            AALINE(x1, y1, x2, y2,
                   DRAW_FASTSETPIXELXY2, DRAW_SETPIXELXY_BLEND_RGB555,
                   draw_end);
        } else if (fmt->Rmask == 0xF800) {
            AALINE(x1, y1, x2, y2,
                   DRAW_FASTSETPIXELXY2, DRAW_SETPIXELXY_BLEND_RGB565,
                   draw_end);
        } else {
            AALINE(x1, y1, x2, y2,
                   DRAW_FASTSETPIXELXY2, DRAW_SETPIXELXY2_BLEND_RGB,
                   draw_end);
        }
    }
}

static void
_dc_draw_line4(struct rtgui_dc * dst, int x1, int y1, int x2, int y2, rtgui_color_t color,
              rt_bool_t draw_end)
{
    if (y1 == y2) {
        HLINE(rt_uint32_t, DRAW_FASTSETPIXEL4, draw_end);
    } else if (x1 == x2) {
        VLINE(rt_uint32_t, DRAW_FASTSETPIXEL4, draw_end);
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        DLINE(rt_uint32_t, DRAW_FASTSETPIXEL4, draw_end);
    } else {
        rt_uint8_t _r, _g, _b, _a;
        const SDL_PixelFormat * fmt = dst->format;
        SDL_GetRGBA(color, fmt, &_r, &_g, &_b, &_a);
        if (fmt->Rmask == 0x00FF0000) {
            if (!fmt->Amask) {
                AALINE(x1, y1, x2, y2,
                       DRAW_FASTSETPIXELXY4, DRAW_SETPIXELXY_BLEND_RGB888,
                       draw_end);
            } else {
                AALINE(x1, y1, x2, y2,
                       DRAW_FASTSETPIXELXY4, DRAW_SETPIXELXY_BLEND_ARGB8888,
                       draw_end);
            }
        } else {
            AALINE(x1, y1, x2, y2,
                   DRAW_FASTSETPIXELXY4, DRAW_SETPIXELXY4_BLEND_RGB,
                   draw_end);
        }
    }
}

typedef void (*DrawLineFunc) (struct rtgui_dc * dst,
                              int x1, int y1, int x2, int y2,
                              rtgui_color_t color, rt_bool_t draw_end);

static DrawLineFunc
_dc_calc_draw_line_func(const SDL_PixelFormat * fmt)
{
    switch (fmt->BytesPerPixel) {
    case 1:
        if (fmt->BitsPerPixel < 8) {
            break;
        }
        return _dc_draw_line1;
    case 2:
        return _dc_draw_line2;
    case 4:
        return _dc_draw_line4;
    }
    return NULL;
}

int
rtgui_dc_draw_line(struct rtgui_dc * dst, int x1, int y1, int x2, int y2, rtgui_color_t color)
{
    DrawLineFunc func;

    if (!dst) {
        return rt_kprintf("SDL_DrawLine(): Passed NULL destination surface");
    }

    func = _dc_calc_draw_line_func(dst->format);
    if (!func) {
        return rt_kprintf("SDL_DrawLine(): Unsupported surface format");
    }

    /* Perform clipping */
    /* FIXME: We don't actually want to clip, as it may change line slope */
    if (!SDL_IntersectRectAndLine(&dst->clip_rect, &x1, &y1, &x2, &y2)) {
        return 0;
    }

    func(dst, x1, y1, x2, y2, color, RT_FALSE);
    return 0;
}

int
rtgui_dc_draw_lines(struct rtgui_dc * dst, const struct rtgui_point * points, int count,
              rtgui_color_t color)
{
    int i;
    int x1, y1;
    int x2, y2;
    rt_bool_t draw_end;
    DrawLineFunc func;

    if (!dst) {
        return rt_kprintf("SDL_DrawLines(): Passed NULL destination surface");
    }

    func = _dc_calc_draw_line_func(dst->format);
    if (!func) {
        return rt_kprintf("SDL_DrawLines(): Unsupported surface format");
    }

    for (i = 1; i < count; ++i) {
        x1 = points[i-1].x;
        y1 = points[i-1].y;
        x2 = points[i].x;
        y2 = points[i].y;

        /* Perform clipping */
        /* FIXME: We don't actually want to clip, as it may change line slope */
        if (!SDL_IntersectRectAndLine(&dst->clip_rect, &x1, &y1, &x2, &y2)) {
            continue;
        }

        /* Draw the end if it was clipped */
        draw_end = (x2 != points[i].x || y2 != points[i].y);

        func(dst, x1, y1, x2, y2, color, draw_end);
    }
    if (points[0].x != points[count-1].x || points[0].y != points[count-1].y) {
        rtgui_draw_point(dst, points[count-1].x, points[count-1].y, color);
    }
    return 0;
}

int
rtgui_draw_point(struct rtgui_dc * dst, int x, int y, rtgui_color_t color)
{
    if (!dst) {
        return rt_kprintf("Passed NULL destination surface");
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        return rt_kprintf("SDL_DrawPoint(): Unsupported surface format");
    }

    /* Perform clipping */
    if (x < dst->clip_rect.x || y < dst->clip_rect.y ||
        x >= (dst->clip_rect.x + dst->clip_rect.w) ||
        y >= (dst->clip_rect.y + dst->clip_rect.h)) {
        return 0;
    }

    switch (dst->format->BytesPerPixel) {
    case 1:
        DRAW_FASTSETPIXELXY1(x, y);
        break;
    case 2:
        DRAW_FASTSETPIXELXY2(x, y);
        break;
    case 3:
        return SDL_Unsupported();
    case 4:
        DRAW_FASTSETPIXELXY4(x, y);
        break;
    }
    return 0;
}

int
rtgui_draw_points(struct rtgui_dc * dst, const struct rtgui_point * points, int count,
               rtgui_color_t color)
{
    int minx, miny;
    int maxx, maxy;
    int i;
    int x, y;

    if (!dst) {
        return rt_kprintf("Passed NULL destination surface");
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        return rt_kprintf("SDL_DrawPoints(): Unsupported surface format");
    }

    minx = dst->clip_rect.x;
    maxx = dst->clip_rect.x + dst->clip_rect.w - 1;
    miny = dst->clip_rect.y;
    maxy = dst->clip_rect.y + dst->clip_rect.h - 1;

    for (i = 0; i < count; ++i) {
        x = points[i].x;
        y = points[i].y;

        if (x < minx || x > maxx || y < miny || y > maxy) {
            continue;
        }

        switch (dst->format->BytesPerPixel) {
        case 1:
            DRAW_FASTSETPIXELXY1(x, y);
            break;
        case 2:
            DRAW_FASTSETPIXELXY2(x, y);
            break;
        case 3:
            return SDL_Unsupported();
        case 4:
            DRAW_FASTSETPIXELXY4(x, y);
            break;
        }
    }
    return 0;
}

static int
_dc_blend_point_rgb555(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode, rt_uint8_t r,
                      rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case RTGUI_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_RGB555(x, y);
        break;
    case RTGUI_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_RGB555(x, y);
        break;
    case RTGUI_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_RGB555(x, y);
        break;
    default:
        DRAW_SETPIXELXY_RGB555(x, y);
        break;
    }
    return 0;
}

static int
_dc_blend_point_rgb565(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode, rt_uint8_t r,
                      rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case RTGUI_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_RGB565(x, y);
        break;
    case RTGUI_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_RGB565(x, y);
        break;
    case RTGUI_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_RGB565(x, y);
        break;
    default:
        DRAW_SETPIXELXY_RGB565(x, y);
        break;
    }
    return 0;
}

static int
_dc_blend_point_rgb888(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode, rt_uint8_t r,
                      rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case RTGUI_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_RGB888(x, y);
        break;
    case RTGUI_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_RGB888(x, y);
        break;
    case RTGUI_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_RGB888(x, y);
        break;
    default:
        DRAW_SETPIXELXY_RGB888(x, y);
        break;
    }
    return 0;
}

static int
_dc_blend_point_argb888(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode,
                        rt_uint8_t r, rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    unsigned inva = 0xff - a;

    switch (blendMode) {
    case RTGUI_BLENDMODE_BLEND:
        DRAW_SETPIXELXY_BLEND_ARGB8888(x, y);
        break;
    case RTGUI_BLENDMODE_ADD:
        DRAW_SETPIXELXY_ADD_ARGB8888(x, y);
        break;
    case RTGUI_BLENDMODE_MOD:
        DRAW_SETPIXELXY_MOD_ARGB8888(x, y);
        break;
    default:
        DRAW_SETPIXELXY_ARGB8888(x, y);
        break;
    }
    return 0;
}

static int
_dc_blend_point_rgb(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode, rt_uint8_t r,
                   rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    SDL_PixelFormat *fmt = dst->format;
    unsigned inva = 0xff - a;

    switch (fmt->BytesPerPixel) {
    case 2:
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DRAW_SETPIXELXY2_BLEND_RGB(x, y);
            break;
        case RTGUI_BLENDMODE_ADD:
            DRAW_SETPIXELXY2_ADD_RGB(x, y);
            break;
        case RTGUI_BLENDMODE_MOD:
            DRAW_SETPIXELXY2_MOD_RGB(x, y);
            break;
        default:
            DRAW_SETPIXELXY2_RGB(x, y);
            break;
        }
        return 0;
    case 4:
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DRAW_SETPIXELXY4_BLEND_RGB(x, y);
            break;
        case RTGUI_BLENDMODE_ADD:
            DRAW_SETPIXELXY4_ADD_RGB(x, y);
            break;
        case RTGUI_BLENDMODE_MOD:
            DRAW_SETPIXELXY4_MOD_RGB(x, y);
            break;
        default:
            DRAW_SETPIXELXY4_RGB(x, y);
            break;
        }
        return 0;
    default:
        return SDL_Unsupported();
    }
}

static int
_dc_blend_point_rgba(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode, rt_uint8_t r,
                    rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    SDL_PixelFormat *fmt = dst->format;
    unsigned inva = 0xff - a;

    switch (fmt->BytesPerPixel) {
    case 4:
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DRAW_SETPIXELXY4_BLEND_RGBA(x, y);
            break;
        case RTGUI_BLENDMODE_ADD:
            DRAW_SETPIXELXY4_ADD_RGBA(x, y);
            break;
        case RTGUI_BLENDMODE_MOD:
            DRAW_SETPIXELXY4_MOD_RGBA(x, y);
            break;
        default:
            DRAW_SETPIXELXY4_RGBA(x, y);
            break;
        }
        return 0;
    default:
        return SDL_Unsupported();
    }
}

int
rtgui_dc_blend_point(struct rtgui_dc * dst, int x, int y, enum RTGUI_BLENDMODE blendMode, rt_uint8_t r,
               rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    if (!dst) {
        return rt_kprintf("Passed NULL destination surface");
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        return rt_kprintf("SDL_BlendPoint(): Unsupported surface format");
    }

    /* Perform clipping */
    if (x < dst->clip_rect.x || y < dst->clip_rect.y ||
        x >= (dst->clip_rect.x + dst->clip_rect.w) ||
        y >= (dst->clip_rect.y + dst->clip_rect.h)) {
        return 0;
    }

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(r, a);
        g = DRAW_MUL(g, a);
        b = DRAW_MUL(b, a);
    }

    switch (dst->format->BitsPerPixel) {
    case 15:
        switch (dst->format->Rmask) {
        case 0x7C00:
            return _dc_blend_point_rgb555(dst, x, y, blendMode, r, g, b, a);
        }
        break;
    case 16:
        switch (dst->format->Rmask) {
        case 0xF800:
            return _dc_blend_point_rgb565(dst, x, y, blendMode, r, g, b, a);
        }
        break;
    case 32:
        switch (dst->format->Rmask) {
        case 0x00FF0000:
            if (!dst->format->Amask) {
                return _dc_blend_point_rgb888(dst, x, y, blendMode, r, g, b,
                                             a);
            } else {
                return _dc_blend_point_argb888(dst, x, y, blendMode, r, g, b,
                                               a);
            }
            break;
        }
        break;
    default:
        break;
    }

    if (!dst->format->Amask) {
        return _dc_blend_point_rgb(dst, x, y, blendMode, r, g, b, a);
    } else {
        return _dc_blend_point_rgba(dst, x, y, blendMode, r, g, b, a);
    }
}

int
rtgui_dc_blend_points(struct rtgui_dc * dst, const SDL_Point * points, int count,
                enum RTGUI_BLENDMODE blendMode, rt_uint8_t r, rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    int minx, miny;
    int maxx, maxy;
    int i;
    int x, y;
    int (*func)(struct rtgui_dc * dst, int x, int y,
                enum RTGUI_BLENDMODE blendMode, rt_uint8_t r, rt_uint8_t g, rt_uint8_t b, rt_uint8_t a) = NULL;
    int status = 0;

    if (!dst) {
        return rt_kprintf("Passed NULL destination surface");
    }

    /* This function doesn't work on surfaces < 8 bpp */
    if (dst->format->BitsPerPixel < 8) {
        return rt_kprintf("SDL_BlendPoints(): Unsupported surface format");
    }

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(r, a);
        g = DRAW_MUL(g, a);
        b = DRAW_MUL(b, a);
    }

    /* FIXME: Does this function pointer slow things down significantly? */
    switch (dst->format->BitsPerPixel) {
    case 15:
        switch (dst->format->Rmask) {
        case 0x7C00:
            func = _dc_blend_point_rgb555;
            break;
        }
        break;
    case 16:
        switch (dst->format->Rmask) {
        case 0xF800:
            func = _dc_blend_point_rgb565;
            break;
        }
        break;
    case 32:
        switch (dst->format->Rmask) {
        case 0x00FF0000:
            if (!dst->format->Amask) {
                func = _dc_blend_point_rgb888;
            } else {
                func = _dc_blend_point_argb888;
            }
            break;
        }
        break;
    default:
        break;
    }

    if (!func) {
        if (!dst->format->Amask) {
            func = _dc_blend_point_rgb;
        } else {
            func = _dc_blend_point_rgba;
        }
    }

    minx = dst->clip_rect.x;
    maxx = dst->clip_rect.x + dst->clip_rect.w - 1;
    miny = dst->clip_rect.y;
    maxy = dst->clip_rect.y + dst->clip_rect.h - 1;

    for (i = 0; i < count; ++i) {
        x = points[i].x;
        y = points[i].y;

        if (x < minx || x > maxx || y < miny || y > maxy) {
            continue;
        }
        status = func(dst, x, y, blendMode, r, g, b, a);
    }
    return status;
}

static void
_dc_blend_line_rgb2(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                   enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                   rt_bool_t draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_BLEND_RGB, DRAW_SETPIXELXY2_BLEND_RGB,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_ADD_RGB, DRAW_SETPIXELXY2_ADD_RGB,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_MOD_RGB, DRAW_SETPIXELXY2_MOD_RGB,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY2_RGB, DRAW_SETPIXELXY2_BLEND_RGB,
                   draw_end);
            break;
        }
    }
}

static void
_dc_blend_line_rgb555(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                     enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                     rt_bool_t draw_end)
{
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB555, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB555, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB555, draw_end);
            break;
        default:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_RGB555, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB555, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB555, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB555, draw_end);
            break;
        default:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_RGB555, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB555, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB555, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB555, draw_end);
            break;
        default:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_RGB555, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_RGB555, DRAW_SETPIXELXY_BLEND_RGB555,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_RGB555, DRAW_SETPIXELXY_ADD_RGB555,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_RGB555, DRAW_SETPIXELXY_MOD_RGB555,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_RGB555, DRAW_SETPIXELXY_BLEND_RGB555,
                   draw_end);
            break;
        }
    }
}

static void
_dc_blend_line_rgb565(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                     enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                     rt_bool_t draw_end)
{
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB565, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB565, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB565, draw_end);
            break;
        default:
            HLINE(rt_uint16_t, DRAW_SETPIXEL_RGB565, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB565, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB565, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB565, draw_end);
            break;
        default:
            VLINE(rt_uint16_t, DRAW_SETPIXEL_RGB565, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_BLEND_RGB565, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_ADD_RGB565, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_MOD_RGB565, draw_end);
            break;
        default:
            DLINE(rt_uint16_t, DRAW_SETPIXEL_RGB565, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_RGB565, DRAW_SETPIXELXY_BLEND_RGB565,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_RGB565, DRAW_SETPIXELXY_ADD_RGB565,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_RGB565, DRAW_SETPIXELXY_MOD_RGB565,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_RGB565, DRAW_SETPIXELXY_BLEND_RGB565,
                   draw_end);
            break;
        }
    }
}

static void
_dc_blend_line_rgb4(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                   enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                   rt_bool_t draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGB, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGB, draw_end);
            break;
        default:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_RGB, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_BLEND_RGB, DRAW_SETPIXELXY4_BLEND_RGB,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_ADD_RGB, DRAW_SETPIXELXY4_ADD_RGB,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_MOD_RGB, DRAW_SETPIXELXY4_MOD_RGB,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_RGB, DRAW_SETPIXELXY4_BLEND_RGB,
                   draw_end);
            break;
        }
    }
}

static void
_dc_blend_line_rgba4(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                    enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                    rt_bool_t draw_end)
{
    const SDL_PixelFormat *fmt = dst->format;
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGBA, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGBA, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGBA, draw_end);
            break;
        default:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_RGBA, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGBA, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGBA, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGBA, draw_end);
            break;
        default:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_RGBA, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGBA, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGBA, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGBA, draw_end);
            break;
        default:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_RGBA, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_BLEND_RGBA, DRAW_SETPIXELXY4_BLEND_RGBA,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_ADD_RGBA, DRAW_SETPIXELXY4_ADD_RGBA,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_MOD_RGBA, DRAW_SETPIXELXY4_MOD_RGBA,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY4_RGBA, DRAW_SETPIXELXY4_BLEND_RGBA,
                   draw_end);
            break;
        }
    }
}

static void
_dc_blend_line_rgb888(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                     enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                     rt_bool_t draw_end)
{
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGB888, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGB888, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGB888, draw_end);
            break;
        default:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_RGB888, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGB888, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGB888, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGB888, draw_end);
            break;
        default:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_RGB888, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_RGB888, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_RGB888, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_RGB888, draw_end);
            break;
        default:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_RGB888, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_RGB888, DRAW_SETPIXELXY_BLEND_RGB888,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_RGB888, DRAW_SETPIXELXY_ADD_RGB888,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_RGB888, DRAW_SETPIXELXY_MOD_RGB888,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_RGB888, DRAW_SETPIXELXY_BLEND_RGB888,
                   draw_end);
            break;
        }
    }
}

static void
_dc_blend_line_argb8888(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
                       enum RTGUI_BLENDMODE blendMode, rt_uint8_t _r, rt_uint8_t _g, rt_uint8_t _b, rt_uint8_t _a,
                       rt_bool_t draw_end)
{
    unsigned r, g, b, a, inva;

    if (blendMode == RTGUI_BLENDMODE_BLEND || blendMode == RTGUI_BLENDMODE_ADD) {
        r = DRAW_MUL(_r, _a);
        g = DRAW_MUL(_g, _a);
        b = DRAW_MUL(_b, _a);
        a = _a;
    } else {
        r = _r;
        g = _g;
        b = _b;
        a = _a;
    }
    inva = (a ^ 0xff);

    if (y1 == y2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_ARGB8888, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_ARGB8888, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_ARGB8888, draw_end);
            break;
        default:
            HLINE(rt_uint32_t, DRAW_SETPIXEL_ARGB8888, draw_end);
            break;
        }
    } else if (x1 == x2) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_ARGB8888, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_ARGB8888, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_ARGB8888, draw_end);
            break;
        default:
            VLINE(rt_uint32_t, DRAW_SETPIXEL_ARGB8888, draw_end);
            break;
        }
    } else if (ABS(x1 - x2) == ABS(y1 - y2)) {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_BLEND_ARGB8888, draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_ADD_ARGB8888, draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_MOD_ARGB8888, draw_end);
            break;
        default:
            DLINE(rt_uint32_t, DRAW_SETPIXEL_ARGB8888, draw_end);
            break;
        }
    } else {
        switch (blendMode) {
        case RTGUI_BLENDMODE_BLEND:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_BLEND_ARGB8888, DRAW_SETPIXELXY_BLEND_ARGB8888,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_ADD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ADD_ARGB8888, DRAW_SETPIXELXY_ADD_ARGB8888,
                   draw_end);
            break;
        case RTGUI_BLENDMODE_MOD:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_MOD_ARGB8888, DRAW_SETPIXELXY_MOD_ARGB8888,
                   draw_end);
            break;
        default:
            AALINE(x1, y1, x2, y2,
                   DRAW_SETPIXELXY_ARGB8888, DRAW_SETPIXELXY_BLEND_ARGB8888,
                   draw_end);
            break;
        }
    }
}

typedef void (*BlendLineFunc) (struct rtgui_dc * dst,
                               int x1, int y1, int x2, int y2,
                               enum RTGUI_BLENDMODE blendMode,
                               rt_uint8_t r, rt_uint8_t g, rt_uint8_t b, rt_uint8_t a,
                               rt_bool_t draw_end);

static BlendLineFunc
_dc_calc_blend_line_func(const SDL_PixelFormat * fmt)
{
    switch (fmt->BytesPerPixel) {
    case 2:
        if (fmt->Rmask == 0x7C00) {
            return _dc_blend_line_rgb555;
        } else if (fmt->Rmask == 0xF800) {
            return _dc_blend_line_rgb565;
        } else {
            return _dc_blend_line_rgb2;
        }
        break;
    case 4:
        if (fmt->Rmask == 0x00FF0000) {
            if (fmt->Amask) {
                return _dc_blend_line_argb8888;
            } else {
                return _dc_blend_line_rgb888;
            }
        } else {
            if (fmt->Amask) {
                return _dc_blend_line_rgba4;
            } else {
                return _dc_blend_line_rgb4;
            }
        }
    }
    return NULL;
}

int
rtgui_dc_blend_line(struct rtgui_dc * dst, int x1, int y1, int x2, int y2,
              enum RTGUI_BLENDMODE blendMode, rt_uint8_t r, rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    BlendLineFunc func;

    if (!dst) {
        return rt_kprintf("SDL_BlendLine(): Passed NULL destination surface");
    }

    func = _dc_calc_blend_line_func(dst->format);
    if (!func) {
        return rt_kprintf("SDL_BlendLine(): Unsupported surface format");
    }

    /* Perform clipping */
    /* FIXME: We don't actually want to clip, as it may change line slope */
    if (!SDL_IntersectRectAndLine(&dst->clip_rect, &x1, &y1, &x2, &y2)) {
        return 0;
    }

    func(dst, x1, y1, x2, y2, blendMode, r, g, b, a, SDL_TRUE);
    return 0;
}

int
rtgui_dc_blend_lines(struct rtgui_dc * dst, const SDL_Point * points, int count,
               enum RTGUI_BLENDMODE blendMode, rt_uint8_t r, rt_uint8_t g, rt_uint8_t b, rt_uint8_t a)
{
    int i;
    int x1, y1;
    int x2, y2;
    rt_bool_t draw_end;
    BlendLineFunc func;

    if (!dst) {
        return rt_kprintf("SDL_BlendLines(): Passed NULL destination surface");
    }

    func = _dc_calc_blend_line_func(dst->format);
    if (!func) {
        return rt_kprintf("SDL_BlendLines(): Unsupported surface format");
    }

    for (i = 1; i < count; ++i) {
        x1 = points[i-1].x;
        y1 = points[i-1].y;
        x2 = points[i].x;
        y2 = points[i].y;

        /* Perform clipping */
        /* FIXME: We don't actually want to clip, as it may change line slope */
        if (!SDL_IntersectRectAndLine(&dst->clip_rect, &x1, &y1, &x2, &y2)) {
            continue;
        }

        /* Draw the end if it was clipped */
        draw_end = (x2 != points[i].x || y2 != points[i].y);

        func(dst, x1, y1, x2, y2, blendMode, r, g, b, a, draw_end);
    }
    if (points[0].x != points[count-1].x || points[0].y != points[count-1].y) {
        rtgui_dc_blend_point(dst, points[count-1].x, points[count-1].y,
                       blendMode, r, g, b, a);
    }
    return 0;
}

