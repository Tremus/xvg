#ifndef XVG_H
#define XVG_H

#include "sokol_gfx.h"
#include "xvg_shaders.glsl.h"
#include <linked_arena.h>
#include <stb_rect_pack.h>
#include <xhl/debug.h>
#include <xhl/maths.h>
#include <xhl/vector.h>

// TODO: combine line tiles with shapes shader
// TODO: increase max stroke width for line plots
// TODO: rounded rectangle scissoring for line plots. Will require sdRoundBox()
// TODO: support fallback fonts for missing glyphs

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XVG_SHAPES_CAPACITY
#define XVG_SHAPES_CAPACITY 8192
#endif
#ifndef XVG_LINE_BUFFER_CAPACITY
#define XVG_LINE_BUFFER_CAPACITY 16384
#endif
#ifndef XVG_LINE_TILE_CAPACITY
#define XVG_LINE_TILE_CAPACITY 128
#endif

#define XVG_ARRLEN(a) (sizeof(a) / sizeof(a[0]))

#ifndef NDEBUG
#define XVG_LABEL(txt) txt
#else
#define XVG_LABEL(...) 0
#endif

#if !defined(XVG_TEXT_SINGLECHANNEL) || !defined(XVG_TEXT_MULTICHANNEL)
#ifdef __APPLE__
#define XVG_TEXT_SINGLECHANNEL
#else
#define XVG_TEXT_MULTICHANNEL
#endif
#endif // XVG_TEXT_SINGLECHANNEL || XVG_TEXT_MULTICHANNEL

typedef enum XVGShapeType
{
    XVG_SHAPE_RECTANGLE, // With sharp corners
    XVG_SHAPE_ROUNDED_RECTANGLE_FILL,
    XVG_SHAPE_ROUNDED_RECTANGLE_STROKE,
    XVG_SHAPE_CIRCLE_FILL,
    XVG_SHAPE_CIRCLE_STROKE,
    XVG_SHAPE_TRIANGLE_FILL,
    XVG_SHAPE_TRIANGLE_STROKE,
    XVG_SHAPE_PIE_FILL,
    XVG_SHAPE_PIE_STROKE,
    XVG_SHAPE_ARC_ROUND_STROKE,
    XVG_SHAPE_ARC_BUTT_STROKE,
} XVGShapeType;

typedef enum XVGColourType
{
    XVG_COLOUR_SOLID,
    XVG_COLOUR_LINEAR_GRADEINT,
    XVG_COLOUR_RADIAL_GRADEINT,
    XVG_COLOUR_CONIC_GRADEINT,
    XVG_COLOUR_BOX_GRADEINT,
} XVGColourType;

typedef struct XVGFontSlot
{
    void*               kbts_font_ptr;
    struct FT_FaceRec_* ft_face;

    void*  data;
    size_t data_size;

    int space_advance;
    int owned;
} XVGFontSlot;

// Used to identify a unique glyph.
// TODO: support multiple fonts
typedef union XVGAtlasRectHeader
{
    struct
    {
        uint32_t glyph_index;
        // TODO: this could probably be packed into an integer. To support sizes like 12.25, multiply & divide by 4
        // This could make room for font ids in the header
        float font_size;
    };
    uint64_t data;
} XVGAtlasRectHeader;

typedef struct XVGAtlasRect
{
    union XVGAtlasRectHeader header;

    uint8_t x, y, w, h;

    int16_t advance_x;
    int16_t advance_y;

    int8_t bearing_x;
    int8_t bearing_y;

    sg_view img_view;
} XVGAtlasRect;

typedef struct XVGAtlas
{
    sg_view img_view;
    bool    dirty;
    bool    full;
} XVGAtlas;

typedef struct XVGFont
{
    int id;
} XVGFont;

