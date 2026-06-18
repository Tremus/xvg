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
    //   - uint tex_idx;
    //   - uint sdf_type && grad_type; // 4 bits each
    //   - float stroke_width;
    //   - float feather;
    uint sdf_data;

    // packed with either:
    // - border radius (unorm4x8)
    // - arc/pie rotate and range (unorm2x16)
    // - round line stroke offsets
    uint borderradius_arcpie;
    
    uint texcoords_xy; // unorm2x16
    uint texcoords_wh; // unorm2x16

    // line pots (unorm2x16)
    // straight line (bool)
    uint buffer_idx_range; // unorm2x16
    uint _unused;

    uint colour1;
    uint colour2;

    vec2 gradient_a;
    vec2 gradient_b;
};

layout(binding=0) readonly buffer vs_xvg_shapes_buffer {
    xvg_shape vtx[];
};

layout(binding=0) uniform vs_xvg_shapes_uniforms {
    vec2 u_size;
    vec2 u_texture_size_1;
    vec2 u_texture_size_2;
    vec2 u_texture_size_3;
    vec2 u_texture_size_4;
    int  u_storage_buffer_offset;
};

#define PI 3.141592653589793

// 0-15
out vec2 p;
out vec2 texcoord;

// 16-31
out flat uint tex_idx;
out flat uint sdf_type;
out flat uint grad_type;
out flat uint buffer_idx_range;

// 32-47
out flat float px_scale;
out flat float px_inc;
out flat float feather;
out flat float stroke_width;

// 48-63
// - border_radius (right)
// - vec2(cos(rotate), sin(rotate))
out flat uint borderradius_arcpie_compressed_1; // snorm2x16
// - border_radius (left)
// - vec2(sin(range), cos(range))
out flat uint borderradius_arcpie_compressed_2; // snorm2x16
out flat uint colour1;
out flat uint colour2;

// 64-79
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

// 80-95, nothing. 16 bytes spare

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
#define XVG_SHAPE_LINE_ROUND       11
#define XVG_SHAPE_LINE_PLOT        12
#define XVG_SHAPE_LINE_PLOT_BG     13

