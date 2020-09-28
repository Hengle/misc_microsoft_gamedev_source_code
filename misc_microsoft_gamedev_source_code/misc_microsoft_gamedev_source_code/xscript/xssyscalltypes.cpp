//==============================================================================
// xssyscalltypes.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "xscript.h"
#include "xssyscalltypes.h"
#include "xssyscallmodule.h"

#pragma warning(disable:4800)

const char* gSyscallTypeString[] = 
{
   "void",
   "d",
   "f",
   "b",
   "b1d1",
   "b1d2",
   "b1d6",
   "b1f1",
   "b1f2",
   "d1b1",
   "d1b1d1",
   "d1b2",
   "d1b2f4b2",
   "d1b3f1b1",
   "d1b4",
   "d1f1",
   "d1f1b1",
   "d1f1b1d1",
   "d1f1d1",
   "d1f1d1b1",
   "d1f1d1f2",
   "d1f1d1f4",
   "d1f1d3",
   "d1f2",
   "d1f2b1",
   "d1f2d1",
   "d1f2d1f1",
   "d1f3",
   "d1f3d1",
   "d1f4",
   "d1f4b1",
   "d1f4d2b1",
   "d1f5",
   "d1f5b1",
   "d1f7",
   "d2b1",
   "d2b1d1",
   "d2b1d1b1",
   "d2b1f1",
   "d2b2",
   "d2f1",
   "d2f1b1",
   "d2f1b2",
   "d2f1d1",
   "d2f1d1f1",
   "d2f2",
   "d2f2b1",
   "d2f2d1",
   "d2f3",
   "d2f3d1",
   "d2f3d2",
   "d2f4d1",
   "d3b1",
   "d3b1d2",
   "d3b2f3",
   "d3b3",
   "d3f1",
   "d3f1b1",
   "d3f2b1",
   "d3f3",
   "d3f3d1b1",
   "d4b1",
   "d4b1d3",
   "d4f1d1",
   "d4f2",
   "d5b1",
   "d5b1d1",
   "d5f1",
   "f1b1",
   "f1b1d4",
   "f1d1",
   "f1d1f1",
   "f1d1f2",
   "f2b1",
   "f2d1",
   "f2d2",
   "f3d1",
   "f3d1b1d2f1",
   "f3d1b2",
	"f3d1b2f1",
	"d2b2f1",
	"d2f4",
	"d3f4d1",
	"f5b1",
   ""
};

struct BOutgoingParms
{
   long  r10;
   long  r9;
   long  r8;
   long  r7;
   long  r6;
   long  r5;
   long  r4;
   long  r3;
};

//==============================================================================
// BXSSyscallModule::callVoidSyscall
//==============================================================================
bool BXSSyscallModule::callVoidSyscall(long syscallID, long *parms, long numberParms)
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(false);
   if (mSyscalls[syscallID] == NULL)
      return(false);
   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||
      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))
      )
      return(false);

   //Do the call.
   long syscallAddress=mSyscalls[syscallID]->getAddress();
   //If we have an invalid address, just return in a default way.  We return true
   //so that the interpreter will keep going as if it all worked.  This allows us
   //to fixup the syscall addresses in a savegame.
   if (syscallAddress == -1)
   {
      return(true);
   }

   float* parmsF = reinterpret_cast<float*>(parms);
   long syscallType=mSyscalls[syscallID]->getSyscallType();

   // Test code to demonstrate generic function calling mechanism for 
   // Xenon script exports.
