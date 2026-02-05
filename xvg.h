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
// TODO: increase max stroke width for line plots
// TODO: support fallback fonts for missing glyphs
// TODO: support intelligent batching of text when using multiple atlases
// TODO: support multiple command lists that can store draw calls independantly and be joined later
         all draw commands will need to receive a ptr to the command list
         eg.
         struct XVGCommandList
        {
             XVG* xvg;
             LinekdArena* arena;
             ... other stuff
        }
        ...
        xvg_draw_thing(XVGCommandList*, ...);
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
#else // Windows & Linux
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
    XVG_SHAPE_LINE_ROUND, // TODO: butt?
    XVG_SHAPE_LINE_PLOT,
} XVGShapeType;

typedef enum XVGColourType
{
    XVG_COLOUR_SOLID,
    XVG_COLOUR_LINEAR_GRADIENT,
    XVG_COLOUR_RADIAL_GRADIENT,
    XVG_COLOUR_CONIC_GRADIENT,
    XVG_COLOUR_DROP_SHADOW,
    XVG_COLOUR_INNER_SHADOW,
} XVGColourType;

// Text alignment flags
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

    // "Tight" alignment removes any vertical padding recommended by the fonts ascender/descender info
    // eg. if the text "yes" (note the lower case) is tight alligned to the top, the top pixels of y, e & s will hug the
    // top. Adding an upper case letter to that sequence (eg. yesterday) will push the other characters down
    // This is meant to be useful when you want text properly centre aligned
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
        // TODO: this could probably be packed into a smaller integer. This could make room for font ids in the header
        unsigned font_size;
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
    unsigned char* img_data;
    sg_view        img_view;
    bool           dirty;
    bool           full;
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
    void*        arena_top;

    LinkedArena* frame_arena;

    struct XVGCommand* first_command;
    struct XVGCommand* current_command;

    struct
    {
        int        shape_buffer_start;
        sg_view    shape_texture;
        sg_sampler shape_sampler;

        int     text_buffer_start;
        sg_view text_texture;
    } draw_start;

    float backingScaleFactor;

    sg_sampler smp_linear;
    sg_sampler smp_nearest_neighbour;
    sg_image   fallback_img;
    sg_view    fallback_img_view;

    struct
    {
        sg_pipeline pip;
        sg_buffer   sbo;
        sg_view     sbv;

        sg_view   line_sbv;
        sg_buffer line_sbo; // normalised y values
    } shapes;

    // Text pipeline
    struct
    {
        sg_pipeline pip;
        sg_buffer   sbo;
        sg_view     sbv;
        sg_sampler  smp;

        struct
        {
            int           idx;
            stbrp_context rectpack_ctx;
            stbrp_node*   nodes;
        } current_atlas;

        struct FT_LibraryRec_* ft_lib;

#ifndef XVG_MAX_FONT_SLOTS
#define XVG_MAX_FONT_SLOTS 4
#endif // XVG_MAX_FONT_SLOTS

        // Will default to the first font a user passes the library
        int         current_font_idx;
        XVGFontSlot fonts[XVG_MAX_FONT_SLOTS];

#ifndef XVG_GLYPH_ATLAS_SLOTS
// #define XVG_GLYPH_ATLAS_SLOTS 1
#define XVG_GLYPH_ATLAS_SLOTS 8
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

#ifndef XVG_MAX_GLYPHS
#define XVG_MAX_GLYPHS 1024
#endif
    size_t     text_buffer_len;
    xvg_text_t text_buffer[XVG_MAX_GLYPHS];
} XVG;

void xvg_init(XVG*);
void xvg_deinit(XVG*);

void xvg_begin_frame(XVG*);
void xvg_end_frame(XVG*, int window_width, int window_height);

// Shapes
// Unlike canvas style APIs, there are no 'fill' and 'stroke' commands. If 'stroke_width' is 0 the shape is implicitly
// filled. Values are in pixels. Stroking is done on the INSIDE of the shape, so you always get accurate widths
// Strokes have a maximum width of 15 with 2 exceptions: line plots (2px max), arcs & rounded lines (31px max)
// 'radius' is in pixels
// 'angle' and 'rotate' are in the less convential but greatly superior 'turns'.
// 0 turns == 0 degrees/0pi. 1 turn == 360 degrees/2pi, 0.25 turn = halfpi/90deg etc.
// Colours are in the highly unintuitive but highly readable ABGR format (0xffff007f is yellow, 50% opacity). This makes
// it easy to copy paste hex codes from apps like Figma, Photoshop, or web browsers. This library swizzles to RGBA

typedef struct XVGGradient
{
    XVGColourType type;
    uint32_t      colour1;
    uint32_t      colour2;
    float         gradient_a[2];
    float         gradient_b[2];

    uint32_t   xy, wh;
    sg_view    texture;
    sg_sampler sampler;
} XVGGradient;

XVGGradient xvg_make_linear_gradient(uint32_t col_1, uint32_t col_2, float x_1, float y_1, float x_2, float y_2);

XVGGradient
xvg_make_radial_gradient(uint32_t col_inner, uint32_t col_outer, float cx, float cy, float x_radius, float y_radius);

XVGGradient xvg_make_conic_gradient(uint32_t col_1, uint32_t col_2, float angle_1, float angle_2);

// Shadows blur radius is on the inside of the shape.
// Be sure to expand the area (w/h) by the radius value when drawing drop shadows
// Be sure to expand the spread by (radius * -1) when drawing inner shadows
// This will help you to maintain the correct shape proportions
// If 'is_inner_shadow' is false, shadow is drop shadow
XVGGradient xvg_make_shadow(
    uint32_t col_outer,
    uint32_t col_inner,
    float    x_translate,
    float    y_translate,
    float    radius,
    float    spread,
    bool     is_inner_shadow);

// x/y/w/h are the coords of the image getting sampled
// Saturation can be applied to change the colour of the image, inluding the opacity ie. ffffff7f (50% opacity)
XVGGradient
xvg_make_image_fill(sg_view texture, sg_sampler sampler, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t sat);

void xvg_gradient_apply_image(
    XVGGradient* grad,
    sg_view      texture,
    sg_sampler   sampler,
    uint32_t     x,
    uint32_t     y,
    uint32_t     w,
    uint32_t     h);

// Hard corners, edges snapped to pixels. Horizontal and vertical only
void xvg_draw_solid_rectangle(XVG*, int x, int y, int width, int height, unsigned col);
void xvg_draw_solid_rectangle_with_gradient(XVG*, int x, int y, int width, int height, XVGGradient grad);

// Soft corners & edges
void xvg_draw_rectangle(XVG*, float x, float y, float w, float h, float br, float stroke_px, uint32_t col);
void xvg_draw_rectangle_with_gradient(
    XVG*        xvg,
    float       x,
    float       y,
    float       w,
    float       h,
    float       br,
    float       stroke,
    XVGGradient grad);
