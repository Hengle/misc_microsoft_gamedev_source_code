// File: vShaderRegs.inc

#define VSHADER_REGS_VER 118

#define MAX_BONES_PER_VERT 4

// Vertex shader skinning regs

#define INSTANCE_DATA_REG              c180

// X = rigid bone index
// Y = instance index multiplier
// Z = instance index divider
// W = instance light array multiplier
#define INSTANCE_CONTROL_REG           c184

#define FIRST_VSHADER_INSTANCE_REG     180
#define NUM_VSHADER_INSTANCE_REGS      8

// Boolean - Skinning
#define ONE_BONE_REG                   b0
#define FOUR_BONES_REG                 b1

// TEX1_REG and TEX2_REG must be adjacent
#define TEX1_REG                       b2
#define TEX2_REG                       b3

// Terrain conform regs
#define WORLD_TO_HEIGHTFIELD_REG       c172
#define HEIGHTFIELD_Y_SCALE_OFS_REG    c176

#define FIRST_VSHADER_MATERIAL_REG     172
#define NUM_VSHADER_MATERIAL_REGS      8

