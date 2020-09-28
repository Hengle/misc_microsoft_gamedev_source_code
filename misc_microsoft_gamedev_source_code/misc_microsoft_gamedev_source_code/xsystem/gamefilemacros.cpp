//==============================================================================
// gamefilemacros.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "xsystem.h"

void gameFileError(const char *text, ...)
{
   const int cMaxTextLength = 200;
   char msg[cMaxTextLength];

   int length;
   va_list ap;
   va_start(ap, text);
   length = bvsnprintf(msg, cMaxTextLength, text, ap);
   va_end(ap);

   if (length > 0 && length < cMaxTextLength)
   {
      blogerrortrace(msg);
      BASSERTM(0, msg);
   }
   else
      BASSERT(0);
}