#define XVG_COLOUR_SOLID           0
#define XVG_COLOUR_LINEAR_GRADIENT 1
#define XVG_COLOUR_RADIAL_GRADIENT 2
#define XVG_COLOUR_CONIC_GRADIENT  3
#define XVG_COLOUR_DROP_SHADOW     4
#define XVG_COLOUR_INNER_SHADOW    5

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

    p        = vec2(is_right  ? 1 : -1, is_bottom ? -1 : 1);
    px_scale = vw / vh;

    vec4 sdf_data = unpackUnorm4x8(vert.sdf_data);

    tex_idx   = (vert.sdf_data >> 0) & 0xff;
    sdf_type  = (vert.sdf_data >> 8) & 0x0f;
    grad_type = (vert.sdf_data >> 12) & 0x0f;
    feather   = sdf_data.z * 1;

    float stroke_width_px =      sdf_data.w * 16;
    stroke_width = px_scale * 2 * stroke_width_px / vw;

    vec2 texcoords_xy = vec2((vert.texcoords_xy & 0xffff),
                              (vert.texcoords_xy >> 16)) - vec2(32767.0);
    vec2 texcoords_rb  = vec2((vert.texcoords_wh & 0xffff),
                              (vert.texcoords_wh >> 16)) + texcoords_xy;

    vec2 texture_size = tex_idx == 1 ? u_texture_size_1 : tex_idx == 2 ? u_texture_size_2 : tex_idx == 3 ? u_texture_size_3 : tex_idx == 4 ? u_texture_size_4 : vec2(1);
    texcoords_xy = texcoords_xy / texture_size;
    texcoords_rb = texcoords_rb / texture_size;

    texcoord = vec2(
        is_right  ? texcoords_rb.x : texcoords_xy.x,
        is_bottom ? texcoords_rb.y : texcoords_xy.y
    );

    vec4 borderradius_arcpie = vec4(0);
    if (sdf_type == XVG_SHAPE_RECTANGLE_FILL
    ||  sdf_type == XVG_SHAPE_RECTANGLE_STROKE
    ||  sdf_type == XVG_SHAPE_LINE_PLOT
    ||  sdf_type == XVG_SHAPE_LINE_PLOT_BG
        )
    {
        borderradius_arcpie = (unpackUnorm4x8(vert.borderradius_arcpie) * 255) / vec4(vh * 0.5);        
    }

    if (sdf_type == XVG_SHAPE_TRIANGLE_FILL
    ||  sdf_type == XVG_SHAPE_TRIANGLE_STROKE
    ||  sdf_type == XVG_SHAPE_PIE_FILL
    ||  sdf_type == XVG_SHAPE_PIE_STROKE
    ||  sdf_type == XVG_SHAPE_ARC_ROUND_STROKE
    ||  sdf_type == XVG_SHAPE_ARC_BUTT_STROKE
    )
    {
        vec2 arcpie   = 2 * PI * unpackUnorm2x16(vert.borderradius_arcpie);
        borderradius_arcpie.xy = vec2(cos(arcpie.x), sin(arcpie.x));
        borderradius_arcpie.zw = vec2(sin(arcpie.y), cos(arcpie.y));
    }

    // 48-63
    borderradius_arcpie_compressed_1 = packSnorm2x16(borderradius_arcpie.xy);
    borderradius_arcpie_compressed_2 = packSnorm2x16(borderradius_arcpie.zw);
    colour1 = vert.colour1;
    colour2 = vert.colour2;

    if (sdf_type == XVG_SHAPE_LINE_PLOT
    ||  sdf_type == XVG_SHAPE_LINE_PLOT_BG
    ||  sdf_type == XVG_SHAPE_LINE_ROUND
    )
    {
        vec2 range = unpackUnorm2x16(vert.buffer_idx_range) * vec2(65535);

        // buffer_idx       = is_right ? range.y : range.x;
        buffer_idx_range = vert.buffer_idx_range;
        px_inc       = 2.0 / u_size.y;
    }

    if (grad_type == XVG_COLOUR_LINEAR_GRADIENT)
    {
        gradient_a = (vert.gradient_a - vert.topleft) / vec2(vw, vh); // stop 1 xy
        gradient_b = (vert.gradient_b - vert.topleft) / vec2(vw, vh); // stop 2 xy
    }
    if (grad_type == XVG_COLOUR_RADIAL_GRADIENT)
    {
        gradient_a = (vert.gradient_a - vert.topleft) / vec2(vw, vh); // stop 2 cx,cy
        gradient_b = vec2(vw, vh) / vert.gradient_b;                  // stop 1 radius
    }
    if (grad_type == XVG_COLOUR_CONIC_GRADIENT)
    {
        float rotation = vert.gradient_a.x* 2 * PI;      // turns to radians
        gradient_a = vec2(cos(rotation), sin(rotation)); // rotation
        gradient_b = vert.gradient_b;                    // range (turns)
    }
    if (grad_type == XVG_COLOUR_DROP_SHADOW || grad_type == XVG_COLOUR_INNER_SHADOW)
    {
        gradient_a = vert.gradient_a / vec2(-vw, vh); // translate x/y
        gradient_b = vert.gradient_b / vh;            // blur radius & spread
    }
}
@end

@fs fs_xvg_shapes
precision mediump float;
layout(binding=2) uniform texture2D fs_xvg_shapes_tex1;
layout(binding=3) uniform texture2D fs_xvg_shapes_tex2;
layout(binding=4) uniform texture2D fs_xvg_shapes_tex3;
layout(binding=5) uniform texture2D fs_xvg_shapes_tex4;
layout(binding=0) uniform sampler fs_xvg_shapes_smp;

struct xvg_line_segment {
    float y;
};

layout(binding=1) readonly buffer fs_xvg_shapes_line_buffer {
    xvg_line_segment line_buffer[];
};

// 0-15
in vec2 p;
in vec2 texcoord;
// 16-31
in flat uint tex_idx;
in flat uint sdf_type;
in flat uint grad_type; 
in flat uint buffer_idx_range;
// 32-47
in flat float px_scale;
in flat float px_inc;
in flat float feather;
in flat float stroke_width;

// 48-63
in flat uint borderradius_arcpie_compressed_1; // snorm2x16
in flat uint borderradius_arcpie_compressed_2; // snorm2x16
in flat uint colour1;
in flat uint colour2;

// 64-79
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
#define XVG_SHAPE_LINE_ROUND       11
#define XVG_SHAPE_LINE_PLOT        12
#define XVG_SHAPE_LINE_PLOT_BG     13

