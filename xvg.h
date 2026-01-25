#ifndef XVG_H
#define XVG_H

#include "sokol_gfx.h"
#include "xvg_shaders.glsl.h"
#include <linked_arena.h>
#include <stb_rect_pack.h>
#include <xhl/debug.h>
#include <xhl/maths.h>
#include <xhl/vector.h>

/*
// TODO: combine line tiles with shapes shader
// TODO: increase max stroke width for line plots
// TODO: rounded rectangle scissoring for line plots. Will require sdRoundBox()
// TODO: support fallback fonts for missing glyphs
// TODO: support intelligent batching of text when using multiple atlases
// TODO: make a xvg_make_gradient() function to help reduce the number of draw_rect_functions()
*/

// #ifdef __cplusplus
// extern "C" {
// #endif

#define XVG_ARRLEN(a) (sizeof(a) / sizeof(a[0]))

#ifndef NDEBUG
#define XVG_LABEL(txt) txt
#else
#define XVG_LABEL(...) 0
#endif

#ifndef XVG_ASSERT
#include <assert.h>
#define XVG_ASSERT(cond) assert(cond)
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
    XVG_COLOUR_LINEAR_GRADIENT,
    XVG_COLOUR_RADIAL_GRADIENT,
    XVG_COLOUR_CONIC_GRADIENT,
    XVG_COLOUR_BOX_GRADIENT,
} XVGColourType;

typedef enum XVGAlign
{
    // Horizontal align
    XVG_ALIGN_LEFT     = 0,      // Default, align text horizontally to left.
    XVG_ALIGN_CENTRE_X = 1 << 0, // Align text horizontally to centre.
    XVG_ALIGN_RIGHT    = 1 << 1, // Align text horizontally to right.
                                 // Vertical align

    XVG_ALIGN_BASELINE = 0,      // Default, align text vertically to baseline.
    XVG_ALIGN_TOP      = 1 << 2, // Align text vertically to top.
    XVG_ALIGN_CENTRE_Y = 1 << 3, // Align text vertically to middle.
    XVG_ALIGN_BOTTOM   = 1 << 4, // Align text vertically to bottom.

    // The following use ascender and descender information from the glyphs in the top & bottom rows to calculate a
    // centre alignment
    XVG_ALIGN_TOP_TIGHT      = 1 << 5,
    XVG_ALIGN_CENTRE_Y_TIGHT = 1 << 6,
    XVG_ALIGN_BOTTOM_TIGHT   = 1 << 7,

    XVG_ALIGN_TL = (XVG_ALIGN_TOP | XVG_ALIGN_LEFT),
    XVG_ALIGN_TC = (XVG_ALIGN_TOP | XVG_ALIGN_CENTRE_X),
    XVG_ALIGN_TR = (XVG_ALIGN_TOP | XVG_ALIGN_RIGHT),

    XVG_ALIGN_CL = (XVG_ALIGN_CENTRE_Y | XVG_ALIGN_LEFT),
    XVG_ALIGN_CC = (XVG_ALIGN_CENTRE_Y | XVG_ALIGN_CENTRE_X),
    XVG_ALIGN_CR = (XVG_ALIGN_CENTRE_Y | XVG_ALIGN_RIGHT),

    XVG_ALIGN_BL = (XVG_ALIGN_BOTTOM | XVG_ALIGN_LEFT),
    XVG_ALIGN_BC = (XVG_ALIGN_BOTTOM | XVG_ALIGN_CENTRE_X),
    XVG_ALIGN_BR = (XVG_ALIGN_BOTTOM | XVG_ALIGN_RIGHT),

    XVG_ALIGN_TL_TIGHT = (XVG_ALIGN_TOP_TIGHT | XVG_ALIGN_LEFT),
    XVG_ALIGN_TC_TIGHT = (XVG_ALIGN_TOP_TIGHT | XVG_ALIGN_CENTRE_X),
    XVG_ALIGN_TR_TIGHT = (XVG_ALIGN_TOP_TIGHT | XVG_ALIGN_RIGHT),

    XVG_ALIGN_CL_TIGHT = (XVG_ALIGN_CENTRE_Y_TIGHT | XVG_ALIGN_LEFT),
    XVG_ALIGN_CC_TIGHT = (XVG_ALIGN_CENTRE_Y_TIGHT | XVG_ALIGN_CENTRE_X),
    XVG_ALIGN_CR_TIGHT = (XVG_ALIGN_CENTRE_Y_TIGHT | XVG_ALIGN_RIGHT),

    XVG_ALIGN_BL_TIGHT = (XVG_ALIGN_BOTTOM_TIGHT | XVG_ALIGN_LEFT),
    XVG_ALIGN_BC_TIGHT = (XVG_ALIGN_BOTTOM_TIGHT | XVG_ALIGN_CENTRE_X),
    XVG_ALIGN_BR_TIGHT = (XVG_ALIGN_BOTTOM_TIGHT | XVG_ALIGN_RIGHT),
} XVGAlign;

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

typedef struct XVGGlyphLayout
{
    XVGAtlasRect rect;
    int          x, y;
} XVGGlyphLayout;

typedef struct XVGTextLayoutRow
{
    // Indexes into glyphs array in struct XVGTextLayout below
    short begin_idx, end_idx;
    short ymin, ymax;
    short xmin, xmax;
    int   cursor_y_px;
} XVGTextLayoutRow;

// Glyphs are shaped and aligned from left > right along the baseline of row one
// Alignment and translation on a screen should be applied at draw time
// This design is to help reduce the amount of work kbts has do to, and avoid doing multiple runs across the text
// Hopefully there is enough data here to make this possible.
// Handling multiple languages is an aftertought here and this design may prove to be bad.
typedef struct XVGTextLayout
{
    // WARNING: this values are scaled accorting to xvg->backingScaleFactor
    // You are free to use them, however you may need to remember to divide by backingScaleFactor to your work in your
    // own pixel space
    short ascender, descender;
    short line_height;
    short xmax; // The right edge of the longest (in pixels) row

    int total_height;
    int total_height_tight;

    int num_rows, cap_rows;
    int num_glyphs, cap_glyphs;
    int offset_rows;
    int offset_glyphs;
} XVGTextLayout;

static XVGTextLayoutRow* xvg_layout_get_rows(const XVGTextLayout* l)
{
    return (XVGTextLayoutRow*)((char*)l + l->offset_rows);
}
static void xvg_layout_set_rows(XVGTextLayout* l, XVGTextLayoutRow* r) { l->offset_rows = ((char*)r - (char*)l); }
static XVGGlyphLayout* xvg_layout_get_glyphs(const XVGTextLayout* l)
{
    return (XVGGlyphLayout*)((char*)l + l->offset_glyphs);
}
static void xvg_layout_set_glyphs(XVGTextLayout* l, XVGGlyphLayout* g) { l->offset_glyphs = ((char*)g - (char*)l); }

