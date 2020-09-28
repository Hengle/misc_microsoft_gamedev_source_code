//============================================================================
//
// File: DXNQDecodeAsm.inl
// Copyright (c) 2005-2006, Ensemble Studios
// RG
//
//============================================================================
#if defined(CODE_ANALYSIS_ENABLED)
static uint FUNC_NAME(
   uint ptrTopSrc,                  // r3
   uint pSrcEndMinus1,              // r4
   uint ptrTopDst,                  // r5
   uint pDstEnd,                    // r6
   uint pAlphaCodebook,             // r7
   uint pAlphaSelectorCodebookD,    // r8
   uint pAlphaSelectorCodebookW)    // r9
{
	return 0;
}
#else
static uint __declspec(naked) FUNC_NAME(
   uint ptrTopSrc,                  // r3
   uint pSrcEndMinus1,              // r4
   uint ptrTopDst,                  // r5
   uint pDstEnd,                    // r6
   uint pAlphaCodebook,             // r7
   uint pAlphaSelectorCodebookD,    // r8
   uint pAlphaSelectorCodebookW)    // r9
{
#define PTRTOPSRC          r3
#define PSRCENDMINUS1      r4
#define PTRTOPDST          r5
#define PDSTEND            r6
#define PALPHACODEBOOK     r7
#define PSELECTORCODEBOOKD r8
#define PSELECTORCODEBOOKW r9

   __asm
   {
      mflr         r12
      bl           __savegprlr
         
      //add r3, r3, r4
      //rlwinm      r3, r3, 1, 30, 31
      
      // 3-10 params    volatile
      // 11-12 scratch  volatile
      // 14-31          nonvolatile
            
      // avail: 10, 11, 16-27
                  
      // r14 pSrc
      // r15 pDst
      // r27 prevpDst
      // r29-r31 packetBits
      // r28 runLen

#define RUNLEN       r28

#define PACKET0BITS  r29
#define PACKET1BITS  r30
#define PACKET2BITS  r31

#define PSRC         r14
#define PDST         r15
#define PDSTPREV     r27
      
      lwz         PSRC, 0(PTRTOPSRC)
      lwz         PDST, 0(PTRTOPDST)
      
      cmpw        cr6, PSRC, PSRCENDMINUS1
      bge         cr6, DXNAsmRet

#if DEST_PREFETCHING      
      mr          PDSTPREV, PDST
#endif      
                        
      nopalign    8

DXNAsmPacketLoop:  

#if DEST_PREFETCHING      
      xor         r25, PDST, PDSTPREV
      ld          PACKET2BITS, 16(PSRC)
      
      li          r24, 256
      dcbt        r24, PSRC
      
      ld          PACKET1BITS, 8(PSRC)                     
      clrrwi      r25, r25, 7
      
      ld          PACKET0BITS, 0(PSRC)         
      rldicl      RUNLEN, PACKET2BITS, 7, 57      // SHR 57
      
      cmplwi      cr6, r25, 0
      bne         cr6, DXNAsmDstPrefetch
      
      nopalign    8
#else
      ld          PACKET2BITS, 16(PSRC)
      
      li          r24, 256
      dcbt        r24, PSRC
      
      ld          PACKET1BITS, 8(PSRC)   
      rldicl      RUNLEN, PACKET2BITS, 7, 57      // SHR 57                  
      
      ld          PACKET0BITS, 0(PSRC)         
#endif      
                  
DXNAsmNoDstPrefetch:
                        
      //                      11111111111    
      //                     111111             
      // 00000000*0000000*0000000*0000000
      // 32      40      48      56    63      
      // 0       8       16      24    31
      //
      //
      //                                                     11111111111
      //                                                   111111111111                      
      // rrrrrrrfffffffffffeeeeeeeeeeeecccccccccccbbbbbbbbbbbbaaaaaaaaaaa
      // 00000000*0000000*0000000*0000000*0000000*0000000*0000000*0000000
      //                                 32      40      48      56    63      
      // 0       8       16      24    31
      
      // PACKET0BITS - packet0Bits
      // PACKET1BITS - packet1Bits
      // PACKET2BITS - packet2Bits
      
      // block0: r18, r19, r20, r21, r22, r23
      // block1: r10, r11, r16, r24, r25, r26
      
      // writeAlphaBlockN(pDst     , (packet0Bits        & 2047), ((packet0Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 2 , (packet0Bits >> 23) & 2047,  ((packet0Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 4 , (packet1Bits        & 2047), ((packet1Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 6 , (packet1Bits >> 23) & 2047,  ((packet1Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \

      //------------------------------------------------------- Blocks 0-1
                  
      rlwinm       r18, PACKET0BITS, 1, 20, 30    // B0AlphaCode0*2             
      
      rlwinm       r19, PACKET0BITS, 22, 19, 30   // B0SelectorCode0*2                      
      
      rlwinm       r20, PACKET0BITS, 23, 18, 29   // B0SelectorCode0*4
                        
      rldicl       r21, PACKET0BITS, 41, 23         // SHR 23 B0AlphaCode1
      
      cmplwi       cr6, RUNLEN, 0
      
      lhzx         r18, r18, PALPHACODEBOOK  // pAlphaCodebook[B0AlphaCode0*2]      
      rldicl       r22, PACKET0BITS, 30, 34       // SHR 34 B0SelectorCode1          
            
      lhzx         r19, r19, PSELECTORCODEBOOKW  // pAlphaSelectorCodebokW[B0SelectorCode0*2]
      rlwinm       r10, PACKET1BITS, 1, 20, 30    // B1AlphaCode0*2             
      
      rlwinm       r11, PACKET1BITS, 22, 19, 30   // B1SelectorCode0*2                      
      
      lwzx         r20, r20, PSELECTORCODEBOOKD // pAlphaSelectorCodebokD[B0SelectorCode0*4]
      rlwinm       r16, PACKET1BITS, 23, 18, 29   // B1SelectorCode0*4
      
      rldicl       r24, PACKET1BITS, 41, 23         // SHR 23 B1AlphaCode1
      
      rlwinm       r21, r21, 1, 20, 30    // B0AlphaCode1*2
      
      rldicl       r25, PACKET1BITS, 30, 34      // SHR 34 B1SelectorCode1          
      lhzx         r10, r10, PALPHACODEBOOK  // pAlphaCodebook[B1AlphaCode0*2]      
      
      rlwinm       r23, r22, 1, 19, 30 // B0SelectorCode1*2
      lhzx         r11, r11, PSELECTORCODEBOOKW  // pAlphaSelectorCodebokW[B1SelectorCode0*2]
      
      rlwinm       r22, r22, 2, 18, 29 // B0SelectorCode1*4
      lwzx         r16, r16, PSELECTORCODEBOOKD // pAlphaSelectorCodebokD[B1SelectorCode0*4]
      
      rlwinm       r24, r24, 1, 20, 30    // B1AlphaCode1*2
      
      rlwinm       r26, r25, 1, 19, 30 // B1SelectorCode1*2
      
      lhzx         r21, r21, PALPHACODEBOOK // pAlphaCodebook[B0AlphaCode1*2]
      rlwinm       r25, r25, 2, 18, 29 // B1SelectorCode1*4
      
      lhzx         r23, r23, PSELECTORCODEBOOKW // pAlphaSelectorCodebookW[B0SelectorCode1*2]
      rotlwi       r18, r18, 16  // pAlphaCodebook[B0AlphaCode0*2] << 16    
      
      lwzx         r22, r22, PSELECTORCODEBOOKD // pAlphaSelectorCodebookD[B0SelectorCode1*4]                        
      rotlwi       r21, r21, 16  // pAlphaCodebook[B0AlphaCode1*2] << 16  

      lhzx         r24, r24, PALPHACODEBOOK // pAlphaCodebook[B1AlphaCode1*2]
      rotlwi       r10, r10, 16  // pAlphaCodebook[B1AlphaCode0*2] << 16    
      
      lhzx         r26, r26, PSELECTORCODEBOOKW // pAlphaSelectorCodebookW[B1SelectorCode1*2]      
      or           r19, r19, r18  //B0    
      
      lwzx         r25, r25, PSELECTORCODEBOOKD // pAlphaSelectorCodebookD[B1SelectorCode1*4]                        
      or           r23, r23, r21  //B0
      
      rotlwi       r24, r24, 16  // pAlphaCodebook[B1AlphaCode1*2] << 16  
      
      or           r11, r11, r10  //B1
                  
      or           r26, r26, r24  //B1

      stw          r19, 0(PDST)   //B0
      stw          r20, 4(PDST)   //B0
      stw          r23, 8(PDST)   //B0
      stw          r22, 12(PDST)  //B0
            
      bne          cr6, DXNAsmProcessRun

DXNAsmDoneRun:      
      stw          r11, 16(PDST)  //B1
      stw          r16, 20(PDST)  //B1
      stw          r26, 24(PDST)  //B1
      stw          r25, 28(PDST)  //B1            
      
      //------------------------------------------------------- Blocks 2-3
      // block2: r18, r19, r20, r21, r22, r23
      // block3: r10, r11, r16, r24, r25, r26
      
      // writeAlphaBlockN(pDst + 8 , (packet2Bits        & 2047), ((packet2Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 10, (packet2Bits >> 23) & 2047,  ((packet2Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 12, (packet0Bits >> 46) & 2047,  ((packet1Bits >> 46) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 14, (packet2Bits >> 46) & 2047,  ((packet0Bits >> 57) | ((packet1Bits >> 58) << 6)), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \      
      
      rlwinm       r19, PACKET2BITS, 22, 19, 30   // B2SelectorCode0*2                      
      
      rlwinm       r18, PACKET2BITS, 1, 20, 30    // B2AlphaCode0*2             
            
      rlwinm       r20, PACKET2BITS, 23, 18, 29   // B2SelectorCode0*4
      
      rldicl       r21, PACKET2BITS, 41, 23       // SHR 23 B2AlphaCode1
      
      rldicl       r22, PACKET2BITS, 30, 34       // 34 B2SelectorCode1          
            
      rlwinm       r21, r21, 1, 20, 30    // B2AlphaCode1*2
      
      lhzx         r18, r18, PALPHACODEBOOK        // pAlphaCodebook[B2AlphaCode0*2]      
      rlwinm       r23, r22, 1, 19, 30    // B2SelectorCode1*2
      
      lhzx         r19, r19, PSELECTORCODEBOOKW    // pSelectorCodebookW[B2SelectorCode0*2]
      rlwinm       r22, r22, 2, 18, 29    // B2SelectorCode1*4
      
      lwzx         r20, r20, PSELECTORCODEBOOKD    // B2_0 pSelectorCodebookD[B2SelectorCode0*4]
      rotlwi       r18, r18, 16                    // pAlphaCodebook[B2AlphaCode0*2] << 16  
      
      rldicl       r26, PACKET1BITS, 6, 58         // SHR 58
      
      or           r19, r19, r18                   // B2_0 first DWORD
      
      rldicl       r11, PACKET1BITS, 18, 46         // SHR B3SelectorCode0
      lhzx         r21, r21, PALPHACODEBOOK        // pAlphaCodebook[B2AlphaCode1*2]      
      
      rldicl       r24, PACKET2BITS, 18, 46         // SHR B3AlphaCode1
      lhzx         r23, r23, PSELECTORCODEBOOKW    // pSelectorCodebookW[B2SelectorCode1*2]
      
      rldicl       r25, PACKET0BITS, 7, 57         // SHR 57
      lwzx         r22, r22, PSELECTORCODEBOOKD    // B2_1 pSelectorCodebookD[B2SelectorCode1*4]
      
            
      rldicl       r10, PACKET0BITS, 18, 46       // SHR B3AlphaCode0
      
      rlwinm       r24, r24, 1, 20, 30    // B3AlphaCode1*2     
      
      rotlwi       r21, r21, 16                    // pAlphaCodebook[B2AlphaCode1*2] << 16  
      
      rlwinm       r10, r10, 1, 20, 30    // B3AlphaCode0*2
      
      rlwinm       r16, r11, 1, 19, 30    // B3SelectorCode0*2
      
      rlwinm       r11, r11, 2, 18, 29    // B3SelectorCode0*4
      
      rlwinm       r26, r26, 6, 20, 25
      
      lhzx         r24, r24, PALPHACODEBOOK        // pAlphaCodebook[B3AlphaCode1*2]      
      or           r23, r23, r21                   // B2_1 first DWORD
      
      or           r26, r26, r25                 // B3SelectorCode1
                  
      stw          r19, 32(PDST)   //B2_0
      
      stw          r20, 36(PDST)   //B2_0
      
      lhzx         r10, r10, PALPHACODEBOOK        // pAlphaCodebook[B3AlphaCode0*2]      
      rlwinm       r25, r26, 1, 19, 30    // B3SelectorCode1*2
      
      lwzx         r11, r11, PSELECTORCODEBOOKD    // B3_0 pSelectorCodebookD[B3SelectorCode0*4]
      rotlwi       r24, r24, 16                    // pAlphaCodebook[B3AlphaCode1*2] << 16  
      
      lhzx         r16, r16, PSELECTORCODEBOOKW    // pSelectorCodebookW[B3SelectorCode0*2]
      rlwinm       r26, r26, 2, 18, 29    // B3SelectorCode1*4
      
      stw          r23, 40(PDST)   //B2_1      
      
      stw          r22, 44(PDST)   //B2_1
      
      rotlwi       r10, r10, 16                    // pAlphaCodebook[B3AlphaCode0*2] << 16  
      lhzx         r25, r25, PSELECTORCODEBOOKW    // pSelectorCodebookW[B3SelectorCode1*2]
      
      addi         PSRC, PSRC, 24
                        
      lwzx         r26, r26, PSELECTORCODEBOOKD    // B3_1 pSelectorCodebookD[B3SelectorCode1*4]
      or           r10, r10, r16                   // B3_0 first DWORD
      
      or           r24, r24, r25                   // B3_1 first DWORD
      
      cmpw         cr6, PSRC, PSRCENDMINUS1      
      
      stw          r10, 48(PDST)   //B3_0
      stw          r11, 52(PDST)   //B3_0
      stw          r24, 56(PDST)   //B3_1
      stw          r26, 60(PDST)   //B3_2
      
      addi         PDST, PDST, 64
            
      blt          cr6, DXNAsmPacketLoop

DXNAsmRet:      
      stw         PSRC, 0(PTRTOPSRC)
      stw         PDST, 0(PTRTOPDST)

      bl          __restgprlr

      nopalign    8
      
DXNAsmProcessRun:

      addi         PDST, PDST, 16

      cmplwi       cr6, RUNLEN, 4
      blt          cr6, DXNAsmProcessRun4D
      
      nopalign    8
      
DXNAsmProcessRun4:
      mr           r18, PDST
      
      stw          r19, 0(PDST)   //B0
      stw          r20, 4(PDST)   //B0
      stw          r23, 8(PDST)   //B0
      stw          r22, 12(PDST)  //B0
      
      stw          r19, 16(PDST)   //B0
      stw          r20, 20(PDST)   //B0
      stw          r23, 24(PDST)   //B0
      stw          r22, 28(PDST)  //B0
      
      addi         PDST, PDST, 64
      subi         RUNLEN, RUNLEN, 4
      
      stw          r19, 32(r18)   //B0
      stw          r20, 36(r18)   //B0
      stw          r23, 40(r18)   //B0
      stw          r22, 44(r18)  //B0
      
      cmplwi       cr6, RUNLEN, 4
      
      stw          r19, 48(r18)   //B0
      stw          r20, 52(r18)   //B0
      stw          r23, 56(r18)   //B0
      stw          r22, 60(r18)  //B0
      
      bge          cr6, DXNAsmProcessRun4
      
DXNAsmProcessRun4D:

      cmplwi       cr6, RUNLEN, 0
                  
      beq          cr6, DXNAsmDoneFinish
      
      subi         RUNLEN, RUNLEN, 1

      stw          r19, 0(PDST)   //B0
      stw          r20, 4(PDST)   //B0
      stw          r23, 8(PDST)   //B0
      stw          r22, 12(PDST)  //B0
      
      addi         PDST, PDST, 16
      
      b            DXNAsmProcessRun4D
      
DXNAsmDoneFinish:

      subi         PDST, PDST, 16
      b            DXNAsmDoneRun      

#if DEST_PREFETCHING
      nopalign    8
      
DXNAsmDstPrefetch:      
      // rg [12/31/06] - This forces all written cachelines out of the cache, which is probably not what we want.
      //dcbf        r0, PDSTPREV

      mr          PDSTPREV, PDST

      addi        r25, PDST, 127
      clrrwi      r25, r25, 7
      addi        r25, r25, 384

      addi        r26, r25, 127
      cmpw        cr6, r26, PDSTEND
      bge         DXNAsmNoDstPrefetch

      dcbz128     r0, r25      
      
      b           DXNAsmNoDstPrefetch
#endif      
   }
}
#endif // CODE_ANALYSIS_ENABLED

#undef PTRTOPSRC          
#undef PSRCENDMINUS1      
#undef PTRTOPDST          
#undef PDSTEND            
#undef PALPHACODEBOOK     
#undef PSELECTORCODEBOOKD 
#undef PSELECTORCODEBOOKW 

#undef RUNLEN      

#undef PACKET0BITS  
#undef PACKET1BITS  
#undef PACKET2BITS  

#undef PSRC         
#undef PDST         
#undef PDSTPREV     

#undef FUNC_NAME
#undef DEST_PREFETCHING
