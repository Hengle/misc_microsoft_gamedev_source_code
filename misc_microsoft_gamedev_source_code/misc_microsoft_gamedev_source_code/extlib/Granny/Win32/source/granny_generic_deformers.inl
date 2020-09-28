/* ========================================================================
   $RCSfile: $
   $Date: $
   $Revision: $
   $Creator: Casey Muratori $
   (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
   ======================================================================== */

// NAME - the name of the deformer routine
// INDIRECT - I if indirect, D if direct
// SOURCE_VERTEX_TYPE - the c type of the source vertex
// DEST_VERTEX_TYPE - the c type of the dest vertex
// DEFORM_P - 1 to deform positions, 0 otherwise
// DEFORM_N - 1 to deform normals, 0 otherwise
// DEFORM_G - 1 to deform tangents, 0 otherwise
// DEFORM_B - 1 to deform binormals, 0 otherwise

void
#if DEFINE_CODE
GRANNY
#endif
NAME(int32x Count,
     void const *SourceInit,
     void *DestInit,
#if INDIRECT
     int32x const *TransformTable,
#endif
     matrix_4x4 const *Transforms,
     int32x CopySize,
     int32x SourceStride,
     int32x DestStride)
#if DEFINE_CODE
{
    COUNT_BLOCK(Stringize(NAME));

    SOURCE_VERTEX_TYPE const *Source = (SOURCE_VERTEX_TYPE const *)SourceInit;
    DEST_VERTEX_TYPE *Dest = (DEST_VERTEX_TYPE *)DestInit;

#if (BONE_COUNT == 0)
#if INDIRECT
    matrix_4x4 const &SummedMatrix = Transforms[TransformTable[0]];
#else
    matrix_4x4 const &SummedMatrix = Transforms[0];
#endif
#endif
    
    while(Count--)
    {
#if USE_SUMMED_MATRICES
        //
        // This is the new-skool path
        //

#if (BONE_COUNT != 0)
        DEF_BLEND_VARIABLES;
#endif

#if (BONE_COUNT == 1)
#if INDIRECT
        matrix_4x4 const &SummedMatrix = Transforms[TransformTable[Source->BoneIndex]];
#else
        matrix_4x4 const &SummedMatrix = Transforms[Source->BoneIndex];
#endif
#elif (BONE_COUNT > 0)
#if INDIRECT
        LOAD_WEIGHT_I(0);
#else
        LOAD_WEIGHT_D(0);
#endif
        SET_MATRIX;
#endif

#if (BONE_COUNT > 1)
#if INDIRECT
        LOAD_WEIGHT_I(1);
#else
        LOAD_WEIGHT_D(1);
#endif
        SUM_MATRIX;
#endif

#if (BONE_COUNT > 2)
#if INDIRECT
        LOAD_WEIGHT_I(2);
#else
        LOAD_WEIGHT_D(2);
#endif
        SUM_MATRIX;
#endif

#if (BONE_COUNT > 3)
#if INDIRECT
        LOAD_WEIGHT_I(3);
#else
        LOAD_WEIGHT_D(3);
#endif
        SUM_MATRIX;
#endif

#if DEFORM_P
        TRANSFORM_POINT(Position);
#endif

#if DEFORM_N
        TRANSFORM_VECTOR(Normal);
#endif

#if DEFORM_G
        TRANSFORM_VECTOR(Tangent);
#endif

#if DEFORM_B
        TRANSFORM_VECTOR(Binormal);
#endif
        
#else // --------------------------------------------------------
        //
        // This is the old-skool path
        //

        DEF_WEIGHTS;

#if DEFORM_P
        INIT_P3;
        LOAD_P3;
#endif

#if DEFORM_N
        INIT_N3;
        LOAD_N3;
#endif
        
#if DEFORM_G
        INIT_G3;
        LOAD_G3;
#endif
        
#if DEFORM_B
        INIT_B3;
        LOAD_B3;
#endif
        
#if (BONE_COUNT > 0)
#if INDIRECT
        LOAD_WEIGHT_I(0);
#else
        LOAD_WEIGHT_D(0);
#endif

#if DEFORM_P
        SUM_P3;
#endif
#if DEFORM_N
        SUM_N3;
#endif
#if DEFORM_G
        SUM_G3;
#endif
#if DEFORM_B
        SUM_B3;
#endif

#endif
        
#if (BONE_COUNT > 1)
#if INDIRECT
        LOAD_WEIGHT_I(1);
#else
        LOAD_WEIGHT_D(1);
#endif

#if DEFORM_P
        SUM_P3;
#endif
#if DEFORM_N
        SUM_N3;
#endif
#if DEFORM_G
        SUM_G3;
#endif
#if DEFORM_B
        SUM_B3;
#endif

#endif

#if (BONE_COUNT > 2)
#if INDIRECT
        LOAD_WEIGHT_I(2);
#else
        LOAD_WEIGHT_D(2);
#endif
        
#if DEFORM_P
        SUM_P3;
#endif
#if DEFORM_N
        SUM_N3;
#endif
#if DEFORM_G
        SUM_G3;
#endif
#if DEFORM_B
        SUM_B3;
#endif

#endif

#if (BONE_COUNT > 3)
#if INDIRECT
        LOAD_WEIGHT_I(3);
#else
        LOAD_WEIGHT_D(3);
#endif
        
#if DEFORM_P
        SUM_P3;
#endif
#if DEFORM_N
        SUM_N3;
#endif
#if DEFORM_G
        SUM_G3;
#endif
#if DEFORM_B
        SUM_B3;
#endif

#endif
#endif

        COPY_AND_ADVANCE(Source, Dest, CopySize, SourceStride, DestStride);
    }
}
#else
;
#endif
