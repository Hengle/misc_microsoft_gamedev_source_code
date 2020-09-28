/* ========================================================================
   $RCSfile: x86_granny_accelerated_deformers.inl,v $
   $Date: 2002/03/02 04:48:25 $
   $Revision: 1.1.1.1 $
   $Creator: Mike Sartain $
   (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
   ======================================================================== */

/*
 * Defines which allow us to inline various asm routines.
 *
 *  1) Debug builds don't pay attention to the __forceinline modifier.
 *  2) __declspec(naked) routines also won't inline.
 *  3) All of this has been fixed on vc7.
 *  4) Unfortunately, I'm assuming this needs to still build with vc6.
 *  5) Which sucks.
 *
 * To workaround this mess:
 *  1) For debug builds we define make function calls to naked routines.
 *  2) For retail builds we remove the naked decl and inline the routine.
 */
#ifdef _DEBUG
    #define DECL_NAKED                __declspec(naked)
    #define CALLINLINEFUNC(_func)     __asm call _func
#else
    #define DECL_NAKED
    #define CALLINLINEFUNC(_func)     _func()
#endif

/*
 * The entire existence of this file is to try and create some sort of
 *  asm macros for the Direct / Indirect routines. To accomplish this
 *  the file is included twice: once with DIRECT_ROUTINES defined and
 *  once without. We add a D or I to the function names appropriately.
 */
#undef FUNC_NAME
#ifdef DIRECT_ROUTINES
  #define FUNC_NAME(_func)          _func##D
#else
  #define FUNC_NAME(_func)          _func##I
#endif


#ifdef DIRECT_ROUTINES

//=========================================================================
// Weight 2 helper routine. Takes a position, normal, two transforms,
//  two weights, and returns the results in xmm0, xmm1.
//=========================================================================
__forceinline void DECL_NAKED
SseDeformW2Helper()
{
    // Input:
    //   esi: Position
    //   edi: Normal
    //   ecx: Trans1
    //   edx: Trans2
    //   xmm6: w0|w0|w0|w0
    //   xmm7: w1|w1|w1|w1

    // returns:
    //   xmm0: DestPosXYZ
    //   xmm1: DestNormXYZ

    // clobbers:
    //   xmm0-4

    __asm
    {
        // Position
        movaps xmm0, [ecx+48]                           // 33|32|31|30

        movss xmm4, [esi]                               // xx|xx|xx|Px
        shufps xmm4, xmm4, 0                            // Px|Px|Px|Px
        mulps xmm4, [ecx]                               // Px|Px|Px|Px * 03|02|01|00
        addps xmm0, xmm4

        movss xmm4, [esi+4]                             // xx|xx|xx|Py
        shufps xmm4, xmm4, 0                            // Py|Py|Py|Py
        mulps xmm4, [ecx+16]                            // Py|Py|Py|Py * 13|12|11|10
        addps xmm0, xmm4

        movss xmm4, [esi+8]                             // xx|xx|xx|Pz
        shufps xmm4, xmm4, 0                            // Pz|Pz|Pz|Pz
        mulps xmm4, [ecx+32]                            // Pz|Pz|Pz|Pz * 23|22|21|20
        addps xmm0, xmm4

        // Normal
        movss xmm1, [edi]                               // xx|xx|xx|Nx
        shufps xmm1, xmm1, 0                            // Nx|Nx|Nx|Nx
        mulps xmm1, [ecx]                               // Nx|Nx|Nx|Nx * 03|02|01|00

        movss xmm4, [edi+4]                             // xx|xx|xx|Ny
        shufps xmm4, xmm4, 0                            // Ny|Ny|Ny|Ny
        mulps xmm4, [ecx+16]                            // Ny|Ny|Ny|Ny * 13|12|11|10
        addps xmm1, xmm4

        movss xmm4, [edi+8]                             // xx|xx|xx|Nz
        shufps xmm4, xmm4, 0                            // Nz|Nz|Nz|Nz
        mulps xmm4, [ecx+32]                            // Nz|Nz|Nz|Nz * 23|22|21|20
        addps xmm1, xmm4

        mulps xmm0, xmm6
        mulps xmm1, xmm6

        // Position
        movaps xmm2, [edx+48]                           // 33|32|31|30

        movss xmm4, [esi]                               // xx|xx|xx|Px
        shufps xmm4, xmm4, 0                            // Px|Px|Px|Px
        mulps xmm4, [edx]                               // Px|Px|Px|Px * 03|02|01|00
        addps xmm2, xmm4

        movss xmm4, [esi+4]                             // xx|xx|xx|Py
        shufps xmm4, xmm4, 0                            // Py|Py|Py|Py
        mulps xmm4, [edx+16]                            // Py|Py|Py|Py * 13|12|11|10
        addps xmm2, xmm4

        movss xmm4, [esi+8]                             // xx|xx|xx|Pz
        shufps xmm4, xmm4, 0                            // Pz|Pz|Pz|Pz
        mulps xmm4, [edx+32]                            // Pz|Pz|Pz|Pz * 23|22|21|20
        addps xmm2, xmm4

        // Normal
        movss xmm3, [edi]                               // xx|xx|xx|Nx
        shufps xmm3, xmm3, 0                            // Nx|Nx|Nx|Nx
        mulps xmm3, [edx]                               // Nx|Nx|Nx|Nx * 03|02|01|00

        movss xmm4, [edi+4]                             // xx|xx|xx|Ny
        shufps xmm4, xmm4, 0                            // Ny|Ny|Ny|Ny
        mulps xmm4, [edx+16]                            // Ny|Ny|Ny|Ny * 13|12|11|10
        addps xmm3, xmm4

        movss xmm4, [edi+8]                             // xx|xx|xx|Nz
        shufps xmm4, xmm4, 0                            // Nz|Nz|Nz|Nz
        mulps xmm4, [edx+32]                            // Nz|Nz|Nz|Nz * 23|22|21|20
        addps xmm3, xmm4

        mulps xmm2, xmm7
        addps xmm0, xmm2

        mulps xmm3, xmm7
        addps xmm1, xmm3

#ifdef _DEBUG
        ret
#endif
    }
}

