//============================================================================
//
// File: DXT5QDecodeAsm.inl
// Copyright (c) 2005-2006, Ensemble Studios
// RG
//
//============================================================================
#if defined(CODE_ANALYSIS_ENABLED)
static uint FUNC_NAME(
   uint ptrTopSrc,                  // r3
   uint srcDataLen,                 // r4
   uint ptrTopDst,                  // r5
   uint pDstEnd,                    // r6
   uint pColorCodebook,             // r7
   uint pColorSelectorCodebookD,    // r8
   uint pAlphaCodebook,             // r9
   uint64 pAlphaSelectorCodebook      // r10 (D in low dword, W in high dword)
   ) 
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
   uint pColorSelectorCodebookD,    // r8
   uint pAlphaCodebook,             // r9
   uint64 pAlphaSelectorCodebook      // r10 (D in low dword, W in high dword)
   )    
{
#define PTRTOPSRC                r3
#define SRCDATALEN               r4
#define PTRTOPDST                r5
#define PDSTEND                  r6
#define PCOLORCODEBOOK           r7
#define PCOLORSELECTORCODEBOOKD  r8
#define PALPHACODEBOOK           r9
#define PALPHASELECTORCODEBOOKD  r10

   __asm
   {
      mflr         r12
      bl           __savegprlr
                        
      // 3-10 params    volatile
      // 11-12 scratch  volatile
      // 14-31          nonvolatile
            
      // avail: 14-31

#define SRCENDMINUS2             r4 
#define PALPHASELECTORCODEBOOKW  r11
#define PDSTPREV                 r27
#define PDST                     r28
#define PSRC                     r29
#define PACKET0BITS              r30
#define PACKET1BITS              r31
      
      rldicl      PALPHASELECTORCODEBOOKW, PALPHASELECTORCODEBOOKD, 32, 32      // SHR 32
      rldicl      PALPHASELECTORCODEBOOKD, PALPHASELECTORCODEBOOKD, 0, 32
      
      lwz         PSRC, 0(PTRTOPSRC)
      lwz         PDST, 0(PTRTOPDST)
           
      add         SRCENDMINUS2, PSRC, SRCDATALEN
      subi        SRCENDMINUS2, SRCENDMINUS2, 32

      cmpw        cr6, PSRC, SRCENDMINUS2
      bge         cr6, DXT5AsmRet

#if DEST_PREFETCHING      
      mr          PDSTPREV, PDST
#endif      
                        
      nopalign    8

DXT5QAsmPacketLoop:  

#if DEST_PREFETCHING      
      sub         r25, PDST, PDSTPREV
      ld          PACKET0BITS, 0(PSRC)                           
      
      li          r24, 256
                  
      cmplwi      cr6, R25, 128
      ld          PACKET1BITS, 8(PSRC)                           
                  
      dcbt        r24, PSRC
      
      bge         cr6, DXT5AsmDstPrefetch
      
      nopalign    8
#else
      ld          PACKET0BITS, 0(PSRC)                           
      ld          PACKET1BITS, 8(PSRC)                           
      
      li          r24, 256
      dcbt        r24, PSRC
#endif      
                  
DXT5AsmNoDstPrefetch:
                        
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
      
      cmpdi        cr6, PACKET0BITS, 0
            
      // r14,r15,r16,r17,r18,r19, r20,r21,r22
      // r23, r24, r25, r26
      
      rldicl       r15, PACKET0BITS, 54, 10      // SHR 10
      
      blt          cr6, DXT5AsmProcessRun
      
      rldicl       r18, PACKET0BITS, 43, 21       // SHR 21
      rldicl       r16, PACKET0BITS, 33, 31      // SHR 31
      rldicl       r17, PACKET0BITS, 22, 42      // SHR 42
      rldicl       r14, PACKET0BITS, 12, 52      // SHR 52
      rlwinm       r19, PACKET0BITS, 1, 21, 30   // SHL 1
                        
      rlwinm       r15, r15, 2, 19, 29          // SHL 2
      rlwinm       r18, r18, 1, 21, 30          // SHL 1
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r17, r17, 1, 21, 30          // SHL 1
      rlwinm       r14, r14, 2, 19, 29          // SHL 2
      
      lhzx         r19, r19, PALPHACODEBOOK
      rldicl       r25, PACKET1BITS, 43, 21       // SHR 21
      
      lwzx         r20, r15, PALPHASELECTORCODEBOOKD
      rldicl       r23, PACKET1BITS, 22, 42      // SHR 42
      
      lhzx         r18, r18, PALPHACODEBOOK
      rlwinm       r24, PACKET1BITS, 2, 20, 29   // SHL 2
      
      lwzx         r21, r16, PALPHASELECTORCODEBOOKD
      rldicl       r15, r15, 63, 1              // SHR 1
      
      lhzx         r17, r17, PALPHACODEBOOK
      rldicl       r16, r16, 63, 1              // SHR 1
      
      lwzx         r22, r14, PALPHASELECTORCODEBOOKD
      rldicl       r14, r14, 63, 1              // SHR 1
      
      lhzx         r15, r15, PALPHASELECTORCODEBOOKW
      rotlwi       r19, r19, 16
      
      lhzx         r16, r16, PALPHASELECTORCODEBOOKW
      rotlwi       r18, r18, 16
      
      lhzx         r14, r14, PALPHASELECTORCODEBOOKW
      rotlwi       r17, r17, 16
                  
      or           r19, r19, r15
      or           r18, r18, r16
      or           r17, r17, r14
      
      // r14, r15, r16, r23, r24, r25, r26
      
      rldicl       r15, PACKET1BITS, 54, 10      // SHR 10
      rldicl       r16, PACKET1BITS, 33, 31      // SHR 31
      rldicl       r14, PACKET1BITS, 12, 52      // SHR 52
      
      ld          PACKET0BITS, 16(PSRC)                           
      rlwinm       r15, r15, 2, 19, 29          // SHL 2
      
      ld          PACKET1BITS, 24(PSRC)                                                   
      rlwinm       r25, r25, 2, 20, 29          // SHL 2
      
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r23, r23, 2, 20, 29          // SHL 2
      rlwinm       r14, r14, 2, 19, 29          // SHL 2
      
      lwzx         r24, r24, PCOLORCODEBOOK
      addi         PDST, PDST, 48           
            
      lwzx         r15, r15, PCOLORSELECTORCODEBOOKD
      addi         PSRC, PSRC, 16
      
      lwzx         r25, r25, PCOLORCODEBOOK
      
      lwzx         r16, r16, PCOLORSELECTORCODEBOOKD
            
      lwzx         r23, r23, PCOLORCODEBOOK
            
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD
      
      stw          r19, -48(PDST)     // A
      stw          r20, -44(PDST)
      stw          r24, -40(PDST)     // C
      stw          r15, -36(PDST)
      
      stw          r18, -32(PDST)    // A
      stw          r21, -28(PDST)
      stw          r25, -24(PDST)    // C
      stw          r16, -20(PDST)
      
      stw          r17, -16(PDST)    // A
      stw          r22, -12(PDST)
      stw          r23, -8(PDST)    // C
      stw          r14, -4(PDST)
                  
      //-----------------------------------------
      
      cmpdi        cr6, PACKET0BITS, 0

      // r14,r15,r16,r17,r18,r19, r20,r21,r22
      // r23, r24, r25, r26

      rldicl       r15, PACKET0BITS, 54, 10      // SHR 10

      blt          cr6, DXT5AsmProcessRun

      rldicl       r18, PACKET0BITS, 43, 21       // SHR 21
      rldicl       r16, PACKET0BITS, 33, 31      // SHR 31
      rldicl       r17, PACKET0BITS, 22, 42      // SHR 42
      rldicl       r14, PACKET0BITS, 12, 52      // SHR 52
      rlwinm       r19, PACKET0BITS, 1, 21, 30   // SHL 1

      rlwinm       r15, r15, 2, 19, 29          // SHL 2
      rlwinm       r18, r18, 1, 21, 30          // SHL 1
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r17, r17, 1, 21, 30          // SHL 1
      rlwinm       r14, r14, 2, 19, 29          // SHL 2

      lhzx         r19, r19, PALPHACODEBOOK
      rldicl       r25, PACKET1BITS, 43, 21       // SHR 21

      lwzx         r20, r15, PALPHASELECTORCODEBOOKD
      rldicl       r23, PACKET1BITS, 22, 42      // SHR 42

      lhzx         r18, r18, PALPHACODEBOOK
      rlwinm       r24, PACKET1BITS, 2, 20, 29   // SHL 2

      lwzx         r21, r16, PALPHASELECTORCODEBOOKD
      rldicl       r15, r15, 63, 1              // SHR 1

      lhzx         r17, r17, PALPHACODEBOOK
      rldicl       r16, r16, 63, 1              // SHR 1

      lwzx         r22, r14, PALPHASELECTORCODEBOOKD
      rldicl       r14, r14, 63, 1              // SHR 1

      lhzx         r15, r15, PALPHASELECTORCODEBOOKW
      rotlwi       r19, r19, 16

      lhzx         r16, r16, PALPHASELECTORCODEBOOKW
      rotlwi       r18, r18, 16

      lhzx         r14, r14, PALPHASELECTORCODEBOOKW
      rotlwi       r17, r17, 16

      or           r19, r19, r15
      or           r18, r18, r16
      or           r17, r17, r14

      // r14, r15, r16, r23, r24, r25, r26

      rldicl       r15, PACKET1BITS, 54, 10      // SHR 10
      rldicl       r16, PACKET1BITS, 33, 31      // SHR 31
      rldicl       r14, PACKET1BITS, 12, 52      // SHR 52

      rlwinm       r15, r15, 2, 19, 29          // SHL 2
      rlwinm       r25, r25, 2, 20, 29          // SHL 2
      rlwinm       r16, r16, 2, 19, 29          // SHL 2
      rlwinm       r23, r23, 2, 20, 29          // SHL 2
      rlwinm       r14, r14, 2, 19, 29          // SHL 2

      lwzx         r24, r24, PCOLORCODEBOOK
      addi         PDST, PDST, 48           

      lwzx         r15, r15, PCOLORSELECTORCODEBOOKD
      addi         PSRC, PSRC, 16

      lwzx         r25, r25, PCOLORCODEBOOK

      lwzx         r16, r16, PCOLORSELECTORCODEBOOKD
      cmpw         cr6, PSRC, SRCENDMINUS2

      lwzx         r23, r23, PCOLORCODEBOOK

      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD

      stw          r19, -48(PDST)     // A
      stw          r20, -44(PDST)
      stw          r24, -40(PDST)     // C
      stw          r15, -36(PDST)

      stw          r18, -32(PDST)    // A
      stw          r21, -28(PDST)
      stw          r25, -24(PDST)    // C
      stw          r16, -20(PDST)

      stw          r17, -16(PDST)    // A
      stw          r22, -12(PDST)
      stw          r23, -8(PDST)    // C
      stw          r14, -4(PDST)
      
      blt          cr6, DXT5QAsmPacketLoop
      
      //-----------------------------------------

DXT5AsmRet:      
      stw         PSRC, 0(PTRTOPSRC)
      stw         PDST, 0(PTRTOPDST)

      bl          __restgprlr

      nopalign    8

      //-----------------------------------------
            
DXT5AsmProcessRun:
      
      rldicl       r14, PACKET0BITS, 0, 55
      cmpldi       cr6, r14, 511		
      beq          cr6, DXT5AsmProcessBigRun
      
      rldicl       r14, PACKET0BITS, 4, 61        // runlen = SHR 7, mask with 7
            
      rlwinm       r15, PACKET0BITS, 1, 27, 30   // alpha = SHL 2 and mask 4 bits
      rldicl       r16, PACKET0BITS, 60, 4       // SHR 4
      rlwinm       r17, r16, 1, 26, 30          // alphaSelector = SHL 1 and mask 5 bits
      rlwinm       r16, r16, 2, 25, 29          // alphaSelector = SHL 2 and mask 5 bits
      
      lhzx         r15, r15, PALPHACODEBOOK
      lwzx         r16, r16, PALPHASELECTORCODEBOOKD      
      lhzx         r17, r17, PALPHASELECTORCODEBOOKW
      rotlwi       r15, r15, 16
      or           r15, r15, r17
                  
      rlwinm       r18, PACKET1BITS, 2, 26, 29   // color = SHL 2 and mask 4 bits
      rldicl       r19, PACKET1BITS, 60, 4       // SHR 4
      rlwinm       r19, r19, 2, 25, 29          // selector = SHL 2 and mask 5 bits
      lwzx         r18, r18, PCOLORCODEBOOK
      lwzx         r19, r19, PCOLORSELECTORCODEBOOKD      
      
DXT5AsmRunLoop:
      cmplwi       cr6, r14, 0
      subi         r14, r14, 1
      
      stw          r15, 0(PDST)
      stw          r16, 4(PDST)
      stw          r18, 8(PDST)
      stw          r19, 12(PDST)
      
      addi         PDST, PDST, 16
      
      bne          cr6, DXT5AsmRunLoop
      
      // r14,r15,r16,r17,r18,r19, r20,r21,r22
      rldicl       r17, PACKET0BITS, 55, 9    // SHR 9
      rldicl       r18, PACKET0BITS, 47, 17   // SHR 17
      rldicl       r14, PACKET0BITS, 38, 26   // SHR 26
      rldicl       r15, PACKET0BITS, 30, 34   // SHR 34
      rldicl       r16, PACKET0BITS, 21, 43   // SHR 43
      rldicl       r19, PACKET0BITS, 13, 51   // SHR 51
      
      rlwinm       r17, r17, 1, 23, 30       // SHL 1 A
      rlwinm       r18, r18, 2, 21, 29        // SHL 2 S 
      rlwinm       r14, r14, 1, 23, 30       // SHL 1 A
      rlwinm       r15, r15, 2, 21, 29       // SHL 2 S
      rlwinm       r16, r16, 1, 23, 30       // SHL 1 A
      rlwinm       r19, r19, 2, 21, 29       // SHL 2 S
      
      lhzx         r17, r17, PALPHACODEBOOK
      lwzx         r20, r18, PALPHASELECTORCODEBOOKD
      lhzx         r14, r14, PALPHACODEBOOK
      lwzx         r21, r15, PALPHASELECTORCODEBOOKD
      lhzx         r16, r16, PALPHACODEBOOK
      lwzx         r22, r19, PALPHASELECTORCODEBOOKD
      
      rldicl       r18, r18, 63, 1           // SHR 1
      rldicl       r15, r15, 63, 1           // SHR 1
      rldicl       r19, r19, 63, 1           // SHR 1
      
      rotlwi       r17, r17, 16
      rotlwi       r14, r14, 16
      rotlwi       r16, r16, 16
      
      lhzx         r18, r18, PALPHASELECTORCODEBOOKW
      lhzx         r15, r15, PALPHASELECTORCODEBOOKW
      lhzx         r19, r19, PALPHASELECTORCODEBOOKW
      
      or           r17, r17, r18
      or           r18, r14, r15
      or           r19, r16, r19
      
      // r14, r15, r16, r23, r24, r25, r26
            
      rldicl       r23, PACKET1BITS, 55, 9    // SHR 9
      rldicl       r24, PACKET1BITS, 47, 17   // SHR 17
      rldicl       r14, PACKET1BITS, 38, 26   // SHR 26
      rldicl       r15, PACKET1BITS, 30, 34   // SHR 34
      rldicl       r16, PACKET1BITS, 21, 43   // SHR 43
      rldicl       r25, PACKET1BITS, 13, 51   // SHR 51
      
      rlwinm       r23, r23, 2, 22, 29       // SHL 2
      rlwinm       r24, r24, 2, 21, 29        // SHL 2
      rlwinm       r14, r14, 2, 22, 29       // SHL 2
      rlwinm       r15, r15, 2, 21, 29       // SHL 2
      rlwinm       r16, r16, 2, 22, 29       // SHL 2
      rlwinm       r25, r25, 2, 21, 29       // SHL 2
      
      lwzx         r23, r23, PCOLORCODEBOOK
      lwzx         r24, r24, PCOLORSELECTORCODEBOOKD
      lwzx         r14, r14, PCOLORCODEBOOK
      lwzx         r15, r15, PCOLORSELECTORCODEBOOKD
      lwzx         r16, r16, PCOLORCODEBOOK
      lwzx         r25, r25, PCOLORSELECTORCODEBOOKD
      
      stw          r17, 0(PDST)  //A
      stw          r20, 4(PDST)
      stw          r23, 8(PDST)  //C
      stw          r24, 12(PDST)
      
      stw          r18, 16(PDST) //A
      stw          r21, 20(PDST)
      stw          r14, 24(PDST) //C
      stw          r15, 28(PDST)
      
      stw          r19, 32(PDST) //A
      stw          r22, 36(PDST)     
      stw          r16, 40(PDST) //C
      stw          r25, 44(PDST)     
                  
      addi         PSRC, PSRC, 16
      addi         PDST, PDST, 48

      cmpw         cr6, PSRC, SRCENDMINUS2
      blt          cr6, DXT5QAsmPacketLoop
      
      b            DXT5AsmRet            
      
      //-----------------------------------------
      nopalign    8

DXT5AsmProcessBigRun:
            
      rldicl       r16, PACKET0BITS, 46, 18   // SHR 18
      rldicl       r17, PACKET0BITS, 36, 28    // SHR 28
      rlwinm       r16, r16, 1, 21, 30         // SHL 1, mask 10 bits
      rlwinm       r18, r17, 1, 20, 30         // SHL 1, mask 11 bits
      rlwinm       r17, r17, 2, 19, 29         // SHL 2, mask 11 bits
      
      rldicl       r15, PACKET1BITS, 46, 18   // SHR 18
      rldicl       r14, PACKET1BITS, 36, 28    // SHR 28
      
      rlwinm       r15, r15, 2, 20, 29        // SHL 2, mask 10 bits
      rlwinm       r14, r14, 2, 19, 29         // SHL 2, mask 11 bits
      
      lhzx         r16, r16, PALPHACODEBOOK
      rldicl       r19, PACKET0BITS, 55, 55     // SHR 9, mask by 511      
            
      lwzx         r17, r17, PALPHASELECTORCODEBOOKD
            
      lhzx         r18, r18, PALPHASELECTORCODEBOOKW
      rotlwi       r16, r16, 16
                  
      lwzx         r15, r15, PCOLORCODEBOOK
      addi         r19, r19, 4                  
      
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD
      or           r16, r16, r18
      
DXT5AsmBigRunLoop4:      
      subi         r19, r19, 4
      mr           r18, PDST

      stw          r16, 0(PDST)
      stw          r17, 4(PDST)                  
      stw          r15, 8(PDST)
      stw          r14, 12(PDST)
      
      stw          r16, 16(PDST)
      stw          r17, 20(PDST)                  
      stw          r15, 24(PDST)
      stw          r14, 28(PDST)
      
      addi         PDST, PDST, 64
      
      stw          r16, 32(r18)
      stw          r17, 36(r18)                  
      stw          r15, 40(r18)
      stw          r14, 44(r18)

      cmplwi       cr6, r19, 4      
      
      stw          r16, 48(r18)
      stw          r17, 52(r18)                  
      stw          r15, 56(r18)
      stw          r14, 60(r18)
            
      bge          cr6, DXT5AsmBigRunLoop4
      
      cmplwi       cr6, r19, 0
      beq          cr6, DXT5AsmBigRunFinish
                                          
DXT5AsmBigRunLoop:      
      subi         r19, r19, 1
                  
      stw          r16, 0(PDST)
      stw          r17, 4(PDST)                  
      stw          r15, 8(PDST)
      stw          r14, 12(PDST)
      
      cmplwi       cr6, r19, 0
      addi         PDST, PDST, 16
      
      bne          cr6, DXT5AsmBigRunLoop

DXT5AsmBigRunFinish:
      rldicl       r16, PACKET0BITS, 25, 39   // SHR 39
      rldicl       r17, PACKET0BITS, 15, 49    // SHR 49
      rlwinm       r16, r16, 1, 21, 30         // SHL 1, mask 10 bits
      rlwinm       r18, r17, 1, 20, 30         // SHL 1, mask 11 bits
      rlwinm       r17, r17, 2, 19, 29         // SHL 2, mask 11 bits
      
      lhzx         r16, r16, PALPHACODEBOOK
      rldicl       r15, PACKET1BITS, 25, 39      // SHR 39
      
      lwzx         r17, r17, PALPHASELECTORCODEBOOKD
      rldicl       r14, PACKET1BITS, 15, 49      // SHR 49
      
      lhzx         r18, r18, PALPHASELECTORCODEBOOKW
      rlwinm       r15, r15, 2, 20, 29          // SHL 2
      
      rotlwi       r16, r16, 16
      or           r16, r16, r18
      
      rlwinm       r14, r14, 2, 19, 29          // SHL 2
                  
      lwzx         r15, r15, PCOLORCODEBOOK
      addi         PSRC, PSRC, 16
      
      lwzx         r14, r14, PCOLORSELECTORCODEBOOKD
            
      stw          r16, 0(PDST)
      stw          r17, 4(PDST)
      stw          r15, 8(PDST)
      stw          r14, 12(PDST)
      
      addi         PDST, PDST, 16
      
      cmpw         cr6, PSRC, SRCENDMINUS2      
      
      blt          cr6, DXT5QAsmPacketLoop

      b            DXT5AsmRet    
           
      //-----------------------------------------

#if DEST_PREFETCHING
      nopalign    8
      
DXT5AsmDstPrefetch:      
      // rg [12/31/06] - This forces all written cachelines out of the cache, which is probably not what we want.
      //dcbf        r0, PDSTPREV

      mr          PDSTPREV, PDST

      addi        r25, PDST, 127
      clrrwi      r25, r25, 7
      addi        r25, r25, 384

      addi        r26, r25, 127
      cmpw        cr6, r26, PDSTEND
      bge         DXT5AsmNoDstPrefetch

      dcbz128     r0, r25      
      
      b           DXT5AsmNoDstPrefetch
#endif      
   }
}
#endif // CODE_ANALYSIS_ENABLED

#undef PTRTOPSRC                
#undef SRCDATALEN               
#undef PTRTOPDST                
#undef PDSTEND                  
#undef PCOLORCODEBOOK           
#undef PCOLORSELECTORCODEBOOKD  
#undef PALPHACODEBOOK           
#undef PALPHASELECTORCODEBOOKD  

#undef SRCENDMINUS2              
#undef PALPHASELECTORCODEBOOKW  
#undef PDSTPREV                 
#undef PDST                     
#undef PSRC                     
#undef PACKET0BITS              
#undef PACKET1BITS              

#undef FUNC_NAME
#undef DEST_PREFETCHING

