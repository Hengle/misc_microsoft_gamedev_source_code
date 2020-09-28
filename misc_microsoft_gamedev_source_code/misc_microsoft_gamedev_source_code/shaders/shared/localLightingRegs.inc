// Constant regs 0-NUM_LOCAL_LIGHT_PSHADER_CONSTANTS-1 are reserved for light constants. 
// There are 8 constants per pixel shader light. Enough for 20 pixel lights per draw.
#define NUM_LOCAL_LIGHT_PSHADER_CONSTANTS 160

// Be careful changing MAX_LOCAL_SHADOW_BUFFER_SLICES! It controls the size of the local shadow buffer texture array.
#define MAX_LOCAL_SHADOW_BUFFER_SLICES 8

#define DUAL_PARABOLOID_NEAR_PLANE 1.0f
#define DUAL_PARABOLOID_FAR_PLANE 100.0f