#define XVG_COLOUR_SOLID  0
#define XVG_COLOUR_LINEAR_GRADIENT 1
#define XVG_COLOUR_RADIAL_GRADIENT 2
#define XVG_COLOUR_CONIC_GRADIENT  3
#define XVG_COLOUR_DROP_SHADOW     4
#define XVG_COLOUR_INNER_SHADOW    5

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
    vec4 col = vec4(1);
    if (tex_idx == 1)
        col = texture(sampler2D(fs_xvg_shapes_tex1, fs_xvg_shapes_smp), texcoord);
    if (tex_idx == 2)
        col = texture(sampler2D(fs_xvg_shapes_tex2, fs_xvg_shapes_smp), texcoord);
    if (tex_idx == 3)
        col = texture(sampler2D(fs_xvg_shapes_tex3, fs_xvg_shapes_smp), texcoord);
    if (tex_idx == 4)
        col = texture(sampler2D(fs_xvg_shapes_tex4, fs_xvg_shapes_smp), texcoord);

    vec2 p_scale = vec2(px_scale, 1);
    float shape = 1;

    vec4 borderradius_arcpie = vec4(unpackSnorm2x16(borderradius_arcpie_compressed_1),
                                    unpackSnorm2x16(borderradius_arcpie_compressed_2));
    if (sdf_type == XVG_SHAPE_RECTANGLE_FILL)
    {
        vec2 b = p_scale;
        float d = sdRoundBox(p * p_scale, b, borderradius_arcpie);
        shape = smoothstep(feather, 0, d + feather * 0.5);
    }
    if (sdf_type == XVG_SHAPE_RECTANGLE_STROKE)
    {
        vec2 b = p_scale;
        float d = sdRoundBox(p * p_scale, b, borderradius_arcpie);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        float inner = smoothstep(feather, 0, d + stroke_width + feather * 0.5);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_CIRCLE_FILL)
    {
        float d = 1 - length(p);
        float outer = smoothstep(0, feather, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_CIRCLE_STROKE)
    {
        float d = 1 - length(p);
        float outer = smoothstep(0, feather, d + feather * 0.5);
        float inner = smoothstep(0, feather, d + feather * 0.5 - stroke_width);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_TRIANGLE_FILL)
    {
        vec2 p2 = vec2(p.x * borderradius_arcpie.x - p.y * borderradius_arcpie.y,
                       p.x * borderradius_arcpie.y + p.y * borderradius_arcpie.x);
        float d = sdEquilateralTriangle(p2, 0.86);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_TRIANGLE_STROKE)
    {
        vec2 p2 = vec2(p.x * borderradius_arcpie.x - p.y * borderradius_arcpie.y,
                       p.x * borderradius_arcpie.y + p.y * borderradius_arcpie.x);
        float d = sdEquilateralTriangle(p2, 0.86);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        float inner = smoothstep(feather, 0, d + feather * 0.5 + stroke_width);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_PIE_FILL)
    {
        vec2 p2 = vec2(p.x * borderradius_arcpie.x - p.y * borderradius_arcpie.y,
                       p.x * borderradius_arcpie.y + p.y * borderradius_arcpie.x);
        float d = sdPie(p2, borderradius_arcpie.zw, 1.0);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_PIE_STROKE)
    {
        vec2 p2 = vec2(p.x * borderradius_arcpie.x - p.y * borderradius_arcpie.y,
                       p.x * borderradius_arcpie.y + p.y * borderradius_arcpie.x);
        float d = sdPie(p2, borderradius_arcpie.zw, 1.0);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        float inner = smoothstep(feather, 0, d + feather * 0.5 + stroke_width);
        shape = outer - inner;
    }
    if (sdf_type == XVG_SHAPE_ARC_ROUND_STROKE)
    {
        vec2 p2 = vec2(p.x * borderradius_arcpie.x - p.y * borderradius_arcpie.y,
                       p.x * borderradius_arcpie.y + p.y * borderradius_arcpie.x);
        float d = sdArc(p2, borderradius_arcpie.zw, 1.0 - stroke_width, stroke_width);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_ARC_BUTT_STROKE)
    {
        vec2 p2 = vec2(p.x * borderradius_arcpie.x - p.y * borderradius_arcpie.y,
                       p.x * borderradius_arcpie.y + p.y * borderradius_arcpie.x);
        float d = sdRing(p2, borderradius_arcpie.zw, 1.0 - stroke_width, stroke_width*2);
        float outer = smoothstep(feather, 0, d + feather * 0.5);
        shape = outer;
    }
    if (sdf_type == XVG_SHAPE_LINE_ROUND)
    {
        vec2 p2  = p * p_scale;

        vec2 pt0 = vec2(-px_scale + stroke_width * 2,
            (buffer_idx_range == 0) ?  (1 - stroke_width * 2) : (-1 + stroke_width * 2));
        vec2 pt1 = vec2(px_scale - stroke_width * 2,
            (buffer_idx_range == 0) ? (-1 + stroke_width * 2) : (1 - stroke_width * 2));

        float d  = sdSegment(p2, pt0, pt1) - stroke_width;
        d        = smoothstep(feather, 0, d + feather * 0.5);
        shape = d;
    }
    if (sdf_type == XVG_SHAPE_LINE_PLOT)
    {
        // Read buffer data

        vec2 range = unpackUnorm2x16(buffer_idx_range) * vec2(65535);

        float buffer_idx = mix(range.x, range.y, p.x * 0.5 + 0.5);

        // buffer_idx       = is_right ? buffer_idx_range.y : buffer_idx_range.x;
        // buffer_begin_idx = int(buffer_idx_range.x);
        // buffer_end_idx   = int(buffer_idx_range.y);

        // If we pad the storage buffer by 1 on each side with real values, then we can get nicer looking clipped edges
        float idx      = min(buffer_idx,     range.y - 1);
        float idx_prev = max(buffer_idx - 1, range.x);
        float idx_next = min(buffer_idx + 1, range.y - 1);

        float line_y      = line_buffer[int(idx)].y;
        float line_y_prev = line_buffer[int(idx_prev)].y;
        float line_y_next = line_buffer[int(idx_next)].y;

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
        vec2 a = vec2(p.x - px_inc, line_y_prev);
        vec2 b = vec2(p.x         , line_y);
        vec2 c = vec2(p.x + px_inc, line_y_next);

        float d1 = sdSegment(p, a, b);
        float d2 = sdSegment(p, b, c);
        float d  = min(d1, d2);

        float f = feather;

        shape = smoothstep(f*0.5, 0, d-stroke_width*0.5 + f*0.5);
        shape = clamp(shape, 0, 1);

        // Crop with rounded rectange
        vec2  crop_b = p_scale;
        float crop_d = sdRoundBox(p * p_scale, crop_b, borderradius_arcpie);
        float crop_shape = smoothstep(feather, 0, crop_d + feather * 0.5);

        shape *= crop_shape;
    }
    if (sdf_type == XVG_SHAPE_LINE_PLOT_BG)
    {
        vec2 range = unpackUnorm2x16(buffer_idx_range) * vec2(65535);
        float buffer_idx = mix(range.x, range.y, p.x * 0.5 + 0.5);
        float idx      = min(buffer_idx,     range.y - 1);
        float line_y      = line_buffer[int(idx)].y;

        float uv_y = p.y * 0.5 + 0.5;
        shape = uv_y < abs(line_y) ? 1.0 : 0.0;
    }


    float t = 0;
    if (grad_type == XVG_COLOUR_LINEAR_GRADIENT)
    {
        vec2 uv_norm = vec2(p.x * 0.5 + 0.5,  p.y * -0.5 + 0.5);

        vec2 v  = gradient_a - gradient_b;
        vec2 w  = gradient_a - uv_norm;
        t = dot(v, w) / dot(v, v);
        t = clamp(t, 0, 1);
    }
    if (grad_type == XVG_COLOUR_RADIAL_GRADIENT)
    {
        // translate & scale
        vec2 uv_norm       = vec2(p.x * 0.5 + 0.5,  p.y * -0.5 + 0.5);
        vec2 ellipse_space = (uv_norm - gradient_a) * gradient_b;

        t = clamp(length(ellipse_space), 0.0, 1.0);
    }
    if (grad_type == XVG_COLOUR_CONIC_GRADIENT)
    {
        // Change start/end position of the gradient
        vec2 p2 = p * p_scale;
        vec2 p3 = vec2(p2.x * gradient_a.x - p2.y * gradient_a.y,
                       p2.x * gradient_a.y + p2.y * gradient_a.x);
        float angle = atan(p3.x, p3.y);

        // Crops the gradient range
        float range = gradient_b.x;
        t = angle / (PI * 2) + 0.5;
        t = smoothstep(0, range, t);
    }
    if (grad_type == XVG_COLOUR_DROP_SHADOW || grad_type == XVG_COLOUR_INNER_SHADOW)
    {
        vec2  xy_offset   = gradient_a;
        float blur_radius = gradient_b.x;
        float blur_spread = gradient_b.y;
        vec2 p2 = (p + xy_offset) * p_scale;

        if (sdf_type == XVG_SHAPE_RECTANGLE_FILL)
        {
            vec4 br = borderradius_arcpie;

            float d = sdRoundBox(p2, p_scale - blur_radius * 2 - blur_spread * 2, br);
            d = smoothstep(blur_radius * 4, 0, d + blur_radius * 2);
            t = d;
        }
        if (sdf_type == XVG_SHAPE_CIRCLE_FILL)
        {
            float d = 1 - length(p2);
            d = 1 - smoothstep(blur_radius * 4, 0, d - (blur_spread * 2));
            t = d;
        }
        if (sdf_type == XVG_SHAPE_TRIANGLE_FILL)
        {
            vec2 p3 = vec2(p2.x * borderradius_arcpie.x - p2.y * borderradius_arcpie.y,
                           p2.x * borderradius_arcpie.y + p2.y * borderradius_arcpie.x);
            float d = sdEquilateralTriangle(p3, 0.86 - blur_radius*4 - blur_spread*4);
            d = smoothstep(blur_radius * 4, 0, d + blur_radius*2);
            t = d;
        }
        if (sdf_type == XVG_SHAPE_PIE_FILL)
        {
            vec2 p3 = vec2(p2.x * borderradius_arcpie.x - p2.y * borderradius_arcpie.y,
                           p2.x * borderradius_arcpie.y + p2.y * borderradius_arcpie.x);
            float d = sdPie(p3, borderradius_arcpie.zw, 1 - blur_radius*2 - blur_spread*2);
            d = smoothstep(blur_radius * 4, 0, d+blur_radius*2);
            t = d;
        }
        if (sdf_type == XVG_SHAPE_ARC_ROUND_STROKE)
        {
            vec2 p3 = vec2(p2.x * borderradius_arcpie.x - p2.y * borderradius_arcpie.y,
                           p2.x * borderradius_arcpie.y + p2.y * borderradius_arcpie.x);
            float d = sdArc(p3, borderradius_arcpie.zw, 1.0 - stroke_width-blur_radius*2- blur_spread*2, stroke_width);
            d = smoothstep(blur_radius * 4, 0, d + blur_radius*2);
            t = d;
        }
        if (sdf_type == XVG_SHAPE_ARC_BUTT_STROKE)
        {
            vec2 p3 = vec2(p2.x * borderradius_arcpie.x - p2.y * borderradius_arcpie.y,
                           p2.x * borderradius_arcpie.y + p2.y * borderradius_arcpie.x);
            float d = sdRing(p3, borderradius_arcpie.zw, 1.0 - stroke_width-blur_radius*2- blur_spread*2, stroke_width*2);
            d = smoothstep(blur_radius * 4, 0, d + blur_radius*2);
            t = d;
        }
        if (sdf_type == XVG_SHAPE_LINE_ROUND)
        {
            vec2 p3 = p * p_scale; // Don't apply translate to this positon

            vec2 pt0 = vec2(-px_scale + stroke_width * 2,
                (buffer_idx_range == 0) ?  (1 - stroke_width * 2) : (-1 + stroke_width * 2));
            vec2 pt1 = vec2(px_scale - stroke_width * 2,
                (buffer_idx_range == 0) ? (-1 + stroke_width * 2) : (1 - stroke_width * 2));

            pt0 -= xy_offset * p_scale;
            pt1 -= xy_offset * p_scale;

            float d  = sdSegment(p3, pt0, pt1) - blur_spread - stroke_width;
            d = smoothstep(blur_radius * 4, 0, d + blur_radius*2);
            t = d;
        }

        // For very complex shapes, remove masking. This helps preseve blurred corners and divots
        // For inner shadows, we want the masking
        shape = grad_type == XVG_COLOUR_DROP_SHADOW ? 1 : shape;
    }
    col *= mix(unpackUnorm4x8(colour1).abgr, unpackUnorm4x8(colour2).abgr, t);

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
    uint topleft; // Unorm2x16. Contains signed xy coords, requires -32767 delta
    uint atlas_coords; // Unorm4x8
    uint atlas_idx;
    uint colour; // Unorm4x8
};

