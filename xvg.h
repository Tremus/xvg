#ifndef XVG_H
#define XVG_H

#include "sokol_gfx.h"
#include "xvg_shaders.glsl.h"
#include <linked_arena.h>
#include <stb_rect_pack.h>

/*
// TODO: increase max stroke width for line plots
// TODO: support fallback fonts for missing glyphs
// TODO: support intelligent batching of text when using multiple atlases
         The simplest solution is probably to bind all atlases at once at include an atlas index to put in a switch case
         in the text shader
// TODO: finish implementing all text alignment stuff, namely multiline centre & right alignment. This will be easy once
         all text atlases are bound together
// TODO: support colour gradients for text
// TODO: provide a simple atlas abstraction so users can render icons with something like nanosvg into the atlas
// TODO: provide functions for clearing atlases. Possibly useful when resizing windows
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
    // WARNING: this values are scaled accorting to xcl->backingScaleFactor
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

    float backingScaleFactor;

    struct FT_LibraryRec_* ft_lib;
    XVGAtlasRect*          atlas_rects;

    struct
    {
        int           idx;
        stbrp_context rectpack_ctx;
        stbrp_node*   nodes;
    } current_atlas;

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

    sg_sampler smp_linear;
    sg_sampler smp_nearest_neighbour;
    sg_image   fallback_img;
    sg_view    fallback_img_view;

    sg_pipeline pip_shapes;
    sg_pipeline pip_text;
} XVG;

// Commands
typedef struct XVGCommandBeginPass
{
    int pass_idx;
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
    sg_view    shape_texture[4];
    sg_sampler shape_sampler;

    int     text_buffer_start;
    int     text_buffer_end;
    sg_view text_texture;
} XVGCommandDraw;

typedef void (*XVGCustomFunc)(void* uptr);

typedef struct XVGCommandCustom
{
    void*         uptr;
    XVGCustomFunc func;
} XVGCommandCustom;

typedef enum XVGCommandType
{
    XVG_CMD_NONE,
    XVG_CMD_BEGIN_PASS,
    XVG_CMD_END_PASS,
    XVG_CMD_SET_SCISSOR,
    XVG_CMD_SET_VIEWPORT,
    XVG_CMD_DRAW,
    XVG_CMD_CUSTOM,
} XVGCommandType;

// TODO: Make this a big fat union and store everything in a fixed size array
//       Replace the sg_pass field in XVGCommandBeginPass with an index into a fixed size array of sg_pass
typedef struct XVGCommand
{
    XVGCommandType type;
    const char*    label;

    union
    {
        void* data;

        XVGCommandBeginPass   beginPass;
        XVGCommandDraw        draw;
        XVGCommandSetScissor  scissor;
        XVGCommandSetViewport viewport;
        XVGCommandCustom      custom;
    };

    int next_idx;
} XVGCommand;

typedef struct XVGCommandList
{
    XVG*         xvg;   // not owned
    LinkedArena* arena; // not owned

    sg_buffer shapes_sbo;
    sg_view   shapes_sbv;

    sg_view   lines_sbv;
    sg_buffer lines_sbo; // normalised y values

    sg_buffer text_sbo;
    sg_view   text_sbv;

    struct
    {
        int first_command_idx;
        int last_command_idx;

        XVGCommandDraw draw;

        unsigned num_commands;
        unsigned num_passes;
        unsigned num_shapes;
        unsigned num_line_segments;
        unsigned num_text;
    } frame;

#ifndef XVG_COMMANDS_CAPACITY
#define XVG_COMMANDS_CAPACITY 128
#endif
#ifndef XVG_PASS_CAPACITY
#define XVG_PASS_CAPACITY 16
#endif
#ifndef XVG_SHAPES_CAPACITY
#define XVG_SHAPES_CAPACITY 1024
#endif
#ifndef XVG_LINE_SEGMENTS_CAPACITY
#define XVG_LINE_SEGMENTS_CAPACITY 8192
#endif
#ifndef XVG_TEXT_CAPACITY
#define XVG_TEXT_CAPACITY 1024
#endif

    XVGCommand commands[XVG_COMMANDS_CAPACITY];
    sg_pass    passes[XVG_PASS_CAPACITY];

    xvg_shape_t        shapes[XVG_SHAPES_CAPACITY];
    xvg_line_segment_t line_segments[XVG_LINE_SEGMENTS_CAPACITY];
    xvg_text_t         text[XVG_TEXT_CAPACITY];
} XVGCommandList;

void xvg_init(XVG*);
void xvg_deinit(XVG*);

// num_xxx sets the max length of the respective buffers
// Pass num_xxx = 0 to set defaults
XVGCommandList* xvg_command_list_create(XVG*);

void xvg_command_begin_pass(XVGCommandList*, const sg_pass*, const char* label);
void xvg_command_end_pass(XVGCommandList*, const char* label);
void xvg_command_set_scissor(XVGCommandList*, int x, int y, int w, int h, const char* label);
void xvg_command_set_viewport(XVGCommandList*, int x, int y, int w, int h, const char* label);
void xvg_command_batch_draw(XVGCommandList*, const char* label);
void xvg_command_custom(XVGCommandList*, void* uptr, XVGCustomFunc func, const char* label);

void xvg_begin_frame(XVG*);
void xvg_end_frame(XVG*);
void xvg_command_list_begin_frame(XVGCommandList* xcl);
void xvg_command_list_end_frame(XVGCommandList* xcl, int window_width, int window_height);

// Shapes
// Unlike canvas style APIs, there are no 'fill' and 'stroke' commands. If 'stroke_width' is 0 the shape is implicitly
// filled. Values are in pixels. Stroking is done on the INSIDE of the shape, so you always get accurate widths
// Strokes have a maximum width of 15 with 2 exceptions: line plots (2px max), arcs & rounded line_segments (31px max)
// 'radius' is in pixels
// 'angle' and 'rotate' are in the less convential but greatly superior 'turns', which is normalised 2pi.
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
void xvg_draw_solid_rectangle(XVGCommandList*, int x, int y, int width, int height, unsigned col);
void xvg_draw_solid_rectangle_with_gradient(XVGCommandList*, int x, int y, int width, int height, XVGGradient grad);

// Soft corners & edges
void xvg_draw_rectangle(XVGCommandList*, float x, float y, float w, float h, float br, float stroke_px, uint32_t col);
void xvg_draw_rectangle_with_gradient(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           br,
    float           stroke,
    XVGGradient     grad);
void xvg_draw_rectangle_with_gradient_ex(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           br_tr,
    float           br_br,
    float           br_tl,
    float           br_bl,
    float           stroke,
    XVGGradient     grad);

void xvg_draw_circle(XVGCommandList*, float cx, float cy, float radius_px, float stroke_width, uint32_t col);
void xvg_draw_circle_with_gradient(XVGCommandList*, float cx, float cy, float radius_px, float sw, XVGGradient grad);

// Equilateral triangle
void xvg_draw_triangle(
    XVGCommandList*,
    float    x,
    float    y,
    float    w,
    float    h,
    float    rotate,
    float    stroke_px,
    uint32_t col);
void xvg_draw_triangle_with_gradient(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           rotate,
    float           stroke,
    XVGGradient     grad);

void xvg_draw_pie(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           angle_start,
    float           angle_end,
    float           stroke_px,
    uint32_t        col);
void xvg_draw_pie_with_gradient(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           angle_start,
    float           angle_end,
    float           stroke_px,
    XVGGradient     grad);

void xvg_draw_arc(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           start_turn,
    float           end_turn,
    float           stroke_px,
    bool            butt,
    uint32_t        col);
void xvg_draw_arc_with_gradient(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           start_turn,
    float           end_turn,
    float           stroke_px,
    bool            butt,
    XVGGradient     grad);

void xvg_draw_line_round_with_gradient(
    XVGCommandList* xcl,
    float           x0,
    float           y0,
    float           x1,
    float           y1,
    float           stroke,
    XVGGradient     grad);
void xvg_draw_line_round(XVGCommandList* xcl, float x0, float y0, float x1, float y1, float stroke_width, unsigned col);

// 'data' is expected to be an array of 'width' length
// 'data' is expected to contain normalised values where 0 == (y + height), and 1 == y
// 'data' is allowed to go beyond [0-1], it will just get cropped
// 'stroke_px' is limited to the range [1-2]
// 'br' crops the line at the corners of the rectangle you're drawing
void xvg_draw_line_plot(
    XVGCommandList* xcl,
    int             x,
    int             y,
    int             w,
    int             h,
    const float*    data,
    float           br,
    float           stroke_px,
    uint32_t        col);

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
    XVGCommandList* xcl,
    float           x,
    float           y,
    const char*     text_begin,
    const char*     text_end,
    unsigned        font_size,
    XVGAlign        align,
    uint32_t        col);
void xvg_draw_text_ex(
    XVGCommandList* xcl,
    float           x,
    float           y,
    const char*     text_start,
    const char*     text_end,
    unsigned        font_size,
    XVGAlign        alignment,
    uint32_t        colour,
    float           break_width,
    float           line_height);

const XVGTextLayout* xvg_create_text_layout(
    XVGCommandList* xcl,
    const char*     text_start,
    const char*     text_end,
    unsigned        font_size,
    float           break_width,
    float           _line_height);
static void xvg_release_text_layout(XVGCommandList* xcl, const XVGTextLayout* layout)
{
    linked_arena_release(xcl->arena, layout);
};
void xvg_draw_text_layout(XVGCommandList* xcl, const XVGTextLayout* layout, int x, int y, int align, uint32_t col);

static unsigned xvg_colour_set_alpha_u8(unsigned col, unsigned char alpha) { return (col & 0xffffff00) | alpha; }
static unsigned xvg_colour_set_alpha_f32(unsigned col, float alpha)
{
    int alphai32 = alpha * 255;
    if (alphai32 < 0)
        alphai32 = 0;
    if (alphai32 > 255)
        alphai32 = 255;
    return xvg_colour_set_alpha_u8(col, (unsigned char)alphai32);
}

// #ifdef __cplusplus
// }
// #endif

#endif // XVG_H

#ifdef XVG_IMPL
#undef XVG_IMPL

#include <string.h>
#include <utf8.h>
#include <xhl/array2.h>
#include <xhl/debug.h>
#include <xhl/files.h>
#include <xhl/maths.h>
#include <xhl/vector.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_ADVANCES_H

#if !defined(XVG_MALLOC) || !defined(XVG_REALLOC) || !defined(XVG_FREE)
#include <stdlib.h>
#define XVG_MALLOC(sz)       malloc(sz)
#define XVG_REALLOC(ptr, sz) realloc(ptr, sz)
#define XVG_FREE(ptr)        free(ptr)
#endif

uint32_t _xvg_compress_sdf_data(
    unsigned      tex_idx,
    XVGShapeType  shape_type,
    XVGColourType col_type,
    float         feather,
    float         stroke_width)
{
    XVG_ASSERT(stroke_width >= 0 && stroke_width < 16);
    xvecu compressed = {
        .r = tex_idx,
        .g = shape_type | (col_type << 4),
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

XVGCommand* _xvg_get_command(XVGCommandList* xcl, XVGCommandType type, const char* label)
{
    int         idx = ++xcl->frame.num_commands < XVG_ARRLEN(xcl->commands) ? xcl->frame.num_commands : 0;
    XVGCommand* cmd = xcl->commands + idx;
    memset(cmd, 0, sizeof(*cmd));

    cmd->type  = type;
    cmd->label = label;

    if (xcl->frame.first_command_idx == 0)
        xcl->frame.first_command_idx = idx;

    if (xcl->frame.last_command_idx)
    {
        XVG_ASSERT(xcl->commands[xcl->frame.last_command_idx].next_idx == 0);
        xcl->commands[xcl->frame.last_command_idx].next_idx = idx;
    }

    xcl->frame.last_command_idx = idx;

    return cmd;
}

void xvg_command_begin_pass(XVGCommandList* xcl, const sg_pass* pass, const char* label)
{
    XVGCommand* cmd = _xvg_get_command(xcl, XVG_CMD_BEGIN_PASS, label);
    int         idx = ++xcl->frame.num_passes < XVG_ARRLEN(xcl->passes) ? xcl->frame.num_passes : 0;
    if (idx != 0)
    {
        xcl->passes[idx] = *pass;
    }
    cmd->beginPass.pass_idx = idx;
}

void xvg_command_end_pass(XVGCommandList* xcl, const char* label)
{
    xvg_command_batch_draw(xcl, XVG_LABEL("xvg_command_end_pass()"));

    _xvg_get_command(xcl, XVG_CMD_END_PASS, label);
}

void xvg_command_set_scissor(XVGCommandList* xcl, int x, int y, int w, int h, const char* label)
{
    xvg_command_batch_draw(xcl, XVG_LABEL("xvg_command_set_scissor()"));

    XVGCommand*           cmd = _xvg_get_command(xcl, XVG_CMD_SET_SCISSOR, label);
    XVGCommandSetScissor* ss  = &cmd->scissor;

    ss->x = x;
    ss->y = y;
    ss->w = w;
    ss->h = h;
}

void xvg_command_set_viewport(XVGCommandList* xcl, int x, int y, int w, int h, const char* label)
{
    xvg_command_batch_draw(xcl, XVG_LABEL("xvg_command_set_viewport()"));

    XVGCommand*            cmd = _xvg_get_command(xcl, XVG_CMD_SET_VIEWPORT, label);
    XVGCommandSetViewport* sv  = &cmd->viewport;

    sv->x = x;
    sv->y = y;
    sv->w = w;
    sv->h = h;
}

void xvg_command_batch_draw(XVGCommandList* xcl, const char* label)
{
    if (xcl->frame.draw.shape_buffer_start == xcl->frame.num_shapes &&
        xcl->frame.draw.text_buffer_start == xcl->frame.num_text)
    {
        return; // nothing to draw
    }

    XVGCommand*     cmd  = _xvg_get_command(xcl, XVG_CMD_DRAW, label);
    XVGCommandDraw* draw = &cmd->draw;

    *draw = xcl->frame.draw;
    memset(&xcl->frame.draw, 0, sizeof(xcl->frame.draw));

    draw->shape_buffer_end = xcl->frame.num_shapes;
    draw->text_buffer_end  = xcl->frame.num_text;

    xcl->frame.draw.shape_buffer_start = xcl->frame.num_shapes;
    xcl->frame.draw.text_buffer_start  = xcl->frame.num_text;
}

void xvg_command_custom(XVGCommandList* xcl, void* uptr, XVGCustomFunc func, const char* label)
{
    xvg_command_batch_draw(xcl, XVG_LABEL("xvg_command_custom()"));

    XVGCommand*       cmd    = _xvg_get_command(xcl, XVG_CMD_CUSTOM, label);
    XVGCommandCustom* custom = &cmd->custom;

    custom->uptr = uptr;
    custom->func = func;
}

// ███████╗██╗  ██╗ █████╗ ██████╗ ███████╗███████╗
// ██╔════╝██║  ██║██╔══██╗██╔══██╗██╔════╝██╔════╝
// ███████╗███████║███████║██████╔╝█████╗  ███████╗
// ╚════██║██╔══██║██╔══██║██╔═══╝ ██╔══╝  ╚════██║
// ███████║██║  ██║██║  ██║██║     ███████╗███████║
// ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚══════╝╚══════╝

xvg_shape_t* _xvg_get_shape(XVGCommandList* xcl)
{
    // Branchless
    static xvg_shape_t stub;

    XVG_ASSERT(xcl->frame.num_shapes < XVG_ARRLEN(xcl->shapes));
    xvg_shape_t* ret = xcl->frame.num_shapes < XVG_ARRLEN(xcl->shapes) ? &xcl->shapes[xcl->frame.num_shapes] : &stub;

    xcl->frame.num_shapes++;
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

XVGGradient xvg_make_conic_gradient(uint32_t col_stop_1, uint32_t col_stop_2, float turn_stop_1, float turn_stop_2)
{
    float angle = 0.5f + turn_stop_1;
    float range = turn_stop_2 - turn_stop_1;
    return (XVGGradient){
        .type       = XVG_COLOUR_CONIC_GRADIENT,
        .colour1    = col_stop_1,
        .colour2    = col_stop_2,
        .gradient_a = {angle, angle},
        .gradient_b = {range, range},
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

// Returns tex_idx
unsigned _xvg_set_bound_texture(XVGCommandList* xcl, const XVGGradient* grad)
{
    if (grad->texture.id == 0)
        return 0;

    unsigned i = 0;
    for (; i < XVG_ARRLEN(xcl->frame.draw.shape_texture); i++)
    {
        if (xcl->frame.draw.shape_texture[i].id == grad->texture.id)
            break;
        if (xcl->frame.draw.shape_texture[i].id == 0)
        {
            xcl->frame.draw.shape_texture[i] = grad->texture;
            break;
        }
    }

    bool tex_full = i == XVG_ARRLEN(xcl->frame.draw.shape_texture);
    // bool replace_texture = xcl->frame.draw.shape_texture.id != grad->texture.id;
    bool replace_sampler =
        xcl->frame.draw.shape_sampler.id != 0 && xcl->frame.draw.shape_sampler.id != grad->sampler.id;
    if (tex_full || replace_sampler)
    {
        xvg_command_batch_draw(xcl, XVG_LABEL("_xvg_set_bound_texture"));

        i                                = 0;
        xcl->frame.draw.shape_texture[i] = grad->texture;
    }

    xcl->frame.draw.shape_sampler = grad->sampler;

    XVG_ASSERT(i < XVG_ARRLEN(xcl->frame.draw.shape_texture));
    return i + 1;
}

void xvg_draw_circle_with_gradient(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           stroke_width,
    XVGGradient     grad)
{
    unsigned tex_idx = _xvg_set_bound_texture(xcl, &grad);

    XVGShapeType shape_type = stroke_width > 0 ? XVG_SHAPE_CIRCLE_STROKE : XVG_SHAPE_CIRCLE_FILL;
    float        feather    = 2.0f / radius_px;

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft      = {cx - radius_px, cy - radius_px},
                    .bottomright  = {cx + radius_px, cy + radius_px},
                    .sdf_data     = _xvg_compress_sdf_data(tex_idx, shape_type, grad.type, feather, stroke_width),
                    .colour1      = grad.colour1,
                    .colour2      = grad.colour2,
                    .gradient_a   = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b   = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy = grad.xy,
                    .texcoords_wh = grad.wh,
    };
}
void xvg_draw_circle(XVGCommandList* xcl, float cx, float cy, float radius_px, float stroke_width, uint32_t col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_circle_with_gradient(xcl, cx, cy, radius_px, stroke_width, grad);
}

void xvg_draw_solid_rectangle_with_gradient(XVGCommandList* xcl, int x, int y, int width, int height, XVGGradient grad)
{
    unsigned tex_idx = _xvg_set_bound_texture(xcl, &grad);

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + width, y + height},
                    .sdf_data            = _xvg_compress_sdf_data(tex_idx, XVG_SHAPE_RECTANGLE, grad.type, 0, 0),
                    .borderradius_arcpie = 0,
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}

void xvg_draw_solid_rectangle(XVGCommandList* xcl, int x, int y, int width, int height, unsigned col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_solid_rectangle_with_gradient(xcl, x, y, width, height, grad);
}

void xvg_draw_rectangle_with_gradient_ex(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           br_tr,
    float           br_br,
    float           br_tl,
    float           br_bl,
    float           stroke,
    XVGGradient     grad)
{
    unsigned     tex_idx    = _xvg_set_bound_texture(xcl, &grad);
    XVGShapeType shape_type = stroke > 0 ? XVG_SHAPE_ROUNDED_RECTANGLE_STROKE : XVG_SHAPE_ROUNDED_RECTANGLE_FILL;

    // float feather = 4.0f / xm_minf(w, h);
    float feather = 4.0f / h;

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(tex_idx, shape_type, grad.type, feather, stroke),
                    .borderradius_arcpie = _xvg_compress_border_radius(br_tr, br_br, br_tl, br_bl),

                    .colour1      = grad.colour1,
                    .colour2      = grad.colour2,
                    .gradient_a   = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b   = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy = grad.xy,
                    .texcoords_wh = grad.wh,
    };
}

void xvg_draw_rectangle(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           br,
    float           stroke_width,
    uint32_t        col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_rectangle_with_gradient_ex(xcl, x, y, w, h, br, br, br, br, stroke_width, grad);
}

void xvg_draw_rectangle_with_gradient(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           br,
    float           stroke_width,
    XVGGradient     grad)
{
    xvg_draw_rectangle_with_gradient_ex(xcl, x, y, w, h, br, br, br, br, stroke_width, grad);
}

void xvg_draw_triangle_with_gradient(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           rotate,
    float           stroke,
    XVGGradient     grad)
{
    unsigned     tex_idx    = _xvg_set_bound_texture(xcl, &grad);
    XVGShapeType shape_type = stroke > 0 ? XVG_SHAPE_TRIANGLE_STROKE : XVG_SHAPE_TRIANGLE_FILL;
    float        feather    = 4.0f / xm_minf(w, h);

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y},
                    .bottomright         = {x + w, y + h},
                    .sdf_data            = _xvg_compress_sdf_data(tex_idx, shape_type, grad.type, feather, stroke),
                    .borderradius_arcpie = _xvg_compress_arc_rotate_and_range(rotate, 0),
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}
void xvg_draw_triangle(
    XVGCommandList* xcl,
    float           x,
    float           y,
    float           w,
    float           h,
    float           rotate,
    float           stroke_width,
    uint32_t        colour)
{
    XVGGradient grad = {.colour1 = colour};
    xvg_draw_triangle_with_gradient(xcl, x, y, w, h, rotate, stroke_width, grad);
}

void xvg_draw_pie_with_gradient(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           start_turn,
    float           end_turn,
    float           stroke_width,
    XVGGradient     grad)
{
    unsigned     tex_idx      = _xvg_set_bound_texture(xcl, &grad);
    XVGShapeType shape_type   = stroke_width > 0 ? XVG_SHAPE_PIE_STROKE : XVG_SHAPE_PIE_FILL;
    float        feather      = 2.0f / radius_px;
    float        angle_range  = end_turn - start_turn;
    float        angle_rotate = (end_turn + start_turn);

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {cx - radius_px, cy - radius_px},
                    .bottomright         = {cx + radius_px, cy + radius_px},
                    .sdf_data            = _xvg_compress_sdf_data(tex_idx, shape_type, grad.type, feather, stroke_width),
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
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           start_turn,
    float           end_turn,
    float           stroke_width,
    uint32_t        colour)
{
    XVGGradient grad = {.colour1 = colour};
    xvg_draw_pie_with_gradient(xcl, cx, cy, radius_px, start_turn, end_turn, stroke_width, grad);
}

void xvg_draw_arc_with_gradient(
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           start_turn,
    float           end_turn,
    float           stroke_width,
    bool            butt,
    XVGGradient     grad)
{
    if (start_turn == end_turn)
        return;

    unsigned tex_idx = _xvg_set_bound_texture(xcl, &grad);

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

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {cx - radius_px, cy - radius_px},
                    .bottomright         = {cx + radius_px, cy + radius_px},
                    .sdf_data            = _xvg_compress_sdf_data(tex_idx, shape_type, grad.type, feather, stroke_width * 0.5f),
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
    XVGCommandList* xcl,
    float           cx,
    float           cy,
    float           radius_px,
    float           start_turn,
    float           end_turn,
    float           stroke_width,
    bool            butt,
    uint32_t        colour)
{
    XVGGradient grad = {.colour1 = colour};
    xvg_draw_arc_with_gradient(xcl, cx, cy, radius_px, start_turn, end_turn, stroke_width, butt, grad);
}

void xvg_draw_line_round_with_gradient(
    XVGCommandList* xcl,
    float           x0,
    float           y0,
    float           x1,
    float           y1,
    float           stroke,
    XVGGradient     grad)
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

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {xl, yt},
                    .bottomright         = {xr, yb},
                    .sdf_data            = _xvg_compress_sdf_data(0, XVG_SHAPE_LINE_ROUND, grad.type, feather, stroke * 0.5f),
                    .borderradius_arcpie = stroke_offsets.u32,
                    .colour1             = grad.colour1,
                    .colour2             = grad.colour2,
                    .gradient_a          = {grad.gradient_a[0], grad.gradient_a[1]},
                    .gradient_b          = {grad.gradient_b[0], grad.gradient_b[1]},
                    .texcoords_xy        = grad.xy,
                    .texcoords_wh        = grad.wh,
    };
}

void xvg_draw_line_round(XVGCommandList* xcl, float x0, float y0, float x1, float y1, float stroke_width, unsigned col)
{
    XVGGradient grad = {.colour1 = col};
    xvg_draw_line_round_with_gradient(xcl, x0, y0, x1, y1, stroke_width, grad);
}

void xvg_draw_line_plot(
    XVGCommandList* xcl,
    int             x,
    int             y,
    int             width,
    int             height,
    const float*    data,
    float           crop_br,
    float           stroke_width,
    uint32_t        colour)
{
    size_t    remaining_capacity = XVG_ARRLEN(xcl->line_segments) - xcl->frame.num_line_segments;
    const int backingScaleFactor = xcl->xvg->backingScaleFactor;

    int N = width * backingScaleFactor;

    if (N > remaining_capacity)
        N = 0;

    XVG_ASSERT(N > 0);
    if (N <= 0)
        return;

    const uint32_t begin_idx = xcl->frame.num_line_segments;
    const uint32_t end_idx   = xcl->frame.num_line_segments + N;

    if (backingScaleFactor == 2)
    {
        // Plain old linear interpolation
        xvg_line_segment_t* dst      = xcl->line_segments + begin_idx;
        int                 work_len = N - 2;
        int                 i, j;

        for (i = 0, j = 0; i < work_len; i += 2, j++)
        {
            float a      = data[j];
            float b      = data[j + 1];
            dst[i].y     = a;
            dst[i + 1].y = (a + b) * 0.5;
        }
        xassert(begin_idx + i == end_idx - 2);
        xassert(j == width - 1);
        dst[i].y     = data[j];
        dst[i + 1].y = data[j];
    }
    else
    {
        xassert(backingScaleFactor == 1);
        xstatic_assert(sizeof(xcl->line_segments[0]) == sizeof(data[0]), "Must match");
        memcpy(xcl->line_segments + begin_idx, data, N * sizeof(xcl->line_segments[0]));
    }

    XVG_ASSERT(end_idx >= 1);
    float feather = 4.0f / height;

    uint32_t line_buffer_range = (uint32_t)begin_idx | (end_idx << 16);

    xvg_shape_t* shape = _xvg_get_shape(xcl);
    *shape             = (xvg_shape_t){
                    .topleft             = {x, y - stroke_width * 0.25f},
                    .bottomright         = {x + width, y + height + stroke_width * 0.25},
                    .sdf_data            = _xvg_compress_sdf_data(0, XVG_SHAPE_LINE_PLOT, 0, feather, stroke_width),
                    .borderradius_arcpie = _xvg_compress_border_radius(crop_br, crop_br, crop_br, crop_br),
                    .colour1             = colour,
                    .buffer_idx_range    = line_buffer_range,
    };

    xcl->frame.num_line_segments = end_idx;
    XVG_ASSERT(xcl->frame.num_line_segments <= XVG_ARRLEN(xcl->line_segments));
}

XVGFontSlot* _xvg_get_current_font_slot(XVG* xcl) { return &xcl->fonts[xcl->current_font_idx]; }

XVGFont _xvg_add_font_from_memory_impl(XVG* xcl, const void* data, size_t datalen, bool owned)
{
    for (int i = 0; i < XVG_ARRLEN(xcl->fonts); i++)
    {
        XVGFontSlot* sl = xcl->fonts + i;
        if (sl->ft_face == NULL)
        {
            // TODO: pass data to kbts
            int err = FT_New_Memory_Face(xcl->ft_lib, data, datalen, 0, &sl->ft_face);
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

XVGFont xvg_add_font_from_path(XVG* xcl, const char* path)
{
    void*  data    = NULL;
    size_t datalen = 0;
    bool   owned   = true;
    bool   ok      = xfiles_read(path, &data, &datalen);
    XVG_ASSERT(ok);
    if (!ok)
        return (XVGFont){0};
    return _xvg_add_font_from_memory_impl(xcl, data, datalen, owned);
}

XVGFont xvg_add_font_from_memory(XVG* xcl, const void* data, size_t datalen)
{
    bool owned = false;
    return _xvg_add_font_from_memory_impl(xcl, data, datalen, owned);
}

void xvg_set_font(XVG* xcl, XVGFont font)
{
    int next_font_idx = font.id - 1;
    if (next_font_idx < 0)
        next_font_idx = 0;
    if (next_font_idx >= XVG_ARRLEN(xcl->fonts))
        next_font_idx = XVG_ARRLEN(xcl->fonts) - 1;
    xcl->current_font_idx = next_font_idx;
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

XVGAtlas* _xvg_get_current_font_atlas(XVG* xcl)
{
    XVG_ASSERT(xcl->current_atlas.idx < XVG_ARRLEN(xcl->atlases));
    return xcl->atlases + xcl->current_atlas.idx;
}

int _xvg_render_glyph(XVG* xcl, uint32_t glyph_index, unsigned font_size)
{
    int num_packed = 0;

    XVGAtlas*    atlas = _xvg_get_current_font_atlas(xcl);
    XVGFontSlot* sl    = _xvg_get_current_font_slot(xcl);

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
        num_packed              = stbrp_pack_rects(&xcl->current_atlas.rectpack_ctx, &rect, 1);

        bool failed_to_pack = num_packed == 0;
        if (failed_to_pack)
        {
            atlas->full = true;

            bool can_create_new_atlas = (xcl->current_atlas.idx + 1) < XVG_ARRLEN(xcl->atlases);
            XVG_ASSERT(can_create_new_atlas);
            if (can_create_new_atlas) // atlas is full
            {
                // make new atlas
                xcl->current_atlas.idx++;
                xcl->atlases[xcl->current_atlas.idx] = _xvg_create_new_atlas();

                atlas = xcl->atlases + xcl->current_atlas.idx;

                // Clear rectpack
                memset(&xcl->current_atlas.rectpack_ctx, 0, sizeof(xcl->current_atlas.rectpack_ctx));
                stbrp_init_target(
                    &xcl->current_atlas.rectpack_ctx,
                    XVG_ATLAS_WIDTH - RECTPACK_PADDING,
                    XVG_ATLAS_HEIGHT - RECTPACK_PADDING,
                    xcl->current_atlas.nodes,
                    xarr_len(xcl->current_atlas.nodes));

                rect       = (stbrp_rect){.w = width_pixels + RECTPACK_PADDING, .h = bmp->rows + RECTPACK_PADDING};
                num_packed = stbrp_pack_rects(&xcl->current_atlas.rectpack_ctx, &rect, 1);
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

            xarr_push(xcl->atlas_rects, arect);

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
XVGAtlasRect _xvg_get_glyph(XVG* xcl, uint32_t glyph_index, unsigned font_size)
{
    const int num_rects = xarr_len(xcl->atlas_rects);

    XVGAtlasRectHeader header = {.glyph_index = glyph_index, .font_size = font_size};

    for (int j = 0; j < num_rects; j++)
    {
        if (xcl->atlas_rects[j].header.data == header.data)
        {
            XVGAtlasRect* rect = xcl->atlas_rects + j;
            XVG_ASSERT(rect->x + rect->w < XVG_ATLAS_WIDTH);
            return *rect;
        }
    }

    int did_raster = _xvg_render_glyph(xcl, glyph_index, font_size);
    if (did_raster)
    {
        XVG_ASSERT(num_rects + 1 == xarr_len(xcl->atlas_rects));
        XVGAtlasRect* rect = xcl->atlas_rects + num_rects;
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

xvg_text_t* _xvg_get_text(XVGCommandList* xcl)
{
    static xvg_text_t stub;

    xvg_text_t* ret = xcl->frame.num_text < XVG_ARRLEN(xcl->text) ? &xcl->text[xcl->frame.num_text] : &stub;

    xcl->frame.num_text++;

    return ret;
}

bool _xvg_push_glyph(XVGCommandList* xcl, int pen_x, int pen_y, const XVGAtlasRect* rect, uint32_t colour)
{
    bool should_push = rect->img_view.id != 0;
    XVG_ASSERT(should_push);
    if (should_push)
    {
        if (xcl->frame.draw.text_texture.id != 0 && xcl->frame.draw.text_texture.id != rect->img_view.id)
            xvg_command_batch_draw(xcl, XVG_LABEL("_xvg_push_glyph"));
        if (xcl->frame.draw.text_texture.id == 0)
            xcl->frame.draw.text_texture = rect->img_view;

        xvg_text_t* obj = _xvg_get_text(xcl);
        // obj->topleft[0] = pen_x + (int)rect->bearing_x; // If using floating point coords
        // obj->topleft[1] = pen_y - (int)rect->bearing_y;
        obj->topleft      = _xvg_pack_xy_coord(pen_x + (int)rect->bearing_x, pen_y - (int)rect->bearing_y);
        obj->atlas_coords = (xvecu){.r = rect->x, .g = rect->y, .b = rect->w, .a = rect->h}.u32;
        obj->colour       = colour;

        XVG_ASSERT(xcl->frame.draw.text_texture.id != 0);
    }
    return should_push;
}

XVGTextLayoutRow* _xvg_begin_row(XVG* xcl, XVGTextLayout* layout)
{
    XVG_ASSERT(layout->num_rows <= layout->cap_rows);
    XVGTextLayoutRow* rows = xvg_layout_get_rows(layout);
    if (layout->num_rows >= layout->cap_rows)
    {
        // realloc
        layout->cap_rows            *= 2;
        XVGTextLayoutRow* next_rows  = linked_arena_alloc(xcl->arena, sizeof(*next_rows) * layout->cap_rows);
        memcpy(next_rows, rows, sizeof(*next_rows) * layout->num_rows);

        xvg_layout_set_rows(layout, next_rows);
        rows = next_rows;
    }

    rows[layout->num_rows++] = (XVGTextLayoutRow){.begin_idx = layout->num_glyphs};
    return rows;
}

void _xvg_end_row(XVGTextLayout* layout, int ymin, int ymax, int cursor_y_px)
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
    XVGCommandList* xcl,
    const char*     text_start,
    const char*     text_end,
    unsigned        font_size,
    float           break_width,
    float           _line_height)
{
    static const XVGTextLayout stub = {0};
    XVGFontSlot*               sl   = _xvg_get_current_font_slot(xcl->xvg);
    XVG_ASSERT(sl->ft_face);
    if (!sl->ft_face)
        return &stub;

    XVG_ASSERT(font_size < 128);
    if (text_end == NULL)
        text_end = text_start + strlen(text_start);
    const size_t text_len           = text_end - text_start;
    const int    backingScaleFactor = xcl->xvg->backingScaleFactor;

    font_size   *= backingScaleFactor;
    break_width *= backingScaleFactor;

    XVGTextLayout* layout = linked_arena_alloc_clear(xcl->arena, sizeof(*layout));
    layout->cap_glyphs    = text_len * 2;

    layout->cap_rows = text_len >> 4;
    if (layout->cap_rows < 8)
        layout->cap_rows = 8;
    XVGTextLayoutRow* rows   = linked_arena_alloc_clear(xcl->arena, sizeof(*rows) * layout->cap_rows);
    XVGGlyphLayout*   glyphs = linked_arena_alloc(xcl->arena, sizeof(*glyphs) * layout->cap_glyphs);
    xvg_layout_set_rows(layout, rows);
    xvg_layout_set_glyphs(layout, glyphs);

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

    rows                       = _xvg_begin_row(xcl->xvg, layout);
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

            _xvg_end_row(layout, line_ymin, line_ymax, CursorY >> 6);
            rows = _xvg_begin_row(xcl->xvg, layout);

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

            XVGAtlasRect rect = _xvg_get_glyph(xcl->xvg, glyph_idx, font_size);

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
                _xvg_end_row(layout, line_ymin, line_ymax, (CursorY + line_height) >> 6);
                rows = _xvg_begin_row(xcl->xvg, layout);

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
    _xvg_end_row(layout, line_ymin, line_ymax, CursorY >> 6);
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

void xvg_draw_text_layout(
    XVGCommandList*      xcl,
    const XVGTextLayout* layout,
    int                  x,
    int                  y,
    int                  alignment,
    uint32_t             colour)
{
    if (layout->num_glyphs == 0)
        return;

    x *= xcl->xvg->backingScaleFactor;
    y *= xcl->xvg->backingScaleFactor;

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
    XVGGlyphLayout* glyph_pos_2   = linked_arena_alloc(xcl->arena, sizeof(*glyph_pos_2) * glyph_pos_len);

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
                bool did_push = _xvg_push_glyph(xcl, x + gpos->x, y + gpos->y, &gpos->rect, colour);
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
    XVGCommandList* xcl,
    float           x,
    float           y,
    const char*     text_start,
    const char*     text_end,
    unsigned        font_size,
    XVGAlign        alignment,
    uint32_t        colour,
    float           break_width,
    float           line_height)
{
    LINKED_ARENA_LEAK_DETECT_BEGIN(xcl->arena);

    const XVGTextLayout* layout =
        xvg_create_text_layout(xcl, text_start, text_end, font_size, break_width, line_height);
    xvg_draw_text_layout(xcl, layout, x, y, alignment, colour);
    xvg_release_text_layout(xcl, layout);

    LINKED_ARENA_LEAK_DETECT_END(xcl->arena);
}

void xvg_draw_text(
    XVGCommandList* xcl,
    float           x,
    float           y,
    const char*     text_start,
    const char*     text_end,
    unsigned        font_size,
    XVGAlign        alignment,
    uint32_t        colour)
{
    xvg_draw_text_ex(xcl, x, y, text_start, text_end, font_size, alignment, colour, 0, 1);
}

void xvg_init(XVG* xcl)
{
    xcl->arena       = linked_arena_create_ex(0, 1024 * 512); // 0.5mb
    xcl->frame_arena = linked_arena_create_ex(0, 1024 * 64);

    xcl->backingScaleFactor = 1;

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

    // Text
    int ft_err = FT_Init_FreeType(&xcl->ft_lib);
    XVG_ASSERT(ft_err == 0);

    xarr_setcap(xcl->atlas_rects, 256);
    xarr_setlen(xcl->atlas_rects, 0);
    xcl->atlases[0]        = _xvg_create_new_atlas();
    xcl->current_atlas.idx = 0;
    xarr_setlen(xcl->current_atlas.nodes, (XVG_ATLAS_WIDTH * 2));

    stbrp_init_target(
        &xcl->current_atlas.rectpack_ctx,
        XVG_ATLAS_WIDTH - RECTPACK_PADDING,
        XVG_ATLAS_HEIGHT - RECTPACK_PADDING,
        xcl->current_atlas.nodes,
        xarr_len(xcl->current_atlas.nodes));

    // Shader stuff
    xcl->smp_linear            = sg_make_sampler(&(sg_sampler_desc){
                   .min_filter    = SG_FILTER_LINEAR,
                   .mag_filter    = SG_FILTER_LINEAR,
                   .mipmap_filter = SG_FILTER_LINEAR,
                   .wrap_u        = SG_WRAP_CLAMP_TO_EDGE,
                   .wrap_v        = SG_WRAP_CLAMP_TO_EDGE,
    });
    xcl->smp_nearest_neighbour = sg_make_sampler(&(sg_sampler_desc){
        .min_filter    = SG_FILTER_NEAREST,
        .mag_filter    = SG_FILTER_NEAREST,
        .mipmap_filter = SG_FILTER_NEAREST,
        .wrap_u        = SG_WRAP_CLAMP_TO_EDGE,
        .wrap_v        = SG_WRAP_CLAMP_TO_EDGE,
    });

    // fallback_img
    static const uint32_t pixel_white = 0xffffffff;

    xcl->fallback_img      = sg_make_image(&(sg_image_desc){
             .width              = 1,
             .height             = 1,
             .pixel_format       = SG_PIXELFORMAT_RGBA8,
             .data.mip_levels[0] = {
                 .ptr  = &pixel_white,
                 .size = sizeof(pixel_white),
        }});
    xcl->fallback_img_view = sg_make_view(&(sg_view_desc){.texture = xcl->fallback_img});

    xcl->pip_shapes = sg_make_pipeline(&(sg_pipeline_desc){
        .shader    = sg_make_shader(_xvg_shapes_shader_desc(sg_query_backend())),
        .colors[0] = BLEND_DEFAULT,
        .label     = XVG_LABEL("xcl-shapes-pipeline")});

    xcl->pip_text = sg_make_pipeline(&(sg_pipeline_desc){
#if defined(XVG_TEXT_MULTICHANNEL)
        .shader    = sg_make_shader(_xvg_text_multichannel_shader_desc(sg_query_backend())),
        .colors[0] = BLEND_DUAL_SOURCE,
#endif
#if defined(XVG_TEXT_SINGLECHANNEL)
        .shader    = sg_make_shader(_xvg_text_singlechannel_shader_desc(sg_query_backend())),
        .colors[0] = BLEND_DEFAULT,
#endif
        .label = XVG_LABEL("xcl-text")});
    XVG_ASSERT(xcl->pip_text.id);
}

void xvg_deinit(XVG* xcl)
{
    xarr_free(xcl->atlas_rects);
    xarr_free(xcl->current_atlas.nodes);

    for (int i = 0; i < XVG_ARRLEN(xcl->fonts); i++)
    {
        XVGFontSlot* sl = &xcl->fonts[i];
        if (sl->data && sl->owned)
        {
            XFILES_FREE(sl->data);
        }
        // TODO: free kbts here?
        if (sl->ft_face)
            FT_Done_Face(sl->ft_face);
    }
    for (int i = 0; i < XVG_ARRLEN(xcl->atlases); i++)
    {
        XVGAtlas* atlas = &xcl->atlases[i];
        if (atlas->img_data)
        {
            XVG_FREE(atlas->img_data);
        }
    }
    FT_Done_FreeType(xcl->ft_lib);

    linked_arena_destroy(xcl->frame_arena);
    linked_arena_destroy(xcl->arena);
}

XVGCommandList* xvg_command_list_create(XVG* xvg)
{
    XVG_ASSERT(xvg->arena_top == NULL); // dont create these within a frame! Create them at startup
    // TODO: support creating these within a frame and not just at startup

    XVGCommandList* xcl = linked_arena_alloc_clear(xvg->arena, sizeof(*xcl));

    xcl->xvg   = xvg;
    xcl->arena = xvg->arena;

    xcl->shapes_sbo = sg_make_buffer(&(sg_buffer_desc){
        .usage.storage_buffer = true,
        .usage.stream_update  = true,
        .size                 = sizeof(xcl->shapes),
        .label                = XVG_LABEL("xcl-shapes"),
    });
    xcl->shapes_sbv = sg_make_view(&(sg_view_desc){
        .storage_buffer = xcl->shapes_sbo,
    });

    xcl->lines_sbo = sg_make_buffer(&(sg_buffer_desc){
        .usage.storage_buffer = true,
        .usage.stream_update  = true,
        .size                 = sizeof(xcl->line_segments),
        .label                = XVG_LABEL("xcl-line-buffer"),
    });
    xcl->lines_sbv = sg_make_view(&(sg_view_desc){.storage_buffer = xcl->lines_sbo});

    xcl->text_sbo = sg_make_buffer(&(sg_buffer_desc){
        .usage.storage_buffer = true,
        .usage.stream_update  = true,
        .size                 = sizeof(xcl->text),
        .label                = XVG_LABEL("xcl-text-buffer"),
    });
    XVG_ASSERT(xcl->text_sbo.id);
    xcl->text_sbv = sg_make_view(&(sg_view_desc){
        .storage_buffer = xcl->text_sbo,
    });
    XVG_ASSERT(xcl->text_sbv.id);

    return xcl;
}

void xvg_begin_frame(XVG* xcl)
{
    // Oh no, you forgot to call xvg_end_frame()
    // Or perhaps you called a function that caused you to continue processing in your or the OS's event loop
    XVG_ASSERT(xcl->arena_top == NULL);
    xcl->arena_top = linked_arena_get_top(xcl->arena);
    linked_arena_clear(xcl->frame_arena);
    xcl->current_font_idx = 0;
}

void xvg_end_frame(XVG* xcl)
{
    linked_arena_release(xcl->arena, xcl->arena_top);
    xcl->arena_top = NULL;

    for (int i = 0; i < XVG_ARRLEN(xcl->atlases); i++)
    {
        XVGAtlas* atlas = &xcl->atlases[i];
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
}

void xvg_command_list_begin_frame(XVGCommandList* xcl) { memset(&xcl->frame, 0, sizeof(xcl->frame)); }

void xvg_command_list_end_frame(XVGCommandList* xcl, int window_width, int window_height)
{
    xvg_command_batch_draw(xcl, XVG_LABEL("xvg_end_frame"));
    XVG* x = xcl->xvg;

    // Upload data
    if (xcl->frame.num_shapes)
    {
        if (xcl->frame.num_shapes > XVG_ARRLEN(xcl->shapes))
            xcl->frame.num_shapes = XVG_ARRLEN(xcl->shapes);
        size_t   num_bytes = sizeof(xcl->shapes[0]) * xcl->frame.num_shapes;
        sg_range range     = {.ptr = xcl->shapes, .size = num_bytes};
        sg_update_buffer(xcl->shapes_sbo, &range);
    }
    if (xcl->frame.num_line_segments)
    {
        if (xcl->frame.num_line_segments > XVG_ARRLEN(xcl->line_segments))
            xcl->frame.num_line_segments = XVG_ARRLEN(xcl->line_segments);
        size_t   num_bytes = sizeof(xcl->line_segments[0]) * xcl->frame.num_line_segments;
        sg_range range     = {.ptr = xcl->line_segments, .size = num_bytes};
        sg_update_buffer(xcl->lines_sbo, &range);
    }
    if (xcl->frame.num_text)
    {
        if (xcl->frame.num_text > XVG_ARRLEN(xcl->text))
            xcl->frame.num_text = XVG_ARRLEN(xcl->text);

        size_t   num_bytes = sizeof(xcl->text[0]) * xcl->frame.num_text;
        sg_range range     = {.ptr = xcl->text, .size = num_bytes};
        sg_update_buffer(xcl->text_sbo, &range);
    }

    // Process commands
    int ncommands        = 0;
    int num_batch_groups = 0;
    int cmd_idx          = xcl->frame.first_command_idx;
    while (cmd_idx > 0 && cmd_idx < XVG_ARRLEN(xcl->commands))
    {
        XVGCommand* cmd = xcl->commands + cmd_idx;
        switch (cmd->type)
        {
        case XVG_CMD_NONE:
            break;
        case XVG_CMD_BEGIN_PASS:
        {
            XVGCommandBeginPass* p = &cmd->beginPass;
            XVG_ASSERT(p->pass_idx > 0 && p->pass_idx < XVG_ARRLEN(xcl->passes));
            sg_begin_pass(&xcl->passes[p->pass_idx]);
            break;
        }
        case XVG_CMD_END_PASS:
            sg_end_pass();
            break;
        case XVG_CMD_SET_SCISSOR:
        {
            XVGCommandSetScissor* s = &cmd->scissor;
            sg_apply_scissor_rect(s->x, s->y, s->w, s->h, true);
            break;
        }
        case XVG_CMD_SET_VIEWPORT:
        {
            XVGCommandSetViewport* s = &cmd->viewport;
            sg_apply_viewport(s->x, s->y, s->w, s->h, true);
            break;
        }
        case XVG_CMD_CUSTOM:
        {
            XVGCommandCustom* custom = &cmd->custom;
            custom->func(custom->uptr);
            break;
        }
        case XVG_CMD_DRAW:
        {
            num_batch_groups++;

            XVGCommandDraw* draw = &cmd->draw;

            draw->shape_buffer_start = xm_mini(draw->shape_buffer_start, XVG_ARRLEN(xcl->shapes));
            draw->shape_buffer_end   = xm_mini(draw->shape_buffer_end, XVG_ARRLEN(xcl->shapes));

            const int num_shapes = draw->shape_buffer_end - draw->shape_buffer_start;
            XVG_ASSERT(num_shapes >= 0);
            if (num_shapes)
            {
                sg_apply_pipeline(x->pip_shapes);

                for (int i = 0; i < XVG_ARRLEN(draw->shape_texture); i++)
                {
                    if (draw->shape_texture[i].id == 0)
                        draw->shape_texture[i] = x->fallback_img_view;
                }

                sg_sampler smp = draw->shape_sampler;
                if (smp.id == 0)
                    smp = x->smp_linear;

                sg_apply_bindings(&(sg_bindings){
                    .views[VIEW_vs_xvg_shapes_buffer]      = xcl->shapes_sbv,
                    .views[VIEW_fs_xvg_shapes_line_buffer] = xcl->lines_sbv,
                    .views[VIEW_fs_xvg_shapes_tex1]        = draw->shape_texture[0],
                    .views[VIEW_fs_xvg_shapes_tex2]        = draw->shape_texture[1],
                    .views[VIEW_fs_xvg_shapes_tex3]        = draw->shape_texture[2],
                    .views[VIEW_fs_xvg_shapes_tex4]        = draw->shape_texture[3],
                    .samplers[SMP_fs_xvg_shapes_smp]       = smp,
                });

                vs_xvg_shapes_uniforms_t uniforms = {
                    .u_size                  = {window_width, window_height},
                    .u_storage_buffer_offset = draw->shape_buffer_start,
                };

                sg_image      img            = sg_query_view_desc(draw->shape_texture[0]).texture.image;
                sg_image_desc img_desc       = sg_query_image_desc(img);
                uniforms.u_texture_size_1[0] = img_desc.width;
                uniforms.u_texture_size_1[1] = img_desc.height;

                img                          = sg_query_view_desc(draw->shape_texture[1]).texture.image;
                img_desc                     = sg_query_image_desc(img);
                uniforms.u_texture_size_2[0] = img_desc.width;
                uniforms.u_texture_size_2[1] = img_desc.height;

                img                          = sg_query_view_desc(draw->shape_texture[2]).texture.image;
                img_desc                     = sg_query_image_desc(img);
                uniforms.u_texture_size_3[0] = img_desc.width;
                uniforms.u_texture_size_3[1] = img_desc.height;

                img                          = sg_query_view_desc(draw->shape_texture[3]).texture.image;
                img_desc                     = sg_query_image_desc(img);
                uniforms.u_texture_size_4[0] = img_desc.width;
                uniforms.u_texture_size_4[1] = img_desc.height;

                sg_apply_uniforms(UB_vs_xvg_shapes_uniforms, &SG_RANGE(uniforms));
                sg_draw(0, 6 * num_shapes, 1);
            }

            draw->text_buffer_start = xm_mini(draw->text_buffer_start, XVG_ARRLEN(xcl->text));
            draw->text_buffer_end   = xm_mini(draw->text_buffer_end, XVG_ARRLEN(xcl->text));

            const int num_text = draw->text_buffer_end - draw->text_buffer_start;
            XVG_ASSERT(num_text >= 0);
            if (num_text)
            {
                XVG_ASSERT(draw->text_texture.id != 0); // You forgot to set the current texture
                sg_apply_pipeline(x->pip_text);
                sg_apply_bindings(&(sg_bindings){
                    .views[VIEW_vs_xvg_text_buffer] = xcl->text_sbv,
                    .views[VIEW_fs_xvg_text_tex]    = draw->text_texture,
                    .samplers[SMP_fs_xvg_text_smp]  = x->smp_nearest_neighbour,
                });

                vs_xvg_text_uniforms_t uniforms = {
                    .u_view_size  = {window_width * x->backingScaleFactor, window_height * x->backingScaleFactor},
                    .u_sbo_offset = draw->text_buffer_start,
                };
                sg_apply_uniforms(UB_vs_xvg_text_uniforms, &SG_RANGE(uniforms));

                sg_draw(0, 6 * num_text, 1);
            }
            break;
        }
        }

        XVG_ASSERT(cmd->next_idx >= 0 && cmd->next_idx < XVG_ARRLEN(xcl->commands));
        cmd_idx = cmd->next_idx;
        ncommands++;
    }
}

#endif // XVG_IMPL