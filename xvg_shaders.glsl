// ███████╗██╗  ██╗ █████╗ ██████╗ ███████╗███████╗
// ██╔════╝██║  ██║██╔══██╗██╔══██╗██╔════╝██╔════╝
// ███████╗███████║███████║██████╔╝█████╗  ███████╗
// ╚════██║██╔══██║██╔══██║██╔═══╝ ██╔══╝  ╚════██║
// ███████║██║  ██║██║  ██║██║     ███████╗███████║
// ╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚══════╝╚══════╝

@vs vs_xvg_shapes

// Tightly packed rectangle
struct xvg_shape
{
    // TODO: compress to int16
    vec2 topleft;
    vec2 bottomright;

    // unorm4x8 comprised of:
    //   - uint sdf_type;   // could probably be compressed to one byte
    //   - uint grad_type;
    //   - float stroke_width;
    //   - float feather;
    uint sdf_data;

    // packed with either:
    // - border radius (unorm4x8)
    // - arc/pie rotate and range (unorm2x16)
    uint borderradius_arcpie;

    uint colour1;
    uint colour2;

    vec2 gradient_a;
    vec2 gradient_b;

    uint buffer_idx_range; // unorm2x16
    // uint  texid; // ???
};

layout(binding=0) readonly buffer vs_xvg_shapes_buffer {
    xvg_shape vtx[];
};

layout(binding=0) uniform vs_xvg_shapes_uniforms {
    vec2 u_size;
    int  u_storage_buffer_offset;
};

#define PI 3.141592653589793

out vec2 uv;
out flat vec2 uv_xy_scale;

out flat uint sdf_type;
out flat uint grad_type;
out flat float feather;
out flat float stroke_width;

out flat uint colour1;
out flat uint colour2;

// either:
// - border_radius
// - vec4(cos(rotate), sin(rotate), sin(range), cos(range))
out flat vec4 borderradius_arcpie;

out float buffer_idx;
out flat int buffer_begin_idx;
out flat int buffer_end_idx;
out flat float px_inc;

// linear_gradient_begin
// radial_gradient_pos
// conic_gradient_rotate
// inner_shadow_translate_xy
out flat vec2 gradient_a;
// linear_gradient_end
// radial_gradient_radius_scale
// conic_gradient_angle
// inner_shadow_blur_radius
out flat vec2 gradient_b;

#define XVG_SHAPE_RECTANGLE_FILL   1
#define XVG_SHAPE_RECTANGLE_STROKE 2
#define XVG_SHAPE_CIRCLE_FILL      3
#define XVG_SHAPE_CIRCLE_STROKE    4
#define XVG_SHAPE_TRIANGLE_FILL    5
#define XVG_SHAPE_TRIANGLE_STROKE  6
#define XVG_SHAPE_PIE_FILL         7
#define XVG_SHAPE_PIE_STROKE       8
#define XVG_SHAPE_ARC_ROUND_STROKE 9
#define XVG_SHAPE_ARC_BUTT_STROKE  10
#define XVG_SHAPE_LINE_PLOT        11

#define XVG_COLOUR_SOLID  0
#define XVG_COLOUR_LINEAR_GRADIENT 1
#define XVG_COLOUR_RADIAL_GRADIENT 2
#define XVG_COLOUR_CONIC_GRADIENT  3
#define XVG_COLOUR_BOX_GRADIENT    4