layout(binding=0) readonly buffer vs_xvg_text_buffer {
    xvg_text vtx[];
};

layout(binding=0) uniform vs_xvg_text_uniforms {
    vec2  u_view_size;
    int   u_sbo_offset;
};

out vec2 texcoord;
out flat uint atlas_idx;
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

    vec4 atlas_coords = unpackUnorm4x8(obj.atlas_coords) * 255;
    texcoord = vec2(
        is_right  ? (atlas_coords.x + atlas_coords.z) : atlas_coords.x,
        is_bottom ? (atlas_coords.y + atlas_coords.w) : atlas_coords.y
    );

    atlas_idx = obj.atlas_idx;

    vec2 topleft = unpackUnorm2x16(obj.topleft) * vec2(65535) - vec2(32767);
    // vec2 topleft = obj.topleft;

    vec2 pos = vec2(
        is_right  ? (topleft.x + atlas_coords.z) : topleft.x,
        is_bottom ? (topleft.y + atlas_coords.w) : topleft.y
    );
    pos = (pos + pos) / u_view_size - vec2(1);

	gl_Position = vec4(pos.x, -pos.y, 0, 1);

    colour = unpackUnorm4x8(obj.colour).abgr;
}
@end

@fs fs_xvg_text_singlechannel
layout(binding=0) uniform sampler fs_xvg_text_smp;
layout(binding=1) uniform texture2D fs_xvg_text_atlas1;
layout(binding=2) uniform texture2D fs_xvg_text_atlas2;
layout(binding=3) uniform texture2D fs_xvg_text_atlas3;
layout(binding=4) uniform texture2D fs_xvg_text_atlas4;
layout(binding=5) uniform texture2D fs_xvg_text_atlas5;
layout(binding=6) uniform texture2D fs_xvg_text_atlas6;
layout(binding=7) uniform texture2D fs_xvg_text_atlas7;
layout(binding=8) uniform texture2D fs_xvg_text_atlas8;