typedef struct XVG
{
    struct FT_LibraryRec_* ft_lib;

#ifndef XVG_MAX_FONT_SLOTS
#define XVG_MAX_FONT_SLOTS 4
#endif // XVG_MAX_FONTS

    // Will default to the first font a user passes the library
    int         current_font_idx;
    XVGFontSlot fonts[XVG_MAX_FONT_SLOTS];

    XVGAtlas*     glyph_atlases;
    XVGAtlasRect* rects;

    struct
    {
        int            idx;
        stbrp_context  ctx;
        stbrp_node*    nodes;
        unsigned char* img_data;
    } current_atlas;

    // Text pipeline
    sg_pipeline text_pip;
    sg_buffer   text_sbo;
    sg_view     text_sbv;
    sg_sampler  text_smp;

#ifndef XVG_MAX_GLYPHS
#define XVG_MAX_GLYPHS 1024
#endif
    size_t     text_buffer_len;
    xvg_text_t text_buffer[XVG_MAX_GLYPHS];

    struct
    {
        sg_pipeline pip;
        sg_buffer   sbo;
        sg_view     sbv;
    } shapes;

    struct
    {
        sg_pipeline pip;
        sg_buffer   line_sbo; // normalised y values
        sg_view     line_sbv;
        sg_buffer   tile_sbo; // vertex pulling
        sg_view     tile_sbv;
    } lines;

    size_t          shapes_buffer_len;
    xvg_shape_t     shapes_buffer[XVG_SHAPES_CAPACITY];
    size_t          line_buffer_len;
    float           line_buffer[XVG_LINE_BUFFER_CAPACITY];
    size_t          tile_buffer_len;
    xvg_line_tile_t tile_buffer[XVG_LINE_TILE_CAPACITY];
} XVG;

void xvg_init(XVG*);
void xvg_deinit(XVG*);

void xvg_begin_frame(XVG*);
void xvg_end_frame(XVG* xvg, int window_width, int window_height);

// These methods should be considered 'private' and are likely to change in future, but in case you need it, here they
// are.

static void _xvg_add_shape(XVG* xvg, const xvg_shape_t* obj)
{
    if (xvg->shapes_buffer_len < XVG_ARRLEN(xvg->shapes_buffer))
        xvg->shapes_buffer[xvg->shapes_buffer_len++] = *obj;
}

static uint32_t _xvg_compress_sdf_data(unsigned sdf_type, unsigned grad_type, float feather, float stroke_width)
{
    xassert(stroke_width >= 0 && stroke_width < 16);
    xvecu compressed = {
        .r = sdf_type,
        .g = grad_type,
        .b = (uint8_t)(xm_minf(255, feather * 255)),
        .a = (uint8_t)(xm_minf(255, stroke_width * 16)),
    };
    return compressed.u32;
}

static uint32_t _xvg_compress_border_radius(float tr, float br, float tl, float bl)
{
    xvecu compressed = {
        .r = tr,
        .g = br,
        .b = tl,
        .a = bl,
    };
    return compressed.u32;
}

static uint32_t _xvg_packUnorm2x16(float low_f, float high_f)
{
    uint16_t low_u16  = (uint16_t)(xm_clampf(low_f, 0.0f, 1.0f) * 65535.0f);
    uint16_t high_u16 = (uint16_t)(xm_clampf(high_f, 0.0f, 1.0f) * 65535.0f);
    return low_u16 | (high_u16 << 16);
}

static uint32_t _xvg_packUnorm4x8(float x_f, float y_f, float z_f, float w_f)
{
    xvecu compressed = {
        .r = (uint8_t)(xm_clampf(x_f, 0.0f, 1.0f) * 255.0f),
        .g = (uint8_t)(xm_clampf(y_f, 0.0f, 1.0f) * 255.0f),
        .b = (uint8_t)(xm_clampf(z_f, 0.0f, 1.0f) * 255.0f),
        .a = (uint8_t)(xm_clampf(w_f, 0.0f, 1.0f) * 255.0f),
    };
    return compressed.u32;
}

static uint32_t _xvg_compress_arc_rotate_and_range(float rotate_radians, float range_radians)
{
    // remap [-PI, PI] to [0-1]
    float rotate_turns = rotate_radians * XM_1_TAUf;
    float range_turns  = range_radians * XM_1_TAUf;
    float rotate_norm  = rotate_turns - floorf(rotate_turns);
    float range_norm   = range_turns - floorf(range_turns);
    xassert(rotate_norm >= 0 && rotate_norm <= 1);
    xassert(range_norm >= 0 && range_norm <= 1);
    return _xvg_packUnorm2x16(rotate_norm, range_norm);
}

// Shapes

// If 'stroke_width' is 0, the circle will be filled
static void xvg_draw_circle(XVG* xvg, float cx, float cy, float radius_px, float stroke_width, uint32_t colour)
{
    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_CIRCLE_STROKE : XVG_SHAPE_CIRCLE_FILL;
    float        feather    = 2.0f / radius_px;
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {cx - radius_px, cy - radius_px},
            .bottomright = {cx + radius_px, cy + radius_px},
            .colour1     = colour,
            .sdf_data    = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
        });
}