void xvg_draw_rectangle_with_gradient_ex(
    XVG*        xvg,
    float       x,
    float       y,
    float       w,
    float       h,
    float       br_tr,
    float       br_br,
    float       br_tl,
    float       br_bl,
    float       stroke,
    XVGGradient grad);

void xvg_draw_circle(XVG*, float cx, float cy, float radius_px, float stroke_width, uint32_t col);
void xvg_draw_circle_with_gradient(XVG*, float cx, float cy, float radius_px, float sw, XVGGradient grad);

// Equilateral triangle
void xvg_draw_triangle(XVG*, float x, float y, float w, float h, float rotate, float stroke_px, uint32_t col);
void xvg_draw_triangle_with_gradient(
    XVG*        xvg,
    float       x,
    float       y,
    float       w,
    float       h,
    float       rotate,
    float       stroke,
    XVGGradient grad);

void xvg_draw_pie(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    angle_start,
    float    angle_end,
    float    stroke_px,
    uint32_t col);
void xvg_draw_pie_with_gradient(
    XVG*        xvg,
    float       cx,
    float       cy,
    float       radius_px,
    float       angle_start,
    float       angle_end,
    float       stroke_px,
    XVGGradient grad);

void xvg_draw_arc(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    start_turn,
    float    end_turn,
    float    stroke_px,
    bool     butt,
    uint32_t col);
void xvg_draw_arc_with_gradient(
    XVG*        xvg,
    float       cx,
    float       cy,
    float       radius_px,
    float       start_turn,
    float       end_turn,
    float       stroke_px,
    bool        butt,
    XVGGradient grad);

void xvg_draw_line_round_with_gradient(XVG*, float x0, float y0, float x1, float y1, float stroke, XVGGradient grad);
void xvg_draw_line_round(XVG*, float x0, float y0, float x1, float y1, float stroke_width, unsigned col);

// 'data' is expected to be an array of 'width' length
// 'data' is expected to contain normalised values where 0 == (y + height), and 1 == y
// 'data' is allowed to go beyond [0-1], it will just get cropped
// 'stroke_px' is limited to the range [1-2]
// 'crop_border_radius' crops the line at the corners of the rectangle you're drawing
void xvg_draw_line_plot(
    XVG*         xvg,
    int          x,
    int          y,
    int          w,
    int          h,
    const float* data,
    float        crop_border_radius,
    float        stroke_px,
    uint32_t     col);

// FONTS
// These functions return font IDs. 0 is considered to be invalid
// By default the first font you add will be used

// 'font_filepath' is expected to be UTF8
// Windows paths are expected to use backlashes ('\'), but forward probably wortk fine
// This library takes ownership of the memory and frees it
XVGFont xvg_add_font_from_path(XVG*, const char* font_filepath);
// Same as above, except takes a file buffer, and now you're responsible for freeing the memory
// Note this memory must remain valid until you call xvg_deinit()
XVGFont xvg_add_font_from_memory(XVG*, const void* font_data, size_t font_datalen);

// Sets active font to draw and create layouts with
void xvg_set_font(XVG*, XVGFont);

void xvg_draw_text(
    XVG*        xvg,
    float       x,
    float       y,
    const char* text_begin,
    const char* text_end,
    unsigned    font_size,
    XVGAlign    align,
    uint32_t    col);
void xvg_draw_text_ex(
    XVG*        xvg,
    float       x,
    float       y,
    const char* text_start,
    const char* text_end,
    unsigned    font_size,
    XVGAlign    alignment,
    uint32_t    colour,
    float       break_width,
    float       line_height);

const XVGTextLayout* xvg_create_text_layout(
    XVG*        xvg,
    const char* text_start,
    const char* text_end,
    unsigned    font_size,
    float       break_width,
    float       _line_height);
static void xvg_release_text_layout(XVG* xvg, const XVGTextLayout* layout)
{
    linked_arena_release(xvg->arena, layout);
};
void xvg_draw_text_layout(XVG* xvg, const XVGTextLayout* layout, int x, int y, int alignment, uint32_t colour);

// Commands
typedef struct XVGCommandBeginPass
{
    sg_pass pass;
} XVGCommandBeginPass;

typedef struct XVGCommandSetScissor
{
    int x, y, w, h;
} XVGCommandSetScissor;
typedef struct XVGCommandSetScissor XVGCommandSetViewport;

typedef struct XVGCommandDraw
{
    int        shape_buffer_start;
    int        shape_buffer_end;
    sg_view    shape_texture;
    sg_sampler shape_sampler;
    int        text_buffer_start;
    int        text_buffer_end;
    sg_view    text_texture;
} XVGCommandDraw;

typedef void (*XVGCustomFunc)(void* uptr);

typedef struct XVGCommandCustom
{
    void*         uptr;
    XVGCustomFunc func;
} XVGCommandCustom;

typedef enum XVGCommandType
{
    XVG_CMD_BEGIN_PASS,
    XVG_CMD_END_PASS,
    XVG_CMD_SET_SCISSOR,
    XVG_CMD_SET_VIEWPORT,
    XVG_CMD_DRAW,
    XVG_CMD_CUSTOM,
} XVGCommandType;

typedef struct XVGCommand
{
    XVGCommandType type;
    const char*    label;
    union
    {
        void* data;

        XVGCommandBeginPass*   beginPass;
        XVGCommandDraw*        draw;
        XVGCommandSetScissor*  scissor;
        XVGCommandSetViewport* viewport;
        XVGCommandCustom*      custom;
    } payload;

    struct XVGCommand* next;
} XVGCommand;

void xvg_command_begin_pass(XVG*, const sg_pass*, const char* label);
void xvg_command_end_pass(XVG*, const char* label);
void xvg_command_set_scissor(XVG*, int x, int y, int w, int h, const char* label);
void xvg_command_set_viewport(XVG*, int x, int y, int w, int h, const char* label);
void xvg_command_batch_draw(XVG*, const char* label);
void xvg_command_custom(XVG*, void* uptr, XVGCustomFunc func, const char* label);

sg_image xvg_make_image_with_mipmaps(const sg_image_desc* desc_);

// #ifdef __cplusplus
// }
// #endif

#endif // XVG_H

#ifdef XVG_IMPL
#undef XVG_IMPL
#include <stb_image_resize2.h>
#include <string.h>
#include <utf8.h>
#include <xhl/array.h>
#include <xhl/files.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

#if !defined(XVG_MALLOC) || !defined(XVG_REALLOC) || !defined(XVG_FREE)
#include <stdlib.h>
#define XVG_MALLOC(sz)       malloc(sz)
#define XVG_REALLOC(ptr, sz) realloc(ptr, sz)
#define XVG_FREE(ptr)        free(ptr)
#endif

