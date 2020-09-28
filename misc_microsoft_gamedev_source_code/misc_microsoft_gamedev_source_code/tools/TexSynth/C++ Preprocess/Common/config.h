// ------------------------------------------------------
#ifndef __CONFIG__
#define __CONFIG__
// ------------------------------------------------------

#define USE_ANN_LIBRARY

// ------------------------------------------------------
// change does *not* require to recompute analyse files

#define NUM_RECOLORED_PCA_COMPONENTS       8
#define NUM_RUNTIME_PCA_COMPONENTS         8

#define NEIGHBORHOOD_RADIUS_SEARCH     1 // 3x3 (size is 2*R+1)
//                                    ( K_NEAREST_SEARCH < min(K_NEAREST , K_NEAREST_DIVERSITY) )
#define K_NEAREST_SEARCH               2 // 1..3

//#define K_NEAREST_RANDOM
#define JITTER_RANDOM

#define PART_CORRECTION_STEPS         2
#define FULL_CORRECTION_STEPS         0

//#define CORRECTION_1212
#define CORRECTION_1234
//#define CORRECTION_9
//#define CORRECTION_SEQUENTIAL

#define PARALLEL_RANDOM

#define QUANTIZE_NUM_BITS             8
#define QUANTIZATION_RADIUS           3.0f
#define USE_QUANTIZATION

#define FORCE_GAUSS false

// -------------- GPU synthesis configuration ----------

#define SEPARATE_KN_INDICES_FROM_PROJ

#define USE_CENTER_PIXEL  // UPDATE correction.fx !!

// ------------------------------------------------------
// change *does* require to recompute analyse files

#define COARSE_LEVELS_SKIP_ANALYSE     3 // ( >= 1 ; 1 -> no level skipped)
#define COARSE_LEVELS_SKIP_SYNTHESIS   1 // ( >= 1 ; 1 -> no level skipped)

#define NEIGHBORHOOD_RADIUS_RECOLOR     2 // 1 - 3x3   2 - 5x5  3 - 7x7  (size is 2*R+1)
//#define NEIGHBORHOOD_RADIUS_SIMSET    3 // 3 - 7x7   4 - 9x9 (size is 2*R+1)

#define MIN_K_NEAREST_DISTANCE         1.0/8.0

#define K_NEAREST                      2 // 2,4,8

#define USE_STACK_EXEMPLAR  // UPDATE correction.fx !!    //// WARNING :::> get rid of pyramid version - code path probably broken

// misc
#define NEIGHBORHOOD_RADIUS_MAX    (max(NEIGHBORHOOD_RADIUS_RT,NEIGHBORHOOD_RADIUS_SEARCH))
#define INIT_CONSTRAINT_THRESHOLD  128.0/256.0 // unit = normalized distance

//#define FIRST_LEVEL_WITH_BORDER    3
#define NUM_LEVELS_WITHOUT_BORDER 4 // (3 last levels for 64^2 and 4 last for 128^3 are considered non toroidal)
                                    // This is arbitrary and experimentally chosen. Most likely related to feature size.
// ----------------------


// ----------------------

#endif
