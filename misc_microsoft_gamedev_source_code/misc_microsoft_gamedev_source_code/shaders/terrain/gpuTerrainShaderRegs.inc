// File: gpuTerrainShaderRegs.inc

#define TERRAIN_SHADER_REGS_VER 101

// Boolean - Local lighting
#define ENABLE_LOCAL_LIGHTS_REG           b2
#define ENABLE_LOCAL_SHADOWING_REG        b3
#define ENABLE_EXTENDED_LOCAL_LIGHTS_REG  b8

// Integer
#define NUM_LOCAL_LIGHTS_REG           i0
#define NUM_EXTENDED_LOCAL_LIGHTS_REG  i1
#define NUM_BLEND_LAYERS_REG           i12
 


// Pixel shader constant regs



#define EXTENDED_LOCAL_LIGHTING_PARAMS_REG c190


#define VIS_CONTROL_0_REG              c196
#define VIS_CONTROL_1_REG              c197
#define VIS_CONTROL_2_REG              c198
#define VIS_CONTROL_3_REG              c199


//THESE ARE FOR COMPOSITING

#define TERRAIN_SHADER_COMPOS_REGS_VER 101

#define DECAL_UVSCALE				   c25
#define DECAL_PROPERTIES			   c26

#define LAYER_DATA					   c27
#define MAX_NUM_LAYERS				   32

#define HDR_DATA_CURRENT			   c58

