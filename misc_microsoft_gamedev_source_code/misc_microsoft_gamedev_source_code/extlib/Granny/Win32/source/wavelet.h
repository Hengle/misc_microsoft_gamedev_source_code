#include "radbase.h"

#define SMALLEST_DWT_ROW 12
#define SMALLEST_DWT_COL 10

// apply DWT (9/7) wavelet by rows
void fDWTrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight );

// apply Harr wavelet by columns
void fHarrcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight );

// apply Harr wavelet by rows
void fHarrrow( S16* dest, S32 ppitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight );

// apply DWT (9/7) wavelet by columns
void fHarrcol( S16* dest, S32 pitch, S16 const* input, S32 inpitch, U32 width, U32 height, S32 starty, S32 subheight );

// perform inverse DWT (9/7) wavelet by rows
void iDWTrow( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, U8 const * row_mask, S32 starty, S32 subheight );

// perform inverse DWT (9/7) wavelet by columns
void iDWTcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, S32 starty, S32 subheight );

// perform Harr wavelet by rows
void iHarrrow( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, U8 const * row_mask, S32 starty, S32 subheight );

// perform Harr wavelet by columns
void iHarrcol( S16 * dest, S32 pitch, S16 const * input, S32 ppitch, U32 width, U32 height, S32 starty, S32 subheight );

// apply DWT (9/7) wavelet across two dimensions (flips to Harr when too small)
void fDWT2D( S16* buffer, S32 pitch, U32 width, U32 height, S16* temp );

// perform inverse DWT (9/7) wavelet across two dimensions with a mask (flips to Harr when too small)
void iDWT2D( S16* buffer, S32 pitch, U32 width, U32 height, U8 const * row_mask, S16* temp );