static void xvg_draw_rounded_rectangle(
    XVG*     xvg,
    float    x,
    float    y,
    float    w,
    float    h,
    float    border_radius,
    float    stroke_width,
    uint32_t colour)
{
    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_ROUNDED_RECTANGLE_STROKE : XVG_SHAPE_ROUNDED_RECTANGLE_FILL;
    float        feather    = 4.0f / xm_minf(w, h);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {x, y},
            .bottomright = {x + w, y + h},
            .sdf_data    = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),

            .borderradius_arcpie =
                _xvg_compress_border_radius(border_radius, border_radius, border_radius, border_radius),
            .colour1 = colour,
        });
}

static void xvg_draw_rounded_rectangle_fill_linear(
    XVG*     xvg,
    float    x,
    float    y,
    float    w,
    float    h,
    float    border_radius,
    uint32_t col_stop_1,
    uint32_t col_stop_2,
    float    x_stop_1,
    float    y_stop_1,
    float    x_stop_2,
    float    y_stop_2)
{
    float feather = 4.0f / xm_minf(w, h);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {x, y},
            .bottomright = {x + w, y + h},
            .sdf_data =
                _xvg_compress_sdf_data(XVG_SHAPE_ROUNDED_RECTANGLE_FILL, XVG_COLOUR_LINEAR_GRADEINT, feather, 0),
            .borderradius_arcpie =
                _xvg_compress_border_radius(border_radius, border_radius, border_radius, border_radius),

            .colour1    = col_stop_1,
            .colour2    = col_stop_2,
            .gradient_a = {x_stop_1, y_stop_1},
            .gradient_b = {x_stop_2, y_stop_2},
        });
}

static void xvg_draw_rounded_rectangle_fill_radial(
    XVG*     xvg,
    float    x,
    float    y,
    float    w,
    float    h,
    float    border_radius,
    uint32_t col_stop_1,
    uint32_t col_stop_2,
    float    cx_stop_1,
    float    cy_stop_1,
    float    x_radius_stop_2,
    float    y_radius_stop_2)
{
    float feather = 4.0f / xm_minf(w, h);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {x, y},
            .bottomright = {x + w, y + h},
            .sdf_data =
                _xvg_compress_sdf_data(XVG_SHAPE_ROUNDED_RECTANGLE_FILL, XVG_COLOUR_RADIAL_GRADEINT, feather, 0),
            .borderradius_arcpie =
                _xvg_compress_border_radius(border_radius, border_radius, border_radius, border_radius),
            .colour1    = col_stop_1,
            .colour2    = col_stop_2,
            .gradient_a = {cx_stop_1, cy_stop_1},
            .gradient_b = {x_radius_stop_2, y_radius_stop_2},
        });
}

static void xvg_draw_rounded_rectangle_fill_conic(
    XVG*     xvg,
    float    x,
    float    y,
    float    w,
    float    h,
    float    border_radius,
    uint32_t col_stop_1,
    uint32_t col_stop_2,
    float    radians_stop_1,
    float    radians_stop_2)
{
    float feather = 4.0f / xm_minf(w, h);

    float range = radians_stop_2 - radians_stop_1;
    float a     = XM_PIf + radians_stop_1;
    float b     = range * XM_1_TAUf;

    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {x, y},
            .bottomright = {x + w, y + h},
            .sdf_data = _xvg_compress_sdf_data(XVG_SHAPE_ROUNDED_RECTANGLE_FILL, XVG_COLOUR_CONIC_GRADEINT, feather, 0),
            .borderradius_arcpie =
                _xvg_compress_border_radius(border_radius, border_radius, border_radius, border_radius),

            .colour1 = col_stop_1,
            .colour2 = col_stop_2,

            .gradient_a = {a, a},
            .gradient_b = {b, b},
        });
}

static void xvg_draw_rounded_rectangle_fill_inner_shadow(
    XVG*     xvg,
    float    x,
    float    y,
    float    w,
    float    h,
    float    border_radius,
    uint32_t col_stop_outer,
    uint32_t col_stop_inner,
    float    x_translate,
    float    y_translate,
    float    blur_radius)
{
    float feather = 4.0f / xm_minf(w, h);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {x, y},
            .bottomright = {x + w, y + h},

            .sdf_data = _xvg_compress_sdf_data(XVG_SHAPE_ROUNDED_RECTANGLE_FILL, XVG_COLOUR_BOX_GRADEINT, feather, 0),
            .borderradius_arcpie =
                _xvg_compress_border_radius(border_radius, border_radius, border_radius, border_radius),

            .colour1    = col_stop_outer,
            .colour2    = col_stop_inner,
            .gradient_a = {x_translate, y_translate},
            .gradient_b = {blur_radius, blur_radius},
        });
}