// Source: https://github.com/floooh/sokol/issues/102
// Modified to use STBIR
sg_image xvg_make_image_with_mipmaps(const sg_image_desc* desc_)
{
    sg_image_desc desc = *desc_;
    // TODO: support floats
    XVG_ASSERT(
        desc.pixel_format == SG_PIXELFORMAT_RGBA8 || desc.pixel_format == SG_PIXELFORMAT_BGRA8 ||
        desc.pixel_format == SG_PIXELFORMAT_R8);

    unsigned num_channels = 1;
    if (desc.pixel_format == SG_PIXELFORMAT_RGBA8 || desc.pixel_format == SG_PIXELFORMAT_BGRA8)
        num_channels = 4;

    stbir_pixel_layout layout_type = desc.pixel_format == SG_PIXELFORMAT_RGBA8   ? STBIR_RGBA
                                     : desc.pixel_format == SG_PIXELFORMAT_BGRA8 ? STBIR_BGRA
                                                                                 : STBIR_1CHANNEL;

    int max_slices = desc.num_slices;
    if (max_slices < 1)
        max_slices = 1;

    int w          = desc.width;
    int h          = desc.height * max_slices;
    int total_size = 0;

    int target_max_mipmap_levels = desc.num_mipmaps;
    if (target_max_mipmap_levels <= 0)
        target_max_mipmap_levels = SG_MAX_MIPMAPS;
    if (target_max_mipmap_levels > SG_MAX_MIPMAPS)
        target_max_mipmap_levels = SG_MAX_MIPMAPS;

    int max_mipmap_levels;
    for (max_mipmap_levels = 1; max_mipmap_levels < target_max_mipmap_levels; ++max_mipmap_levels)
    {
        w /= 2;
        h /= 2;

        if (w < 1 || h < 1)
            break;

        total_size += (w * h * num_channels);
    }

    unsigned char* big_target = XVG_MALLOC(total_size);
    unsigned char* target     = big_target;
    XVG_ASSERT(big_target);

    int target_width  = desc.width;
    int target_height = desc.height;
    int dst_height    = target_height * max_slices;

    for (int level = 1; level < max_mipmap_levels; ++level)
    {
        unsigned char* src = (unsigned char*)desc.data.mip_levels[level - 1].ptr;
        if (!src)
            break;

        int src_w      = target_width;
        int src_h      = target_height;
        target_width  /= 2;
        target_height /= 2;
        if (target_width < 1 && target_height < 1)
            break;

        if (target_width < 1)
            target_width = 1;

        if (target_height < 1)
            target_height = 1;

        dst_height              /= 2;
        unsigned       img_size  = target_width * dst_height * num_channels;
        unsigned char* dst       = target;

        XVG_ASSERT(dst < big_target + total_size);

        for (int slice = 0; slice < max_slices; ++slice)
        {
            stbir_resize_uint8_srgb(
                src,
                src_w,
                src_h,
                src_w * num_channels,
                dst,
                target_width,
                target_height,
                target_width * num_channels,
                layout_type);

            src += (src_w * src_h * num_channels);
            dst += (target_width * target_height * num_channels);
        }
        desc.data.mip_levels[level].ptr   = target;
        desc.data.mip_levels[level].size  = img_size;
        target                           += img_size;
        desc.num_mipmaps                  = level + 1;
    }
    XVG_ASSERT(desc.num_mipmaps == max_mipmap_levels);

    sg_image img = sg_make_image(&desc);
    XVG_FREE(big_target);
    return img;
}