typedef struct XVG
{
    LinkedArena* arena;

    LinkedArena* frame_arena;

    float backingScaleFactor;

    sg_sampler smp_linear;
    sg_sampler smp_nearest_neighbour;

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

    // Text pipeline
    struct
    {
        sg_pipeline pip;
        sg_buffer   sbo;
        sg_view     sbv;
        sg_sampler  smp;

        struct
        {
            int            idx;
            stbrp_context  ctx;
            stbrp_node*    nodes;
            unsigned char* img_data;
        } current_atlas;

        struct FT_LibraryRec_* ft_lib;

#ifndef XVG_MAX_FONT_SLOTS
#define XVG_MAX_FONT_SLOTS 4
#endif // XVG_MAX_FONT_SLOTS

        // Will default to the first font a user passes the library
        int         current_font_idx;
        XVGFontSlot fonts[XVG_MAX_FONT_SLOTS];

#ifndef XVG_GLYPH_ATLAS_SLOTS
#define XVG_GLYPH_ATLAS_SLOTS 1
// #define XVG_GLYPH_ATLAS_SLOTS 8
#endif // XVG_GLYPH_ATLAS_SLOTS
        XVGAtlas atlases[XVG_GLYPH_ATLAS_SLOTS];

        XVGAtlasRect* rects;
    } text;

#ifndef XVG_SHAPES_CAPACITY
#define XVG_SHAPES_CAPACITY 1024
#endif
    size_t      shapes_buffer_len;
    xvg_shape_t shapes_buffer[XVG_SHAPES_CAPACITY];

#ifndef XVG_LINE_BUFFER_CAPACITY
#define XVG_LINE_BUFFER_CAPACITY 8192
#endif
    size_t             line_buffer_len;
    xvg_line_segment_t line_buffer[XVG_LINE_BUFFER_CAPACITY];

#ifndef XVG_LINE_TILE_CAPACITY
#define XVG_LINE_TILE_CAPACITY 32
#endif
    size_t          tile_buffer_len;
    xvg_line_tile_t tile_buffer[XVG_LINE_TILE_CAPACITY];

#ifndef XVG_MAX_GLYPHS
#define XVG_MAX_GLYPHS 1024
#endif
    size_t     text_buffer_len;
    xvg_text_t text_buffer[XVG_MAX_GLYPHS];
} XVG;

void xvg_init(XVG*);
void xvg_deinit(XVG*);

void xvg_begin_frame(XVG*);
void xvg_end_frame(XVG* xvg, int window_width, int window_height);

// Shapes
// Unlike canvas style APIs, there are no 'fill' and 'stroke' commands. If 'stroke_width' is 0 the shape is implicitly
// filled. Values are in pixels. Stroking is done on the INSIDE of the shape, so you always get accurate widths
// 'radius' is in pixels
// 'angle' and 'rotate' is in radians [-PI, PI]
// Colours are in the highly unintuitive but highly readable ABGR format (0xffff007f is yellow, 50% opacity). This makes
// it easy to copy paste hex codes from apps like Figma, Photoshop, or web browsers. This library swizzles to RGBA

typedef struct XVGGradient
{
    XVGColourType type;
    uint32_t      colour1;
    uint32_t      colour2;
    float         gradient_a[2];
    float         gradient_b[2];
} XVGGradient;

XVGGradient xvg_make_linear_gradient(uint32_t col_1, uint32_t col_2, float x_1, float y_1, float x_2, float y_2);

XVGGradient
xvg_make_radial_gradient(uint32_t col_inner, uint32_t col_outer, float cx, float cy, float x_radius, float y_radius);

XVGGradient xvg_make_conic_gradient(uint32_t col_1, uint32_t col_2, float angle_1, float angle_2);

XVGGradient
xvg_make_inner_shadow(uint32_t col_outer, uint32_t col_inner, float x_translate, float y_translate, float blur_radius);

void xvg_draw_rectangle(XVG* xvg, float x, float y, float w, float h, float br, float stroke_px, uint32_t col);

void xvg_draw_rectangle_with_gradient(XVG* xvg, float x, float y, float w, float h, float br, XVGGradient grad);

void xvg_draw_circle(XVG* xvg, float cx, float cy, float radius_px, float stroke_width, uint32_t col);

// Equilateral triangle
void xvg_draw_triangle(XVG* xvg, float x, float y, float w, float h, float rotate_rad, float stroke_px, uint32_t col);

void xvg_draw_pie(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    angle_start,
    float    angle_end,
    float    stroke_px,
    uint32_t col);

void xvg_draw_arc(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    angle_start,
    float    angle_end,
    float    stroke_px,
    bool     butt,
    uint32_t col);

// 'data' is expected to be an array of 'width' length
// 'data' is expected to contain normalised values where 0 == (y + height), and 1 == y
// 'data' is allowed to go beyond [0-1], however you should consider using sg_apply_scissor_rect()
// 'stroke_width' is limited to the range [1-2]
void xvg_draw_line_plot(XVG* xvg, int x, int y, int w, int h, const float* data, float stroke_px, uint32_t col);

// FONTS
// These functions return font IDs. 0 is considered to be invalid
// By default the first font you add will be used

// 'font_filepath' is expected to be UTF8
// Windows paths are expected to use backlashes ('\'), but forward probably wortk fine
// This library takes ownership of the memory and frees it
XVGFont xvg_add_font_from_path(XVG* xvg, const char* font_filepath);
// Same as above, except takes a file buffer, and now you're responsible for freeing the memory
// Note this memory must remain valid until you call xvg_deinit()
XVGFont xvg_add_font_from_memory(XVG* xvg, const void* font_data, size_t font_datalen);

// Sets active font to draw and create layouts with
void xvg_set_font(XVG* xvg, XVGFont);

void xvg_draw_text(
    XVG*        xvg,
    float       x,
    float       y,
    const char* text_start,
    const char* text_end,
    float       font_size,
    XVGAlign    align,
    uint32_t    col);

// #ifdef __cplusplus
// }
// #endif

#endif // XVG_H

#ifdef XVG_IMPL
#undef XVG_IMPL
#include <string.h>
#include <utf8.h>
#include <xhl/array.h>
#include <xhl/files.h>

#if defined(XVG_TEXT_SINGLECHANNEL) || defined(XVG_TEXT_MULTICHANNEL)
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H
#endif

#if !defined(XVG_MALLOC) || !defined(XVG_REALLOC) || !defined(XVG_FREE)
#include <stdlib.h>
#define XVG_MALLOC(sz)       malloc(sz)
#define XVG_REALLOC(ptr, sz) realloc(ptr, sz)
#define XVG_FREE(ptr)        free(ptr)
#endif

uint32_t _xvg_compress_sdf_data(unsigned sdf_type, unsigned grad_type, float feather, float stroke_width)
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

uint32_t _xvg_compress_border_radius(float tr, float br, float tl, float bl)
{
    xvecu compressed = {
        .r = tr,
        .g = br,
        .b = tl,
        .a = bl,
    };
    return compressed.u32;
}

uint32_t _xvg_packUnorm2x16(float low_f, float high_f)
{
    uint16_t low_u16  = (uint16_t)(xm_clampf(low_f, 0.0f, 1.0f) * 65535.0f);
    uint16_t high_u16 = (uint16_t)(xm_clampf(high_f, 0.0f, 1.0f) * 65535.0f);
    return low_u16 | (high_u16 << 16);
}

