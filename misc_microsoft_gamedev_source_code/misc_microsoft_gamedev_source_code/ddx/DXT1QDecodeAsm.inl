//============================================================================
//
// File: DXT1QDecodeAsm.inl
// Copyright (c) 2005-2006, Ensemble Studios
// RG
//
//============================================================================

extern "C" void __savegprlr(void);
extern "C" void __restgprlr(void);

#if defined(CODE_ANALYSIS_ENABLED)
static uint FUNC_NAME(
   uint ptrTopSrc,                  // r3
   uint srcDataLen,                 // r4
   uint ptrTopDst,                  // r5
   uint pDstEnd,                    // r6
   uint pColorCodebook,             // r7
   uint pColorSelectorCodebookD)    // r8
{
	return 0;
}
#else
static uint __declspec(naked) FUNC_NAME(
   uint ptrTopSrc,                  // r3
   uint srcDataLen,                 // r4
   uint ptrTopDst,                  // r5
   uint pDstEnd,                    // r6
   uint pColorCodebook,             // r7
   uint pColorSelectorCodebookD)    // r8
{
#define PTRTOPSRC          r3
#define SRCDATALEN         r4
#define PTRTOPDST          r5
#define PDSTEND            r6
#define PCOLORCODEBOOK     r7
#define PCOLORSELECTORCODEBOOKD r8

   __asm
   {
      mflr         r12
      bl           __savegprlr
            
      // 3-8 params    volatile
      // 11-12 scratch  volatile
      // 14-31          nonvolatile
            
      // avail: 9, 10, 11, 14-31

#define SRCENDMINUS3 r4 
#define PDSTPREV     r28
#define PDST         r29
#define PSRC         r30
#define PACKETBITS   r31

      lwz         PSRC, 0(PTRTOPSRC)
      lwz         PDST, 0(PTRTOPDST)
           
      add         SRCENDMINUS3, PSRC, SRCDATALEN
      subi        SRCENDMINUS3, SRCENDMINUS3, 24

      cmpw        cr6, PSRC, SRCENDMINUS3
      bge         cr6, DXT1AsmRet

#if DEST_PREFETCHING      
      mr          PDSTPREV, PDST
#endif      
                        
      nopalign    8

DXT1QAsmPacketLoop:  

#if DEST_PREFETCHING      
      xor         r25, PDST, PDSTPREV
      ld          PACKETBITS, 0(PSRC)
      
      li          r24, 256
      dcbt        r24, PSRC
      
      clrrwi      r25, r25, 7
                  
      cmplwi      cr6, r25, 0
      bne         cr6, DXT1AsmDstPrefetch
      
      nopalign    8
#else
      ld          PACKETBITS, 0(PSRC)                           
      
      li          r24, 256
      dcbt        r24, PSRC
#endif      
                  
DXT1AsmNoDstPrefetch:
                        
      //                             ****     
      // 00000000*0000000*0000000*0000000
      // 32      40      48      56    63      
      // 0       8       16      24    31
      //
      //                                                       
      // 00000000*0000000*0000000*0000000*0000000*0000000*0000000*0000000
      //                                 32      40      48      56    63      
      // 0       8       16      24    31
            
      //------------------------------------------------------- 
      
      cmpdi        cr6, PACKETBITS, 0
      blt          cr6, DXT1AsmProcessRun
      
      rldicl       r10, PACKETBITS, 54, 10      // SHR 10
      rldicl       r9, PACKETBITS, 43, 21       // SHR 21
      rldicl       r16, PACKETBITS, 33, 31      // SHR 31
      rldicl       r17, PACKETBITS, 22, 42      // SHR 42
      rldicl       r14, PACKETBITS, 12, 52      // SHR 52
      rlwinm       r11, PACKETBITS, 2, 20, 29   // SHL 2
      
      rlwinm       r10, r10, 2, 19, 29          // SHL 2
      rlwinm       r9, r9, 2, 20, 29            // SHL 2
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r17, r17, 2, 20, 29          // SHL 2
      rlwinm       r14, r14, 2, 19, 29          // SHL 2
      
      lwzx         r11, r11, PCOLORCODEBOOK
      lwzx         r10, r10, PCOLORSELECTORCODEBOOKD
      lwzx         r9,  r9,  PCOLORCODEBOOK
      lwzx         r16, r16, PCOLORSELECTORCODEBOOKD
      lwzx         r17, r17, PCOLORCODEBOOK
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD
      
      stw          r11, 0(PDST)
      stw          r10, 4(PDST)
      
      stw          r9, 8(PDST)
      stw          r16, 12(PDST)
      
      stw          r17, 16(PDST)
      stw          r14, 20(PDST)
      
      //------------------------------------------------------- 
      
      ld          PACKETBITS, 8(PSRC)
      
      addi         PSRC, PSRC, 8
      addi         PDST, PDST, 24
      
      cmpdi        cr6, PACKETBITS, 0
      blt          cr6, DXT1AsmProcessRun

      rldicl       r10, PACKETBITS, 54, 10      // SHR 10
      rldicl       r9, PACKETBITS, 43, 21       // SHR 21
      rldicl       r16, PACKETBITS, 33, 31      // SHR 31
      rldicl       r17, PACKETBITS, 22, 42      // SHR 42
      rldicl       r14, PACKETBITS, 12, 52      // SHR 52
      rlwinm       r11, PACKETBITS, 2, 20, 29   // SHL 2

      rlwinm       r10, r10, 2, 19, 29          // SHL 2
      rlwinm       r9, r9, 2, 20, 29            // SHL 2
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r17, r17, 2, 20, 29          // SHL 2
      rlwinm       r14, r14, 2, 19, 29          // SHL 2

      lwzx         r11, r11, PCOLORCODEBOOK
      lwzx         r10, r10, PCOLORSELECTORCODEBOOKD
      lwzx         r9,  r9,  PCOLORCODEBOOK
      lwzx         r16, r16, PCOLORSELECTORCODEBOOKD
      lwzx         r17, r17, PCOLORCODEBOOK
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD

      stw          r11, 0(PDST)
      stw          r10, 4(PDST)

      stw          r9, 8(PDST)
      stw          r16, 12(PDST)

      stw          r17, 16(PDST)
      stw          r14, 20(PDST)
      
      //------------------------------------------------------- 

      ld           PACKETBITS, 8(PSRC)

      addi         PSRC, PSRC, 8
      addi         PDST, PDST, 24

      cmpdi        cr6, PACKETBITS, 0
      blt          cr6, DXT1AsmProcessRun

      rldicl       r10, PACKETBITS, 54, 10      // SHR 10
      rldicl       r9, PACKETBITS, 43, 21       // SHR 21
      rldicl       r16, PACKETBITS, 33, 31      // SHR 31
      rldicl       r17, PACKETBITS, 22, 42      // SHR 42
      rldicl       r14, PACKETBITS, 12, 52      // SHR 52
      rlwinm       r11, PACKETBITS, 2, 20, 29   // SHL 2

      rlwinm       r10, r10, 2, 19, 29          // SHL 2
      rlwinm       r9, r9, 2, 20, 29            // SHL 2
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r17, r17, 2, 20, 29          // SHL 2
      rlwinm       r14, r14, 2, 19, 29          // SHL 2

      lwzx         r11, r11, PCOLORCODEBOOK
      lwzx         r10, r10, PCOLORSELECTORCODEBOOKD
      lwzx         r9,  r9,  PCOLORCODEBOOK
      lwzx         r16, r16, PCOLORSELECTORCODEBOOKD
      lwzx         r17, r17, PCOLORCODEBOOK
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD

      stw          r11, 0(PDST)
      stw          r10, 4(PDST)

      stw          r9, 8(PDST)
      stw          r16, 12(PDST)

      stw          r17, 16(PDST)
      stw          r14, 20(PDST)

      addi         PSRC, PSRC, 8
      addi         PDST, PDST, 24           
            
      cmpw         cr6, PSRC, SRCENDMINUS3
      blt          cr6, DXT1QAsmPacketLoop
      
      //-----------------------------------------

DXT1AsmRet:      
      stw         PSRC, 0(PTRTOPSRC)
      stw         PDST, 0(PTRTOPDST)

      bl          __restgprlr

      nopalign    8

      //-----------------------------------------
            
DXT1AsmProcessRun:
      
      rldicl       r10, PACKETBITS, 0, 55
      cmpldi       cr6, r10, 511		
      beq          cr6, DXT1AsmProcessBigRun
      
      rldicl       r9, PACKETBITS, 4, 61        // runlen = SHR 7, mask with 7
                  
      rlwinm       r10, PACKETBITS, 2, 26, 29   // color = SHL 2 and mask 4 bits
      
      rldicl       r14, PACKETBITS, 60, 4       // SHR 4
      rlwinm       r14, r14, 2, 25, 29          // selector = SHL 2 and mask 5 bits
      
      lwzx         r10, r10, PCOLORCODEBOOK
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD      
      
DXT1AsmRunLoop:
      cmplwi       cr6, r9, 0
      subi         r9, r9, 1
      
      stw          r10, 0(PDST)
      stw          r14, 4(PDST)
      addi         PDST, PDST, 8
      
      bne          cr6, DXT1AsmRunLoop
            
      rldicl       r10, PACKETBITS, 55, 9    // SHR 9
      rldicl        r9, PACKETBITS, 47, 17   // SHR 17
      rldicl       r14, PACKETBITS, 38, 26   // SHR 26
      rldicl       r15, PACKETBITS, 30, 34   // SHR 34
      rldicl       r16, PACKETBITS, 21, 43   // SHR 43
      rldicl       r17, PACKETBITS, 13, 51   // SHR 51
      
      rlwinm       r10, r10, 2, 22, 29       // SHL 2
      rlwinm        r9, r9, 2, 21, 29        // SHL 2
      rlwinm       r14, r14, 2, 22, 29       // SHL 2
      rlwinm       r15, r15, 2, 21, 29       // SHL 2
      rlwinm       r16, r16, 2, 22, 29       // SHL 2
      rlwinm       r17, r17, 2, 21, 29       // SHL 2
      
      lwzx         r10, r10, PCOLORCODEBOOK
      lwzx          r9,  r9, PCOLORSELECTORCODEBOOKD
      lwzx         r14, r14, PCOLORCODEBOOK
      lwzx         r15, r15, PCOLORSELECTORCODEBOOKD
      lwzx         r16, r16, PCOLORCODEBOOK
      lwzx         r17, r17, PCOLORSELECTORCODEBOOKD
      
      stw          r10,  0(PDST)
      stw          r9,   4(PDST)
      stw          r14,  8(PDST)
      stw          r15, 12(PDST)
      stw          r16, 16(PDST)
      stw          r17, 20(PDST)     
            
      addi         PSRC, PSRC, 8
      addi         PDST, PDST, 24

      cmpw         cr6, PSRC, SRCENDMINUS3
      blt          cr6, DXT1QAsmPacketLoop
      
      b            DXT1AsmRet            
      
      //-----------------------------------------
      nopalign    8

DXT1AsmProcessBigRun:

      rldicl       r9, PACKETBITS, 55, 55     // SHR 9, mask by 511
      addi         r9, r9, 4
               
      rldicl       r10, PACKETBITS, 46, 18   // SHR 18
      
      cmplwi       cr6, r9, 8
            
      rldicl       r14, PACKETBITS, 36, 28    // SHR 28
      
      rlwinm       r10, r10, 2, 20, 29        // SHL 2, mask 10 bits
      rlwinm       r14, r14, 2, 19, 29         // SHL 2, mask 11 bits
            
      lwzx         r10, r10, PCOLORCODEBOOK
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD
      
      //rldicr       r16, r10, 32, 31
      //or           r16, r16, r14
      
      blt          cr6, DXT1AsmBigRunLoop
      
      // 8 blocks, 8 bytes per block, 16 DWORD's, 8 QWORD's, 8*8=64 bytes
                              
DXT1AsmBigRunLoop4:
      subi         r9, r9, 8
      
      mr           r15, PDST
                              
      //std          r16, 0(PDST)
      stw          r10, 0(PDST)
      stw          r14, 4(PDST)
            
      cmplwi       cr6, r9, 8
      
      //std          r16, 8(PDST)
      //std          r16, 16(PDST)
      //std          r16, 24(PDST)
      
      stw          r10, 8(PDST)
      stw          r14, 12(PDST)
      
      stw          r10, 16(PDST)
      stw          r14, 20(PDST)
      
      stw          r10, 24(PDST)
      stw          r14, 28(PDST)
      
      addi         PDST, PDST, 64
      
      //std          r16, 32(r15)
      //std          r16, 40(r15)
      //std          r16, 48(r15)
      //std          r16, 56(r15)
      
      stw          r10, 32(r15)
      stw          r14, 36(r15)
      
      stw          r10, 40(r15)
      stw          r14, 44(r15)
      
      stw          r10, 48(r15)
      stw          r14, 52(r15)
      
      stw          r10, 56(r15)
      stw          r14, 60(r15)
                       
      bge          cr6, DXT1AsmBigRunLoop4

      cmplwi       cr6, r9, 0
      beq          cr6, DXT1AsmBigRunFinish
      
DXT1AsmBigRunLoop:      
      subi         r9, r9, 1
                  
      stw          r10, 0(PDST)
      stw          r14, 4(PDST)
      
      cmplwi       cr6, r9, 0
      addi         PDST, PDST, 8
      
      bne          cr6, DXT1AsmBigRunLoop

DXT1AsmBigRunFinish:
      mr           r15, PDST
      
      rldicl       r10, PACKETBITS, 25, 39      // SHR 39
      rldicl       r11, PACKETBITS, 15, 49      // SHR 49
      
      rlwinm       r10, r10, 2, 20, 29          // SHL 2
      rlwinm       r11, r11, 2, 19, 29          // SHL 2
      
      lwzx         r10, r10, PCOLORCODEBOOK
      addi         PSRC, PSRC, 8
      
      lwzx         r11, r11, PCOLORSELECTORCODEBOOKD
      addi         PDST, PDST, 8
      
      cmpw         cr6, PSRC, SRCENDMINUS3
      
      stw          r10, 0(r15)
      stw          r11, 4(r15)
      
      blt          cr6, DXT1QAsmPacketLoop

      b            DXT1AsmRet    
           
      //-----------------------------------------

#if DEST_PREFETCHING
      nopalign    8
      
DXT1AsmDstPrefetch:      
      // rg [12/31/06] - This forces all written cachelines out of the cache, which is probably not what we want.
      //dcbf        r0, PDSTPREV

      mr          PDSTPREV, PDST

      addi        r25, PDST, 127
      clrrwi      r25, r25, 7
      addi        r25, r25, 384

      addi        r26, r25, 127
      cmpw        cr6, r26, PDSTEND
      bge         DXT1AsmNoDstPrefetch

      dcbz128     r0, r25      
      
      b           DXT1AsmNoDstPrefetch
#endif      
   }
}
#endif // CODE_ANALYSIS_ENABLED

#undef SRCENDMINUS3 
#undef PDSTPREV     
#undef PDST         
#undef PSRC         
#undef PACKETBITS   

#undef SRCENDMINUS3  
#undef PDSTPREV     
#undef PDST         
#undef PSRC         
#undef PACKETBITS   

#undef FUNC_NAME
#undef DEST_PREFETCHING

