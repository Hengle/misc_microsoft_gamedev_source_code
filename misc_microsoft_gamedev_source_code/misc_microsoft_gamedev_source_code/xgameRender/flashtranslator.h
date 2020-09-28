//============================================================================
// flashtranslator.h
// Copyright 2008 (c) Ensemble Studios
//============================================================================

#pragma once
#include "scaleformIncludes.h"

//============================================================================
//============================================================================
class BFlashTranslator : public GFxTranslator
{
   public: 
      BFlashTranslator(uint wwMode);
      virtual ~BFlashTranslator();

      virtual UInt GetCaps() const         { return Cap_CustomWordWrapping; }
      virtual bool OnWordWrapping(LineFormatDesc* pdesc);      
   private:
      bool mgsWordWrap(LineFormatDesc* pDesc);
};