uint32_t _xvg_packUnorm4x8(float x_f, float y_f, float z_f, float w_f)
{
    xvecu compressed = {
        .r = (uint8_t)(xm_clampf(x_f, 0.0f, 1.0f) * 255.0f),
        .g = (uint8_t)(xm_clampf(y_f, 0.0f, 1.0f) * 255.0f),
        .b = (uint8_t)(xm_clampf(z_f, 0.0f, 1.0f) * 255.0f),
        .a = (uint8_t)(xm_clampf(w_f, 0.0f, 1.0f) * 255.0f),
    };
    return compressed.u32;
}

uint32_t _xvg_compress_arc_rotate_and_range(float rotate_radians, float range_radians)
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

uint32_t _xvg_pack_xy_coord(int x, int y)
{
    union
    {
        struct
        {
            int16_t low;
            int16_t high;
        };
        uint32_t u32;
    } v;
    v.low   = x;
    v.high  = y;
    v.low  += 32767;
    v.high += 32767;
    return v.u32;
}

// ███████╗██╗  ██╗ █████╗ ██████╗ ███████╗███████╗
// ██╔════╝██║  ██║██╔══██╗██╔══██╗██╔════╝██╔════╝
// ███████╗███████║███████║██████╔╝█████╗  ███████╗
// ╚════██║██╔══██║██╔══██║██╔═══╝ ██╔══╝  ╚════██║
// ███████║██║  ██║██║  ██║██║     ███████╗███████║
// ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚══════╝╚══════╝

xvg_shape_t* _xvg_get_shape(XVG* xvg)
{
    // Branchless
    static xvg_shape_t stub;

    xvg_shape_t* ret =
        xvg->shapes_buffer_len < XVG_ARRLEN(xvg->shapes_buffer) ? &xvg->shapes_buffer[xvg->shapes_buffer_len] : &stub;

    xvg->shapes_buffer_len++;
    if (xvg->shapes_buffer_len > XVG_ARRLEN(xvg->shapes_buffer))
        xvg->shapes_buffer_len = XVG_ARRLEN(xvg->shapes_buffer);
    return ret;
}

XVGGradient xvg_make_linear_gradient(
    uint32_t col_stop_1,
    uint32_t col_stop_2,
    float    x_stop_1,
    float    y_stop_1,
    float    x_stop_2,
    float    y_stop_2)
{
    return (XVGGradient){
        .type       = XVG_COLOUR_LINEAR_GRADIENT,
        .colour1    = col_stop_1,
        .colour2    = col_stop_2,
        .gradient_a = {x_stop_1, y_stop_1},
        .gradient_b = {x_stop_2, y_stop_2},
    };
}

XVGGradient xvg_make_radial_gradient(
    uint32_t col_stop_1,
    uint32_t col_stop_2,
    float    cx_stop_1,
    float    cy_stop_1,
    float    x_radius_stop_2,
    float    y_radius_stop_2)
{
    return (XVGGradient){
        .type       = XVG_COLOUR_RADIAL_GRADIENT,
        .colour1    = col_stop_1,
        .colour2    = col_stop_2,
        .gradient_a = {cx_stop_1, cy_stop_1},
        .gradient_b = {x_radius_stop_2, y_radius_stop_2},
    };
}

XVGGradient
xvg_make_conic_gradient(uint32_t col_stop_1, uint32_t col_stop_2, float radians_stop_1, float radians_stop_2)
{
    float range = radians_stop_2 - radians_stop_1;
    float a     = XM_PIf + radians_stop_1;
    float b     = range * XM_1_TAUf;
    return (XVGGradient){
        .type       = XVG_COLOUR_CONIC_GRADIENT,
        .colour1    = col_stop_1,
        .colour2    = col_stop_2,
        .gradient_a = {a, a},
        .gradient_b = {b, b},
    };
}

XVGGradient xvg_make_inner_shadow(
    uint32_t col_stop_outer,
    uint32_t col_stop_inner,
    float    x_translate,
    float    y_translate,
    float    blur_radius)
{
    return (XVGGradient){
        .type       = XVG_COLOUR_BOX_GRADIENT, // TODO: convert this to inner shadow
        .colour1    = col_stop_outer,
        .colour2    = col_stop_inner,
        .gradient_a = {x_translate, y_translate},
        .gradient_b = {blur_radius, blur_radius},
    };
}

void xvg_draw_circle(XVG* xvg, float cx, float cy, float radius_px, float stroke_width, uint32_t colour)
{
    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_CIRCLE_STROKE : XVG_SHAPE_CIRCLE_FILL;
    float        feather    = 2.0f / radius_px;

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft     = {cx - radius_px, cy - radius_px},
                    .bottomright = {cx + radius_px, cy + radius_px},
                    .colour1     = colour,
                    .sdf_data    = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
    };
}

void xvg_draw_rectangle(XVG* xvg, float x, float y, float w, float h, float br, float stroke_width, uint32_t col)
{
    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_ROUNDED_RECTANGLE_STROKE : XVG_SHAPE_ROUNDED_RECTANGLE_FILL;
    float        feather    = 4.0f / xm_minf(w, h);

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
                    .borderradius_arcpie = _xvg_compress_border_radius(br, br, br, br),
                    .colour1             = col,
    };
}

void xvg_draw_rectangle_with_gradient(XVG* xvg, float x, float y, float w, float h, float br, XVGGradient grad)
{
    float feather = 4.0f / xm_minf(w, h);

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(XVG_SHAPE_ROUNDED_RECTANGLE_FILL, grad.type, feather, 0),
                    .borderradius_arcpie = _xvg_compress_border_radius(br, br, br, br),

                    .colour1    = grad.colour1,
                    .colour2    = grad.colour2,
                    .gradient_a = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b = {grad.gradient_b[0], grad.gradient_b[1]},
    };
}

void xvg_draw_triangle(
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

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(rotate_radians, 0),
                    .colour1             = colour,
    };
}

void xvg_draw_pie(
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

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {cx - radius_px, cy - radius_px},
                    .bottomright         = {cx + radius_px, cy + radius_px},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, 0, feather, stroke_width),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(angle_rotate * 0.5f, angle_range * 0.5f),
                    .colour1             = colour,
    };
}

void xvg_draw_arc(
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

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft     = {cx - radius_px, cy - radius_px},
                    .bottomright = {cx + radius_px, cy + radius_px},
                    .sdf_data    = _xvg_compress_sdf_data(
            butt ? XVG_SHAPE_ARC_BUTT_STROKE : XVG_SHAPE_ARC_ROUND_STROKE,
            0,
            feather,
            stroke_width),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(angle_rotate * 0.5f, angle_range * 0.5f),
                    .colour1             = colour,
    };
}