static void xvg_draw_triangle(
    XVG*     xvg,
    float    x,
    float    y,
    float    w,
    float    h,
    float    rotate_radians,
    float    stroke_width,
    uint32_t colour)
{
    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_TRIANGLE_STROKE : XVG_SHAPE_TRIANGLE_FILL;
    float        feather    = 4.0f / xm_minf(w, h);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft             = {x, y},
            .bottomright         = {x + w, y + h},
            .sdf_data            = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
            .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(rotate_radians, 0),
            .colour1             = colour,
        });
}

static void xvg_draw_pie(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    start_radians,
    float    end_radians,
    float    stroke_width,
    uint32_t colour)
{
    XVGShapeType shape_type   = stroke_width > 0 ? XVG_SHAPE_PIE_STROKE : XVG_SHAPE_PIE_FILL;
    float        feather      = 2.0f / radius_px;
    float        angle_range  = end_radians - start_radians;
    float        angle_rotate = (end_radians + start_radians);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft             = {cx - radius_px, cy - radius_px},
            .bottomright         = {cx + radius_px, cy + radius_px},
            .sdf_data            = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
            .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(angle_rotate * 0.5f, angle_range * 0.5f),
            .colour1             = colour,
        });
}

static void xvg_draw_arc(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    start_radians,
    float    end_radians,
    float    stroke_width,
    bool     butt,
    uint32_t colour)
{
    float feather      = 2.0f / radius_px;
    float angle_range  = end_radians - start_radians;
    float angle_rotate = (end_radians + start_radians);
    _xvg_add_shape(
        xvg,
        &(xvg_shape_t){
            .topleft     = {cx - radius_px, cy - radius_px},
            .bottomright = {cx + radius_px, cy + radius_px},
            .sdf_data    = _xvg_compress_sdf_data(
                butt ? XVG_SHAPE_ARC_BUTT_STROKE : XVG_SHAPE_ARC_ROUND_STROKE,
                0,
                feather,
                stroke_width),
            .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(angle_rotate * 0.5f, angle_range * 0.5f),
            .colour1             = colour,
        });
}

// 'data' is expected to be an array of 'width' length
// 'data' is expected to contain normalised values where 0 == (y + height), and 1 == y
// 'data' is allowed to go beyond [0-1], however you should consider using sg_apply_scissor_rect()
// 'stroke_width' is limited to the range [1-2]
void xvg_draw_line_plot(
    XVG*         xvg,
    int          x,
    int          y,
    int          width,
    int          height,
    const float* data,
    float        stroke_width,
    uint32_t     colour);

// FONTS
// These functions return font IDs. 0 is considered to be invalid

// 'font_filepath' is expected to be UTF8
// Windows paths are expected to use backlashes ('\'), but forward probably wortk fine
// This library takes ownership of the memory and frees it
XVGFont xvg_add_font_from_path(XVG* xvg, const char* font_filepath);
// Same as above, except takes a file buffer, and now you're responsible for freeing the memory
// Note this memory must remain valid until you call xvg_deinit()
XVGFont xvg_add_font_from_memory(XVG* xvg, const void* font_data, size_t font_datalen);

// Sets active font to draw and create layouts with
void xvg_set_font(XVG* xvg, XVGFont);

#ifdef __cplusplus
}
#endif

#endif // XVG_H

#ifdef XVG_IMPL
#undef XVG_IMPL
#include <string.h>
#include <xhl/files.h>

#if defined(XVG_TEXT_SINGLECHANNEL) || defined(XVG_TEXT_MULTICHANNEL)
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#endif