void main() {
    uint v_idx = gl_VertexIndex / 6u;
    uint i_idx = gl_VertexIndex - v_idx * 6;

    xvg_shape vert = vtx[v_idx + u_storage_buffer_offset];

    //  0.5f,  0.5f,
    // -0.5f, -0.5f,
    //  0.5f, -0.5f,
    // -0.5f,  0.5f,
    // 0, 1, 2,
    // 1, 2, 3,

    // Is odd
    bool is_right = (gl_VertexIndex & 1) == 1;
    bool is_bottom = i_idx >= 2 && i_idx <= 4;

    vec2 pos = vec2(
        is_right  ? vert.bottomright.x : vert.topleft.x,
        is_bottom ? vert.bottomright.y : vert.topleft.y
    );

    pos = (pos + pos) / u_size - vec2(1);
    pos.y = -pos.y;

    float vw = vert.bottomright.x - vert.topleft.x;
    float vh = vert.bottomright.y - vert.topleft.y;

    gl_Position = vec4(pos, 1, 1);

    uv = vec2(is_right  ? 1 : -1, is_bottom ? -1 : 1);
    // uv_xy_scale = vec2(vw > vh ? vw / vh : 1, vh > vw ? vh / vw : 1);
    uv_xy_scale = vec2(vw / vh, 1);

    colour1 = vert.colour1;
    colour2 = vert.colour2;

    vec4 sdf_data = unpackUnorm4x8(vert.sdf_data);

    sdf_type     = uint(sdf_data.x * 255);
    grad_type    = uint(sdf_data.y * 255); 
    feather      =      sdf_data.z * 1;
    stroke_width =      sdf_data.w * 16;
    stroke_width = 2 * stroke_width / vw * uv_xy_scale.x;

    if (sdf_type == XVG_SHAPE_RECTANGLE_FILL ||
        sdf_type == XVG_SHAPE_RECTANGLE_STROKE || 
        sdf_type == XVG_SHAPE_LINE_PLOT)
    {
        borderradius_arcpie = (unpackUnorm4x8(vert.borderradius_arcpie) * 255) / vec4(vh * 0.5);        
    }

    if (sdf_type == XVG_SHAPE_TRIANGLE_FILL    ||
        sdf_type == XVG_SHAPE_TRIANGLE_STROKE  ||
        sdf_type == XVG_SHAPE_PIE_FILL         ||
        sdf_type == XVG_SHAPE_PIE_STROKE       ||
        sdf_type == XVG_SHAPE_ARC_ROUND_STROKE ||
        sdf_type == XVG_SHAPE_ARC_BUTT_STROKE)
    {
        vec2 arcpie   = 2 * PI * unpackUnorm2x16(vert.borderradius_arcpie);
        borderradius_arcpie.xy = vec2(cos(arcpie.x), sin(arcpie.x));
        borderradius_arcpie.zw = vec2(sin(arcpie.y), cos(arcpie.y));
    }

    if (sdf_type == XVG_SHAPE_LINE_PLOT)
    {
        vec2 buffer_idx_range = unpackUnorm2x16(vert.buffer_idx_range) * vec2(65535);

        buffer_idx       = is_right ? buffer_idx_range.y : buffer_idx_range.x;
        buffer_begin_idx = int(buffer_idx_range.x);
        buffer_end_idx   = int(buffer_idx_range.y);
        px_inc       = 2.0 / u_size.y;
        stroke_width = px_inc * sdf_data.w * 16;
    }

    if (grad_type == XVG_COLOUR_LINEAR_GRADIENT)
    {
        gradient_a = (vert.gradient_a - vert.topleft) / vec2(vw, vh);  // stop 1 xy
        gradient_b = (vert.gradient_b - vert.topleft) / vec2(vw, vh); // stop 2 xy
    }
    if (grad_type == XVG_COLOUR_RADIAL_GRADIENT)
    {
        gradient_a = (vert.gradient_a - vert.topleft) / vec2(vw, vh); // stop 2 cx,cy
        gradient_b = vec2(vw, vh) / vert.gradient_b;                    // stop 1 radius
    }
    if (grad_type == XVG_COLOUR_CONIC_GRADIENT)
    {
        gradient_a = vec2(cos(vert.gradient_a.x), sin(vert.gradient_a.x)); // rotation, radians
        gradient_b = vert.gradient_b;                                      // range, radians
    }
    if (grad_type == XVG_COLOUR_BOX_GRADIENT)
    {
        gradient_a = vec2(vert.gradient_a) / vec2(-vw, vh); // translate x/y
        gradient_b = vec2(vert.gradient_b     / vh);        // blur radius
    }
}
@end