// ██╗     ██╗███╗   ██╗███████╗███████╗
// ██║     ██║████╗  ██║██╔════╝██╔════╝
// ██║     ██║██╔██╗ ██║█████╗  ███████╗
// ██║     ██║██║╚██╗██║██╔══╝  ╚════██║
// ███████╗██║██║ ╚████║███████╗███████║
// ╚══════╝╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝

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
    xassert(xvg->tile_buffer_len <= XVG_ARRLEN(xvg->tile_buffer));
    size_t remaining_capacity = XVG_ARRLEN(xvg->line_buffer) - xvg->tile_buffer_len;

    int N = width;

    if (N > remaining_capacity)
        N = 0;

    if (N == 0)
        return;

    int end_idx = xvg->line_buffer_len + N;

    if (xvg->backingScaleFactor == 1)
    {
        xstatic_assert(sizeof(xvg->line_buffer[0]) == sizeof(data[0]), "Must match");
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

XVGFontSlot* _xvg_get_current_font_slot(XVG* xvg) { return &xvg->text.fonts[xvg->text.current_font_idx]; }

XVGFont _xvg_add_font_from_memory_impl(XVG* xvg, const void* data, size_t datalen, bool owned)
{
    for (int i = 0; i < XVG_ARRLEN(xvg->text.fonts); i++)
    {
        XVGFontSlot* sl = xvg->text.fonts + i;
        if (sl->ft_face == NULL)
        {
            // TODO: pass data to kbts
            int err = FT_New_Memory_Face(xvg->text.ft_lib, data, datalen, 0, &sl->ft_face);
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
    if (next_font_idx >= XVG_ARRLEN(xvg->text.fonts))
        next_font_idx = XVG_ARRLEN(xvg->text.fonts) - 1;
    xvg->text.current_font_idx = next_font_idx;
}

// ████████╗███████╗██╗  ██╗████████╗
// ╚══██╔══╝██╔════╝╚██╗██╔╝╚══██╔══╝
//    ██║   █████╗   ╚███╔╝    ██║
//    ██║   ██╔══╝   ██╔██╗    ██║
//    ██║   ███████╗██╔╝ ██╗   ██║
//    ╚═╝   ╚══════╝╚═╝  ╚═╝   ╚═╝
enum
{
#if defined(XVG_TEXT_SINGLECHANNEL)
    XVG_FT_RENDER_MODE       = FT_RENDER_MODE_NORMAL,
    XVG_FT_LOAD              = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL,
    XVG_FT_PIXEL_MODE        = FT_PIXEL_MODE_GRAY,
    XVG_FT_BITMAP_CHANNELS   = 1,
    XVG_GLYPH_ATLAS_CHANNELS = 1,
    XVG_SG_PIXEL_FORMAT      = SG_PIXELFORMAT_R8,
#endif
#if defined(XVG_TEXT_MULTICHANNEL)
    XVG_FT_RENDER_MODE       = FT_RENDER_MODE_LCD, // subpixel antialiasing, horizontal screen
    XVG_FT_LOAD              = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LCD,
    XVG_FT_PIXEL_MODE        = FT_PIXEL_MODE_LCD,
    XVG_FT_BITMAP_CHANNELS   = 3,
    XVG_GLYPH_ATLAS_CHANNELS = 4,
    XVG_SG_PIXEL_FORMAT      = SG_PIXELFORMAT_RGBA8,
#endif

    XVG_ATLAS_WIDTH      = 256,
    XVG_ATLAS_HEIGHT     = XVG_ATLAS_WIDTH,
    XVG_ATLAS_ROW_STRIDE = XVG_ATLAS_WIDTH * XVG_GLYPH_ATLAS_CHANNELS,

    RECTPACK_PADDING = 1,
};

XVGAtlas _xvg_create_new_atlas()
{
    sg_image img = sg_make_image(&(sg_image_desc){
        .width                = XVG_ATLAS_WIDTH,
        .height               = XVG_ATLAS_HEIGHT,
        .pixel_format         = XVG_SG_PIXEL_FORMAT,
        .usage.dynamic_update = true,
    });
    xassert(img.id);
    XVGAtlas atlas = {.img_view = sg_make_view(&(sg_view_desc){.texture.image = img})};
    xassert(atlas.img_view.id);
    return atlas;
}

XVGAtlas* _xvg_get_current_font_atlas(XVG* xvg)
{
    xassert(xvg->text.current_atlas.idx < XVG_ARRLEN(xvg->text.atlases));
    return xvg->text.atlases + xvg->text.current_atlas.idx;
}

int _xvg_render_glyph(XVG* xvg, uint32_t glyph_index, float font_size)
{
    int num_packed = 0;

    XVGAtlas*    atlas = _xvg_get_current_font_atlas(xvg);
    XVGFontSlot* sl    = _xvg_get_current_font_slot(xvg);

    if (!sl->ft_face || atlas->full)
        return num_packed;

    int err = FT_Load_Glyph(sl->ft_face, glyph_index, XVG_FT_LOAD);
    xassert(!err);

    const FT_GlyphSlot glyph = sl->ft_face->glyph;
    const FT_Bitmap*   bmp   = &glyph->bitmap;
    xassert(bmp->pixel_mode == XVG_FT_PIXEL_MODE);
    xassert((bmp->width % XVG_FT_BITMAP_CHANNELS) == 0); // note: FT width is measured in bytes (subpixels)
    // Note all glyphs have height/rows... (spaces?)
    if (bmp->width && bmp->rows)
    {
        int        width_pixels = bmp->width / XVG_FT_BITMAP_CHANNELS;
        stbrp_rect rect         = {.w = width_pixels + RECTPACK_PADDING, .h = bmp->rows + RECTPACK_PADDING};
        num_packed              = stbrp_pack_rects(&xvg->text.current_atlas.ctx, &rect, 1);

        bool failed_to_pack = num_packed == 0;
        if (failed_to_pack)
        {
            atlas->full = true;

            bool can_create_new_atlas = (xvg->text.current_atlas.idx + 1) < XVG_ARRLEN(xvg->text.atlases);
            if (can_create_new_atlas) // atlas is full
            {
                // make new atlas
                xvg->text.current_atlas.idx++;
                xvg->text.atlases[xvg->text.current_atlas.idx] = _xvg_create_new_atlas();

                atlas = xvg->text.atlases + xvg->text.current_atlas.idx;

                // Clear rectpack
                memset(&xvg->text.current_atlas.ctx, 0, sizeof(xvg->text.current_atlas.ctx));
                stbrp_init_target(
                    &xvg->text.current_atlas.ctx,
                    XVG_ATLAS_WIDTH - RECTPACK_PADDING,
                    XVG_ATLAS_HEIGHT - RECTPACK_PADDING,
                    xvg->text.current_atlas.nodes,
                    xarr_len(xvg->text.current_atlas.nodes));

                rect       = (stbrp_rect){.w = width_pixels + RECTPACK_PADDING, .h = bmp->rows + RECTPACK_PADDING};
                num_packed = stbrp_pack_rects(&xvg->text.current_atlas.ctx, &rect, 1);
                xassert(num_packed == 1);
            }
        }

        if (num_packed)
        {
            int expected_height = glyph->metrics.height >> 6;
            xassert(expected_height == bmp->rows);
            XVGAtlasRect arect;
            arect.header.glyph_index = glyph_index;
            arect.header.font_size   = font_size;
            arect.bearing_x          = glyph->bitmap_left;
            arect.bearing_y          = glyph->bitmap_top;
            arect.x                  = rect.x + RECTPACK_PADDING;
            arect.y                  = rect.y + RECTPACK_PADDING;
            arect.w                  = width_pixels;
            arect.h                  = bmp->rows;
            arect.img_view           = atlas->img_view;
            xassert(glyph->advance.x < (1 << 15));
            xassert(glyph->advance.y < (1 << 15));
            arect.advance_x = glyph->advance.x;
            arect.advance_y = glyph->advance.y;
            xassert(arect.x + arect.w < XVG_ATLAS_WIDTH);
            xassert(arect.y + arect.h < XVG_ATLAS_HEIGHT);

            xarr_push(xvg->text.rects, arect);

            for (int y = 0; y < bmp->rows; y++)
            {
#if defined(XVG_TEXT_SINGLECHANNEL)
                unsigned char* dst = xvg->text.current_atlas.img_data + (arect.y + y) * XVG_ATLAS_ROW_STRIDE + arect.x;
                unsigned char* src = bmp->buffer + y * bmp->pitch;

                unsigned char(*src_view)[512]  = (void*)src;
                src_view                      += 0;
                unsigned char(*dst_view)[512]  = (void*)dst;
                dst_view                      += 0;

                memcpy(dst, src, width_pixels);

                dst_view += 0;
#endif
#if defined(XVG_TEXT_MULTICHANNEL)
                unsigned char* dst = xvg->text.current_atlas.img_data + (arect.y + y) * XVG_ATLAS_ROW_STRIDE +
                                     arect.x * XVG_GLYPH_ATLAS_CHANNELS;
                unsigned char* src = bmp->buffer + y * bmp->pitch;

                for (int x = 0; x < width_pixels; x++, dst += XVG_GLYPH_ATLAS_CHANNELS, src += XVG_FT_BITMAP_CHANNELS)
                {
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = 0;
                }
#endif
            }

            atlas->dirty = true;
        }
    }

    return num_packed;
}

// Get cached rect. Rasters the rect to an atlas if not already cached
// TODO: also compare font id
// TODO: use fallback fonts. This may require accepting utf32 codepoints to detect language
XVGAtlasRect _xvg_get_glyph(XVG* xvg, uint32_t glyph_index, float font_size)
{
    const int num_rects = xarr_len(xvg->text.rects);

    XVGAtlasRectHeader header = {.glyph_index = glyph_index, .font_size = font_size};

    for (int j = 0; j < num_rects; j++)
    {
        if (xvg->text.rects[j].header.data == header.data)
        {
            XVGAtlasRect* rect = xvg->text.rects + j;
            xassert(rect->x + rect->w < XVG_ATLAS_WIDTH);
            return *rect;
        }
    }

    int did_raster = _xvg_render_glyph(xvg, glyph_index, font_size);
    if (did_raster)
    {
        xassert(num_rects + 1 == xarr_len(xvg->text.rects));
        XVGAtlasRect* rect = xvg->text.rects + num_rects;
        xassert(rect->x + rect->w < XVG_ATLAS_WIDTH);
        return *rect;
    }

    // Note: this stub has a texture view id of 0
    // sokol_gfx should assert in debug mode when trying to bind a texture view with an id of 0
    // In release it should skip all draws using that view. This is our desired behaviour
    static const XVGAtlasRect stub = {0};
    return stub;
}

bool _xvg_push_glyph(XVG* xvg, int pen_x, int pen_y, const XVGAtlasRect* rect, uint32_t colour)
{
    bool should_push  = xvg->text_buffer_len < XVG_ARRLEN(xvg->text_buffer);
    should_push      &= rect->img_view.id != 0;
    XVG_ASSERT(should_push);
    if (should_push)
    {
        xvg_text_t* obj   = xvg->text_buffer + xvg->text_buffer_len;
        obj->topleft      = _xvg_pack_xy_coord(pen_x + rect->bearing_x, pen_y - rect->bearing_y);
        obj->atlas_coords = (xvecu){.r = rect->x, .g = rect->y, .b = rect->w, .a = rect->h}.u32;
        obj->colour       = colour;

        xvg->text_buffer_len++;
    }
    return should_push;
}

XVGTextLayoutRow* xvg__startRow(XVG* xvg, XVGTextLayout* layout)
{
    XVG_ASSERT(layout->num_rows <= layout->cap_rows);
    XVGTextLayoutRow* rows = xvg_layout_get_rows(layout);
    if (layout->num_rows >= layout->cap_rows)
    {
        // realloc
        layout->cap_rows            *= 2;
        XVGTextLayoutRow* next_rows  = linked_arena_alloc(xvg->arena, sizeof(*next_rows) * layout->cap_rows);
        memcpy(next_rows, rows, sizeof(*next_rows) * layout->num_rows);

        xvg_layout_set_rows(layout, next_rows);
        rows = next_rows;
    }

    rows[layout->num_rows++] = (XVGTextLayoutRow){.begin_idx = layout->num_glyphs};
    return rows;
}

void xvg__endRow(XVG* xvg, XVGTextLayout* layout, int ymin, int ymax, int cursor_y_px)
{
    // XVG_ASSERT(layout->num_rows > 0);
    if (layout->num_rows > 0)
    {
        XVGTextLayoutRow* rows   = xvg_layout_get_rows(layout);
        XVGGlyphLayout*   glyphs = xvg_layout_get_glyphs(layout);
        XVGGlyphLayout*   g      = &glyphs[layout->num_glyphs - 1];
        XVGTextLayoutRow* row    = &rows[layout->num_rows - 1];

        if (row->begin_idx != layout->num_glyphs)
        {
            XVG_ASSERT(ymax != 0 || ymin != 0);
            row->end_idx     = layout->num_glyphs;
            row->ymin        = ymin;
            row->ymax        = ymax;
            row->xmax        = g->x + g->rect.w;
            row->cursor_y_px = cursor_y_px;
        }
    }
}

// Lazy and fast layout
const XVGTextLayout* xvg_create_text_layout(
    XVG*        xvg,
    const char* text_start,
    const char* text_end,
    float       font_size,
    float       break_width,
    float       _line_height)
{
    static const XVGTextLayout stub = {0};

    XVG_ASSERT(font_size < 128);
    if (text_end == NULL)
        text_end = text_start + strlen(text_start);
    const size_t text_len = text_end - text_start;

    font_size   *= xvg->backingScaleFactor;
    break_width *= xvg->backingScaleFactor;

    XVGTextLayout* layout = linked_arena_alloc_clear(xvg->arena, sizeof(*layout));
    layout->cap_glyphs    = text_len * 2;

    layout->cap_rows = text_len >> 4;
    if (layout->cap_rows < 8)
        layout->cap_rows = 8;
    XVGTextLayoutRow* rows   = linked_arena_alloc_clear(xvg->arena, sizeof(*rows) * layout->cap_rows);
    XVGGlyphLayout*   glyphs = linked_arena_alloc(xvg->arena, sizeof(*glyphs) * layout->cap_glyphs);
    xvg_layout_set_rows(layout, rows);
    xvg_layout_set_glyphs(layout, glyphs);

    XVGFontSlot* sl = _xvg_get_current_font_slot(xvg);
    if (!sl->ft_face)
        return &stub;

    FT_FaceRec* face = sl->ft_face;
    FT_Set_Pixel_Sizes(face, 0, font_size);

    const FT_Size_Metrics* m = &face->size->metrics;

    int64_t line_height = (double)m->height * _line_height;

    layout->ascender    = m->ascender >> 6;
    layout->descender   = m->descender >> 6;
    layout->line_height = line_height >> 6;

    xassert(sl->space_advance);
    const int64_t space_advance = FT_MulFix(sl->space_advance, m->x_scale) / 2;

    // Clang-cl makes INT64_MAX a negative integer if we aren't super explicit with types here
    const int64_t break_row_x = break_width != 0 ? (int64_t)(break_width * 64) : (int64_t)INT64_MAX;
    xassert(break_row_x >= 0);

    int64_t CursorX = 0, CursorY = 0;
    int     line_xmax = 0, line_xmin = 0;
    int     line_ymax = 0, line_ymin = 0;
    int     layout_xmax = 0;

    rows                       = xvg__startRow(xvg, layout);
    const char* iter           = text_start;
    unsigned    prev_glyph_idx = 0;

    int     num_glyphs_at_last_space = 0;
    int64_t CursorX_after_last_space = 0;
    while (iter != text_end)
    {
        int cp = 0;
        iter   = utf8codepoint(iter, &cp);

        switch (cp)
        {
        // case 32: // SP, space
        case 10: // LF, \n
        {
            layout_xmax = xm_maxi(layout_xmax, line_xmax);

            xvg__endRow(xvg, layout, line_ymin, line_ymax, CursorY >> 6);
            rows = xvg__startRow(xvg, layout);

            prev_glyph_idx = 0;
            line_xmin      = 0;
            line_xmax      = 0;
            line_ymin      = 0;
            line_ymax      = 0;

            num_glyphs_at_last_space = 0;
            CursorX_after_last_space = 0;

            CursorX  = 0;
            CursorY += line_height;
            break;
        }
        default:
        {
            unsigned glyph_idx = FT_Get_Char_Index(face, cp);
            xassert(glyph_idx != 0);

            XVGAtlasRect rect = _xvg_get_glyph(xvg, glyph_idx, font_size);

            bool add_to_metadata = layout->num_glyphs < layout->cap_glyphs;

            if (cp == 32) // space
            {
                rect.w         = space_advance >> 6;
                rect.advance_x = space_advance;
            }

            if (add_to_metadata)
            {
                line_ymax = xm_maxi(line_ymax, rect.bearing_y);
                line_ymin = xm_mini(line_ymin, rect.bearing_y - rect.h);
                FT_Vector kerning;
                FT_Get_Kerning(face, prev_glyph_idx, glyph_idx, FT_KERNING_DEFAULT, &kerning);
                int glyph_px_x = (CursorX + kerning.x) >> 6;
                int glyph_px_y = (CursorY + kerning.y) >> 6;
                XVG_ASSERT(glyph_px_x >= 0);

                line_xmax = xm_maxi(line_xmax, glyph_px_x + rect.w);

                glyphs[layout->num_glyphs++] = (XVGGlyphLayout){.x = glyph_px_x, .y = glyph_px_y, .rect = rect};
            }
            xassert(rect.advance_x > 0);

            CursorX        += rect.advance_x;
            prev_glyph_idx  = glyph_idx;

            if (cp == 32) // space
            {
                num_glyphs_at_last_space = layout->num_glyphs;
                CursorX_after_last_space = CursorX;
            }

            if (CursorX > break_row_x)
            {
                // Break word
                xassert(layout->num_rows);
                xassert(num_glyphs_at_last_space <= layout->num_glyphs);
                xvg__endRow(xvg, layout, line_ymin, line_ymax, (CursorY + line_height) >> 6);
                rows = xvg__startRow(xvg, layout);

                XVGTextLayoutRow* prev_row    = &rows[layout->num_rows - 2];
                XVGTextLayoutRow* current_row = &rows[layout->num_rows - 1];

                prev_glyph_idx = 0;
                line_xmin      = 0;
                line_xmax      = 0;
                line_ymin      = 0;
                line_ymax      = 0;

                int end_idx = -1;
                if (num_glyphs_at_last_space > 0)
                {
                    xassert(layout->num_rows >= 2);
                    XVGGlyphLayout* break_glyph = &glyphs[num_glyphs_at_last_space - 1];

                    end_idx           = prev_row->end_idx;
                    prev_row->end_idx = num_glyphs_at_last_space;
                    prev_row->xmax    = break_glyph->x + break_glyph->rect.w;

                    xassert(prev_row->begin_idx <= prev_row->end_idx);
                    layout_xmax = xm_maxi(layout_xmax, prev_row->xmax);
                }
                current_row->begin_idx = num_glyphs_at_last_space;
                current_row->end_idx   = end_idx > 0 ? end_idx : layout->num_glyphs;
                xassert(current_row->begin_idx <= current_row->end_idx);

                if (current_row->begin_idx < layout->num_glyphs)
                {
                    const int offset_x = glyphs[current_row->begin_idx].x;
                    for (int i = current_row->begin_idx; i < layout->num_glyphs; i++)
                    {
                        XVGGlyphLayout* gp = &glyphs[i];
                        // Apply offsets to glyphs on new line
                        gp->x -= offset_x;
                        gp->y  = (CursorY + line_height) >> 6;
                        xassert(gp->x >= 0);

                        // recalculate row stats
                        line_ymax = xm_maxi(line_ymax, gp->rect.bearing_y);
                        line_ymin = xm_mini(line_ymin, gp->rect.bearing_y - rect.h);
                        line_xmax = xm_maxi(line_xmax, gp->x + gp->rect.w);
                    }
                }
                layout_xmax = xm_maxi(layout_xmax, line_xmax);

                CursorX  = CursorX_after_last_space > 0 ? (CursorX - CursorX_after_last_space) : 0;
                CursorY += line_height;
                xassert(CursorX >= 0);

                num_glyphs_at_last_space = -1;
                CursorX_after_last_space = -1;

                // This awful looking code helps to skip multiple spaces that may appear at the beginning  of a new line
                // A more clever person than I could probably express this better
                int num_skipped = 0;
                while (*iter == ' ')
                {
                    iter++;
                    num_skipped++;
                }
                if (num_skipped == 1)
                    iter--;
            }
            break;
        }
        }
    }
    layout_xmax = xm_maxi(layout_xmax, line_xmax);
    xvg__endRow(xvg, layout, line_ymin, line_ymax, CursorY >> 6);
    layout->xmax = layout_xmax;

    layout->total_height       = (CursorY + (m->ascender - m->descender)) >> 6;
    int row_0_top              = rows[0].ymax;
    int row_n_1_bottom         = rows[layout->num_rows - 1].ymin - rows[layout->num_rows - 1].cursor_y_px;
    layout->total_height_tight = row_0_top - row_n_1_bottom;

#ifndef NDEBUG
    int64_t break_row_x_px = break_row_x >> 6;
    int     xmax_2         = 0;
    for (int i = 0; i < layout->num_rows; i++)
    {
        const XVGTextLayoutRow* r = rows + i;
        XVG_ASSERT(layout_xmax >= r->xmax);
        for (int j = r->begin_idx; j < r->end_idx; j++)
        {
            const XVGGlyphLayout* g  = glyphs + j;
            int                   gr = g->x + g->rect.w;
            XVG_ASSERT(gr <= rows[i].xmax);
            XVG_ASSERT(gr <= break_row_x_px);
        }
        xmax_2 = xm_maxi(xmax_2, r->xmax);
    }
    XVG_ASSERT(xmax_2 == layout_xmax);
#endif

    XVG_ASSERT(layout->num_rows);
    XVG_ASSERT(layout->num_glyphs);
    XVG_ASSERT(rows[0].begin_idx < rows[0].end_idx);

    return layout;
}

void xvg_draw_text_layout(XVG* xvg, const XVGTextLayout* layout, int x, int y, int alignment, uint32_t colour)
{
    x *= xvg->backingScaleFactor;
    y *= xvg->backingScaleFactor;

    const XVGTextLayoutRow* rows = xvg_layout_get_rows(layout);

    int text_px_right = layout->xmax;

    // TODO: handle multi-line centre & right alignmnt
    if (alignment & XVG_ALIGN_CENTRE_X)
        x -= (text_px_right / 2);
    else if (alignment & XVG_ALIGN_RIGHT)
        x -= text_px_right;

    // By taking the ymin/ymax of the entire run of text, we can tightly fit the text vertically to where the user has
    // requested
    // If we use the ascender/descender metrics from Freetype, then there is always a little bit og padding. The padding
    // actually looks good, but it strips some control from the user
    if (alignment & XVG_ALIGN_CENTRE_Y)
    {
        y += (layout->ascender - (layout->total_height / 2));
    }
    else if (alignment & XVG_ALIGN_TOP)
    {
        y += layout->ascender;
    }
    else if (alignment & XVG_ALIGN_BOTTOM)
    {
        y += (layout->ascender - layout->total_height);
    }
    if (alignment & XVG_ALIGN_CENTRE_Y_TIGHT)
    {
        int half_height_rounded_up  = layout->total_height_tight - (layout->total_height_tight / 2);
        y                          += rows->ymax - half_height_rounded_up;
    }
    else if (alignment & XVG_ALIGN_TOP_TIGHT)
    {
        y += rows->ymax;
    }
    else if (alignment & XVG_ALIGN_BOTTOM_TIGHT)
    {
        const XVGTextLayoutRow* r  = &rows[layout->num_rows - 1];
        y                         += r->ymin - r->cursor_y_px;
    }

    // XVGGlyphLayout(*view_glyphs)[512] = (void*)layout->glyphs;
    // XVG_ASSERT(layout->num_rows == 1);

    // Glyphs we need to render may live across several glyph atlases
    // Here we will attempt to batch all glyph draws sharing the same atlas
    // We may perform multiple passes over the glyph_pos buffer as we put all glyphs sharing a buffer into a batch
    // at a time, and build a
    size_t          glyph_pos_len = layout->num_glyphs;
    XVGGlyphLayout* glyph_pos_1   = xvg_layout_get_glyphs(layout);
    XVGGlyphLayout* glyph_pos_2   = linked_arena_alloc(xvg->arena, sizeof(*glyph_pos_2) * glyph_pos_len);

    int             glyphs_consumed      = 0;
    XVGGlyphLayout* search_glyphs        = glyph_pos_1;
    int             search_glyphs_len    = glyph_pos_len;
    XVGGlyphLayout* remaining_glyphs     = glyph_pos_2;
    int             remaining_glyphs_len = 0;

    int inf_loop_protection = 0;
    while (glyphs_consumed < glyph_pos_len)
    {
        XVG_ASSERT(++inf_loop_protection < 50);
        sg_view target_atlas_view = {0};
        for (int i = 0; i < search_glyphs_len && target_atlas_view.id == 0; i++)
        {
            XVGGlyphLayout* gpos = &search_glyphs[i];
            target_atlas_view    = gpos->rect.img_view;
        }
        if (target_atlas_view.id == 0)
        {
            // Failed to find any glyphs to render. May be all spaces
            break;
        }

        // sg_view target_atlas_view = search_glyphs[0].rect.img_view;
        // XVG_ASSERT(target_atlas_view.id != 0);

        size_t text_buf_begin_len = xvg->text_buffer_len;

        // Iterate through remainder of array, putting every glyph with matching atlas into our batch
        for (int i = 0; i < search_glyphs_len; i++)
        {
            XVGGlyphLayout* gpos = &search_glyphs[i];

            bool should_push = target_atlas_view.id == gpos->rect.img_view.id;

            if (should_push)
            {
                bool did_push = _xvg_push_glyph(xvg, x + gpos->x, y + gpos->y, &gpos->rect, colour);
                glyphs_consumed++;
            }
            else if (gpos->rect.img_view.id == 0)
            {
                glyphs_consumed++;
            }
            else
            {
                // Build temp buffer with remaining glyphs
                remaining_glyphs[remaining_glyphs_len++] = *gpos;
            }
        }
        size_t text_buf_end_len = xvg->text_buffer_len;

        if (text_buf_end_len != text_buf_begin_len)
        {
            // TODO: save metadata and paint info here
            // xassert(false);
            // _xvg_command_draw_text(
            //     xvg,
            //     XVG_LABEL(__FUNCTION__),
            //     text_buf_begin_len,
            //     text_buf_end_len,
            //     xvg->state.paint.innerColour,
            //     target_atlas_view);
        }

        XVG_ASSERT(glyphs_consumed <= glyph_pos_len);
        if (glyphs_consumed < glyph_pos_len)
        {
            XVGGlyphLayout* tmp_glyphs = search_glyphs;

            search_glyphs     = remaining_glyphs;
            search_glyphs_len = remaining_glyphs_len;

            remaining_glyphs     = tmp_glyphs;
            remaining_glyphs_len = 0;
        }
    }
}
static void xvg_release_text_layout(XVG* xvg, const XVGTextLayout* layout) { linked_arena_release(xvg->arena, layout); }

void xvg_draw_text_ex(
    XVG*        xvg,
    float       x,
    float       y,
    const char* text_start,
    const char* text_end,
    float       font_size,
    XVGAlign    alignment,
    uint32_t    colour,
    float       break_width,
    float       line_height)
{
    LINKED_ARENA_LEAK_DETECT_BEGIN(xvg->arena);

    const XVGTextLayout* layout =
        xvg_create_text_layout(xvg, text_start, text_end, font_size, break_width, line_height);
    xvg_draw_text_layout(xvg, layout, x, y, alignment, colour);
    xvg_release_text_layout(xvg, layout);

    LINKED_ARENA_LEAK_DETECT_END(xvg->arena);
}

void xvg_draw_text(
    XVG*        xvg,
    float       x,
    float       y,
    const char* text_start,
    const char* text_end,
    float       font_size,
    XVGAlign    alignment,
    uint32_t    colour)
{
    xvg_draw_text_ex(xvg, x, y, text_start, text_end, font_size, alignment, colour, 0, 1);
}

void xvg_init(XVG* xvg)
{
    xvg->backingScaleFactor = 1;

    xvg->arena = linked_arena_create_ex(0, 1024 * 64);

    static const sg_color_target_state BLEND_DEFAULT = {
        .write_mask = SG_COLORMASK_RGBA,
        .blend      = {
                 .enabled          = true,
                 .src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA,
                 .src_factor_alpha = SG_BLENDFACTOR_ONE,
                 .dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                 .dst_factor_alpha = SG_BLENDFACTOR_ONE,
        }};
    static const sg_color_target_state BLEND_DUAL_SOURCE = {
        .write_mask = SG_COLORMASK_RGB,
        .blend      = {
                 .enabled = true,
            //  .src_factor_rgb = SG_BLENDFACTOR_SRC1_COLOR, // use if no premultiplied alpha
                 .src_factor_rgb = SG_BLENDFACTOR_ONE, // use if premultiplied alpha
                 .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC1_COLOR,
        }};

    // Shader stuff
    {
        xvg->smp_linear            = sg_make_sampler(&(sg_sampler_desc){
                       .min_filter    = SG_FILTER_LINEAR,
                       .mag_filter    = SG_FILTER_LINEAR,
                       .mipmap_filter = SG_FILTER_LINEAR,
                       .wrap_u        = SG_WRAP_CLAMP_TO_EDGE,
                       .wrap_v        = SG_WRAP_CLAMP_TO_EDGE,
        });
        xvg->smp_nearest_neighbour = sg_make_sampler(&(sg_sampler_desc){
            .min_filter    = SG_FILTER_NEAREST,
            .mag_filter    = SG_FILTER_NEAREST,
            .mipmap_filter = SG_FILTER_NEAREST,
            .wrap_u        = SG_WRAP_CLAMP_TO_EDGE,
            .wrap_v        = SG_WRAP_CLAMP_TO_EDGE,
        });

        xvg->shapes.pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader    = sg_make_shader(_xvg_shapes_shader_desc(sg_query_backend())),
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

        xvg->lines.pip = sg_make_pipeline(&(sg_pipeline_desc){
            .shader    = sg_make_shader(_xvg_lines_shader_desc(sg_query_backend())),
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

    // Text
    {
        int ft_err = FT_Init_FreeType(&xvg->text.ft_lib);
        xassert(ft_err == 0);

        xvg->text.sbo = sg_make_buffer(&(sg_buffer_desc){
            .usage.storage_buffer = true,
            .usage.stream_update  = true,
            .size                 = sizeof(xvg->text_buffer),
            .label                = "text SBO",
        });
        xassert(xvg->text.sbo.id);
        xvg->text.sbv = sg_make_view(&(sg_view_desc){
            .storage_buffer = xvg->text.sbo,
        });
        xassert(xvg->text.sbv.id);

        xvg->text.pip = sg_make_pipeline(&(sg_pipeline_desc) {
#if defined(XVG_TEXT_MULTICHANNEL)
            .shader    = sg_make_shader(_xvg_text_multichannel_shader_desc(sg_query_backend())),
            .colors[0] = BLEND_DUAL_SOURCE,
#endif
#if defined(XVG_TEXT_SINGLECHANNEL)
            .shader    = sg_make_shader(_xvg_text_singlechannel_shader_desc(sg_query_backend()));
            .colors[0] = BLEND_DEFAULT,
#endif
            .label = XVG_LABEL("xvg-text")
        });
        xassert(xvg->text.pip.id);

        xarr_setcap(xvg->text.rects, 64);
        xarr_setlen(xvg->text.rects, 0);
        xvg->text.atlases[0]        = _xvg_create_new_atlas();
        xvg->text.current_atlas.idx = 0;
        xarr_setlen(xvg->text.current_atlas.nodes, (XVG_ATLAS_WIDTH * 2));

        size_t img_size                  = XVG_ATLAS_HEIGHT * XVG_ATLAS_ROW_STRIDE;
        xvg->text.current_atlas.img_data = XVG_MALLOC(img_size);
        memset(xvg->text.current_atlas.img_data, 0, img_size);
        stbrp_init_target(
            &xvg->text.current_atlas.ctx,
            XVG_ATLAS_WIDTH - RECTPACK_PADDING,
            XVG_ATLAS_HEIGHT - RECTPACK_PADDING,
            xvg->text.current_atlas.nodes,
            xarr_len(xvg->text.current_atlas.nodes));
    }
}

void xvg_deinit(XVG* xvg)
{
    xarr_free(xvg->text.rects);
    XVG_FREE(xvg->text.current_atlas.img_data);
    xarr_free(xvg->text.current_atlas.nodes);

    for (int i = 0; i < XVG_ARRLEN(xvg->text.fonts); i++)
    {
        XVGFontSlot* sl = &xvg->text.fonts[i];
        if (sl->data && sl->owned)
        {
            XFILES_FREE(sl->data);
        }
        // TODO: free kbts here?
        if (sl->ft_face)
            FT_Done_Face(sl->ft_face);
    }
    FT_Done_FreeType(xvg->text.ft_lib);

    linked_arena_destroy(xvg->arena);
}

void xvg_begin_frame(XVG* xvg)
{
    xvg->text.current_font_idx = 0;
    xvg->shapes_buffer_len     = 0;
    xvg->line_buffer_len       = 0;
    xvg->tile_buffer_len       = 0;
    xvg->text_buffer_len       = 0;
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
    if (xvg->text_buffer_len)
    {
        size_t   num_bytes = sizeof(xvg->text_buffer[0]) * xvg->text_buffer_len;
        sg_range range     = {.ptr = xvg->text_buffer, .size = num_bytes};
        sg_update_buffer(xvg->text.sbo, &range);
    }
    for (int i = 0; i < XVG_ARRLEN(xvg->text.atlases); i++)
    {
        XVGAtlas* atlas = &xvg->text.atlases[i];
        if (atlas->dirty)
        {
            sg_view_desc desc = sg_query_view_desc(atlas->img_view);
            sg_update_image(
                desc.texture.image,
                &(sg_image_data){
                    .mip_levels[0] = {
                        .ptr  = xvg->text.current_atlas.img_data,
                        .size = XVG_ATLAS_HEIGHT * XVG_ATLAS_ROW_STRIDE,
                    }});
            atlas->dirty = false;
        }
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

    if (xvg->text_buffer_len)
    {
        sg_apply_pipeline(xvg->text.pip);
        XVGAtlas atlas = xvg->text.atlases[xvg->text.current_atlas.idx];
        sg_apply_bindings(&(sg_bindings){
            .views[VIEW_vs_xvg_text_buffer] = xvg->text.sbv,
            .views[VIEW_fs_xvg_text_tex]    = atlas.img_view,
            .samplers[SMP_fs_xvg_text_smp]  = xvg->smp_nearest_neighbour,
        });

        vs_xvg_text_uniforms_t uniforms = {
            .u_view_size  = {window_width, window_height},
            .u_sbo_offset = 0,
        };
        sg_apply_uniforms(UB_vs_xvg_text_uniforms, &SG_RANGE(uniforms));

        sg_draw(0, 6 * xvg->text_buffer_len, 1);
    }
}

#endif // XVG_IMPL