void xvg_draw_line_plot(
    XVG*         xvg,
    int          x,
    int          y,
    int          width,
    int          height,
    const float* data,
    float        stroke_width,
    uint32_t     colour)
{
    xassert(xvg->tile_buffer_len < XVG_ARRLEN(xvg->tile_buffer));
    if (xvg->tile_buffer_len >= XVG_ARRLEN(xvg->tile_buffer))
        return;

    int end_idx = xvg->line_buffer_len + width;
    if (end_idx > XVG_ARRLEN(xvg->line_buffer))
        end_idx = XVG_ARRLEN(xvg->line_buffer);

    const int N = end_idx - xvg->line_buffer_len;
    xassert(N > 0);
    if (N <= 0)
        return;

    int backingScaleFactor = 1;
    if (backingScaleFactor == 1)
    {
        memcpy(xvg->line_buffer + xvg->line_buffer_len, data, N * sizeof(xvg->line_buffer[0]));
    }
    else
    {
        // TODO: handle retina screens. Linear interpolation between points should be fine
        xassert(false);
    }

    xassert(end_idx >= 1);
    xvg->tile_buffer[xvg->tile_buffer_len++] = (xvg_line_tile_t){
        .topleft          = {x, y},
        .bottomright      = {x + width, y + height},
        .buffer_begin_idx = xvg->line_buffer_len,
        .buffer_end_idx   = end_idx,
        .stroke_width     = stroke_width,
        .colour           = colour,
    };

    xvg->line_buffer_len += N;
    xassert(xvg->line_buffer_len <= XVG_ARRLEN(xvg->line_buffer));
}

XVGFont _xvg_add_font_from_memory_impl(XVG* xvg, const void* data, size_t datalen, bool owned)
{
    for (int i = 0; i < XVG_ARRLEN(xvg->fonts); i++)
    {
        XVGFontSlot* sl = xvg->fonts + i;
        if (sl->ft_face == NULL)
        {
            // TODO: pass data to kbts
            int err = FT_New_Memory_Face(xvg->ft_lib, data, datalen, 0, &sl->ft_face);
            xassert(err == 0);
            if (err != 0)
                break;

            sl->data      = (void*)data;
            sl->data_size = datalen;
            sl->owned     = owned;

            FT_UInt  space_char = 32;
            FT_Fixed advance    = 0;
            err                 = FT_Get_Advance(sl->ft_face, space_char, FT_LOAD_NO_SCALE, &advance);
            xassert(err == 0);
            sl->space_advance = advance;

            return (XVGFont){i + 1};
        }
    }
    return (XVGFont){0};
}

XVGFont xvg_add_font_from_path(XVG* xvg, const char* path)
{
    void*  data    = NULL;
    size_t datalen = 0;
    bool   owned   = true;
    bool   ok      = xfiles_read(path, &data, &datalen);
    xassert(ok);
    if (!ok)
        return (XVGFont){0};
    return _xvg_add_font_from_memory_impl(xvg, data, datalen, owned);
}

XVGFont xvg_add_font_from_memory(XVG* xvg, const void* data, size_t datalen)
{
    bool owned = false;
    return _xvg_add_font_from_memory_impl(xvg, data, datalen, owned);
}

void xvg_set_font(XVG* xvg, XVGFont font)
{
    int next_font_idx = font.id - 1;
    if (next_font_idx < 0)
        next_font_idx = 0;
    if (next_font_idx >= XVG_ARRLEN(xvg->fonts))
        next_font_idx = XVG_ARRLEN(xvg->fonts) - 1;
    xvg->current_font_idx = next_font_idx;
}

