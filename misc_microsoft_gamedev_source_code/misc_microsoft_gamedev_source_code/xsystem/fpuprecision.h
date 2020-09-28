//============================================================================
//
//  fpuprecision.h
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once

// TODO: Rename this to BScopedFPUPrecision
class BFPUPrecision
{
public:
   BFPUPrecision()
   {
      _controlfp_s(&mOrigControlWord, 0, 0);
   }

   BFPUPrecision(bool useHighPrecison)
   {
      _controlfp_s(&mOrigControlWord, 0, 0);
      uint currentControlWord;
      _controlfp_s(&currentControlWord, useHighPrecison ? _PC_64 : _PC_24, MCW_PC);
   }

   ~BFPUPrecision()
   {
      uint currentControlWord;
      _controlfp_s(&currentControlWord, mOrigControlWord, 0xfffff);
   }

   static void setHighPrecision(void)
   {
      uint currentControlWord;
      _controlfp_s(&currentControlWord, _PC_64, MCW_PC);
   }

   static void setLowPrecision(void)
   {
      uint currentControlWord;
      _controlfp_s(&currentControlWord, _PC_24, MCW_PC);
   }

   static bool isHighPrecision(void) 
   {
      uint currentControlWord;
      _controlfp_s(&currentControlWord, 0, 0);
      return (currentControlWord & MCW_PC)  == _PC_64;
   }

   static bool isLowPrecision(void) 
   {
      uint currentControlWord;
      _controlfp_s(&currentControlWord, 0, 0);
      return (currentControlWord & MCW_PC)  == _PC_24;
   }

private:
   uint mOrigControlWord;   
};

