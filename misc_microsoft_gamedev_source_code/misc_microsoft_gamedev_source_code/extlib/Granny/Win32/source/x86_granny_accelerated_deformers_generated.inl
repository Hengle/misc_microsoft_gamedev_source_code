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
NAME(int32x Count,
     void const *SourceInit,
     void *DestInit,
#if INDIRECT
     int32x const *TransformTable,
#endif
     matrix_4x4 const *Transforms,
     int32x CopySize,
     int32x SourceStride, int32x DestStride)
#if DEFINE_CODE
{
    COUNT_BLOCK(Stringize(NAME));

    SOURCE_VERTEX_TYPE const *Source = (SOURCE_VERTEX_TYPE const *)SourceInit;
    DEST_VERTEX_TYPE *Dest = (DEST_VERTEX_TYPE *)DestInit;

    // TODO: WTF, why is UV explicitly specified?
    if(!IS_ALIGNED_16(Transforms))
    {
        FALLBACK_NAME(
            Count,
            SourceInit,
            DestInit,
#if INDIRECT
            TransformTable,
#endif
            Transforms,
            SizeOf(Dest->UV)/SizeOf(Dest->UV[0]),
            SourceStride, DestStride);
        return;
    }

#if (BONE_COUNT == 0)
#if INDIRECT
    matrix_4x4 const *Trans0 = Transforms + TransformTable[0];
#else
    matrix_4x4 const *Trans0 = Transforms;
#endif
    SseSetup1XMMMatrix(Trans0);
#endif
    
    while(Count--)
    {
        real32 UV0 = Source->UV[0];
        real32 UV1 = Source->UV[1];

        //
#if (BONE_COUNT == 1)
        //
        
#if INDIRECT
        uint32 ind0 = TransformTable[Source->BoneIndex];
#else
        uint32 ind0 = Source->BoneIndex;
#endif

        matrix_4x4 const *Trans0 = Transforms + ind0;
        SseSetup1XMMMatrix(Trans0);

        //
#elif (BONE_COUNT == 2)
        //
        
#if INDIRECT
        uint32 ind0 = TransformTable[Source->BoneIndices[0]];
        uint32 ind1 = TransformTable[Source->BoneIndices[1]];
#else
        uint32 ind0 = Source->BoneIndices[0];
        uint32 ind1 = Source->BoneIndices[1];
#endif

        // Set up Trans0 * w0 + Trans1 * w1 in xmm4-7
        matrix_4x4 const *Trans0 = Transforms + ind0;
        matrix_4x4 const *Trans1 = Transforms + ind1;
        SseSetup2BoneWeightsXMMMatrix(Source->BoneWeights, Trans0, Trans1);

        //
#elif (BONE_COUNT == 4)
        //
        
#if INDIRECT
        uint32 ind0 = TransformTable[Source->BoneIndices[0]];
        uint32 ind1 = TransformTable[Source->BoneIndices[1]];
        uint32 ind2 = TransformTable[Source->BoneIndices[2]];
        uint32 ind3 = TransformTable[Source->BoneIndices[3]];
#else
        uint32 ind0 = Source->BoneIndices[0];
        uint32 ind1 = Source->BoneIndices[1];
        uint32 ind2 = Source->BoneIndices[2];
        uint32 ind3 = Source->BoneIndices[3];
#endif
        
        matrix_4x4 const *Trans0 = Transforms + ind0;
        matrix_4x4 const *Trans1 = Transforms + ind1;
        matrix_4x4 const *Trans2 = Transforms + ind2;
        matrix_4x4 const *Trans3 = Transforms + ind3;

        // Set up Trans0 * w0 + Trans1 * w1 ... in xmm4-7
        SseSetup4BoneWeightsXMMMatrix(Source->BoneWeights,
            Trans0, Trans1, Trans2, Trans3);

        //
#elif (BONE_COUNT != 0)
#error BONE_COUNT is out-of-range
#endif
        //

#if (DEFORM_P && DEFORM_N)
        SseMultiplyPositionByXMMMatrix(Source->Position);
        SseMultiplyNormalByXMMMatrix1(Source->Normal);

        __asm
        {
            mov edi, [Dest]

            // Dest->Position
            movlps [edi]DEST_VERTEX_TYPE.Position, xmm0        // X, Y
            movhlps xmm0, xmm0
            movss [edi+8]DEST_VERTEX_TYPE.Position, xmm0       // Z

            // Dest->Normal
            movlps [edi]DEST_VERTEX_TYPE.Normal, xmm1          // X, Y
            movhlps xmm1, xmm1
            movss [edi+8]DEST_VERTEX_TYPE.Normal, xmm1         // Z
        }
#endif

#if DEFORM_G
        SseMultiplyNormalByXMMMatrix0(Source->Tangent);

#if DEFORM_B
        SseMultiplyNormalByXMMMatrix1(Source->Binormal);
        __asm
        {
            mov edi, [Dest]

            // Dest->Tangent
            movlps [edi]DEST_VERTEX_TYPE.Tangent, xmm0        // X, Y
            movhlps xmm0, xmm0
            movss [edi+8]DEST_VERTEX_TYPE.Tangent, xmm0       // Z

            // Dest->Binormal
            movlps [edi]DEST_VERTEX_TYPE.Binormal, xmm1          // X, Y
            movhlps xmm1, xmm1
            movss [edi+8]DEST_VERTEX_TYPE.Binormal, xmm1         // Z
        }
#else
        __asm
        {
            mov edi, [Dest]

            // Dest->Tangent
            movlps [edi]DEST_VERTEX_TYPE.Tangent, xmm0        // X, Y
            movhlps xmm0, xmm0
            movss [edi+8]DEST_VERTEX_TYPE.Tangent, xmm0       // Z
        }
#endif

#endif

        Dest->UV[0] = UV0;
        Dest->UV[1] = UV1;

        Source++;
        Dest++;
    }
}
#else
;
#endif
