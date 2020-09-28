// File: shaderRegs.inc

#define SHADER_REGS_VER 115

#define MAX_BONES_PER_VERT 4

// Vertex shader skinning regs

#define INSTANCE_DATA_REG              c0

// X = rigid bone index
// Y = instance index multiplier
// Z = instance index divider
// W = instance light array multiplier
#define INSTANCE_CONTROL_REG           c4

// Boolean - Skinning
#define ONE_BONE_REG                   b0
#define FOUR_BONES_REG                 b1

// Boolean - Local lighting
#define ENABLE_LOCAL_LIGHTS_REG        b2
#define ENABLE_LOCAL_SHADOWING_REG     b3

// Boolean - Maps
#define ENABLE_SPEC_MAP_REG            b5
#define ENABLE_OPACITY_MAP_REG         b6
#define ENABLE_XFORM_MAP_REG           b7
#define ENABLE_SELF_MAP_REG            b8
#define ENABLE_AO_MAP_REG              b9
#define ENABLE_ENV_MAP_REG             b10
#define ENABLE_ENVMASK_MAP_REG         b11

#define FIRST_MAP_REG                  5
#define LAST_MAP_REG                   11

// Integer
#define NUM_LOCAL_LIGHTS_REG           i0

// Pixel shader sampler regs

#define DIFFUSE_SAMPLER_REG            s0
#define BUMP_SAMPLER_REG               s1
#define SPECULAR_SAMPLER_REG           s2
#define OPACITY_SAMPLER_REG            s3
#define XFORM_SAMPLER_REG              s4
#define SELF_SAMPLER_REG               s5
#define AO_SAMPLER_REG                 s6
#define ENV_SAMPLER_REG                s7
#define ENV_MASK_SAMPLER_REG           s8

// Pixel shader constant regs

// W of tint color is overall alpha
#define TINT_COLOR_REG                 c192
#define SPEC_COLOR_POWER_REG           c193
#define HDR_TEX_SCALE_REG              c194
#define ENV_CONTROL_REG                c195

#define VIS_CONTROL_0_REG              c196
#define VIS_CONTROL_1_REG              c197
#define VIS_CONTROL_2_REG              c198
#define VIS_CONTROL_3_REG              c199