uint32_t _xvg_compress_sdf_data(XVGShapeType shape_type, XVGColourType col_type, float feather, float stroke_width)
{
    XVG_ASSERT(stroke_width >= 0 && stroke_width < 16);
    xvecu compressed = {
        .r = shape_type,
        .g = col_type,
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

uint32_t _xvg_compress_arc_rotate_and_range(float rotate_turns, float range_turns)
{
    float rotate_norm = rotate_turns - floorf(rotate_turns);
    float range_norm  = range_turns - floorf(range_turns);
    XVG_ASSERT(rotate_norm >= 0 && rotate_norm <= 1);
    XVG_ASSERT(range_norm >= 0 && range_norm <= 1);
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

XVGCommand* _xvg_alloc_command(XVG* xvg, XVGCommandType type, const char* label)
{
    XVGCommand* cmd = linked_arena_alloc_clear(xvg->frame_arena, sizeof(*cmd));

    cmd->type  = type;
    cmd->label = label;

    if (xvg->first_command == NULL)
        xvg->first_command = cmd;

    if (xvg->current_command)
    {
        XVG_ASSERT(xvg->current_command->next == NULL);
        xvg->current_command->next = cmd;
    }

    xvg->current_command = cmd;

    return cmd;
}

void xvg_command_begin_pass(XVG* xvg, const sg_pass* pass, const char* label)
{
    XVGCommand*          cmd = _xvg_alloc_command(xvg, XVG_CMD_BEGIN_PASS, label);
    XVGCommandBeginPass* bp  = linked_arena_alloc_clear(xvg->frame_arena, sizeof(*bp));
    cmd->payload.beginPass   = bp;

    bp->pass = *pass;
}

void xvg_command_end_pass(XVG* xvg, const char* label)
{
    xvg_command_batch_draw(xvg, XVG_LABEL("xvg_command_end_pass()"));

    _xvg_alloc_command(xvg, XVG_CMD_END_PASS, label);
}

void xvg_command_set_scissor(XVG* xvg, int x, int y, int w, int h, const char* label)
{
    xvg_command_batch_draw(xvg, XVG_LABEL("xvg_command_set_scissor()"));

    XVGCommand*           cmd = _xvg_alloc_command(xvg, XVG_CMD_SET_SCISSOR, label);
    XVGCommandSetScissor* ss  = linked_arena_alloc_clear(xvg->frame_arena, sizeof(*ss));
    cmd->payload.scissor      = ss;

    ss->x = x;
    ss->y = y;
    ss->w = w;
    ss->h = h;
}

void xvg_command_set_viewport(XVG* xvg, int x, int y, int w, int h, const char* label)
{
    xvg_command_batch_draw(xvg, XVG_LABEL("xvg_command_set_viewport()"));

    XVGCommand*            cmd = _xvg_alloc_command(xvg, XVG_CMD_SET_VIEWPORT, label);
    XVGCommandSetViewport* sv  = linked_arena_alloc_clear(xvg->frame_arena, sizeof(*sv));
    cmd->payload.viewport      = sv;

    sv->x = x;
    sv->y = y;
    sv->w = w;
    sv->h = h;
}

void xvg_command_batch_draw(XVG* xvg, const char* label)
{
    if (xvg->draw_start.shape_buffer_start == xvg->shapes_buffer_len &&
        xvg->draw_start.text_buffer_start == xvg->text_buffer_len)
    {
        return; // nothing to draw
    }

    XVGCommand*     cmd  = _xvg_alloc_command(xvg, XVG_CMD_DRAW, label);
    XVGCommandDraw* draw = linked_arena_alloc_clear(xvg->frame_arena, sizeof(*draw));
    cmd->payload.draw    = draw;

    draw->shape_buffer_start = xvg->draw_start.shape_buffer_start;
    draw->shape_buffer_end   = xvg->shapes_buffer_len;
    draw->shape_texture = xvg->draw_start.shape_texture.id ? xvg->draw_start.shape_texture : xvg->fallback_img_view;
    draw->shape_sampler = xvg->draw_start.shape_sampler.id ? xvg->draw_start.shape_sampler : xvg->smp_nearest_neighbour;

    draw->text_buffer_start = xvg->draw_start.text_buffer_start;
    draw->text_buffer_end   = xvg->text_buffer_len;
    draw->text_texture      = xvg->draw_start.text_texture;

    xvg->draw_start.shape_buffer_start = xvg->shapes_buffer_len;
    xvg->draw_start.shape_texture.id   = 0;
    xvg->draw_start.shape_sampler.id   = 0;
    xvg->draw_start.text_buffer_start  = xvg->text_buffer_len;
    xvg->draw_start.text_texture.id    = 0;
}

void xvg_command_custom(XVG* xvg, void* uptr, XVGCustomFunc func, const char* label)
{
    xvg_command_batch_draw(xvg, XVG_LABEL("xvg_command_custom()"));

    XVGCommand*       cmd    = _xvg_alloc_command(xvg, XVG_CMD_CUSTOM, label);
    XVGCommandCustom* custom = linked_arena_alloc_clear(xvg->frame_arena, sizeof(*custom));
    cmd->payload.custom      = custom;

    custom->uptr = uptr;
    custom->func = func;
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

XVGGradient xvg_make_shadow(
    uint32_t col_stop_outer,
    uint32_t col_stop_inner,
    float    x_translate,
    float    y_translate,
    float    radius,
    float    spread,
    bool     is_inner_shadow)
{
    return (XVGGradient){
        .type       = is_inner_shadow ? XVG_COLOUR_INNER_SHADOW : XVG_COLOUR_DROP_SHADOW,
        .colour1    = col_stop_outer,
        .colour2    = col_stop_inner,
        .gradient_a = {x_translate, y_translate},
        .gradient_b = {radius, spread},
    };
}

void xvg_gradient_apply_image(
    XVGGradient* grad,
    sg_view      texture,
    sg_sampler   sampler,
    uint32_t     x,
    uint32_t     y,
    uint32_t     w,
    uint32_t     h)
{
    grad->xy      = x | (y << 16);
    grad->wh      = w | (h << 16);
    grad->texture = texture;
    grad->sampler = sampler;
}

// x/y/w/h are the coords of the image getting sampled
XVGGradient
xvg_make_image_fill(sg_view texture, sg_sampler sampler, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t sat)
{
    XVG_ASSERT(texture.id);
    XVG_ASSERT(sampler.id);
    return (XVGGradient){
        .xy      = x | (y << 16),
        .wh      = w | (h << 16),
        .texture = texture,
        .sampler = sampler,
        .colour1 = sat,
    };
}

void _xvg_set_bound_texture(XVG* xvg, const XVGGradient* grad)
{
    bool replace_texture = xvg->draw_start.shape_texture.id != grad->texture.id;
    bool replace_sampler = xvg->draw_start.shape_sampler.id != grad->sampler.id;
    if (replace_texture || replace_sampler)
    {
        xvg_command_batch_draw(xvg, XVG_LABEL("_xvg_set_bound_texture"));
        xvg->draw_start.shape_texture = grad->texture;
        xvg->draw_start.shape_sampler = grad->sampler;
    }
}

void xvg_draw_circle_with_gradient(XVG* xvg, float cx, float cy, float radius_px, float stroke_width, XVGGradient grad)
{
    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_CIRCLE_STROKE : XVG_SHAPE_CIRCLE_FILL;
    float        feather    = 2.0f / radius_px;

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft      = {cx - radius_px, cy - radius_px},
                    .bottomright  = {cx + radius_px, cy + radius_px},
                    .sdf_data     = _xvg_compress_sdf_data(shape_type, grad.type, feather, stroke_width),
                    .colour1      = grad.colour1,
                    .colour2      = grad.colour2,
                    .gradient_a   = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b   = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy = grad.xy,
                    .texcoords_wh = grad.wh,
    };
}
void xvg_draw_circle(XVG* xvg, float cx, float cy, float radius_px, float stroke_width, uint32_t col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_circle_with_gradient(xvg, cx, cy, radius_px, stroke_width, grad);
}

void xvg_draw_solid_rectangle_with_gradient(XVG* xvg, int x, int y, int width, int height, XVGGradient grad)
{
    _xvg_set_bound_texture(xvg, &grad);

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + width, y + height},
                    .sdf_data            = _xvg_compress_sdf_data(XVG_SHAPE_RECTANGLE, grad.type, 0, 0),
                    .borderradius_arcpie = 0,
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}

void xvg_draw_solid_rectangle(XVG* xvg, int x, int y, int width, int height, unsigned col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_solid_rectangle_with_gradient(xvg, x, y, width, height, grad);
}

void xvg_draw_rectangle_with_gradient_ex(
    XVG*        xvg,
    float       x,
    float       y,
    float       w,
    float       h,
    float       br_tr,
    float       br_br,
    float       br_tl,
    float       br_bl,
    float       stroke,
    XVGGradient grad)
{
    _xvg_set_bound_texture(xvg, &grad);
    XVGShapeType shape_type = stroke > 0 ? XVG_SHAPE_ROUNDED_RECTANGLE_STROKE : XVG_SHAPE_ROUNDED_RECTANGLE_FILL;

    // float feather = 4.0f / xm_minf(w, h);
    float feather = 4.0f / h;

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, grad.type, feather, stroke),
                    .borderradius_arcpie = _xvg_compress_border_radius(br_tr, br_br, br_tl, br_bl),

                    .colour1      = grad.colour1,
                    .colour2      = grad.colour2,
                    .gradient_a   = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b   = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy = grad.xy,
                    .texcoords_wh = grad.wh,
    };
}

void xvg_draw_rectangle(XVG* xvg, float x, float y, float w, float h, float br, float stroke_width, uint32_t col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_rectangle_with_gradient_ex(xvg, x, y, w, h, br, br, br, br, stroke_width, grad);
}

void xvg_draw_rectangle_with_gradient(
    XVG*        xvg,
    float       x,
    float       y,
    float       w,
    float       h,
    float       br,
    float       stroke_width,
    XVGGradient grad)
{
    xvg_draw_rectangle_with_gradient_ex(xvg, x, y, w, h, br, br, br, br, stroke_width, grad);
}

void xvg_draw_triangle_with_gradient(
    XVG*        xvg,
    float       x,
    float       y,
    float       w,
    float       h,
    float       rotate,
    float       stroke,
    XVGGradient grad)
{
    _xvg_set_bound_texture(xvg, &grad);
    XVGShapeType shape_type = stroke > 0 ? XVG_SHAPE_TRIANGLE_STROKE : XVG_SHAPE_TRIANGLE_FILL;
    float        feather    = 4.0f / xm_minf(w, h);

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, grad.type, feather, stroke),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(rotate, 0),
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}
void xvg_draw_triangle(XVG* xvg, float x, float y, float w, float h, float rotate, float stroke_width, uint32_t colour)
{
    XVGGradient grad = {.colour1 = colour};
    xvg_draw_triangle_with_gradient(xvg, x, y, w, h, rotate, stroke_width, grad);
}