#endif // DIRECT_ROUTINES

//=========================================================================
// SseDeformPWNT3432HelperD and SseDeformPWNT3432HelperI
//=========================================================================
void
FUNC_NAME(SseDeformPWNT3432Helper)(int32x Count, pwnt3432_vertex const *Source,
    pnt332_vertex *Dest, SSEABUF *pbuf)
{
    // esi: Source
    // ebx: Transforms

    _asm
    {
        push ebx
        push ecx
        push esi
        push edi

        mov esi, [Source]
        mov ebx, [pbuf]

        ALIGN 16

transform_loop:

        // set up the pointer to our Normal
        lea edi, [esi]pwnt3432_vertex.Normal

        // Store our tex coordinates in xmm5
        movlps xmm5, [esi]pwnt3432_vertex.UV

        //$NOTE: If there are more textures coords, read them into the mmx
        // registers mm0-mm7 here. Then when writing the transformed vert
        // out towards the bottom of the routine use the movntq instruction
        // to blast the stuff out. Movntps requires 16 byte alignment so
        // you may need to change that to movups.

        // set up the Trans pointers
#ifndef DIRECT_ROUTINES
        mov eax, [ebx]SSEABUF.TransformTable
#endif
        xor ecx, ecx
        mov cl, [esi]pwnt3432_vertex.BoneIndices[0]
#ifndef DIRECT_ROUTINES
        mov ecx, [eax+ecx*4]
#endif
        shl ecx, 6 // matrix_4x4 is 64 bytes
        add ecx, [ebx]SSEABUF.Transforms
        xor edx, edx
        mov dl, [esi]pwnt3432_vertex.BoneIndices[1]
#ifndef DIRECT_ROUTINES
        mov edx, [eax+edx*4]
#endif
        shl edx,6
        add edx, [ebx]SSEABUF.Transforms

        // set up our boneweights
        xor eax, eax
        mov al, [esi]pwnt3432_vertex.BoneWeights[0]
        cvtsi2ss xmm6, eax
        mulss xmm6, [OneOver255]
        shufps xmm6, xmm6, 0

        mov al, [esi]pwnt3432_vertex.BoneWeights[1]
        cvtsi2ss xmm7, eax
        mulss xmm7, [OneOver255]
        shufps xmm7, xmm7, 0
    }

    // call the transform dude
    CALLINLINEFUNC(SseDeformW2Helper);

    _asm
    {
        // store the results in our stack buffers
        movaps [ebx]SSEABUF.DestPosXYZ, xmm0;
        movaps [ebx]SSEABUF.DestNormXYZ, xmm1;

        // set up Trans pointers 2 and 3
#ifndef DIRECT_ROUTINES
        mov eax, [ebx]SSEABUF.TransformTable
#endif
        xor ecx, ecx
        mov cl, [esi]pwnt3432_vertex.BoneIndices[2]
#ifndef DIRECT_ROUTINES
        mov ecx, [eax+ecx*4]
#endif
        shl ecx, 6
        add ecx, [ebx]SSEABUF.Transforms
        xor edx, edx
        mov dl, [esi]pwnt3432_vertex.BoneIndices[3]
#ifndef DIRECT_ROUTINES
        mov edx, [eax+edx*4]
#endif
        shl edx,6
        add edx, [ebx]SSEABUF.Transforms

        // set up our boneweights
        xor eax, eax
        mov al, [esi]pwnt3432_vertex.BoneWeights[2]
        cvtsi2ss xmm6, eax
        mulss xmm6, [OneOver255]
        shufps xmm6, xmm6, 0

        mov al, [esi]pwnt3432_vertex.BoneWeights[3]
        cvtsi2ss xmm7, eax
        mulss xmm7, [OneOver255]
        shufps xmm7, xmm7, 0
    }

    // call the transform dude
    CALLINLINEFUNC(SseDeformW2Helper);

    _asm
    {
        // add the previous results to this one
        addps xmm0, [ebx]SSEABUF.DestPosXYZ;
        addps xmm1, [ebx]SSEABUF.DestNormXYZ;

        // Now we've got this:
        // xmm0: xx|Px|Py|Pz
        // xmm1: xx|Nx|Ny|Nz

        // Munge the stuff all around and spit it to our output buffer
        shufps xmm0, xmm0, 0x24             // Pz|Px|Py|Pz
        movss  xmm0, xmm1                   // Pz|Px|Py|Nx
        shufps xmm0, xmm0, 0x27             // Nz|Px|Py|Pz
        shufps xmm1, xmm1, 0x09             // xx|xx|Nx|Ny
        movlhps xmm1, xmm5                  // Tv|Tu|Nx|Ny

        // Spit them out to memory
        mov edx, [Dest]

        movntps [edx], xmm0
        movntps [edx+16], xmm1

        // Next
        add [Dest], size pnt332_vertex

        add esi, size pwnt3432_vertex
        prefetchnta [esi + 3 * size pwnt3432_vertex]

        dec [Count]
        jnz transform_loop

        emms

        pop edi
        pop esi
        pop ecx
        pop ebx
    }
}