void xvg_init(XVG* xvg)
{
    int ft_err = FT_Init_FreeType(&xvg->ft_lib);
    xassert(ft_err == 0);

    // Shader stuff
    {
        static const sg_color_target_state BLEND_DEFAULT = {
            .write_mask = SG_COLORMASK_RGBA,
            .blend      = {
                     .enabled          = true,
                     .src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA,
                     .src_factor_alpha = SG_BLENDFACTOR_ONE,
                     .dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                     .dst_factor_alpha = SG_BLENDFACTOR_ONE,
            }};

        xvg->shapes.pip =
            sg_make_pipeline(&(sg_pipeline_desc){.shader = sg_make_shader(_xvg_shapes_shader_desc(sg_query_backend())),
                                                 .colors[0] = BLEND_DEFAULT,
                                                 .label     = XVG_LABEL("xvg-shapes-pipeline")});

        xvg->shapes.sbo = sg_make_buffer(&(sg_buffer_desc){
            .usage.storage_buffer = true,
            .usage.stream_update  = true,
            .size                 = sizeof(xvg->shapes_buffer),
            .label                = XVG_LABEL("xvg-shapes"),
        });
        xvg->shapes.sbv = sg_make_view(&(sg_view_desc){
            .storage_buffer = xvg->shapes.sbo,
        });

        xvg->lines.pip =
            sg_make_pipeline(&(sg_pipeline_desc){.shader = sg_make_shader(_xvg_lines_shader_desc(sg_query_backend())),
                                                 .colors[0] = BLEND_DEFAULT,
                                                 .label     = XVG_LABEL("xvg-line-pipeline")});

        xvg->lines.line_sbo = sg_make_buffer(&(sg_buffer_desc){
            .usage.storage_buffer = true,
            .usage.stream_update  = true,
            .size                 = sizeof(xvg->line_buffer),
            .label                = XVG_LABEL("xvg-line-buffer"),
        });
        xvg->lines.line_sbv = sg_make_view(&(sg_view_desc){.storage_buffer = xvg->lines.line_sbo});

        xvg->lines.tile_sbo = sg_make_buffer(&(sg_buffer_desc){
            .usage.storage_buffer = true,
            .usage.stream_update  = true,
            .size                 = sizeof(xvg->tile_buffer),
            .label                = XVG_LABEL("xvg-tile-buffer"),
        });
        xvg->lines.tile_sbv = sg_make_view(&(sg_view_desc){.storage_buffer = xvg->lines.tile_sbo});
    }
}

void xvg_deinit(XVG* xvg)
{
    for (int i = 0; i < XVG_ARRLEN(xvg->fonts); i++)
    {
        XVGFontSlot* sl = &xvg->fonts[i];
        if (sl->data && sl->owned)
        {
            XFILES_FREE(sl->data);
        }
        // TODO: free kbts here?
        if (sl->ft_face)
            FT_Done_Face(sl->ft_face);
    }
    FT_Done_FreeType(xvg->ft_lib);
}

void xvg_begin_frame(XVG* xvg)
{
    xvg->shapes_buffer_len = 0;
    xvg->line_buffer_len   = 0;
    xvg->tile_buffer_len   = 0;
}

void xvg_end_frame(XVG* xvg, int window_width, int window_height)
{
    // Upload data
    if (xvg->shapes_buffer_len)
    {
        size_t   num_bytes = sizeof(xvg->shapes_buffer[0]) * xvg->shapes_buffer_len;
        sg_range range     = {.ptr = xvg->shapes_buffer, .size = num_bytes};
        sg_update_buffer(xvg->shapes.sbo, &range);
    }
    if (xvg->line_buffer_len)
    {
        size_t   num_bytes = sizeof(xvg->line_buffer[0]) * xvg->line_buffer_len;
        sg_range range     = {.ptr = xvg->line_buffer, .size = num_bytes};
        sg_update_buffer(xvg->lines.line_sbo, &range);
    }
    if (xvg->tile_buffer_len)
    {
        size_t   num_bytes = sizeof(xvg->tile_buffer[0]) * xvg->tile_buffer_len;
        sg_range range     = {.ptr = xvg->tile_buffer, .size = num_bytes};
        sg_update_buffer(xvg->lines.tile_sbo, &range);
    }

    // Draw shapes

    if (xvg->shapes_buffer_len)
    {
        sg_apply_pipeline(xvg->shapes.pip);

        sg_apply_bindings(&(sg_bindings){
            .views[VIEW_vs_xvg_tiles_buffer] = xvg->shapes.sbv,
        });
        vs_xvg_shapes_uniforms_t uniforms = {
            .u_size                  = {(float)window_width, (float)window_height},
            .u_storage_buffer_offset = 0,
        };
        sg_apply_uniforms(UB_vs_xvg_shapes_uniforms, &SG_RANGE(uniforms));
        sg_draw(0, xvg->shapes_buffer_len * 6, 1);
    }

    if (xvg->tile_buffer_len)
    {
        sg_apply_pipeline(xvg->lines.pip);
        sg_apply_bindings(&(sg_bindings){
            .views[VIEW_vs_xvg_tiles_buffer] = xvg->lines.tile_sbv,
            .views[VIEW_fs_xvg_line_buffer]  = xvg->lines.line_sbv,
        });

        vs_xvg_tiles_uniforms_t uniforms = {
            .u_size = {(float)window_width, (float)window_height},
        };
        sg_apply_uniforms(UB_vs_xvg_tiles_uniforms, &SG_RANGE(uniforms));

        sg_draw(0, 6 * xvg->tile_buffer_len, 1);
    }
}

#endif // XVG_IMPL