void xvg_draw_pie_with_gradient(
    XVG*        xvg,
    float       cx,
    float       cy,
    float       radius_px,
    float       start_turn,
    float       end_turn,
    float       stroke_width,
    XVGGradient grad)
{
    _xvg_set_bound_texture(xvg, &grad);
    XVGShapeType shape_type   = stroke_width > 0 ? XVG_SHAPE_PIE_STROKE : XVG_SHAPE_PIE_FILL;
    float        feather      = 2.0f / radius_px;
    float        angle_range  = end_turn - start_turn;
    float        angle_rotate = (end_turn + start_turn);

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {cx - radius_px, cy - radius_px},
                    .bottomright         = {cx + radius_px, cy + radius_px},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, grad.type, feather, stroke_width),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(angle_rotate * 0.5f, angle_range * 0.5f),
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}

void xvg_draw_pie(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    start_turn,
    float    end_turn,
    float    stroke_width,
    uint32_t colour)
{
    XVGGradient grad = {.colour1 = colour};
    xvg_draw_pie_with_gradient(xvg, cx, cy, radius_px, start_turn, end_turn, stroke_width, grad);
}

void xvg_draw_arc_with_gradient(
    XVG*        xvg,
    float       cx,
    float       cy,
    float       radius_px,
    float       start_turn,
    float       end_turn,
    float       stroke_width,
    bool        butt,
    XVGGradient grad)
{
    _xvg_set_bound_texture(xvg, &grad);

    XVGShapeType shape_type = butt ? XVG_SHAPE_ARC_BUTT_STROKE : XVG_SHAPE_ARC_ROUND_STROKE;
    float        feather    = 2.0f / radius_px;

    float turn_low  = xm_minf(start_turn, end_turn);
    float turn_high = xm_maxf(start_turn, end_turn);

    float rotate_turns = turn_low;
    float range_turns  = turn_high - turn_low;

    rotate_turns = range_turns * 0.5f + rotate_turns;
    range_turns  = range_turns * 0.5f;
    if (butt)
    {
        range_turns  -= 0.25f;
        rotate_turns += 0.5f;
    }

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {cx - radius_px, cy - radius_px},
                    .bottomright         = {cx + radius_px, cy + radius_px},
                    .sdf_data            = _xvg_compress_sdf_data(shape_type, grad.type, feather, stroke_width * 0.5f),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(rotate_turns, range_turns),
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}

void xvg_draw_arc(
    XVG*     xvg,
    float    cx,
    float    cy,
    float    radius_px,
    float    start_turn,
    float    end_turn,
    float    stroke_width,
    bool     butt,
    uint32_t colour)
{
    XVGGradient grad = {.colour1 = colour};
    xvg_draw_arc_with_gradient(xvg, cx, cy, radius_px, start_turn, end_turn, stroke_width, butt, grad);
}

void xvg_draw_line_round_with_gradient(XVG* xvg, float x0, float y0, float x1, float y1, float stroke, XVGGradient grad)
{
    float xl = xm_minf(x0, x1) - stroke;
    float xr = xm_maxf(x0, x1) + stroke;
    float yt = xm_minf(y0, y1) - stroke;
    float yb = xm_maxf(y0, y1) + stroke;

    float feather = 4.0f / (yb - yt);

    xvecu stroke_offsets = {
        .r = x0 > x1 ? 255 : 0,
        .g = y0 > y1 ? 255 : 0,
        .b = x0 > x1 ? 0 : 255,
        .a = y0 > y1 ? 0 : 255,
    };

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {xl, yt},
                    .bottomright         = {xr, yb},
                    .sdf_data            = _xvg_compress_sdf_data(XVG_SHAPE_LINE_ROUND, grad.type, feather, stroke * 0.5f),
                    .borderradius_arcpie = stroke_offsets.u32,
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}

void xvg_draw_line_round(XVG* xvg, float x0, float y0, float x1, float y1, float stroke_width, unsigned col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_line_round_with_gradient(xvg, x0, y0, x1, y1, stroke_width, grad);
}

void xvg_draw_line_plot(
    XVG*         xvg,
    int          x,
    int          y,
    int          width,
    int          height,
    const float* data,
    float        crop_br,
    float        stroke_width,
    uint32_t     colour)
{
    size_t remaining_capacity = XVG_ARRLEN(xvg->line_buffer) - xvg->line_buffer_len;

    int N = width;

    if (N > remaining_capacity)
        N = 0;

    XVG_ASSERT(N > 0);
    if (N == 0)
        return;

    uint32_t end_idx = xvg->line_buffer_len + N;

    if (xvg->backingScaleFactor == 1)
    {
        xstatic_assert(sizeof(xvg->line_buffer[0]) == sizeof(data[0]), "Must match");
        memcpy(xvg->line_buffer + xvg->line_buffer_len, data, N * sizeof(xvg->line_buffer[0]));
    }
    else
    {
        // TODO: handle retina screens. Linear interpolation between points should be fine
        XVG_ASSERT(false);
    }

    XVG_ASSERT(end_idx >= 1);
    float feather = 4.0f / height;

    uint32_t line_buffer_range = (uint32_t)xvg->line_buffer_len | (end_idx << 16);

    xvg_shape_t* shape = _xvg_get_shape(xvg);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y - stroke_width * 0.25f},
                    .bottomright         = {x + width, y + height + stroke_width * 0.25},
                    .sdf_data            = _xvg_compress_sdf_data(XVG_SHAPE_LINE_PLOT, 0, feather, stroke_width),
                    .borderradius_arcpie = _xvg_compress_border_radius(crop_br, crop_br, crop_br, crop_br),
                    .colour1             = colour,
                    .buffer_idx_range    = line_buffer_range,
    };

    xvg->line_buffer_len += N;
    XVG_ASSERT(xvg->line_buffer_len <= XVG_ARRLEN(xvg->line_buffer));
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
            XVG_ASSERT(err == 0);
            if (err != 0)
                break;

            sl->data      = (void*)data;
            sl->data_size = datalen;
            sl->owned     = owned;

            FT_UInt  space_char = 32;
            FT_Fixed advance    = 0;
            err                 = FT_Get_Advance(sl->ft_face, space_char, FT_LOAD_NO_SCALE, &advance);
            XVG_ASSERT(err == 0);
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
    XVG_ASSERT(ok);
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
    XVG_ASSERT(img.id);

    XVGAtlas atlas = {.img_view = sg_make_view(&(sg_view_desc){.texture.image = img})};
    XVG_ASSERT(atlas.img_view.id);

    const size_t img_size = XVG_ATLAS_HEIGHT * XVG_ATLAS_ROW_STRIDE;
    atlas.img_data        = XVG_MALLOC(img_size);
    memset(atlas.img_data, 0, img_size);

    return atlas;
}