/*#ifdef XBOX  

   if (numberParms <= 8 && (syscallType == cSyscallType_d))
   {

      BOutgoingParms outParms;
      memset(&outParms, 0, sizeof(outParms));
      long *pVar = &outParms.r3;
      for (long i =0; i < numberParms; i++)
      {
        
        *pVar = parms[i];
        pVar--;
      }

      BOutgoingParms *pOutParms = &outParms;

      //-- DO THE CALL
      _asm
      {
      //-- NOT OPTIMIZED
            //-- load the start of outParms
            lwz              r3, pOutParms
            lwz              r10, 0(r3)
            lwz              r4, pOutParms
            lwz              r9, 4(r4)
            lwz              r5, pOutParms
            lwz              r8, 8(r5)
            lwz              r6, pOutParms
            lwz              r7, 12(r6)
            lwz              r6, 16(r3)
            lwz              r5, 20(r4)
            lwz              r14, pOutParms
            lwz              r4, 24(r14)
            lwz              r12, pOutParms
            lwz              r3, 28(r12)
            lwz              r11,128(r1)
            mtctr            r11
            bctrl            
      }

      return true;
   }

  
#endif*/

   switch(syscallType)
   {
      case cSyscallType_void: ((XSPROC)syscallAddress)(); break;

      case cSyscallType_d:
         switch (numberParms)
         {
            case 1:  ((XSPROC_d1) syscallAddress)(parms      [0]); break;
            case 2:  ((XSPROC_d2) syscallAddress)(parms      [0], parms      [1]); break;
            case 3:  ((XSPROC_d3) syscallAddress)(parms      [0], parms      [1], parms      [2]); break;
            case 4:  ((XSPROC_d4) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3]); break;
            case 5:  ((XSPROC_d5) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4]); break;
            case 6:  ((XSPROC_d6) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
            case 7:  ((XSPROC_d7) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
            case 8:  ((XSPROC_d8) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7]); break;
            case 9:  ((XSPROC_d9) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8]); break;
            case 10: ((XSPROC_d10)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9]); break;
            case 11: ((XSPROC_d11)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10]); break;
            case 12: ((XSPROC_d12)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11]); break;
            case 13: ((XSPROC_d13)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12]); break;
            case 14: ((XSPROC_d14)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12], parms      [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_f:
         switch (numberParms)
         {
            case 1:  ((XSPROC_f1) syscallAddress)(parmsF     [0]); break;
            case 2:  ((XSPROC_f2) syscallAddress)(parmsF     [0], parmsF     [1]); break;
            case 3:  ((XSPROC_f3) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2]); break;
            case 4:  ((XSPROC_f4) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
            case 5:  ((XSPROC_f5) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
            case 6:  ((XSPROC_f6) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
            case 7:  ((XSPROC_f7) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
            case 8:  ((XSPROC_f8) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
            case 9:  ((XSPROC_f9) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8]); break;
            case 10: ((XSPROC_f10)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9]); break;
            case 11: ((XSPROC_f11)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10]); break;
            case 12: ((XSPROC_f12)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11]); break;
            case 13: ((XSPROC_f13)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12]); break;
            case 14: ((XSPROC_f14)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12], parmsF     [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b:
         switch (numberParms)
         {
            case 1:  ((XSPROC_b1) syscallAddress)((bool)parms[0]); break;
            case 2:  ((XSPROC_b2) syscallAddress)((bool)parms[0], (bool)parms[1]); break;
            case 3:  ((XSPROC_b3) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2]); break;
            case 4:  ((XSPROC_b4) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3]); break;
            case 5:  ((XSPROC_b5) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
            case 6:  ((XSPROC_b6) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
            case 7:  ((XSPROC_b7) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6]); break;
            case 8:  ((XSPROC_b8) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7]); break;
            case 9:  ((XSPROC_b9) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8]); break;
            case 10: ((XSPROC_b10)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9]); break;
            case 11: ((XSPROC_b11)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10]); break;
            case 12: ((XSPROC_b12)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11]); break;
            case 13: ((XSPROC_b13)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12]); break;
            case 14: ((XSPROC_b14)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12], (bool)parms[13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b1d1        : ((XSPROC_b1d1)        syscallAddress)((bool)parms[0], parms      [1]); break;
      case cSyscallType_b1d2        : ((XSPROC_b1d2)        syscallAddress)((bool)parms[0], parms      [1], parms      [2]); break;
      case cSyscallType_b1d6        : ((XSPROC_b1d6)        syscallAddress)((bool)parms[0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
      case cSyscallType_b1f1        : ((XSPROC_b1f1)        syscallAddress)((bool)parms[0], parmsF     [1]); break;
      case cSyscallType_b1f2        : ((XSPROC_b1f2)        syscallAddress)((bool)parms[0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1b1        : ((XSPROC_d1b1)        syscallAddress)(parms      [0], (bool)parms[1]); break;
      case cSyscallType_d1b1d1      : ((XSPROC_d1b1d1)      syscallAddress)(parms      [0], (bool)parms[1], parms      [2]); break;
      case cSyscallType_d1b2        : ((XSPROC_d1b2)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2]); break;
      case cSyscallType_d1b2f4b2    : ((XSPROC_d1b2f4b2)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], (bool)parms[7], (bool)parms[8]); break;
      case cSyscallType_d1b3f1b1    : ((XSPROC_d1b3f1b1)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1b4        : ((XSPROC_d1b4)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d1f1        : ((XSPROC_d1f1)        syscallAddress)(parms      [0], parmsF     [1]); break;
      case cSyscallType_d1f1b1      : ((XSPROC_d1f1b1)      syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_d1f1b1d1    : ((XSPROC_d1f1b1d1)    syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d1f1d1      : ((XSPROC_d1f1d1)      syscallAddress)(parms      [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_d1f1d1b1    : ((XSPROC_d1f1d1b1)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d1f1d1f2    : ((XSPROC_d1f1d1f2)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f1d1f4    : ((XSPROC_d1f1d1f4)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
      case cSyscallType_d1f1d3      : ((XSPROC_d1f1d3)      syscallAddress)(parms      [0], parmsF     [1], parms      [2], parms      [3], parms      [4]); break;
      case cSyscallType_d1f2        : ((XSPROC_d1f2)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1f2b1      : ((XSPROC_d1f2b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d1f2d1      : ((XSPROC_d1f2d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d1f2d1f1    : ((XSPROC_d1f2d1f1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d1f3        : ((XSPROC_d1f3)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d1f3d1      : ((XSPROC_d1f3d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d1f4        : ((XSPROC_d1f4)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f4b1      : ((XSPROC_d1f4b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1f4d2b1    : ((XSPROC_d1f4d2b1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d1f5        : ((XSPROC_d1f5)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d1f5b1      : ((XSPROC_d1f5b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], (bool)parms[6]); break;
      case cSyscallType_d1f7        : ((XSPROC_d1f7)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d2b1        : ((XSPROC_d2b1)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2]); break;
      case cSyscallType_d2b1d1      : ((XSPROC_d2b1d1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d2b1d1b1    : ((XSPROC_d2b1d1b1)    syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d2b1f1      : ((XSPROC_d2b1f1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parmsF     [3]); break;
      case cSyscallType_d2b2        : ((XSPROC_d2b2)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2], (bool)parms[3]); break;
      case cSyscallType_d2f1        : ((XSPROC_d2f1)        syscallAddress)(parms      [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_d2f1b1      : ((XSPROC_d2f1b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d2f1b2      : ((XSPROC_d2f1b2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d2f1d1      : ((XSPROC_d2f1d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d2f1d1f1    : ((XSPROC_d2f1d1f1)    syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d2f2        : ((XSPROC_d2f2)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d2f2b1      : ((XSPROC_d2f2b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d2f2d1      : ((XSPROC_d2f2d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d2f3        : ((XSPROC_d2f3)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d2f3d1      : ((XSPROC_d2f3d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d2f3d2      : ((XSPROC_d2f3d2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6]); break;
      case cSyscallType_d2f4d1      : ((XSPROC_d2f4d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6]); break;
      case cSyscallType_d3b1        : ((XSPROC_d3b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d3b1d2      : ((XSPROC_d3b1d2)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], parms      [4], parms      [5]); break;
      case cSyscallType_d3b2f3      : ((XSPROC_d3b2f3)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d3b3        : ((XSPROC_d3b3)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
      case cSyscallType_d3f1        : ((XSPROC_d3f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3]); break;
      case cSyscallType_d3f1b1      : ((XSPROC_d3f1b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d3f2b1      : ((XSPROC_d3f2b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d3f3        : ((XSPROC_d3f3)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d3f3d1b1    : ((XSPROC_d3f3d1b1)    syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d4b1        : ((XSPROC_d4b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d4b1d3      : ((XSPROC_d4b1d3)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parms      [7]); break;
      case cSyscallType_d4f1d1      : ((XSPROC_d4f1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d4f2        : ((XSPROC_d4f2)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d5b1        : ((XSPROC_d5b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5]); break;
      case cSyscallType_d5b1d1      : ((XSPROC_d5b1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5], parms      [6]); break;
      case cSyscallType_d5f1        : ((XSPROC_d5f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parmsF     [5]); break;
      case cSyscallType_f1b1        : ((XSPROC_f1b1)        syscallAddress)(parmsF     [0], (bool)parms[1]); break;
      case cSyscallType_f1b1d4      : ((XSPROC_f1b1d4)      syscallAddress)(parmsF     [0], (bool)parms[1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
      case cSyscallType_f1d1        : ((XSPROC_f1d1)        syscallAddress)(parmsF     [0], parms      [1]); break;
      case cSyscallType_f1d1f1      : ((XSPROC_f1d1f1)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_f1d1f2      : ((XSPROC_f1d1f2)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_f2b1        : ((XSPROC_f2b1)        syscallAddress)(parmsF     [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_f2d1        : ((XSPROC_f2d1)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_f2d2        : ((XSPROC_f2d2)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2], parms      [3]); break;
      case cSyscallType_f3d1        : ((XSPROC_f3d1)        syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_f3d1b1d2f1  : ((XSPROC_f3d1b1d2f1)  syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parmsF     [7]); break;
      case cSyscallType_f3d1b2      : ((XSPROC_f3d1b2)      syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], (bool)parms[5]); break;
		case cSyscallType_f3d1b2f1		: ((XSPROC_f3d1b2f1)		syscallAddress)(parmsF		[0], parmsF		 [1], parmsF	  [2], parms		[3], (bool)parms[4], (bool)parms[5], parmsF[6]); break;
		case cSyscallType_d2b2f1		: ((XSPROC_d2b2f1)		syscallAddress)(parms		[0], parms		 [1], (bool)parms[2], (bool)parms[3], parmsF[4]); break;
		case cSyscallType_d2f4			: ((XSPROC_d2f4)			syscallAddress)(parms		[0], parms		 [1], parmsF[2], parmsF[3], parmsF[4], parmsF[5]); break;
		case cSyscallType_d3f4d1		: ((XSPROC_d3f4d1)		syscallAddress)(parms		[0], parms		 [1], parms[2], parmsF[3], parmsF[4], parmsF[5], parmsF[6], parms[7]); break;
		case cSyscallType_f5b1		   : ((XSPROC_f5b1)		   syscallAddress)(parmsF   	[0], parmsF		 [1], parmsF[2], parmsF[3], parmsF[4],(bool)parms[5]); break;
   }

   return(true);
}
//==============================================================================
// BXSSyscallModule::callIntegerSyscall
//==============================================================================
bool BXSSyscallModule::callIntegerSyscall(long syscallID, long *parms, long numberParms, long *sRV, BXSData *data)
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(false);
   if (mSyscalls[syscallID] == NULL)
      return(false);
   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||
      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))
      || (sRV == NULL))
      return(false);

   //Do the call.
   long syscallAddress=mSyscalls[syscallID]->getAddress();
   //If we have an invalid address, just return in a default way.  We return true
   //so that the interpreter will keep going as if it all worked.  This allows us
   //to fixup the syscall addresses in a savegame.
   if (syscallAddress == -1)
   {
      *sRV=-1;
      if (data != NULL)
         data->setTempReturnValue((BYTE*)sRV, sizeof(long));
      return(true);
   }

   float* parmsF = reinterpret_cast<float*>(parms);
   long syscallType=mSyscalls[syscallID]->getSyscallType();

   switch(syscallType)
   {
      case cSyscallType_void: *sRV=((XSFUNd)syscallAddress)(); break;

      case cSyscallType_d:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNd_d1) syscallAddress)(parms      [0]); break;
            case 2:  *sRV=((XSFUNd_d2) syscallAddress)(parms      [0], parms      [1]); break;
            case 3:  *sRV=((XSFUNd_d3) syscallAddress)(parms      [0], parms      [1], parms      [2]); break;
            case 4:  *sRV=((XSFUNd_d4) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3]); break;
            case 5:  *sRV=((XSFUNd_d5) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4]); break;
            case 6:  *sRV=((XSFUNd_d6) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
            case 7:  *sRV=((XSFUNd_d7) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
            case 8:  *sRV=((XSFUNd_d8) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7]); break;
            case 9:  *sRV=((XSFUNd_d9) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8]); break;
            case 10: *sRV=((XSFUNd_d10)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9]); break;
            case 11: *sRV=((XSFUNd_d11)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10]); break;
            case 12: *sRV=((XSFUNd_d12)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11]); break;
            case 13: *sRV=((XSFUNd_d13)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12]); break;
            case 14: *sRV=((XSFUNd_d14)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12], parms      [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_f:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNd_f1) syscallAddress)(parmsF     [0]); break;
            case 2:  *sRV=((XSFUNd_f2) syscallAddress)(parmsF     [0], parmsF     [1]); break;
            case 3:  *sRV=((XSFUNd_f3) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2]); break;
            case 4:  *sRV=((XSFUNd_f4) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
            case 5:  *sRV=((XSFUNd_f5) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
            case 6:  *sRV=((XSFUNd_f6) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
            case 7:  *sRV=((XSFUNd_f7) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
            case 8:  *sRV=((XSFUNd_f8) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
            case 9:  *sRV=((XSFUNd_f9) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8]); break;
            case 10: *sRV=((XSFUNd_f10)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9]); break;
            case 11: *sRV=((XSFUNd_f11)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10]); break;
            case 12: *sRV=((XSFUNd_f12)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11]); break;
            case 13: *sRV=((XSFUNd_f13)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12]); break;
            case 14: *sRV=((XSFUNd_f14)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12], parmsF     [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNd_b1) syscallAddress)((bool)parms[0]); break;
            case 2:  *sRV=((XSFUNd_b2) syscallAddress)((bool)parms[0], (bool)parms[1]); break;
            case 3:  *sRV=((XSFUNd_b3) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2]); break;
            case 4:  *sRV=((XSFUNd_b4) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3]); break;
            case 5:  *sRV=((XSFUNd_b5) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
            case 6:  *sRV=((XSFUNd_b6) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
            case 7:  *sRV=((XSFUNd_b7) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6]); break;
            case 8:  *sRV=((XSFUNd_b8) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7]); break;
            case 9:  *sRV=((XSFUNd_b9) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8]); break;
            case 10: *sRV=((XSFUNd_b10)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9]); break;
            case 11: *sRV=((XSFUNd_b11)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10]); break;
            case 12: *sRV=((XSFUNd_b12)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11]); break;
            case 13: *sRV=((XSFUNd_b13)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12]); break;
            case 14: *sRV=((XSFUNd_b14)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12], (bool)parms[13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b1d1        : *sRV=((XSFUNd_b1d1)        syscallAddress)((bool)parms[0], parms      [1]); break;
      case cSyscallType_b1d2        : *sRV=((XSFUNd_b1d2)        syscallAddress)((bool)parms[0], parms      [1], parms      [2]); break;
      case cSyscallType_b1d6        : *sRV=((XSFUNd_b1d6)        syscallAddress)((bool)parms[0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
      case cSyscallType_b1f1        : *sRV=((XSFUNd_b1f1)        syscallAddress)((bool)parms[0], parmsF     [1]); break;
      case cSyscallType_b1f2        : *sRV=((XSFUNd_b1f2)        syscallAddress)((bool)parms[0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1b1        : *sRV=((XSFUNd_d1b1)        syscallAddress)(parms      [0], (bool)parms[1]); break;
      case cSyscallType_d1b1d1      : *sRV=((XSFUNd_d1b1d1)      syscallAddress)(parms      [0], (bool)parms[1], parms      [2]); break;
      case cSyscallType_d1b2        : *sRV=((XSFUNd_d1b2)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2]); break;
      case cSyscallType_d1b2f4b2    : *sRV=((XSFUNd_d1b2f4b2)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], (bool)parms[7], (bool)parms[8]); break;
      case cSyscallType_d1b3f1b1    : *sRV=((XSFUNd_d1b3f1b1)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1b4        : *sRV=((XSFUNd_d1b4)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d1f1        : *sRV=((XSFUNd_d1f1)        syscallAddress)(parms      [0], parmsF     [1]); break;
      case cSyscallType_d1f1b1      : *sRV=((XSFUNd_d1f1b1)      syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_d1f1b1d1    : *sRV=((XSFUNd_d1f1b1d1)    syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d1f1d1      : *sRV=((XSFUNd_d1f1d1)      syscallAddress)(parms      [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_d1f1d1b1    : *sRV=((XSFUNd_d1f1d1b1)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d1f1d1f2    : *sRV=((XSFUNd_d1f1d1f2)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f1d1f4    : *sRV=((XSFUNd_d1f1d1f4)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
      case cSyscallType_d1f1d3      : *sRV=((XSFUNd_d1f1d3)      syscallAddress)(parms      [0], parmsF     [1], parms      [2], parms      [3], parms      [4]); break;
      case cSyscallType_d1f2        : *sRV=((XSFUNd_d1f2)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1f2b1      : *sRV=((XSFUNd_d1f2b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d1f2d1      : *sRV=((XSFUNd_d1f2d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d1f2d1f1    : *sRV=((XSFUNd_d1f2d1f1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d1f3        : *sRV=((XSFUNd_d1f3)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d1f3d1      : *sRV=((XSFUNd_d1f3d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d1f4        : *sRV=((XSFUNd_d1f4)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f4b1      : *sRV=((XSFUNd_d1f4b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1f4d2b1    : *sRV=((XSFUNd_d1f4d2b1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d1f5        : *sRV=((XSFUNd_d1f5)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d1f5b1      : *sRV=((XSFUNd_d1f5b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], (bool)parms[6]); break;
      case cSyscallType_d1f7        : *sRV=((XSFUNd_d1f7)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d2b1        : *sRV=((XSFUNd_d2b1)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2]); break;
      case cSyscallType_d2b1d1      : *sRV=((XSFUNd_d2b1d1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d2b1d1b1    : *sRV=((XSFUNd_d2b1d1b1)    syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d2b1f1      : *sRV=((XSFUNd_d2b1f1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parmsF     [3]); break;
      case cSyscallType_d2b2        : *sRV=((XSFUNd_d2b2)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2], (bool)parms[3]); break;
      case cSyscallType_d2f1        : *sRV=((XSFUNd_d2f1)        syscallAddress)(parms      [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_d2f1b1      : *sRV=((XSFUNd_d2f1b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d2f1b2      : *sRV=((XSFUNd_d2f1b2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d2f1d1      : *sRV=((XSFUNd_d2f1d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d2f1d1f1    : *sRV=((XSFUNd_d2f1d1f1)    syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d2f2        : *sRV=((XSFUNd_d2f2)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d2f2b1      : *sRV=((XSFUNd_d2f2b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d2f2d1      : *sRV=((XSFUNd_d2f2d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d2f3        : *sRV=((XSFUNd_d2f3)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d2f3d1      : *sRV=((XSFUNd_d2f3d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d2f3d2      : *sRV=((XSFUNd_d2f3d2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6]); break;
      case cSyscallType_d2f4d1      : *sRV=((XSFUNd_d2f4d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6]); break;
      case cSyscallType_d3b1        : *sRV=((XSFUNd_d3b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d3b1d2      : *sRV=((XSFUNd_d3b1d2)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], parms      [4], parms      [5]); break;
      case cSyscallType_d3b2f3      : *sRV=((XSFUNd_d3b2f3)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d3b3        : *sRV=((XSFUNd_d3b3)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
      case cSyscallType_d3f1        : *sRV=((XSFUNd_d3f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3]); break;
      case cSyscallType_d3f1b1      : *sRV=((XSFUNd_d3f1b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d3f2b1      : *sRV=((XSFUNd_d3f2b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d3f3        : *sRV=((XSFUNd_d3f3)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d3f3d1b1    : *sRV=((XSFUNd_d3f3d1b1)    syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d4b1        : *sRV=((XSFUNd_d4b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d4b1d3      : *sRV=((XSFUNd_d4b1d3)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parms      [7]); break;
      case cSyscallType_d4f1d1      : *sRV=((XSFUNd_d4f1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d4f2        : *sRV=((XSFUNd_d4f2)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d5b1        : *sRV=((XSFUNd_d5b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5]); break;
      case cSyscallType_d5b1d1      : *sRV=((XSFUNd_d5b1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5], parms      [6]); break;
      case cSyscallType_d5f1        : *sRV=((XSFUNd_d5f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parmsF     [5]); break;
      case cSyscallType_f1b1        : *sRV=((XSFUNd_f1b1)        syscallAddress)(parmsF     [0], (bool)parms[1]); break;
      case cSyscallType_f1b1d4      : *sRV=((XSFUNd_f1b1d4)      syscallAddress)(parmsF     [0], (bool)parms[1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
      case cSyscallType_f1d1        : *sRV=((XSFUNd_f1d1)        syscallAddress)(parmsF     [0], parms      [1]); break;
      case cSyscallType_f1d1f1      : *sRV=((XSFUNd_f1d1f1)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_f1d1f2      : *sRV=((XSFUNd_f1d1f2)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_f2b1        : *sRV=((XSFUNd_f2b1)        syscallAddress)(parmsF     [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_f2d1        : *sRV=((XSFUNd_f2d1)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_f2d2        : *sRV=((XSFUNd_f2d2)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2], parms      [3]); break;
      case cSyscallType_f3d1        : *sRV=((XSFUNd_f3d1)        syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_f3d1b1d2f1  : *sRV=((XSFUNd_f3d1b1d2f1)  syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parmsF     [7]); break;
      case cSyscallType_f3d1b2      : *sRV=((XSFUNd_f3d1b2)      syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], (bool)parms[5]); break;
		case cSyscallType_f3d1b2f1		: *sRV=((XSFUNd_f3d1b2f1)	  syscallAddress)(parmsF	  [0], parmsF     [1], parmsF	  [2], parms		[3], (bool)parms[4], (bool)parms[5], parmsF[6]); break;
		case cSyscallType_d2b2f1		: *sRV=((XSFUNd_d2b2f1)		syscallAddress)(parms		[0], parms		 [1], (bool)parms[2], (bool)parms[3], parmsF[4]); break;
		case cSyscallType_d2f4			: *sRV=((XSFUNd_d2f4)			syscallAddress)(parms		[0], parms		 [1], parmsF[2], parmsF[3], parmsF[4], parmsF[5]); break;
		case cSyscallType_d3f4d1		: *sRV=((XSFUNd_d3f4d1)		syscallAddress)(parms		[0], parms		 [1], parms[2], parmsF[3], parmsF[4], parmsF[5], parmsF[6], parms[7]); break;
		case cSyscallType_f5b1		   : *sRV=((XSFUNd_f5b1)		syscallAddress)(parmsF   	[0], parmsF		 [1], parmsF[2], parmsF[3], parmsF[4],(bool)parms[5]); break;
   }

   if (data != NULL)
      data->setTempReturnValue((BYTE*)sRV, sizeof(long));

   return(true);
}
//==============================================================================
// BXSSyscallModule::callFloatSyscall
//==============================================================================
bool BXSSyscallModule::callFloatSyscall(long syscallID, long *parms, long numberParms, float *sRV, BXSData *data)
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(false);
   if (mSyscalls[syscallID] == NULL)
      return(false);
   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||
      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))
      || (sRV == NULL))
      return(false);

   //Do the call.
   long syscallAddress=mSyscalls[syscallID]->getAddress();
   //If we have an invalid address, just return in a default way.  We return true
   //so that the interpreter will keep going as if it all worked.  This allows us
   //to fixup the syscall addresses in a savegame.
   if (syscallAddress == -1)
   {
      *sRV=-1.0f;
      if (data != NULL)
         data->setTempReturnValue((BYTE*)sRV, sizeof(float));
      return(true);
   }

   float* parmsF = reinterpret_cast<float*>(parms);
   long syscallType=mSyscalls[syscallID]->getSyscallType();

   switch(syscallType)
   {
      case cSyscallType_void: *sRV=((XSFUNf)syscallAddress)(); break;

      case cSyscallType_d:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNf_d1) syscallAddress)(parms      [0]); break;
            case 2:  *sRV=((XSFUNf_d2) syscallAddress)(parms      [0], parms      [1]); break;
            case 3:  *sRV=((XSFUNf_d3) syscallAddress)(parms      [0], parms      [1], parms      [2]); break;
            case 4:  *sRV=((XSFUNf_d4) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3]); break;
            case 5:  *sRV=((XSFUNf_d5) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4]); break;
            case 6:  *sRV=((XSFUNf_d6) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
            case 7:  *sRV=((XSFUNf_d7) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
            case 8:  *sRV=((XSFUNf_d8) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7]); break;
            case 9:  *sRV=((XSFUNf_d9) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8]); break;
            case 10: *sRV=((XSFUNf_d10)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9]); break;
            case 11: *sRV=((XSFUNf_d11)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10]); break;
            case 12: *sRV=((XSFUNf_d12)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11]); break;
            case 13: *sRV=((XSFUNf_d13)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12]); break;
            case 14: *sRV=((XSFUNf_d14)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12], parms      [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_f:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNf_f1) syscallAddress)(parmsF     [0]); break;
            case 2:  *sRV=((XSFUNf_f2) syscallAddress)(parmsF     [0], parmsF     [1]); break;
            case 3:  *sRV=((XSFUNf_f3) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2]); break;
            case 4:  *sRV=((XSFUNf_f4) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
            case 5:  *sRV=((XSFUNf_f5) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
            case 6:  *sRV=((XSFUNf_f6) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
            case 7:  *sRV=((XSFUNf_f7) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
            case 8:  *sRV=((XSFUNf_f8) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
            case 9:  *sRV=((XSFUNf_f9) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8]); break;
            case 10: *sRV=((XSFUNf_f10)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9]); break;
            case 11: *sRV=((XSFUNf_f11)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10]); break;
            case 12: *sRV=((XSFUNf_f12)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11]); break;
            case 13: *sRV=((XSFUNf_f13)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12]); break;
            case 14: *sRV=((XSFUNf_f14)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12], parmsF     [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNf_b1) syscallAddress)((bool)parms[0]); break;
            case 2:  *sRV=((XSFUNf_b2) syscallAddress)((bool)parms[0], (bool)parms[1]); break;
            case 3:  *sRV=((XSFUNf_b3) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2]); break;
            case 4:  *sRV=((XSFUNf_b4) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3]); break;
            case 5:  *sRV=((XSFUNf_b5) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
            case 6:  *sRV=((XSFUNf_b6) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
            case 7:  *sRV=((XSFUNf_b7) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6]); break;
            case 8:  *sRV=((XSFUNf_b8) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7]); break;
            case 9:  *sRV=((XSFUNf_b9) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8]); break;
            case 10: *sRV=((XSFUNf_b10)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9]); break;
            case 11: *sRV=((XSFUNf_b11)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10]); break;
            case 12: *sRV=((XSFUNf_b12)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11]); break;
            case 13: *sRV=((XSFUNf_b13)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12]); break;
            case 14: *sRV=((XSFUNf_b14)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12], (bool)parms[13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b1d1        : *sRV=((XSFUNf_b1d1)        syscallAddress)((bool)parms[0], parms      [1]); break;
      case cSyscallType_b1d2        : *sRV=((XSFUNf_b1d2)        syscallAddress)((bool)parms[0], parms      [1], parms      [2]); break;
      case cSyscallType_b1d6        : *sRV=((XSFUNf_b1d6)        syscallAddress)((bool)parms[0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
      case cSyscallType_b1f1        : *sRV=((XSFUNf_b1f1)        syscallAddress)((bool)parms[0], parmsF     [1]); break;
      case cSyscallType_b1f2        : *sRV=((XSFUNf_b1f2)        syscallAddress)((bool)parms[0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1b1        : *sRV=((XSFUNf_d1b1)        syscallAddress)(parms      [0], (bool)parms[1]); break;
      case cSyscallType_d1b1d1      : *sRV=((XSFUNf_d1b1d1)      syscallAddress)(parms      [0], (bool)parms[1], parms      [2]); break;
      case cSyscallType_d1b2        : *sRV=((XSFUNf_d1b2)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2]); break;
      case cSyscallType_d1b2f4b2    : *sRV=((XSFUNf_d1b2f4b2)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], (bool)parms[7], (bool)parms[8]); break;
      case cSyscallType_d1b3f1b1    : *sRV=((XSFUNf_d1b3f1b1)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1b4        : *sRV=((XSFUNf_d1b4)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d1f1        : *sRV=((XSFUNf_d1f1)        syscallAddress)(parms      [0], parmsF     [1]); break;
      case cSyscallType_d1f1b1      : *sRV=((XSFUNf_d1f1b1)      syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_d1f1b1d1    : *sRV=((XSFUNf_d1f1b1d1)    syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d1f1d1      : *sRV=((XSFUNf_d1f1d1)      syscallAddress)(parms      [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_d1f1d1b1    : *sRV=((XSFUNf_d1f1d1b1)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d1f1d1f2    : *sRV=((XSFUNf_d1f1d1f2)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f1d1f4    : *sRV=((XSFUNf_d1f1d1f4)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
      case cSyscallType_d1f1d3      : *sRV=((XSFUNf_d1f1d3)      syscallAddress)(parms      [0], parmsF     [1], parms      [2], parms      [3], parms      [4]); break;
      case cSyscallType_d1f2        : *sRV=((XSFUNf_d1f2)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1f2b1      : *sRV=((XSFUNf_d1f2b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d1f2d1      : *sRV=((XSFUNf_d1f2d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d1f2d1f1    : *sRV=((XSFUNf_d1f2d1f1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d1f3        : *sRV=((XSFUNf_d1f3)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d1f3d1      : *sRV=((XSFUNf_d1f3d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d1f4        : *sRV=((XSFUNf_d1f4)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f4b1      : *sRV=((XSFUNf_d1f4b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1f4d2b1    : *sRV=((XSFUNf_d1f4d2b1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d1f5        : *sRV=((XSFUNf_d1f5)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d1f5b1      : *sRV=((XSFUNf_d1f5b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], (bool)parms[6]); break;
      case cSyscallType_d1f7        : *sRV=((XSFUNf_d1f7)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d2b1        : *sRV=((XSFUNf_d2b1)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2]); break;
      case cSyscallType_d2b1d1      : *sRV=((XSFUNf_d2b1d1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d2b1d1b1    : *sRV=((XSFUNf_d2b1d1b1)    syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d2b1f1      : *sRV=((XSFUNf_d2b1f1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parmsF     [3]); break;
      case cSyscallType_d2b2        : *sRV=((XSFUNf_d2b2)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2], (bool)parms[3]); break;
      case cSyscallType_d2f1        : *sRV=((XSFUNf_d2f1)        syscallAddress)(parms      [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_d2f1b1      : *sRV=((XSFUNf_d2f1b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d2f1b2      : *sRV=((XSFUNf_d2f1b2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d2f1d1      : *sRV=((XSFUNf_d2f1d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d2f1d1f1    : *sRV=((XSFUNf_d2f1d1f1)    syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d2f2        : *sRV=((XSFUNf_d2f2)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d2f2b1      : *sRV=((XSFUNf_d2f2b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d2f2d1      : *sRV=((XSFUNf_d2f2d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d2f3        : *sRV=((XSFUNf_d2f3)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d2f3d1      : *sRV=((XSFUNf_d2f3d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d2f3d2      : *sRV=((XSFUNf_d2f3d2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6]); break;
      case cSyscallType_d2f4d1      : *sRV=((XSFUNf_d2f4d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6]); break;
      case cSyscallType_d3b1        : *sRV=((XSFUNf_d3b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d3b1d2      : *sRV=((XSFUNf_d3b1d2)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], parms      [4], parms      [5]); break;
      case cSyscallType_d3b2f3      : *sRV=((XSFUNf_d3b2f3)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d3b3        : *sRV=((XSFUNf_d3b3)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
      case cSyscallType_d3f1        : *sRV=((XSFUNf_d3f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3]); break;
      case cSyscallType_d3f1b1      : *sRV=((XSFUNf_d3f1b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d3f2b1      : *sRV=((XSFUNf_d3f2b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d3f3        : *sRV=((XSFUNf_d3f3)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d3f3d1b1    : *sRV=((XSFUNf_d3f3d1b1)    syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d4b1        : *sRV=((XSFUNf_d4b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d4b1d3      : *sRV=((XSFUNf_d4b1d3)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parms      [7]); break;
      case cSyscallType_d4f1d1      : *sRV=((XSFUNf_d4f1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d4f2        : *sRV=((XSFUNf_d4f2)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d5b1        : *sRV=((XSFUNf_d5b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5]); break;
      case cSyscallType_d5b1d1      : *sRV=((XSFUNf_d5b1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5], parms      [6]); break;
      case cSyscallType_d5f1        : *sRV=((XSFUNf_d5f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parmsF     [5]); break;
      case cSyscallType_f1b1        : *sRV=((XSFUNf_f1b1)        syscallAddress)(parmsF     [0], (bool)parms[1]); break;
      case cSyscallType_f1b1d4      : *sRV=((XSFUNf_f1b1d4)      syscallAddress)(parmsF     [0], (bool)parms[1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
      case cSyscallType_f1d1        : *sRV=((XSFUNf_f1d1)        syscallAddress)(parmsF     [0], parms      [1]); break;
      case cSyscallType_f1d1f1      : *sRV=((XSFUNf_f1d1f1)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_f1d1f2      : *sRV=((XSFUNf_f1d1f2)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_f2b1        : *sRV=((XSFUNf_f2b1)        syscallAddress)(parmsF     [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_f2d1        : *sRV=((XSFUNf_f2d1)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_f2d2        : *sRV=((XSFUNf_f2d2)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2], parms      [3]); break;
      case cSyscallType_f3d1        : *sRV=((XSFUNf_f3d1)        syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_f3d1b1d2f1  : *sRV=((XSFUNf_f3d1b1d2f1)  syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parmsF     [7]); break;
      case cSyscallType_f3d1b2      : *sRV=((XSFUNf_f3d1b2)      syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], (bool)parms[5]); break;
		case cSyscallType_f3d1b2f1		: *sRV=((XSFUNf_f3d1b2f1)	  syscallAddress)(parmsF		[0], parmsF		 [1], parmsF	  [2], parms		[3], (bool)parms[4], (bool)parms[5], parmsF[6]); break;
		case cSyscallType_d2b2f1		: *sRV=((XSFUNf_d2b2f1)		syscallAddress)(parms		[0], parms		 [1], (bool)parms[2], (bool)parms[3], parmsF[4]); break;
		case cSyscallType_d2f4			: *sRV=((XSFUNf_d2f4)			syscallAddress)(parms		[0], parms		 [1], parmsF[2], parmsF[3], parmsF[4], parmsF[5]); break;
		case cSyscallType_d3f4d1		: *sRV=((XSFUNf_d3f4d1)		syscallAddress)(parms		[0], parms		 [1], parms[2], parmsF[3], parmsF[4], parmsF[5], parmsF[6], parms[7]); break;
		case cSyscallType_f5b1   		: *sRV=((XSFUNf_f5b1)		syscallAddress)(parmsF   	[0], parmsF		 [1], parmsF[2], parmsF[3], parmsF[4],(bool)parms[5]); break;
   }

   if (data != NULL)
      data->setTempReturnValue((BYTE*)sRV, sizeof(float));

   return(true);
}
//==============================================================================
// BXSSyscallModule::callBoolSyscall
//==============================================================================
bool BXSSyscallModule::callBoolSyscall(long syscallID, long *parms, long numberParms, bool *sRV, BXSData *data)
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(false);
   if (mSyscalls[syscallID] == NULL)
      return(false);
   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||
      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))
      || (sRV == NULL))
      return(false);

   //Do the call.
   long syscallAddress=mSyscalls[syscallID]->getAddress();
   //If we have an invalid address, just return in a default way.  We return true
   //so that the interpreter will keep going as if it all worked.  This allows us
   //to fixup the syscall addresses in a savegame.
   if (syscallAddress == -1)
   {
      *sRV=false;
      if (data != NULL)
         data->setTempReturnValue((BYTE*)sRV, sizeof(bool));
      return(true);
   }

   float* parmsF = reinterpret_cast<float*>(parms);
   long syscallType=mSyscalls[syscallID]->getSyscallType();

   switch(syscallType)
   {
      case cSyscallType_void: *sRV=((XSFUNb)syscallAddress)(); break;

      case cSyscallType_d:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNb_d1) syscallAddress)(parms      [0]); break;
            case 2:  *sRV=((XSFUNb_d2) syscallAddress)(parms      [0], parms      [1]); break;
            case 3:  *sRV=((XSFUNb_d3) syscallAddress)(parms      [0], parms      [1], parms      [2]); break;
            case 4:  *sRV=((XSFUNb_d4) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3]); break;
            case 5:  *sRV=((XSFUNb_d5) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4]); break;
            case 6:  *sRV=((XSFUNb_d6) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5]); break;

            case 7:  *sRV=((XSFUNb_d7) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
            case 8:  *sRV=((XSFUNb_d8) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7]); break;
            case 9:  *sRV=((XSFUNb_d9) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8]); break;
            case 10: *sRV=((XSFUNb_d10)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9]); break;
            case 11: *sRV=((XSFUNb_d11)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10]); break;
            case 12: *sRV=((XSFUNb_d12)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11]); break;
            case 13: *sRV=((XSFUNb_d13)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12]); break;
            case 14: *sRV=((XSFUNb_d14)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12], parms      [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_f:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNb_f1) syscallAddress)(parmsF     [0]); break;
            case 2:  *sRV=((XSFUNb_f2) syscallAddress)(parmsF     [0], parmsF     [1]); break;
            case 3:  *sRV=((XSFUNb_f3) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2]); break;
            case 4:  *sRV=((XSFUNb_f4) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
            case 5:  *sRV=((XSFUNb_f5) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
            case 6:  *sRV=((XSFUNb_f6) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
            case 7:  *sRV=((XSFUNb_f7) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
            case 8:  *sRV=((XSFUNb_f8) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
            case 9:  *sRV=((XSFUNb_f9) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8]); break;
            case 10: *sRV=((XSFUNb_f10)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9]); break;
            case 11: *sRV=((XSFUNb_f11)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10]); break;
            case 12: *sRV=((XSFUNb_f12)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11]); break;
            case 13: *sRV=((XSFUNb_f13)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12]); break;
            case 14: *sRV=((XSFUNb_f14)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12], parmsF     [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNb_b1) syscallAddress)((bool)parms[0]); break;
            case 2:  *sRV=((XSFUNb_b2) syscallAddress)((bool)parms[0], (bool)parms[1]); break;
            case 3:  *sRV=((XSFUNb_b3) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2]); break;
            case 4:  *sRV=((XSFUNb_b4) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3]); break;
            case 5:  *sRV=((XSFUNb_b5) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
            case 6:  *sRV=((XSFUNb_b6) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
            case 7:  *sRV=((XSFUNb_b7) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6]); break;
            case 8:  *sRV=((XSFUNb_b8) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7]); break;
            case 9:  *sRV=((XSFUNb_b9) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8]); break;
            case 10: *sRV=((XSFUNb_b10)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9]); break;
            case 11: *sRV=((XSFUNb_b11)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10]); break;
            case 12: *sRV=((XSFUNb_b12)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11]); break;
            case 13: *sRV=((XSFUNb_b13)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12]); break;
            case 14: *sRV=((XSFUNb_b14)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12], (bool)parms[13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b1d1        : *sRV=((XSFUNb_b1d1)        syscallAddress)((bool)parms[0], parms      [1]); break;
      case cSyscallType_b1d2        : *sRV=((XSFUNb_b1d2)        syscallAddress)((bool)parms[0], parms      [1], parms      [2]); break;
      case cSyscallType_b1d6        : *sRV=((XSFUNb_b1d6)        syscallAddress)((bool)parms[0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
      case cSyscallType_b1f1        : *sRV=((XSFUNb_b1f1)        syscallAddress)((bool)parms[0], parmsF     [1]); break;
      case cSyscallType_b1f2        : *sRV=((XSFUNb_b1f2)        syscallAddress)((bool)parms[0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1b1        : *sRV=((XSFUNb_d1b1)        syscallAddress)(parms      [0], (bool)parms[1]); break;
      case cSyscallType_d1b1d1      : *sRV=((XSFUNb_d1b1d1)      syscallAddress)(parms      [0], (bool)parms[1], parms      [2]); break;
      case cSyscallType_d1b2        : *sRV=((XSFUNb_d1b2)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2]); break;
      case cSyscallType_d1b2f4b2    : *sRV=((XSFUNb_d1b2f4b2)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], (bool)parms[7], (bool)parms[8]); break;
      case cSyscallType_d1b3f1b1    : *sRV=((XSFUNb_d1b3f1b1)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1b4        : *sRV=((XSFUNb_d1b4)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d1f1        : *sRV=((XSFUNb_d1f1)        syscallAddress)(parms      [0], parmsF     [1]); break;
      case cSyscallType_d1f1b1      : *sRV=((XSFUNb_d1f1b1)      syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_d1f1b1d1    : *sRV=((XSFUNb_d1f1b1d1)    syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d1f1d1      : *sRV=((XSFUNb_d1f1d1)      syscallAddress)(parms      [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_d1f1d1b1    : *sRV=((XSFUNb_d1f1d1b1)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d1f1d1f2    : *sRV=((XSFUNb_d1f1d1f2)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f1d1f4    : *sRV=((XSFUNb_d1f1d1f4)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
      case cSyscallType_d1f1d3      : *sRV=((XSFUNb_d1f1d3)      syscallAddress)(parms      [0], parmsF     [1], parms      [2], parms      [3], parms      [4]); break;
      case cSyscallType_d1f2        : *sRV=((XSFUNb_d1f2)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1f2b1      : *sRV=((XSFUNb_d1f2b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d1f2d1      : *sRV=((XSFUNb_d1f2d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d1f2d1f1    : *sRV=((XSFUNb_d1f2d1f1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d1f3        : *sRV=((XSFUNb_d1f3)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d1f3d1      : *sRV=((XSFUNb_d1f3d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d1f4        : *sRV=((XSFUNb_d1f4)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f4b1      : *sRV=((XSFUNb_d1f4b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1f4d2b1    : *sRV=((XSFUNb_d1f4d2b1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d1f5        : *sRV=((XSFUNb_d1f5)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d1f5b1      : *sRV=((XSFUNb_d1f5b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], (bool)parms[6]); break;
      case cSyscallType_d1f7        : *sRV=((XSFUNb_d1f7)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d2b1        : *sRV=((XSFUNb_d2b1)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2]); break;
      case cSyscallType_d2b1d1      : *sRV=((XSFUNb_d2b1d1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d2b1d1b1    : *sRV=((XSFUNb_d2b1d1b1)    syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d2b1f1      : *sRV=((XSFUNb_d2b1f1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parmsF     [3]); break;
      case cSyscallType_d2b2        : *sRV=((XSFUNb_d2b2)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2], (bool)parms[3]); break;
      case cSyscallType_d2f1        : *sRV=((XSFUNb_d2f1)        syscallAddress)(parms      [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_d2f1b1      : *sRV=((XSFUNb_d2f1b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d2f1b2      : *sRV=((XSFUNb_d2f1b2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d2f1d1      : *sRV=((XSFUNb_d2f1d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d2f1d1f1    : *sRV=((XSFUNb_d2f1d1f1)    syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d2f2        : *sRV=((XSFUNb_d2f2)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d2f2b1      : *sRV=((XSFUNb_d2f2b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d2f2d1      : *sRV=((XSFUNb_d2f2d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d2f3        : *sRV=((XSFUNb_d2f3)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d2f3d1      : *sRV=((XSFUNb_d2f3d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d2f3d2      : *sRV=((XSFUNb_d2f3d2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6]); break;
      case cSyscallType_d2f4d1      : *sRV=((XSFUNb_d2f4d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6]); break;
      case cSyscallType_d3b1        : *sRV=((XSFUNb_d3b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d3b1d2      : *sRV=((XSFUNb_d3b1d2)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], parms      [4], parms      [5]); break;
      case cSyscallType_d3b2f3      : *sRV=((XSFUNb_d3b2f3)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d3b3        : *sRV=((XSFUNb_d3b3)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
      case cSyscallType_d3f1        : *sRV=((XSFUNb_d3f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3]); break;
      case cSyscallType_d3f1b1      : *sRV=((XSFUNb_d3f1b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d3f2b1      : *sRV=((XSFUNb_d3f2b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d3f3        : *sRV=((XSFUNb_d3f3)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d3f3d1b1    : *sRV=((XSFUNb_d3f3d1b1)    syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d4b1        : *sRV=((XSFUNb_d4b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d4b1d3      : *sRV=((XSFUNb_d4b1d3)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parms      [7]); break;
      case cSyscallType_d4f1d1      : *sRV=((XSFUNb_d4f1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d4f2        : *sRV=((XSFUNb_d4f2)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d5b1        : *sRV=((XSFUNb_d5b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5]); break;
      case cSyscallType_d5b1d1      : *sRV=((XSFUNb_d5b1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5], parms      [6]); break;
      case cSyscallType_d5f1        : *sRV=((XSFUNb_d5f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parmsF     [5]); break;
      case cSyscallType_f1b1        : *sRV=((XSFUNb_f1b1)        syscallAddress)(parmsF     [0], (bool)parms[1]); break;
      case cSyscallType_f1b1d4      : *sRV=((XSFUNb_f1b1d4)      syscallAddress)(parmsF     [0], (bool)parms[1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
      case cSyscallType_f1d1        : *sRV=((XSFUNb_f1d1)        syscallAddress)(parmsF     [0], parms      [1]); break;
      case cSyscallType_f1d1f1      : *sRV=((XSFUNb_f1d1f1)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_f1d1f2      : *sRV=((XSFUNb_f1d1f2)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_f2b1        : *sRV=((XSFUNb_f2b1)        syscallAddress)(parmsF     [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_f2d1        : *sRV=((XSFUNb_f2d1)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_f2d2        : *sRV=((XSFUNb_f2d2)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2], parms      [3]); break;
      case cSyscallType_f3d1        : *sRV=((XSFUNb_f3d1)        syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_f3d1b1d2f1  : *sRV=((XSFUNb_f3d1b1d2f1)  syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parmsF     [7]); break;
      case cSyscallType_f3d1b2      : *sRV=((XSFUNb_f3d1b2)      syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], (bool)parms[5]); break;
		case cSyscallType_f3d1b2f1		: *sRV=((XSFUNb_f3d1b2f1)		syscallAddress)(parmsF		[0], parmsF		 [1], parmsF	  [2], parms		[3], (bool)parms[4], (bool)parms[5], parmsF[6]); break;
		case cSyscallType_d2b2f1		: *sRV=((XSFUNb_d2b2f1)		syscallAddress)(parms		[0], parms		 [1], (bool)parms[2], (bool)parms[3], parmsF[4]); break;
		case cSyscallType_d2f4			: *sRV=((XSFUNb_d2f4)			syscallAddress)(parms		[0], parms		 [1], parmsF[2], parmsF[3], parmsF[4], parmsF[5]); break;
		case cSyscallType_d3f4d1		: *sRV=((XSFUNb_d3f4d1)		syscallAddress)(parms		[0], parms		 [1], parms[2], parmsF[3], parmsF[4], parmsF[5], parmsF[6], parms[7]); break;
		case cSyscallType_f5b1   		: *sRV=((XSFUNb_f5b1)		syscallAddress)(parmsF   	[0], parmsF		 [1], parmsF[2], parmsF[3], parmsF[4],(bool)parms[5]); break;
   }

   if (data != NULL)
      data->setTempReturnValue((BYTE*)sRV, sizeof(bool));

   return(true);
}
//==============================================================================
// BXSSyscallModule::callStringSyscall
//==============================================================================
bool BXSSyscallModule::callStringSyscall(long syscallID, long *parms, long numberParms, BXSData *data)
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(false);
   if (mSyscalls[syscallID] == NULL)
      return(false);
   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||
      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))
      )
      return(false);

   //Do the call.
   long syscallAddress=mSyscalls[syscallID]->getAddress();
   //If we have an invalid address, just return in a default way.  We return true
   //so that the interpreter will keep going as if it all worked.  This allows us

   //to fixup the syscall addresses in a savegame.
   static const char invalidString[]="INVALID";
   if (syscallAddress == -1)
   {
      if (data!=NULL)
         data->setTempReturnValue((BYTE*)invalidString, strlen(invalidString));
      return(true);
   }

   float* parmsF = reinterpret_cast<float*>(parms);
   long syscallType=mSyscalls[syscallID]->getSyscallType();
   char *rV=NULL;

   switch(syscallType)
   {
      case cSyscallType_void: ((XSFUNs)syscallAddress)(); break;

      case cSyscallType_d:
         switch (numberParms)
         {
            case 1:  rV=((XSFUNs_d1) syscallAddress)(parms      [0]); break;
            case 2:  rV=((XSFUNs_d2) syscallAddress)(parms      [0], parms      [1]); break;
            case 3:  rV=((XSFUNs_d3) syscallAddress)(parms      [0], parms      [1], parms      [2]); break;
            case 4:  rV=((XSFUNs_d4) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3]); break;
            case 5:  rV=((XSFUNs_d5) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4]); break;
            case 6:  rV=((XSFUNs_d6) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
            case 7:  rV=((XSFUNs_d7) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
            case 8:  rV=((XSFUNs_d8) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7]); break;
            case 9:  rV=((XSFUNs_d9) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8]); break;
            case 10: rV=((XSFUNs_d10)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9]); break;
            case 11: rV=((XSFUNs_d11)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10]); break;
            case 12: rV=((XSFUNs_d12)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11]); break;
            case 13: rV=((XSFUNs_d13)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12]); break;
            case 14: rV=((XSFUNs_d14)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12], parms      [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_f:
         switch (numberParms)
         {
            case 1:  rV=((XSFUNs_f1) syscallAddress)(parmsF     [0]); break;
            case 2:  rV=((XSFUNs_f2) syscallAddress)(parmsF     [0], parmsF     [1]); break;
            case 3:  rV=((XSFUNs_f3) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2]); break;
            case 4:  rV=((XSFUNs_f4) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
            case 5:  rV=((XSFUNs_f5) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
            case 6:  rV=((XSFUNs_f6) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
            case 7:  rV=((XSFUNs_f7) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
            case 8:  rV=((XSFUNs_f8) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
            case 9:  rV=((XSFUNs_f9) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8]); break;
            case 10: rV=((XSFUNs_f10)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9]); break;
            case 11: rV=((XSFUNs_f11)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10]); break;
            case 12: rV=((XSFUNs_f12)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11]); break;
            case 13: rV=((XSFUNs_f13)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12]); break;
            case 14: rV=((XSFUNs_f14)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12], parmsF     [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b:
         switch (numberParms)
         {
            case 1:  rV=((XSFUNs_b1) syscallAddress)((bool)parms[0]); break;
            case 2:  rV=((XSFUNs_b2) syscallAddress)((bool)parms[0], (bool)parms[1]); break;
            case 3:  rV=((XSFUNs_b3) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2]); break;
            case 4:  rV=((XSFUNs_b4) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3]); break;
            case 5:  rV=((XSFUNs_b5) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
            case 6:  rV=((XSFUNs_b6) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
            case 7:  rV=((XSFUNs_b7) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6]); break;
            case 8:  rV=((XSFUNs_b8) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7]); break;
            case 9:  rV=((XSFUNs_b9) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8]); break;
            case 10: rV=((XSFUNs_b10)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9]); break;
            case 11: rV=((XSFUNs_b11)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10]); break;
            case 12: rV=((XSFUNs_b12)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11]); break;
            case 13: rV=((XSFUNs_b13)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12]); break;
            case 14: rV=((XSFUNs_b14)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12], (bool)parms[13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b1d1        : rV=((XSFUNs_b1d1)        syscallAddress)((bool)parms[0], parms      [1]); break;
      case cSyscallType_b1d2        : rV=((XSFUNs_b1d2)        syscallAddress)((bool)parms[0], parms      [1], parms      [2]); break;
      case cSyscallType_b1d6        : rV=((XSFUNs_b1d6)        syscallAddress)((bool)parms[0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
      case cSyscallType_b1f1        : rV=((XSFUNs_b1f1)        syscallAddress)((bool)parms[0], parmsF     [1]); break;
      case cSyscallType_b1f2        : rV=((XSFUNs_b1f2)        syscallAddress)((bool)parms[0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1b1        : rV=((XSFUNs_d1b1)        syscallAddress)(parms      [0], (bool)parms[1]); break;
      case cSyscallType_d1b1d1      : rV=((XSFUNs_d1b1d1)      syscallAddress)(parms      [0], (bool)parms[1], parms      [2]); break;
      case cSyscallType_d1b2        : rV=((XSFUNs_d1b2)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2]); break;
      case cSyscallType_d1b2f4b2    : rV=((XSFUNs_d1b2f4b2)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], (bool)parms[7], (bool)parms[8]); break;
      case cSyscallType_d1b3f1b1    : rV=((XSFUNs_d1b3f1b1)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1b4        : rV=((XSFUNs_d1b4)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d1f1        : rV=((XSFUNs_d1f1)        syscallAddress)(parms      [0], parmsF     [1]); break;
      case cSyscallType_d1f1b1      : rV=((XSFUNs_d1f1b1)      syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_d1f1b1d1    : rV=((XSFUNs_d1f1b1d1)    syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d1f1d1      : rV=((XSFUNs_d1f1d1)      syscallAddress)(parms      [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_d1f1d1b1    : rV=((XSFUNs_d1f1d1b1)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d1f1d1f2    : rV=((XSFUNs_d1f1d1f2)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f1d1f4    : rV=((XSFUNs_d1f1d1f4)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
      case cSyscallType_d1f1d3      : rV=((XSFUNs_d1f1d3)      syscallAddress)(parms      [0], parmsF     [1], parms      [2], parms      [3], parms      [4]); break;
      case cSyscallType_d1f2        : rV=((XSFUNs_d1f2)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1f2b1      : rV=((XSFUNs_d1f2b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d1f2d1      : rV=((XSFUNs_d1f2d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d1f2d1f1    : rV=((XSFUNs_d1f2d1f1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d1f3        : rV=((XSFUNs_d1f3)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d1f3d1      : rV=((XSFUNs_d1f3d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d1f4        : rV=((XSFUNs_d1f4)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f4b1      : rV=((XSFUNs_d1f4b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1f4d2b1    : rV=((XSFUNs_d1f4d2b1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d1f5        : rV=((XSFUNs_d1f5)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d1f5b1      : rV=((XSFUNs_d1f5b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], (bool)parms[6]); break;
      case cSyscallType_d1f7        : rV=((XSFUNs_d1f7)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d2b1        : rV=((XSFUNs_d2b1)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2]); break;
      case cSyscallType_d2b1d1      : rV=((XSFUNs_d2b1d1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d2b1d1b1    : rV=((XSFUNs_d2b1d1b1)    syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d2b1f1      : rV=((XSFUNs_d2b1f1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parmsF     [3]); break;
      case cSyscallType_d2b2        : rV=((XSFUNs_d2b2)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2], (bool)parms[3]); break;
      case cSyscallType_d2f1        : rV=((XSFUNs_d2f1)        syscallAddress)(parms      [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_d2f1b1      : rV=((XSFUNs_d2f1b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d2f1b2      : rV=((XSFUNs_d2f1b2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d2f1d1      : rV=((XSFUNs_d2f1d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d2f1d1f1    : rV=((XSFUNs_d2f1d1f1)    syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d2f2        : rV=((XSFUNs_d2f2)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d2f2b1      : rV=((XSFUNs_d2f2b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d2f2d1      : rV=((XSFUNs_d2f2d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d2f3        : rV=((XSFUNs_d2f3)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d2f3d1      : rV=((XSFUNs_d2f3d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d2f3d2      : rV=((XSFUNs_d2f3d2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6]); break;
      case cSyscallType_d2f4d1      : rV=((XSFUNs_d2f4d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6]); break;
      case cSyscallType_d3b1        : rV=((XSFUNs_d3b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d3b1d2      : rV=((XSFUNs_d3b1d2)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], parms      [4], parms      [5]); break;
      case cSyscallType_d3b2f3      : rV=((XSFUNs_d3b2f3)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d3b3        : rV=((XSFUNs_d3b3)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
      case cSyscallType_d3f1        : rV=((XSFUNs_d3f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3]); break;
      case cSyscallType_d3f1b1      : rV=((XSFUNs_d3f1b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d3f2b1      : rV=((XSFUNs_d3f2b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d3f3        : rV=((XSFUNs_d3f3)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d3f3d1b1    : rV=((XSFUNs_d3f3d1b1)    syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d4b1        : rV=((XSFUNs_d4b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d4b1d3      : rV=((XSFUNs_d4b1d3)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parms      [7]); break;
      case cSyscallType_d4f1d1      : rV=((XSFUNs_d4f1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d4f2        : rV=((XSFUNs_d4f2)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d5b1        : rV=((XSFUNs_d5b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5]); break;
      case cSyscallType_d5b1d1      : rV=((XSFUNs_d5b1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5], parms      [6]); break;
      case cSyscallType_d5f1        : rV=((XSFUNs_d5f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parmsF     [5]); break;
      case cSyscallType_f1b1        : rV=((XSFUNs_f1b1)        syscallAddress)(parmsF     [0], (bool)parms[1]); break;
      case cSyscallType_f1b1d4      : rV=((XSFUNs_f1b1d4)      syscallAddress)(parmsF     [0], (bool)parms[1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
      case cSyscallType_f1d1        : rV=((XSFUNs_f1d1)        syscallAddress)(parmsF     [0], parms      [1]); break;
      case cSyscallType_f1d1f1      : rV=((XSFUNs_f1d1f1)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_f1d1f2      : rV=((XSFUNs_f1d1f2)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_f2b1        : rV=((XSFUNs_f2b1)        syscallAddress)(parmsF     [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_f2d1        : rV=((XSFUNs_f2d1)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_f2d2        : rV=((XSFUNs_f2d2)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2], parms      [3]); break;
      case cSyscallType_f3d1        : rV=((XSFUNs_f3d1)        syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_f3d1b1d2f1  : rV=((XSFUNs_f3d1b1d2f1)  syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parmsF     [7]); break;
      case cSyscallType_f3d1b2      : rV=((XSFUNs_f3d1b2)      syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], (bool)parms[5]); break;
		case cSyscallType_f3d1b2f1		: rV=((XSFUNs_f3d1b2f1)		syscallAddress)(parmsF		[0], parmsF		 [1], parmsF	  [2], parms		[3], (bool)parms[4], (bool)parms[5], parmsF[6]); break;
		case cSyscallType_d2b2f1		: rV=((XSFUNs_d2b2f1)		syscallAddress)(parms		[0], parms		 [1], (bool)parms[2], (bool)parms[3], parmsF[4]); break;
		case cSyscallType_d2f4			: rV=((XSFUNs_d2f4)			syscallAddress)(parms		[0], parms		 [1], parmsF[2], parmsF[3], parmsF[4], parmsF[5]); break;
		case cSyscallType_d3f4d1		: rV=((XSFUNs_d3f4d1)		syscallAddress)(parms		[0], parms		 [1], parms[2], parmsF[3], parmsF[4], parmsF[5], parmsF[6], parms[7]); break;
		case cSyscallType_f5b1  		: rV=((XSFUNs_f5b1)  		syscallAddress)(parmsF   	[0], parmsF		 [1], parmsF[2], parmsF[3], parmsF[4],(bool)parms[5]); break;
   }

   if (rV == NULL)
      return(false);

   if (data != NULL)
   {
      long stringLength=strlen(rV)+1;
      long newValueIndex=data->allocateUserHeapValue(BXSVariableEntry::cStringVariable, stringLength);
      if (newValueIndex < 0)
         return(false);
      if (data->setUserHeapValue(newValueIndex, BXSVariableEntry::cStringVariable, stringLength, (BYTE*)rV) == false)
         return(false);
      if (data->incrementUserHeapRefCount(newValueIndex) == false)
         return(false);
      //Set the temp return value.
      data->setTempReturnValue((BYTE*)&newValueIndex, sizeof(long));
   }

   return(true);
}
//==============================================================================
// BXSSyscallModule::callVectorSyscall
//==============================================================================
bool BXSSyscallModule::callVectorSyscall(long syscallID, long *parms, long numberParms, BVector *sRV, BXSData *data)
{
   //Bomb checks.
   if ((syscallID < 0) || (syscallID >= mSyscalls.getNumber()) )
      return(false);
   if (mSyscalls[syscallID] == NULL)
      return(false);
   if ( ((mSyscalls[syscallID]->getContext() == false) && (mSyscalls[syscallID]->getNumberParameters() != numberParms)) ||
      ((mSyscalls[syscallID]->getContext() == true) && (mSyscalls[syscallID]->getNumberParameters() != numberParms-1))
      || (sRV == NULL))
      return(false);

   //Do the call.
   long syscallAddress=mSyscalls[syscallID]->getAddress();
   //If we have an invalid address, just return in a default way.  We return true
   //so that the interpreter will keep going as if it all worked.  This allows us
   //to fixup the syscall addresses in a savegame.
   if (syscallAddress == -1)
   {
      *sRV=cInvalidVector;
      if (data != NULL)
         data->setTempReturnValue((BYTE*)sRV, sizeof(BVector));
      return(true);
   }

   float* parmsF = reinterpret_cast<float*>(parms);
   long syscallType=mSyscalls[syscallID]->getSyscallType();

   switch(syscallType)
   {
      case cSyscallType_void: *sRV=((XSFUNv)syscallAddress)(); break;

      case cSyscallType_d:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNv_d1) syscallAddress)(parms      [0]); break;
            case 2:  *sRV=((XSFUNv_d2) syscallAddress)(parms      [0], parms      [1]); break;
            case 3:  *sRV=((XSFUNv_d3) syscallAddress)(parms      [0], parms      [1], parms      [2]); break;
            case 4:  *sRV=((XSFUNv_d4) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3]); break;
            case 5:  *sRV=((XSFUNv_d5) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4]); break;
            case 6:  *sRV=((XSFUNv_d6) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
            case 7:  *sRV=((XSFUNv_d7) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
            case 8:  *sRV=((XSFUNv_d8) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7]); break;
            case 9:  *sRV=((XSFUNv_d9) syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8]); break;
            case 10: *sRV=((XSFUNv_d10)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9]); break;
            case 11: *sRV=((XSFUNv_d11)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10]); break;
            case 12: *sRV=((XSFUNv_d12)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11]); break;
            case 13: *sRV=((XSFUNv_d13)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12]); break;
            case 14: *sRV=((XSFUNv_d14)syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6], parms      [7], parms      [8], parms      [9], parms      [10], parms      [11], parms      [12], parms      [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_f:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNv_f1) syscallAddress)(parmsF     [0]); break;
            case 2:  *sRV=((XSFUNv_f2) syscallAddress)(parmsF     [0], parmsF     [1]); break;
            case 3:  *sRV=((XSFUNv_f3) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2]); break;
            case 4:  *sRV=((XSFUNv_f4) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
            case 5:  *sRV=((XSFUNv_f5) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
            case 6:  *sRV=((XSFUNv_f6) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
            case 7:  *sRV=((XSFUNv_f7) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
            case 8:  *sRV=((XSFUNv_f8) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
            case 9:  *sRV=((XSFUNv_f9) syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8]); break;
            case 10: *sRV=((XSFUNv_f10)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9]); break;
            case 11: *sRV=((XSFUNv_f11)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10]); break;
            case 12: *sRV=((XSFUNv_f12)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11]); break;
            case 13: *sRV=((XSFUNv_f13)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12]); break;
            case 14: *sRV=((XSFUNv_f14)syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7], parmsF     [8], parmsF     [9], parmsF     [10], parmsF     [11], parmsF     [12], parmsF     [13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b:
         switch (numberParms)
         {
            case 1:  *sRV=((XSFUNv_b1) syscallAddress)((bool)parms[0]); break;
            case 2:  *sRV=((XSFUNv_b2) syscallAddress)((bool)parms[0], (bool)parms[1]); break;
            case 3:  *sRV=((XSFUNv_b3) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2]); break;
            case 4:  *sRV=((XSFUNv_b4) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3]); break;
            case 5:  *sRV=((XSFUNv_b5) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
            case 6:  *sRV=((XSFUNv_b6) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
            case 7:  *sRV=((XSFUNv_b7) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6]); break;
            case 8:  *sRV=((XSFUNv_b8) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7]); break;
            case 9:  *sRV=((XSFUNv_b9) syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8]); break;
            case 10: *sRV=((XSFUNv_b10)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9]); break;
            case 11: *sRV=((XSFUNv_b11)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10]); break;
            case 12: *sRV=((XSFUNv_b12)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11]); break;
            case 13: *sRV=((XSFUNv_b13)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12]); break;
            case 14: *sRV=((XSFUNv_b14)syscallAddress)((bool)parms[0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4], (bool)parms[5], (bool)parms[6], (bool)parms[7], (bool)parms[8], (bool)parms[9], (bool)parms[10], (bool)parms[11], (bool)parms[12], (bool)parms[13]); break;
            default: return(false);
         }
         break;

      case cSyscallType_b1d1        : *sRV=((XSFUNv_b1d1)        syscallAddress)((bool)parms[0], parms      [1]); break;
      case cSyscallType_b1d2        : *sRV=((XSFUNv_b1d2)        syscallAddress)((bool)parms[0], parms      [1], parms      [2]); break;
      case cSyscallType_b1d6        : *sRV=((XSFUNv_b1d6)        syscallAddress)((bool)parms[0], parms      [1], parms      [2], parms      [3], parms      [4], parms      [5], parms      [6]); break;
      case cSyscallType_b1f1        : *sRV=((XSFUNv_b1f1)        syscallAddress)((bool)parms[0], parmsF     [1]); break;
      case cSyscallType_b1f2        : *sRV=((XSFUNv_b1f2)        syscallAddress)((bool)parms[0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1b1        : *sRV=((XSFUNv_d1b1)        syscallAddress)(parms      [0], (bool)parms[1]); break;
      case cSyscallType_d1b1d1      : *sRV=((XSFUNv_d1b1d1)      syscallAddress)(parms      [0], (bool)parms[1], parms      [2]); break;
      case cSyscallType_d1b2        : *sRV=((XSFUNv_d1b2)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2]); break;
      case cSyscallType_d1b2f4b2    : *sRV=((XSFUNv_d1b2f4b2)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], (bool)parms[7], (bool)parms[8]); break;
      case cSyscallType_d1b3f1b1    : *sRV=((XSFUNv_d1b3f1b1)    syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1b4        : *sRV=((XSFUNv_d1b4)        syscallAddress)(parms      [0], (bool)parms[1], (bool)parms[2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d1f1        : *sRV=((XSFUNv_d1f1)        syscallAddress)(parms      [0], parmsF     [1]); break;
      case cSyscallType_d1f1b1      : *sRV=((XSFUNv_d1f1b1)      syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_d1f1b1d1    : *sRV=((XSFUNv_d1f1b1d1)    syscallAddress)(parms      [0], parmsF     [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d1f1d1      : *sRV=((XSFUNv_d1f1d1)      syscallAddress)(parms      [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_d1f1d1b1    : *sRV=((XSFUNv_d1f1d1b1)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d1f1d1f2    : *sRV=((XSFUNv_d1f1d1f2)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f1d1f4    : *sRV=((XSFUNv_d1f1d1f4)    syscallAddress)(parms      [0], parmsF     [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6]); break;
      case cSyscallType_d1f1d3      : *sRV=((XSFUNv_d1f1d3)      syscallAddress)(parms      [0], parmsF     [1], parms      [2], parms      [3], parms      [4]); break;
      case cSyscallType_d1f2        : *sRV=((XSFUNv_d1f2)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2]); break;
      case cSyscallType_d1f2b1      : *sRV=((XSFUNv_d1f2b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d1f2d1      : *sRV=((XSFUNv_d1f2d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d1f2d1f1    : *sRV=((XSFUNv_d1f2d1f1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d1f3        : *sRV=((XSFUNv_d1f3)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d1f3d1      : *sRV=((XSFUNv_d1f3d1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d1f4        : *sRV=((XSFUNv_d1f4)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d1f4b1      : *sRV=((XSFUNv_d1f4b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d1f4d2b1    : *sRV=((XSFUNv_d1f4d2b1)    syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d1f5        : *sRV=((XSFUNv_d1f5)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d1f5b1      : *sRV=((XSFUNv_d1f5b1)      syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], (bool)parms[6]); break;
      case cSyscallType_d1f7        : *sRV=((XSFUNv_d1f7)        syscallAddress)(parms      [0], parmsF     [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d2b1        : *sRV=((XSFUNv_d2b1)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2]); break;
      case cSyscallType_d2b1d1      : *sRV=((XSFUNv_d2b1d1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3]); break;
      case cSyscallType_d2b1d1b1    : *sRV=((XSFUNv_d2b1d1b1)    syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d2b1f1      : *sRV=((XSFUNv_d2b1f1)      syscallAddress)(parms      [0], parms      [1], (bool)parms[2], parmsF     [3]); break;
      case cSyscallType_d2b2        : *sRV=((XSFUNv_d2b2)        syscallAddress)(parms      [0], parms      [1], (bool)parms[2], (bool)parms[3]); break;
      case cSyscallType_d2f1        : *sRV=((XSFUNv_d2f1)        syscallAddress)(parms      [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_d2f1b1      : *sRV=((XSFUNv_d2f1b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3]); break;
      case cSyscallType_d2f1b2      : *sRV=((XSFUNv_d2f1b2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], (bool)parms[3], (bool)parms[4]); break;
      case cSyscallType_d2f1d1      : *sRV=((XSFUNv_d2f1d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_d2f1d1f1    : *sRV=((XSFUNv_d2f1d1f1)    syscallAddress)(parms      [0], parms      [1], parmsF     [2], parms      [3], parmsF     [4]); break;
      case cSyscallType_d2f2        : *sRV=((XSFUNv_d2f2)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_d2f2b1      : *sRV=((XSFUNv_d2f2b1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d2f2d1      : *sRV=((XSFUNv_d2f2d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parms      [4]); break;
      case cSyscallType_d2f3        : *sRV=((XSFUNv_d2f3)        syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4]); break;
      case cSyscallType_d2f3d1      : *sRV=((XSFUNv_d2f3d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d2f3d2      : *sRV=((XSFUNv_d2f3d2)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parms      [5], parms      [6]); break;
      case cSyscallType_d2f4d1      : *sRV=((XSFUNv_d2f4d1)      syscallAddress)(parms      [0], parms      [1], parmsF     [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6]); break;
      case cSyscallType_d3b1        : *sRV=((XSFUNv_d3b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3]); break;
      case cSyscallType_d3b1d2      : *sRV=((XSFUNv_d3b1d2)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], parms      [4], parms      [5]); break;
      case cSyscallType_d3b2f3      : *sRV=((XSFUNv_d3b2f3)      syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], parmsF     [5], parmsF     [6], parmsF     [7]); break;
      case cSyscallType_d3b3        : *sRV=((XSFUNv_d3b3)        syscallAddress)(parms      [0], parms      [1], parms      [2], (bool)parms[3], (bool)parms[4], (bool)parms[5]); break;
      case cSyscallType_d3f1        : *sRV=((XSFUNv_d3f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3]); break;
      case cSyscallType_d3f1b1      : *sRV=((XSFUNv_d3f1b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], (bool)parms[4]); break;
      case cSyscallType_d3f2b1      : *sRV=((XSFUNv_d3f2b1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], (bool)parms[5]); break;
      case cSyscallType_d3f3        : *sRV=((XSFUNv_d3f3)        syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d3f3d1b1    : *sRV=((XSFUNv_d3f3d1b1)    syscallAddress)(parms      [0], parms      [1], parms      [2], parmsF     [3], parmsF     [4], parmsF     [5], parms      [6], (bool)parms[7]); break;
      case cSyscallType_d4b1        : *sRV=((XSFUNv_d4b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4]); break;
      case cSyscallType_d4b1d3      : *sRV=((XSFUNv_d4b1d3)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parms      [7]); break;
      case cSyscallType_d4f1d1      : *sRV=((XSFUNv_d4f1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parms      [5]); break;
      case cSyscallType_d4f2        : *sRV=((XSFUNv_d4f2)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parmsF     [4], parmsF     [5]); break;
      case cSyscallType_d5b1        : *sRV=((XSFUNv_d5b1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5]); break;
      case cSyscallType_d5b1d1      : *sRV=((XSFUNv_d5b1d1)      syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], (bool)parms[5], parms      [6]); break;
      case cSyscallType_d5f1        : *sRV=((XSFUNv_d5f1)        syscallAddress)(parms      [0], parms      [1], parms      [2], parms      [3], parms      [4], parmsF     [5]); break;
      case cSyscallType_f1b1        : *sRV=((XSFUNv_f1b1)        syscallAddress)(parmsF     [0], (bool)parms[1]); break;
      case cSyscallType_f1b1d4      : *sRV=((XSFUNv_f1b1d4)      syscallAddress)(parmsF     [0], (bool)parms[1], parms      [2], parms      [3], parms      [4], parms      [5]); break;
      case cSyscallType_f1d1        : *sRV=((XSFUNv_f1d1)        syscallAddress)(parmsF     [0], parms      [1]); break;
      case cSyscallType_f1d1f1      : *sRV=((XSFUNv_f1d1f1)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2]); break;
      case cSyscallType_f1d1f2      : *sRV=((XSFUNv_f1d1f2)      syscallAddress)(parmsF     [0], parms      [1], parmsF     [2], parmsF     [3]); break;
      case cSyscallType_f2b1        : *sRV=((XSFUNv_f2b1)        syscallAddress)(parmsF     [0], parmsF     [1], (bool)parms[2]); break;
      case cSyscallType_f2d1        : *sRV=((XSFUNv_f2d1)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2]); break;
      case cSyscallType_f2d2        : *sRV=((XSFUNv_f2d2)        syscallAddress)(parmsF     [0], parmsF     [1], parms      [2], parms      [3]); break;
      case cSyscallType_f3d1        : *sRV=((XSFUNv_f3d1)        syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3]); break;
      case cSyscallType_f3d1b1d2f1  : *sRV=((XSFUNv_f3d1b1d2f1)  syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], parms      [5], parms      [6], parmsF     [7]); break;
      case cSyscallType_f3d1b2      : *sRV=((XSFUNv_f3d1b2)      syscallAddress)(parmsF     [0], parmsF     [1], parmsF     [2], parms      [3], (bool)parms[4], (bool)parms[5]); break;
		case cSyscallType_f3d1b2f1		: *sRV=((XSFUNv_f3d1b2f1)		syscallAddress)(parmsF		[0], parmsF		 [1], parmsF	  [2], parms		[3], (bool)parms[4], (bool)parms[5], parmsF[6]); break;
		case cSyscallType_d2b2f1		: *sRV=((XSFUNv_d2b2f1)		syscallAddress)(parms		[0], parms		 [1], (bool)parms[2], (bool)parms[3], parmsF[4]); break;
		case cSyscallType_d2f4			: *sRV=((XSFUNv_d2f4)			syscallAddress)(parms		[0], parms		 [1], parmsF[2], parmsF[3], parmsF[4], parmsF[5]); break;
		case cSyscallType_d3f4d1		: *sRV=((XSFUNv_d3f4d1)		syscallAddress)(parms		[0], parms		 [1], parms[2], parmsF[3], parmsF[4], parmsF[5], parmsF[6], parms[7]); break;
		case cSyscallType_f5b1  		: *sRV=((XSFUNv_f5b1)		syscallAddress)(parmsF   	[0], parmsF		 [1], parmsF[2], parmsF[3], parmsF[4],(bool)parms[5]); break;
   }

   if (data != NULL)
      data->setTempReturnValue((BYTE*)sRV, sizeof(BVector));

   return(true);
}

#pragma warning(default:4800)
