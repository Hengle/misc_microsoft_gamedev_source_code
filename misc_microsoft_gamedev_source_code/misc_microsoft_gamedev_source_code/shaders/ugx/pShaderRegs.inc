// File: pShaderRegs.inc
// Important: If you modify the # of registers allocated to pixel/vertex shaders, be sure to also change 
// FIRST_VERTEX_SHADER_SAMPLER in ugxGeomUberSectionRenderer.cpp!

#define PSHADER_REGS_VER 120

// Boolean - Local lighting
#define ENABLE_LOCAL_PIXEL_LIGHTS_REG  b0
#define ENABLE_LOCAL_SHADOWING_REG     b1

// Boolean - Maps
// Should be sequential!
#define ENABLE_SPEC_MAP_REG            b2
#define ENABLE_OPACITY_MAP_REG         b3
#define ENABLE_XFORM_MAP_REG           b4
#define ENABLE_SELF_MAP_REG            b5
#define ENABLE_AO_MAP_REG              b6
#define ENABLE_ENV_MAP_REG             b7
#define ENABLE_ENVMASK_MAP_REG         b8
#define ENABLE_EMXFORM_MAP_REG         b9
#define ENABLE_HIGHLIGHT_MAP_REG       b10
#define ENABLE_MODULATE_MAP_REG        b11

// If you add any more maps, be sure to update FIRST_MAP_ENABLE_REG/LAST_MAP_ENABLE_REG !
#define FIRST_MAP_ENABLE_REG           2
#define LAST_MAP_ENABLE_REG            11

#define ENABLE_TWO_SIDED_REG           b12

#define ENABLE_LOCAL_REFL_REG          b13

#define ENABLE_BLACKMAP_REG            b14

#define ENABLE_LIGHT_BUFFERING_REG     b15

#define ENABLE_ADD_TEX_REG             b16
#define ENABLE_LERP_TEX_REG            b17

// Integer
#define NUM_LOCAL_PIXEL_LIGHTS_REG     i0

// Pixel shader sampler regs

#define DIFFUSE_SAMPLER_REG               s0
#define BUMP_SAMPLER_REG                  s1
#define SPECULAR_SAMPLER_REG              s2
#define OPACITY_SAMPLER_REG               s3
#define XFORM_SAMPLER_REG                 s4
#define SELF_SAMPLER_REG                  s5
#define AO_SAMPLER_REG                    s6
#define ENV_SAMPLER_REG                   s7
#define ENV_MASK_SAMPLER_REG              s8
#define EMXFORM_SAMPLER_REG               s9
#define HIGHLIGHT_SAMPLER_REG             s10
#define BLACKMAP_SAMPLER_REG              s11
#define MODULATE_SAMPLER_REG              s12
#define LIGHT_BUFFER_COLOR_SAMPLER_REG    s13
#define LIGHT_BUFFER_VECTOR_SAMPLER_REG   s14

#define ADD_TEX_SAMPLER_REG               s15
#define LERP_TEX_SAMPLER_REG              s16

// Pixel shader constant regs

// W of tint color is overall alpha
#define TINT_COLOR_REG                 c160
#define SPEC_COLOR_POWER_REG           c161
// x, y, z - HDR tex intensity scales
#define HDR_TEX_SCALE_REG              c162

// x - vis index or fresnel power
// y - sharpness
// z - fresnel
// w - intensity
#define ENV_VIS_CONTROL_REG            c163

// UV Ofs:
#define UV_OFFSET_0_REG                c164
#define UV_OFFSET_1_REG                c165
#define UV_OFFSET_2_REG                c166
#define UV_OFFSET_3_REG                c167
#define UV_OFFSET_4_REG                c168
#define UV_OFFSET_5_REG                c169
#define NUM_UV_OFFSET_REGS             6

// Diffuse Bump Spec Opacity XForm Self AO EnvMask EMXForm  Distortion
// x       y    z    w       x     y    z  w       x        y
#define UV_CHANNEL_0_REG               c170
#define UV_CHANNEL_1_REG               c171
#define UV_CHANNEL_2_REG               c172
#define NUM_UV_CHANNEL_REGS            3

#define BLACKMAP_PARAMS0_REG           c173
#define BLACKMAP_PARAMS1_REG           c174
#define BLACKMAP_PARAMS2_REG           c175

#define WORLD_TO_LIGHTBUF0_REG         c176
#define WORLD_TO_LIGHTBUF1_REG         c177
#define WORLD_TO_LIGHTBUF2_REG         c178

#define ADD_TEX_PARAMS_REG             c179
#define ADD_TEX_UVXFORM0_REG           c180
#define ADD_TEX_UVXFORM1_REG           c181

// The lerp tex params must be sequential!
#define LERP_TEX_PARAMS_REG            c182
#define LERP_TEX_UVXFORM0_REG          c183
#define LERP_TEX_UVXFORM1_REG          c184

#define FIRST_PSHADER_MATERIAL_REG     160
#define NUM_PSHADER_MATERIAL_REGS      28