@fs fs_xvg_shapes
precision mediump float;

struct xvg_line_segment {
    float y;
};

layout(binding=1) readonly buffer fs_xvg_line_buffer {
    xvg_line_segment line_buffer[];
};

in vec2 uv;
in flat vec2 uv_xy_scale;

in flat uint sdf_type;
in flat uint grad_type;
in flat float feather;
in flat float stroke_width;

in flat uint colour1;
in flat uint colour2;

in flat vec4 borderradius_arcpie;

in float buffer_idx;
in flat int buffer_begin_idx;
in flat int buffer_end_idx;
in flat float px_inc;

in flat vec2 gradient_a;
in flat vec2 gradient_b;

out vec4 frag_color;

#define PI 3.141592653589793
#define XVG_SHAPE_RECTANGLE_FILL   1
#define XVG_SHAPE_RECTANGLE_STROKE 2
#define XVG_SHAPE_CIRCLE_FILL      3
#define XVG_SHAPE_CIRCLE_STROKE    4
#define XVG_SHAPE_TRIANGLE_FILL    5
#define XVG_SHAPE_TRIANGLE_STROKE  6
#define XVG_SHAPE_PIE_FILL         7
#define XVG_SHAPE_PIE_STROKE       8
#define XVG_SHAPE_ARC_ROUND_STROKE 9
#define XVG_SHAPE_ARC_BUTT_STROKE  10
#define XVG_SHAPE_LINE_PLOT        11

#define XVG_COLOUR_SOLID  0
#define XVG_COLOUR_LINEAR_GRADIENT 1
#define XVG_COLOUR_RADIAL_GRADIENT 2
#define XVG_COLOUR_CONIC_GRADIENT  3
#define XVG_COLOUR_BOX_GRADIENT    4

// The MIT License
// Copyright © 2017 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// b.x = half width
// b.y = half height
// r.x = roundness top-right  
// r.y = roundness boottom-right
// r.z = roundness top-left
// r.w = roundness bottom-left
float sdRoundBox(in vec2 p, in vec2 b, in vec4 r)
{
    r.xy = (p.x>0.0)?r.xy : r.zw;
    r.x  = (p.y>0.0)?r.x  : r.y;
    vec2 q = abs(p)-b+r.x;
    return min(max(q.x,q.y),0.0) + length(max(q,0.0)) - r.x;
}

// (r is half the base)
float sdEquilateralTriangle( in vec2 p, in float r )
{
    const float k = sqrt(3.0);
    p.x = abs(p.x);
    p -= vec2(0.5,0.5*k)*max(p.x+k*p.y,0.0);
    p -= vec2(clamp(p.x,-r,r),-r/k );
    return length(p)*sign(-p.y);
}

// c is the sin/cos of the angle. r is the radius
float sdPie( in vec2 p, in vec2 c, in float r )
{
    p.x = abs(p.x);
    float l = length(p) - r;
	float m = length(p - c*clamp(dot(p,c),0.0,r) );
    return max(l,m*sign(c.y*p.x-c.x*p.y));
}

// sc is the sin/cos of the aperture
float sdArc( in vec2 p, in vec2 sc, in float ra, float rb )
{
    p.x = abs(p.x);
    return ((sc.y*p.x>sc.x*p.y) ? length(p-sc*ra) : 
                                  abs(length(p)-ra)) - rb;
}

float sdRing( in vec2 p, in vec2 n, in float r, float th )
{
    p.x = abs(p.x);
    p = mat2x2(-n.x,-n.y,n.y,-n.x)*p;
    return max( abs(length(p)-r)-th*0.5,
                length(vec2(p.x,max(0.0,abs(r-p.y)-th*0.5)))*sign(p.x) );
}