XVGAtlas* _xvg_get_current_font_atlas(XVG* xvg)
{
    XVG_ASSERT(xvg->text.current_atlas.idx < XVG_ARRLEN(xvg->text.atlases));
    return xvg->text.atlases + xvg->text.current_atlas.idx;
}

int _xvg_render_glyph(XVG* xvg, uint32_t glyph_index, unsigned font_size)
{
    int num_packed = 0;

    XVGAtlas*    atlas = _xvg_get_current_font_atlas(xvg);
    XVGFontSlot* sl    = _xvg_get_current_font_slot(xvg);

    if (!sl->ft_face || atlas->full)
        return num_packed;

    int err = FT_Load_Glyph(sl->ft_face, glyph_index, XVG_FT_LOAD);
    XVG_ASSERT(!err);

    const FT_GlyphSlot glyph = sl->ft_face->glyph;
    const FT_Bitmap*   bmp   = &glyph->bitmap;
    XVG_ASSERT(bmp->pixel_mode == XVG_FT_PIXEL_MODE);
    XVG_ASSERT((bmp->width % XVG_FT_BITMAP_CHANNELS) == 0); // note: FT width is measured in bytes (subpixels)
    // Note all glyphs have height/rows... (spaces?)
    if (bmp->width && bmp->rows)
    {
        int        width_pixels = bmp->width / XVG_FT_BITMAP_CHANNELS;
        stbrp_rect rect         = {.w = width_pixels + RECTPACK_PADDING, .h = bmp->rows + RECTPACK_PADDING};
        num_packed              = stbrp_pack_rects(&xvg->text.current_atlas.rectpack_ctx, &rect, 1);

        bool failed_to_pack = num_packed == 0;
        if (failed_to_pack)
        {
            atlas->full = true;

            bool can_create_new_atlas = (xvg->text.current_atlas.idx + 1) < XVG_ARRLEN(xvg->text.atlases);
            XVG_ASSERT(can_create_new_atlas);
            if (can_create_new_atlas) // atlas is full
            {
                // make new atlas
                xvg->text.current_atlas.idx++;
                xvg->text.atlases[xvg->text.current_atlas.idx] = _xvg_create_new_atlas();

                atlas = xvg->text.atlases + xvg->text.current_atlas.idx;

                // Clear rectpack
                memset(&xvg->text.current_atlas.rectpack_ctx, 0, sizeof(xvg->text.current_atlas.rectpack_ctx));
                stbrp_init_target(
                    &xvg->text.current_atlas.rectpack_ctx,
                    XVG_ATLAS_WIDTH - RECTPACK_PADDING,
                    XVG_ATLAS_HEIGHT - RECTPACK_PADDING,
                    xvg->text.current_atlas.nodes,
                    xarr_len(xvg->text.current_atlas.nodes));

                rect       = (stbrp_rect){.w = width_pixels + RECTPACK_PADDING, .h = bmp->rows + RECTPACK_PADDING};
                num_packed = stbrp_pack_rects(&xvg->text.current_atlas.rectpack_ctx, &rect, 1);
                XVG_ASSERT(num_packed == 1);
            }
        }

        if (num_packed)
        {
            int expected_height = glyph->metrics.height >> 6;
            XVG_ASSERT(expected_height == bmp->rows);
            XVG_ASSERT(glyph->bitmap_left >= -128 && glyph->bitmap_left < 127);
            XVG_ASSERT(glyph->bitmap_top >= -128 && glyph->bitmap_top < 127);
            XVGAtlasRect arect       = {0};
            arect.header.glyph_index = glyph_index;
            arect.header.font_size   = font_size;
            arect.bearing_x          = glyph->bitmap_left;
            arect.bearing_y          = glyph->bitmap_top;
            arect.x                  = rect.x + RECTPACK_PADDING;
            arect.y                  = rect.y + RECTPACK_PADDING;
            arect.w                  = width_pixels;
            arect.h                  = bmp->rows;
            arect.img_view           = atlas->img_view;
            XVG_ASSERT(glyph->advance.x < (1 << 15));
            XVG_ASSERT(glyph->advance.y < (1 << 15));
            arect.advance_x = glyph->advance.x + (glyph->lsb_delta - glyph->rsb_delta);
            arect.advance_y = glyph->advance.y;
            XVG_ASSERT(arect.x + arect.w < XVG_ATLAS_WIDTH);
            XVG_ASSERT(arect.y + arect.h < XVG_ATLAS_HEIGHT);

            XVG_ASSERT(bmp->pitch >= 0);

            xarr_push(xvg->text.rects, arect);

            for (int y = 0; y < bmp->rows; y++)
            {
#if defined(XVG_TEXT_SINGLECHANNEL)
                unsigned char* dst = atlas->img_data + (arect.y + y) * XVG_ATLAS_ROW_STRIDE + arect.x;
                unsigned char* src = bmp->buffer + y * bmp->pitch;

                unsigned char(*src_view)[512]  = (void*)src;
                src_view                      += 0;
                unsigned char(*dst_view)[512]  = (void*)dst;
                dst_view                      += 0;

                memcpy(dst, src, width_pixels);

                dst_view += 0;
#endif
#if defined(XVG_TEXT_MULTICHANNEL)
                unsigned char* dst =
                    atlas->img_data + (arect.y + y) * XVG_ATLAS_ROW_STRIDE + (arect.x) * XVG_GLYPH_ATLAS_CHANNELS;
                unsigned char* src = bmp->buffer + y * bmp->pitch;

                for (int x = 0; x < width_pixels; x++, dst += XVG_GLYPH_ATLAS_CHANNELS, src += XVG_FT_BITMAP_CHANNELS)
                {
                    int r  = src[0];
                    int g  = src[1];
                    int b  = src[2];
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
XVGAtlasRect _xvg_get_glyph(XVG* xvg, uint32_t glyph_index, unsigned font_size)
{
    const int num_rects = xarr_len(xvg->text.rects);

    XVGAtlasRectHeader header = {.glyph_index = glyph_index, .font_size = font_size};

    for (int j = 0; j < num_rects; j++)
    {
        if (xvg->text.rects[j].header.data == header.data)
        {
            XVGAtlasRect* rect = xvg->text.rects + j;
            XVG_ASSERT(rect->x + rect->w < XVG_ATLAS_WIDTH);
            return *rect;
        }
    }

    int did_raster = _xvg_render_glyph(xvg, glyph_index, font_size);
    if (did_raster)
    {
        XVG_ASSERT(num_rects + 1 == xarr_len(xvg->text.rects));
        XVGAtlasRect* rect = xvg->text.rects + num_rects;
        XVG_ASSERT(rect->x + rect->w < XVG_ATLAS_WIDTH);
        return *rect;
    }

    // Note: this stub has a texture view id of 0
    // sokol_gfx should assert in debug mode when trying to bind a texture view with an id of 0
    // In release it should skip all draws using that view. This is our desired behaviour
    static XVGAtlasRect stub = {0};
    memset(&stub, 0, sizeof(stub));
    return stub;
}

xvg_text_t* _xvg_get_text(XVG* xvg)
{
    static xvg_text_t stub;

    xvg_text_t* ret =
        xvg->text_buffer_len < XVG_ARRLEN(xvg->text_buffer) ? &xvg->text_buffer[xvg->text_buffer_len] : &stub;

    xvg->text_buffer_len++;
    if (xvg->text_buffer_len > XVG_ARRLEN(xvg->text_buffer))
        xvg->text_buffer_len = XVG_ARRLEN(xvg->text_buffer);
    return ret;
}

bool _xvg_push_glyph(XVG* xvg, int pen_x, int pen_y, const XVGAtlasRect* rect, uint32_t colour)
{
    bool should_push = rect->img_view.id != 0;
    XVG_ASSERT(should_push);
    if (should_push)
    {
        if (xvg->draw_start.text_texture.id != 0 && xvg->draw_start.text_texture.id != rect->img_view.id)
            xvg_command_batch_draw(xvg, XVG_LABEL("_xvg_push_glyph"));
        if (xvg->draw_start.text_texture.id == 0)
            xvg->draw_start.text_texture = rect->img_view;

        xvg_text_t* obj = _xvg_get_text(xvg);
        // obj->topleft[0] = pen_x + (int)rect->bearing_x; // If using floating point coords
        // obj->topleft[1] = pen_y - (int)rect->bearing_y;
        obj->topleft      = _xvg_pack_xy_coord(pen_x + (int)rect->bearing_x, pen_y - (int)rect->bearing_y);
        obj->atlas_coords = (xvecu){.r = rect->x, .g = rect->y, .b = rect->w, .a = rect->h}.u32;
        obj->colour       = colour;

        xvg->text_buffer_len++;

        XVG_ASSERT(xvg->draw_start.text_texture.id != 0);
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
    unsigned    font_size,
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

    XVG_ASSERT(sl->space_advance);
    const int64_t space_advance = FT_MulFix(sl->space_advance, m->x_scale) / 2;

    // Clang-cl makes INT64_MAX a negative integer if we aren't super explicit with types here
    const int64_t break_row_x = break_width != 0 ? (int64_t)(break_width * 64) : (int64_t)INT64_MAX;
    XVG_ASSERT(break_row_x >= 0);

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
            // char     c         = cp;
            unsigned glyph_idx = FT_Get_Char_Index(face, cp);
            XVG_ASSERT(glyph_idx != 0);

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
            XVG_ASSERT(rect.advance_x > 0);

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
                XVG_ASSERT(layout->num_rows);
                XVG_ASSERT(num_glyphs_at_last_space <= layout->num_glyphs);
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
                    XVG_ASSERT(layout->num_rows >= 2);
                    XVGGlyphLayout* break_glyph = &glyphs[num_glyphs_at_last_space - 1];

                    end_idx           = prev_row->end_idx;
                    prev_row->end_idx = num_glyphs_at_last_space;
                    prev_row->xmax    = break_glyph->x + break_glyph->rect.w;

                    XVG_ASSERT(prev_row->begin_idx <= prev_row->end_idx);
                    layout_xmax = xm_maxi(layout_xmax, prev_row->xmax);
                }
                current_row->begin_idx = num_glyphs_at_last_space;
                current_row->end_idx   = end_idx > 0 ? end_idx : layout->num_glyphs;
                XVG_ASSERT(current_row->begin_idx <= current_row->end_idx);

                if (current_row->begin_idx < layout->num_glyphs)
                {
                    const int offset_x = glyphs[current_row->begin_idx].x;
                    for (int i = current_row->begin_idx; i < layout->num_glyphs; i++)
                    {
                        XVGGlyphLayout* gp = &glyphs[i];
                        // Apply offsets to glyphs on new line
                        gp->x -= offset_x;
                        gp->y  = (CursorY + line_height) >> 6;
                        XVG_ASSERT(gp->x >= 0);

                        // recalculate row stats
                        line_ymax = xm_maxi(line_ymax, gp->rect.bearing_y);
                        line_ymin = xm_mini(line_ymin, gp->rect.bearing_y - rect.h);
                        line_xmax = xm_maxi(line_xmax, gp->x + gp->rect.w);
                    }
                }
                layout_xmax = xm_maxi(layout_xmax, line_xmax);

                CursorX  = CursorX_after_last_space > 0 ? (CursorX - CursorX_after_last_space) : 0;
                CursorY += line_height;
                XVG_ASSERT(CursorX >= 0);

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

void xvg_draw_text_ex(
    XVG*        xvg,
    float       x,
    float       y,
    const char* text_start,
    const char* text_end,
    unsigned    font_size,
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
    unsigned    font_size,
    XVGAlign    alignment,
    uint32_t    colour)
{
    xvg_draw_text_ex(xvg, x, y, text_start, text_end, font_size, alignment, colour, 0, 1);
}

void xvg_init(XVG* xvg)
{
    xvg->arena       = linked_arena_create_ex(0, 1024 * 64);
    xvg->frame_arena = linked_arena_create_ex(0, 1024 * 64);

    xvg->backingScaleFactor = 1;

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

        // fallback_img
        static const uint32_t pixel_white = 0xffffffff;

        xvg->fallback_img      = sg_make_image(&(sg_image_desc){
                 .width              = 1,
                 .height             = 1,
                 .pixel_format       = SG_PIXELFORMAT_RGBA8,
                 .data.mip_levels[0] = {
                     .ptr  = &pixel_white,
                     .size = sizeof(pixel_white),
            }});
        xvg->fallback_img_view = sg_make_view(&(sg_view_desc){.texture = xvg->fallback_img});

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

        xvg->shapes.line_sbo = sg_make_buffer(&(sg_buffer_desc){
            .usage.storage_buffer = true,
            .usage.stream_update  = true,
            .size                 = sizeof(xvg->line_buffer),
            .label                = XVG_LABEL("xvg-line-buffer"),
        });
        xvg->shapes.line_sbv = sg_make_view(&(sg_view_desc){.storage_buffer = xvg->shapes.line_sbo});
    }

    // Text
    {
        int ft_err = FT_Init_FreeType(&xvg->text.ft_lib);
        XVG_ASSERT(ft_err == 0);

        xvg->text.sbo = sg_make_buffer(&(sg_buffer_desc){
            .usage.storage_buffer = true,
            .usage.stream_update  = true,
            .size                 = sizeof(xvg->text_buffer),
            .label                = "text SBO",
        });
        XVG_ASSERT(xvg->text.sbo.id);
        xvg->text.sbv = sg_make_view(&(sg_view_desc){
            .storage_buffer = xvg->text.sbo,
        });
        XVG_ASSERT(xvg->text.sbv.id);

        xvg->text.pip = sg_make_pipeline(&(sg_pipeline_desc){
#if defined(XVG_TEXT_MULTICHANNEL)
            .shader    = sg_make_shader(_xvg_text_multichannel_shader_desc(sg_query_backend())),
            .colors[0] = BLEND_DUAL_SOURCE,
#endif
#if defined(XVG_TEXT_SINGLECHANNEL)
            .shader    = sg_make_shader(_xvg_text_singlechannel_shader_desc(sg_query_backend())),
            .colors[0] = BLEND_DEFAULT,
#endif
            .label = XVG_LABEL("xvg-text")});
        XVG_ASSERT(xvg->text.pip.id);

        xarr_setcap(xvg->text.rects, 256);
        xarr_setlen(xvg->text.rects, 0);
        xvg->text.atlases[0]        = _xvg_create_new_atlas();
        xvg->text.current_atlas.idx = 0;
        xarr_setlen(xvg->text.current_atlas.nodes, (XVG_ATLAS_WIDTH * 2));

        stbrp_init_target(
            &xvg->text.current_atlas.rectpack_ctx,
            XVG_ATLAS_WIDTH - RECTPACK_PADDING,
            XVG_ATLAS_HEIGHT - RECTPACK_PADDING,
            xvg->text.current_atlas.nodes,
            xarr_len(xvg->text.current_atlas.nodes));
    }
}

void xvg_deinit(XVG* xvg)
{
    xarr_free(xvg->text.rects);
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
    for (int i = 0; i < XVG_ARRLEN(xvg->text.atlases); i++)
    {
        XVGAtlas* atlas = &xvg->text.atlases[i];
        if (atlas->img_data)
        {
            XVG_FREE(atlas->img_data);
        }
    }
    FT_Done_FreeType(xvg->text.ft_lib);

    linked_arena_destroy(xvg->frame_arena);
    linked_arena_destroy(xvg->arena);
}

void xvg_begin_frame(XVG* xvg)
{
    // Oh no, you forgot to call xvg_end_frame()
    // Or perhaps you called a function that caused you to continue processing in your or the OS's event loop
    XVG_ASSERT(xvg->arena_top == NULL);
    xvg->arena_top = linked_arena_get_top(xvg->arena);
    linked_arena_clear(xvg->frame_arena);

    xvg->first_command   = NULL;
    xvg->current_command = NULL;
    memset(&xvg->draw_start, 0, sizeof(xvg->draw_start));

    xvg->text.current_font_idx = 0;
    xvg->shapes_buffer_len     = 0;
    xvg->line_buffer_len       = 0;
    xvg->text_buffer_len       = 0;
}

void xvg_end_frame(XVG* xvg, int window_width, int window_height)
{
    xvg_command_batch_draw(xvg, XVG_LABEL("xvg_end_frame"));

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
        sg_update_buffer(xvg->shapes.line_sbo, &range);
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
                        .ptr  = atlas->img_data,
                        .size = XVG_ATLAS_HEIGHT * XVG_ATLAS_ROW_STRIDE,
                    }});
            atlas->dirty = false;
        }
    }

    // Process commands
    int         ncommands = 0;
    XVGCommand* cmd       = xvg->first_command;
    while (cmd != NULL)
    {
        switch (cmd->type)
        {
        case XVG_CMD_BEGIN_PASS:
        {
            XVGCommandBeginPass* p = cmd->payload.beginPass;
            sg_begin_pass(&p->pass);
            break;
        }
        case XVG_CMD_END_PASS:
            sg_end_pass();
            break;
        case XVG_CMD_SET_SCISSOR:
        {
            XVGCommandSetScissor* s = cmd->payload.scissor;
            sg_apply_scissor_rect(s->x, s->y, s->w, s->h, true);
            break;
        }
        case XVG_CMD_SET_VIEWPORT:
        {
            XVGCommandSetViewport* s = cmd->payload.viewport;
            sg_apply_viewport(s->x, s->y, s->w, s->h, true);
            break;
        }
        case XVG_CMD_CUSTOM:
        {
            XVGCommandCustom* custom = cmd->payload.custom;
            custom->func(custom->uptr);
            break;
        }
        case XVG_CMD_DRAW:
        {
            XVGCommandDraw* draw = cmd->payload.draw;

            const int num_shapes = draw->shape_buffer_end - draw->shape_buffer_start;
            XVG_ASSERT(num_shapes >= 0);
            if (num_shapes)
            {
                sg_apply_pipeline(xvg->shapes.pip);

                sg_apply_bindings(&(sg_bindings){
                    .views[VIEW_vs_xvg_shapes_buffer]      = xvg->shapes.sbv,
                    .views[VIEW_fs_xvg_shapes_line_buffer] = xvg->shapes.line_sbv,
                    .views[VIEW_fs_xvg_shapes_tex]         = draw->shape_texture,
                    .samplers[SMP_fs_xvg_shapes_smp]       = draw->shape_sampler,
                });

                sg_view_desc             view_desc  = sg_query_view_desc(draw->shape_texture);
                sg_image_desc            img_desc   = sg_query_image_desc(view_desc.texture.image);
                unsigned                 img_width  = img_desc.width;
                unsigned                 img_height = img_desc.height;
                vs_xvg_shapes_uniforms_t uniforms   = {
                      .u_size                  = {window_width, window_height},
                      .u_texture_size          = {img_width, img_height},
                      .u_storage_buffer_offset = draw->shape_buffer_start,
                };
                sg_apply_uniforms(UB_vs_xvg_shapes_uniforms, &SG_RANGE(uniforms));
                sg_draw(0, 6 * num_shapes, 1);
            }

            const int num_text = draw->text_buffer_end - draw->text_buffer_start;
            XVG_ASSERT(num_text >= 0);
            if (num_text)
            {
                XVG_ASSERT(draw->text_texture.id != 0); // You forgot to set the current texture
                sg_apply_pipeline(xvg->text.pip);
                sg_apply_bindings(&(sg_bindings){
                    .views[VIEW_vs_xvg_text_buffer] = xvg->text.sbv,
                    .views[VIEW_fs_xvg_text_tex]    = draw->text_texture,
                    .samplers[SMP_fs_xvg_text_smp]  = xvg->smp_nearest_neighbour,
                });

                vs_xvg_text_uniforms_t uniforms = {
                    .u_view_size  = {window_width * xvg->backingScaleFactor, window_height * xvg->backingScaleFactor},
                    .u_sbo_offset = draw->text_buffer_start,
                };
                sg_apply_uniforms(UB_vs_xvg_text_uniforms, &SG_RANGE(uniforms));

                sg_draw(0, 6 * num_text, 1);
            }
            break;
        }
        }

        cmd = cmd->next;
        ncommands++;
    }

    linked_arena_release(xvg->arena, xvg->arena_top);
    xvg->arena_top = NULL;
}

#endif // XVG_IMPL