in vec2 texcoord;
in flat uint atlas_idx;
in flat vec4 colour;

out vec4 frag_colour;

void main() {
    ivec2 itexcoord = ivec2(texcoord);
    float alpha;

    if (atlas_idx == 1)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas1, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 2)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas2, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 3)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas3, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 4)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas4, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 5)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas5, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 6)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas6, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 7)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas7, fs_xvg_text_smp), itexcoord, 0).r;
    if (atlas_idx == 8)
        alpha = texelFetch(sampler2D(fs_xvg_text_atlas8, fs_xvg_text_smp), itexcoord, 0).r;

    frag_colour = vec4(colour.rgb, colour.a * alpha);
}
@end

@fs fs_xvg_text_multichannel
layout(binding=0) uniform sampler fs_xvg_text_smp;
layout(binding=1) uniform texture2D fs_xvg_text_atlas1;
layout(binding=2) uniform texture2D fs_xvg_text_atlas2;
layout(binding=3) uniform texture2D fs_xvg_text_atlas3;
layout(binding=4) uniform texture2D fs_xvg_text_atlas4;
layout(binding=5) uniform texture2D fs_xvg_text_atlas5;
layout(binding=6) uniform texture2D fs_xvg_text_atlas6;
layout(binding=7) uniform texture2D fs_xvg_text_atlas7;
layout(binding=8) uniform texture2D fs_xvg_text_atlas8;