float sdSegment(in vec2 p, in vec2 a, in vec2 b)
{
    vec2  ba = b - a;
    vec2  pa = p - a;
    float h  = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - h * ba);
}

void main()
{
    float shape = 1;
    vec4 col = vec4(0);
    if (sdf_type == XVG_SHAPE_RECTANGLE_FILL)
    {
        vec2 b = uv_xy_scale;
        float d = sdRoundBox(uv * uv_xy_scale, b, borderradius_arcpie);
        shape = smoothstep(feather, 0, d + feather * 0.5);
    }
    if (sdf_type == XVG_SHAPE_RECTANGLE_STROKE)
    {
        vec2 b = uv_xy_scale;
        float d = sdRoundBox(uv * uv_xy_scale, b, borderradius_arcpie);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        float inner = smoothstep(feather, 0, d + stroke_width + feather * 0.5);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_CIRCLE_FILL)
    {
        float d = 1 - length(uv);
        float outer = smoothstep(0, feather, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_CIRCLE_STROKE)
    {
        float d = 1 - length(uv);
        float outer = smoothstep(0, feather, d + feather * 0.5);
        float inner = smoothstep(0, feather, d + feather * 0.5 - stroke_width);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_TRIANGLE_FILL)
    {
        vec2 p = vec2(uv.x * borderradius_arcpie.x - uv.y * borderradius_arcpie.y,
                      uv.x * borderradius_arcpie.y + uv.y * borderradius_arcpie.x);
        float d = sdEquilateralTriangle(p, 0.86);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_TRIANGLE_STROKE)
    {
        vec2 p = vec2(uv.x * borderradius_arcpie.x - uv.y * borderradius_arcpie.y,
                      uv.x * borderradius_arcpie.y + uv.y * borderradius_arcpie.x);
        float d = sdEquilateralTriangle(p, 0.86);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        float inner = smoothstep(feather, 0, d + feather * 0.5 + stroke_width);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_PIE_FILL)
    {
        vec2 uv_rotated = vec2(uv.x * borderradius_arcpie.x - uv.y * borderradius_arcpie.y,
                               uv.x * borderradius_arcpie.y + uv.y * borderradius_arcpie.x);
        float d = sdPie(uv_rotated, borderradius_arcpie.zw, 1.0);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_PIE_STROKE)
    {
        vec2 uv_rotated = vec2(uv.x * borderradius_arcpie.x - uv.y * borderradius_arcpie.y,
                               uv.x * borderradius_arcpie.y + uv.y * borderradius_arcpie.x);
        float d = sdPie(uv_rotated, borderradius_arcpie.zw, 1.0);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        float inner = smoothstep(feather, 0, d + feather * 0.5 + stroke_width);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_ARC_ROUND_STROKE)
    {
        vec2 uv_rotated = vec2(uv.x * borderradius_arcpie.x - uv.y * borderradius_arcpie.y,
                               uv.x * borderradius_arcpie.y + uv.y * borderradius_arcpie.x);
        float d = sdArc(uv_rotated, borderradius_arcpie.zw, 1.0 - stroke_width * 0.5, stroke_width * 0.5);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_ARC_BUTT_STROKE)
    {
        vec2 uv_rotated = vec2(uv.x * borderradius_arcpie.x - uv.y * borderradius_arcpie.y,
                               uv.x * borderradius_arcpie.y + uv.y * borderradius_arcpie.x);
        float d = sdRing(uv_rotated, borderradius_arcpie.zw, 1.0 - stroke_width * 0.5, stroke_width);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_LINE_PLOT)
    {
        // Read buffer data

        // If we pad the storage buffer by 1 on each side with real values, then we can get nicer looing clipped edges
        uint idx      = min(int(buffer_idx),     buffer_end_idx - 1);
        uint idx_prev = max(int(buffer_idx) - 1, buffer_begin_idx);
        uint idx_next = min(int(buffer_idx) + 1, buffer_end_idx - 1);

        float line_y      = line_buffer[idx].y;
        float line_y_prev = line_buffer[idx_prev].y;
        float line_y_next = line_buffer[idx_next].y;

        line_y      = line_y      * 2 - 1;
        line_y_prev = line_y_prev * 2 - 1;
        line_y_next = line_y_next * 2 - 1;

        // Technically makes the line less accurate, but it helps top half the stroke width getting cropped at the top & bottom edges of the rectangle. 
        // Might remove later if this causes other issues...
        float stroke_scale  = 1 - stroke_width;
        line_y             *= stroke_scale;
        line_y_prev        *= stroke_scale;
        line_y_next        *= stroke_scale;

        // build points
        vec2 p = uv;
        vec2 a = vec2(p.x - px_inc, line_y_prev);
        vec2 b = vec2(p.x         , line_y);
        vec2 c = vec2(p.x + px_inc, line_y_next);

        float d1 = sdSegment(p, a, b);
        float d2 = sdSegment(p, b, c);
        float d = min(d1, d2);

        float shape_vertical   = smoothstep(stroke_width, 0, abs(d));
        float shape_horizontal = smoothstep(stroke_width, 0, abs(line_y - p.y));

        shape = max(shape_vertical, shape_horizontal);
        shape = sqrt(shape); // gamma

        // TODO crop with rounded rectange

        vec2  crop_b = uv_xy_scale;
        float crop_d = sdRoundBox(uv * uv_xy_scale, crop_b, borderradius_arcpie);
        float crop_shape = smoothstep(feather, 0, crop_d + feather * 0.5);

        shape *= crop_shape;
    }

    float t = 0;
    if (grad_type == XVG_COLOUR_SOLID)
    {
        col = unpackUnorm4x8(colour1).abgr; // swizzle
    }
    if (grad_type == XVG_COLOUR_LINEAR_GRADIENT)
    {
        vec2 uv_norm = vec2(uv.x * 0.5 + 0.5,  uv.y * -0.5 + 0.5);

        vec2 v  = gradient_a - gradient_b;
        vec2 w  = gradient_a - uv_norm;
        t = dot(v, w) / dot(v, v);
        t = clamp(t, 0, 1);
    }
    if (grad_type == XVG_COLOUR_RADIAL_GRADIENT)
    {
        // translate & scale
        vec2 uv_norm       = vec2(uv.x * 0.5 + 0.5,  uv.y * -0.5 + 0.5);
        vec2 ellipse_space = (uv_norm - gradient_a) * gradient_b;

        t = clamp(length(ellipse_space), 0.0, 1.0);
    }
    if (grad_type == XVG_COLOUR_CONIC_GRADIENT)
    {
        // Change start/end position of the gradient
        vec2 p = uv * uv_xy_scale;
        vec2 uv_rotated = vec2(p.x * gradient_a.x - p.y * gradient_a.y,
                               p.x * gradient_a.y + p.y * gradient_a.x);
        float angle = atan(uv_rotated.x, uv_rotated.y);

        // Crops the gradient range
        float range = gradient_b.x;
        t = angle / (PI * 2) + 0.5;
        t = smoothstep(0, range, t);
    }
    if (grad_type == XVG_COLOUR_BOX_GRADIENT)
    {
        vec2  xy_offset   = gradient_a;
        float blur_radius = gradient_b.x;

        vec2 p = (uv + xy_offset) * uv_xy_scale;
        vec2 half_wh  = uv_xy_scale - blur_radius * 2;
        vec4 br = borderradius_arcpie - blur_radius;

        float d = sdRoundBox(p, half_wh, br);
        t = 1 - d / (blur_radius * 2);
        t = clamp(t, 0, 1);
    }
    col = mix(unpackUnorm4x8(colour1).abgr, unpackUnorm4x8(colour2).abgr, t);

    col.a *= shape;
    // col = shape == 0 ? (vec4(1) - col) : col;

    frag_color = col;
    // frag_color = vec4(1);
}
@end

@program _xvg_shapes vs_xvg_shapes fs_xvg_shapes

// ████████╗███████╗██╗  ██╗████████╗
// ╚══██╔══╝██╔════╝╚██╗██╔╝╚══██╔══╝
//    ██║   █████╗   ╚███╔╝    ██║
//    ██║   ██╔══╝   ██╔██╗    ██║
//    ██║   ███████╗██╔╝ ██╗   ██║
//    ╚═╝   ╚══════╝╚═╝  ╚═╝   ╚═╝

@vs vs_xvg_text

struct xvg_text
{
    uint atlas_coords; // Unorm4x8
    uint topleft; // Unorm2x16. Contains signed xy coords, requires -32767 delta

    uint colour; // Unorm4x8
};

layout(binding=0) readonly buffer vs_xvg_text_buffer {
    xvg_text vtx[];
};

layout(binding=0) uniform vs_xvg_text_uniforms {
    vec2 u_view_size;
    int  u_sbo_offset;
};

out vec2 texcoord;
out flat vec4 colour;

void main() {
    uint v_idx = gl_VertexIndex / 6u;
    uint i_idx = gl_VertexIndex - v_idx * 6;

    xvg_text obj = vtx[v_idx + u_sbo_offset];

    //  0.5f,  0.5f,
    // -0.5f, -0.5f,
    //  0.5f, -0.5f,
    // -0.5f,  0.5f,
    // 0, 1, 2,
    // 1, 2, 3,

    // Is odd
    bool is_right = (gl_VertexIndex & 1) == 1;
    bool is_bottom = i_idx >= 2 && i_idx <= 4;

    vec4 atlas_coords = unpackUnorm4x8(obj.atlas_coords);
    vec2 glyph_size   = atlas_coords.zw * vec2(255);
    texcoord = vec2(
        is_right  ? (atlas_coords.x + atlas_coords.z) : atlas_coords.x,
        is_bottom ? (atlas_coords.y + atlas_coords.w) : atlas_coords.y
    );

    vec2 topleft = unpackUnorm2x16(obj.topleft) * vec2(65535) - vec2(32767);

    vec2 pos = vec2(
        is_right  ? (topleft.x + glyph_size.x) : topleft.x,
        is_bottom ? (topleft.y + glyph_size.y) : topleft.y
    );
    pos = (pos + pos) / u_view_size - vec2(1);

	gl_Position = vec4(pos.x, -pos.y, 0, 1);

    colour = unpackUnorm4x8(obj.colour).abgr;
}
@end

@fs fs_xvg_text_singlechannel
layout(binding=1) uniform texture2D fs_xvg_text_tex;
layout(binding=0) uniform sampler fs_xvg_text_smp;

in vec2 texcoord;
in flat vec4 colour;

out vec4 frag_colour;

void main() {
    float alpha = texture(sampler2D(fs_xvg_text_tex, fs_xvg_text_smp), texcoord).r;
    frag_colour = vec4(colour.rgb, colour.a * alpha);
}
@end

@fs fs_xvg_text_multichannel
layout(binding=1) uniform texture2D fs_xvg_text_tex;
layout(binding=0) uniform sampler fs_xvg_text_smp;

in vec2 texcoord;
in flat vec4 colour;

layout(location=0, index=0) out vec4 frag_colour;
layout(location=0, index=1) out vec4 blend_weights;

void main() {
    vec3 pixel_coverages = texture(sampler2D(fs_xvg_text_tex, fs_xvg_text_smp), texcoord).rgb;

    frag_colour = colour * vec4(pixel_coverages, 1);
	blend_weights = vec4(colour.a * pixel_coverages, colour.a);
}
@end

@program _xvg_text_singlechannel vs_xvg_text fs_xvg_text_singlechannel
@program _xvg_text_multichannel  vs_xvg_text fs_xvg_text_multichannel