//=========================================================================
// SseDeformPWNT3232HelperD and SseDeformPWNT3232HelperI
//=========================================================================
void
FUNC_NAME(SseDeformPWNT3232Helper)(int32x Count, pwnt3232_vertex const *Source,
    pnt332_vertex *Dest, SSEABUF *pbuf)
{
    // esi: Source
    // ebx: Transforms

    _asm
    {
        push ebx
        push ecx
        push esi
        push edi

        mov esi, [Source]
        mov ebx, [pbuf]

        ALIGN 16

transform_loop:

        // set up the pointer to our Normal
        lea edi, [esi]pwnt3232_vertex.Normal

        // Store our tex coordinates in mm0
        movlps xmm5, [esi]pwnt3232_vertex.UV

        //$NOTE: If there are more textures coords, read them into the mmx
        // registers mm0-mm7 here. Then when writing the transformed vert
        // out towards the bottom of the routine use the movntq instruction
        // to blast the stuff out. Movntps requires 16 byte alignment so
        // you may need to change that to movups.

        // set up the Trans pointers
#ifndef DIRECT_ROUTINES
        mov eax, [ebx]SSEABUF.TransformTable
#endif
        xor ecx, ecx
        mov cl, [esi]pwnt3232_vertex.BoneIndices[0]
#ifndef DIRECT_ROUTINES
        mov ecx, [eax+ecx*4]
#endif
        shl ecx, 6 // matrix_4x4 is 64 bytes
        add ecx, [ebx]SSEABUF.Transforms
        xor edx, edx
        mov dl, [esi]pwnt3232_vertex.BoneIndices[1]
#ifndef DIRECT_ROUTINES
        mov edx, [eax+edx*4]
#endif
        shl edx,6
        add edx, [ebx]SSEABUF.Transforms

        // set up our boneweights
        xor eax, eax
        mov al, [esi]pwnt3432_vertex.BoneWeights[0]
        cvtsi2ss xmm6, eax
        mulss xmm6, [OneOver255]
        shufps xmm6, xmm6, 0

        mov al, [esi]pwnt3432_vertex.BoneWeights[1]
        cvtsi2ss xmm7, eax
        mulss xmm7, [OneOver255]
        shufps xmm7, xmm7, 0
    }

    // call the transform dude
    CALLINLINEFUNC(SseDeformW2Helper);

    _asm
    {
        // Now we've got this:
        // xmm0: xx|Px|Py|Pz
        // xmm1: xx|Nx|Ny|Nz

        // Munge the stuff all around and spit it to our output buffer
        shufps xmm0, xmm0, 0x24             // Pz|Px|Py|Pz
        movss  xmm0, xmm1                   // Pz|Px|Py|Nx
        shufps xmm0, xmm0, 0x27             // Nz|Px|Py|Pz
        shufps xmm1, xmm1, 0x09             // xx|xx|Nx|Ny
        movlhps xmm1, xmm5                  // Tv|Tu|Nx|Ny

        // Spit them out to memory
        mov edx, [Dest]

        movntps [edx], xmm0
        movntps [edx+16], xmm1

        // Next
        add [Dest], size pnt332_vertex

        add esi, size pwnt3232_vertex
        prefetchnta [esi + 3 * size pwnt3232_vertex]

        dec [Count]
        jnz transform_loop

        emms

        pop edi
        pop esi
        pop ecx
        pop ebx
    }
}