in vec2 texcoord;
in flat uint atlas_idx;
in flat vec4 colour;

layout(location=0, index=0) out vec4 frag_colour;
layout(location=0, index=1) out vec4 blend_weights;

void main() {
    // Normalised coords are too unreliable, even if the texture atlas is very small and when we're using
    // 'nearest neighbour' samplers, they really aren't great at finding the sample we want...
    ivec2 itexcoord = ivec2(texcoord);

    // vec3 pixel_coverages = texelFetch(sampler2D(fs_xvg_text_tex, fs_xvg_text_smp), itexcoord, 0).rgb;
    // vec3 pixel_coverages = texture(sampler2D(fs_xvg_text_tex, fs_xvg_text_smp), texcoord).rgb;

    // NOTE: if we start text_idx from 1, then its really obvious when we haven't initialised it
    vec4 pixel_coverages = vec4(0);
    if (atlas_idx == 1)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas1, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 2)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas2, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 3)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas3, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 4)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas4, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 5)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas5, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 6)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas6, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 7)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas7, fs_xvg_text_smp), itexcoord, 0);
    if (atlas_idx == 8)
        pixel_coverages = texelFetch(sampler2D(fs_xvg_text_atlas8, fs_xvg_text_smp), itexcoord, 0);


    frag_colour = colour * vec4(pixel_coverages.rgb, 1);
	blend_weights = vec4(colour.a * pixel_coverages.rgb, colour.a);
}
@end

@program _xvg_text_singlechannel vs_xvg_text fs_xvg_text_singlechannel
@program _xvg_text_multichannel  vs_xvg_text fs_xvg_text_multichannel