#ifdef DIRECT_ROUTINES

//=========================================================================
// Weight 1 helper routine. Transforms 2 pwnt3132_vertex's. 
//=========================================================================
__forceinline void DECL_NAKED
SseDeformW1Helper()
{
    // esi: pwnt3132_vertex
    // ecx: Trans1
    // edi: pwnt3132_vertex
    // edx: Trans2

    // returns:
    //   xmm0: xx|Px|Py|Pz
    //   xmm1: xx|Nx|Ny|Nz
    //   xmm2: xx|Px|Py|Pz
    //   xmm3: xx|Nx|Ny|Nz

    // clobbers:
    //   xmm0-4

    __asm
    {
        // Position
        movaps xmm0, [ecx+48]                           // 33|32|31|30

        movss xmm4, [esi]pwnt3132_vertex.Position[0]    // xx|xx|xx|Px
        shufps xmm4, xmm4, 0                            // Px|Px|Px|Px
        mulps xmm4, [ecx]                               // Px|Px|Px|Px * 03|02|01|00
        addps xmm0, xmm4

        movss xmm4, [esi]pwnt3132_vertex.Position[4]    // xx|xx|xx|Py
        shufps xmm4, xmm4, 0                            // Py|Py|Py|Py
        mulps xmm4, [ecx+16]                            // Py|Py|Py|Py * 13|12|11|10
        addps xmm0, xmm4

        movss xmm4, [esi]pwnt3132_vertex.Position[8]    // xx|xx|xx|Pz
        shufps xmm4, xmm4, 0                            // Pz|Pz|Pz|Pz
        mulps xmm4, [ecx+32]                            // Pz|Pz|Pz|Pz * 23|22|21|20
        addps xmm0, xmm4

        // Normal
        movss xmm1, [esi]pwnt3132_vertex.Normal[0]      // xx|xx|xx|Nx
        shufps xmm1, xmm1, 0                            // Nx|Nx|Nx|Nx
        mulps xmm1, [ecx]                               // Nx|Nx|Nx|Nx * 03|02|01|00

        movss xmm4, [esi]pwnt3132_vertex.Normal[4]      // xx|xx|xx|Ny
        shufps xmm4, xmm4, 0                            // Ny|Ny|Ny|Ny
        mulps xmm4, [ecx+16]                            // Ny|Ny|Ny|Ny * 13|12|11|10
        addps xmm1, xmm4

        movss xmm4, [esi]pwnt3132_vertex.Normal[8]      // xx|xx|xx|Nz
        shufps xmm4, xmm4, 0                            // Nz|Nz|Nz|Nz
        mulps xmm4, [ecx+32]                            // Nz|Nz|Nz|Nz * 23|22|21|20
        addps xmm1, xmm4

        // Position
        movaps xmm2, [edx+48]                           // 33|32|31|30

        movss xmm4, [edi]pwnt3132_vertex.Position[0]    // xx|xx|xx|Px
        shufps xmm4, xmm4, 0                            // Px|Px|Px|Px
        mulps xmm4, [edx]                               // Px|Px|Px|Px * 03|02|01|00
        addps xmm2, xmm4

        movss xmm4, [edi]pwnt3132_vertex.Position[4]    // xx|xx|xx|Py
        shufps xmm4, xmm4, 0                            // Py|Py|Py|Py
        mulps xmm4, [edx+16]                            // Py|Py|Py|Py * 13|12|11|10
        addps xmm2, xmm4

        movss xmm4, [edi]pwnt3132_vertex.Position[8]    // xx|xx|xx|Pz
        shufps xmm4, xmm4, 0                            // Pz|Pz|Pz|Pz
        mulps xmm4, [edx+32]                            // Pz|Pz|Pz|Pz * 23|22|21|20
        addps xmm2, xmm4

        // Normal
        movss xmm3, [edi]pwnt3132_vertex.Normal[0]      // xx|xx|xx|Nx
        shufps xmm3, xmm3, 0                            // Nx|Nx|Nx|Nx
        mulps xmm3, [edx]                               // Nx|Nx|Nx|Nx * 03|02|01|00

        movss xmm4, [edi]pwnt3132_vertex.Normal[4]      // xx|xx|xx|Ny
        shufps xmm4, xmm4, 0                            // Ny|Ny|Ny|Ny
        mulps xmm4, [edx+16]                            // Ny|Ny|Ny|Ny * 13|12|11|10
        addps xmm3, xmm4

        movss xmm4, [edi]pwnt3132_vertex.Normal[8]      // xx|xx|xx|Nz
        shufps xmm4, xmm4, 0                            // Nz|Nz|Nz|Nz
        mulps xmm4, [edx+32]                            // Nz|Nz|Nz|Nz * 23|22|21|20
        addps xmm3, xmm4

#ifdef _DEBUG
        ret
#endif
    }
}

#endif // DIRECT_ROUTINES

//=========================================================================
// SseDeformPWNT3132HelperD and SseDeformPWNT3132HelperI
//=========================================================================
void
FUNC_NAME(SseDeformPWNT3132Helper)(int32x Count, pwnt3132_vertex const *Source,
    pnt332_vertex *Dest, SSEABUF *pbuf)
{
    _asm
    {
        push ebx
        push ecx
        push esi
        push edi

        mov esi, [Source]
        mov ebx, [pbuf]

        ALIGN 16

transform_loop:

        // Set up pointer to next vertex
        lea edi, [esi + size pwnt3132_vertex]

        // Store our tex coordinates in xmm6 and 7
        movlps xmm6, [esi]pwnt3132_vertex.UV
        movlps xmm7, [edi]pwnt3132_vertex.UV

        //$NOTE: If there are more textures coords, read them into the mmx
        // registers mm0-mm7 here. Then when writing the transformed vert
        // out towards the bottom of the routine use the movntq instruction
        // to blast the stuff out. Movntps requires 16 byte alignment so
        // you may need to change that to movups.

        // set up the Trans1 pointer
#ifndef DIRECT_ROUTINES
        mov eax, [ebx]SSEABUF.TransformTable
#endif
        mov ecx, [esi]pwnt3132_vertex.BoneIndex
#ifndef DIRECT_ROUTINES
        mov ecx, [eax+ecx*4]
#endif
        shl ecx, 6 // matrix_4x4 is 64 bytes
        add ecx, [ebx]SSEABUF.Transforms

        // set up the Trans2 pointer
        mov edx, [edi]pwnt3132_vertex.BoneIndex
#ifndef DIRECT_ROUTINES
        mov edx, [eax+edx*4]
#endif
        shl edx, 6 // matrix_4x4 is 64 bytes
        add edx, [ebx]SSEABUF.Transforms
    }

    // call the transform dude
    CALLINLINEFUNC(SseDeformW1Helper);

    _asm
    {
        // xmm0: xx|Px|Py|Pz
        // xmm1: xx|Nx|Ny|Nz

        shufps xmm0, xmm0, 0x24             // Pz|Px|Py|Pz
        movss  xmm0, xmm1                   // Pz|Px|Py|Nx
        shufps xmm0, xmm0, 0x27             // Nz|Px|Py|Pz
        shufps xmm1, xmm1, 0x09             // xx|xx|Nx|Ny
        movlhps xmm1, xmm6                  // Tv|Tu|Nx|Ny

        // xmm2: xx|Px|Py|Pz
        // xmm3: xx|Nx|Ny|Nz

        shufps xmm2, xmm2, 0x24             // Pz|Px|Py|Pz
        movss  xmm2, xmm3                   // Pz|Px|Py|Nx
        shufps xmm2, xmm2, 0x27             // Nz|Px|Py|Pz
        shufps xmm3, xmm3, 0x09             // xx|xx|Nx|Ny
        movlhps xmm3, xmm7                  // Tv|Tu|Nx|Ny

        // Spit them out to memory
        mov edx, [Dest]

        movntps [edx], xmm0
        movntps [edx+16], xmm1
        movntps [edx+32], xmm2
        movntps [edx+48], xmm3

        // Next
        add [Dest], size pnt332_vertex*2

        add esi, size pwnt3132_vertex*2
        prefetchnta [esi + 3 * size pwnt3132_vertex]
        prefetchnta [esi + 4 * size pwnt3132_vertex]

        dec [Count]
        jnz transform_loop

        emms

        pop edi
        pop esi
        pop ecx
        pop ebx
    }
}

