//==============================================================================
// converttoken.cpp
//
// Copyright (c) 1999 - 2003 Ensemble Studios
//==============================================================================
#include "xcore.h"
#include "math\vector.h"
#include "convertToken.h"

// Tokens we use for parsing multi-part items.
const BCHAR_T separators[] = B(" ,\t\r\n");

#ifndef ARGBToDWORD
   #define ARGBToDWORD(a, r, g, b) DWORD((a<<24)|(r<<16)|(g<<8)|(b))
#endif

//==============================================================================
// convertTokenToFloat(char *token, float &num)
//==============================================================================
bool convertTokenToFloat(const char *token, float &num)
{
   num = static_cast<float>(atof(token));
   return true;
}

//==============================================================================
// convertTokenToInt(char *token, long &num)
//==============================================================================
bool convertTokenToInt(const char *token, long &num)
{
   num = atoi(token);
   return true;   
}


//==============================================================================
// convertTokenToInt
//==============================================================================
bool convertTokenToInt(const char *token, int &num)
{
   num = atoi(token);
   return true;  
}


//==============================================================================
// convertTokenToDWORD
//==============================================================================
bool convertTokenToDWORD(const char *token, DWORD &num)
{
   num = atoi(token);
   return true;  
}

//==============================================================================
// convertTokenToBYTE
//==============================================================================
bool convertTokenToBYTE(const char *token, BYTE &num)
{
   int val = atoi(token);
   if ((val < 0) || (val > 255))
      return false;

   num = static_cast<BYTE>(val);      
      
   return true;  
}


//=============================================================================
// convertTokenToBOOL
//=============================================================================
bool convertTokenToBOOL(const char *token, BOOL &b)
{
   if(_stricmp(token, "true") == 0 || _stricmp(token, "1") == 0)
   {
      b=TRUE;
      return(true);
   }
   if(_stricmp(token, "false") == 0|| _stricmp(token, "0") == 0)
   {
      b=FALSE;
      return(true);
   }

   return(false);
}


//=============================================================================
// convertTokenToDWORDColor
//=============================================================================
bool convertTokenToDWORDColor(const BString &token, DWORD &color, BYTE defaultAlpha)
{
   BString temp;

   // Get param1.
   BYTE param1=0;
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToBYTE(temp.getPtr(), param1);
      if(!ok)
         return(false);
   }

   // Get param2.
   BYTE param2=0;
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToBYTE(temp.getPtr(), param2);
   if(!ok)
      return(false);

   // Get param3.
   BYTE param3=0;
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToBYTE(temp.getPtr(), param3);
   if(!ok)
      return(false);

   // Get param4 -- which is optional and so it's not a failure if it's missing.
   BYTE param4=0;
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos>=0)
   {
      convertTokenToBYTE(temp.getPtr(), param4);

      // If there was an alpha, params are ARGB.
      color = ARGBToDWORD(param1, param2, param3, param4);
   }
   else
   {
      // No alpha means RGB
      color = ARGBToDWORD(defaultAlpha, param1, param2, param3);
   }

   // Success.
   return(true);
}

//=============================================================================
// convertTokenToLongXY
//=============================================================================
bool convertTokenToLongXY(const BString &token, long &x, long &y)
{
   BString temp;

   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToInt(temp.getPtr(), x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToInt(temp.getPtr(), y);
   if(!ok)
      return(false);

   // Success.
   return(true);
}

//=============================================================================
// convertTokenToVector
//=============================================================================
bool convertTokenToVector(const BString &token, BVector& v)
{
   BString temp;

   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToFloat(temp.getPtr(), v.x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToFloat(temp.getPtr(), v.y);
   if(!ok)
      return(false);

   // Get param3.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), v.z);
   if(!ok)
      return(false);

   // Success.
   return(true);
}


//=============================================================================
// convertTokenToVector4
//=============================================================================
bool convertTokenToVector4(const BString &token, BVector4& v)
{
   BString temp;

   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToFloat(temp.getPtr(), v.x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToFloat(temp.getPtr(), v.y);
   if(!ok)
      return(false);

   // Get param3.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), v.z);
   if(!ok)
      return(false);

   // Get param4.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), v.w);
   if(!ok)
      return(false);

   // Success.
   return(true);
}


//=============================================================================
// convertTokenToBool
//=============================================================================
bool convertTokenToBool(const BCHAR_T *token, bool &b)
{
   if(bcsicmp(token, B("true")) == 0 || bcsicmp(token, B("1")) == 0)
   {
      b=true;
      return(true);
   }
   if(bcsicmp(token, B("false")) == 0|| bcsicmp(token, B("0")) == 0)
   {
      b=false;
      return(true);
   }

   return(false);
}

#ifdef XBOX
//=============================================================================
// convertTokenToVector4
//=============================================================================
bool convertTokenToXMHALF4(const BString &token, XMHALF4& v)
{
   BString temp;

   float x = 0.0f;
   float y = 0.0f;
   float z = 0.0f;
   float w = 0.0f;
   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToFloat(temp.getPtr(), x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToFloat(temp.getPtr(), y);
   if(!ok)
      return(false);

   // Get param3.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), z);
   if(!ok)
      return(false);

   // Get param4.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), w);
   if(!ok)
      return(false);

   v = XMHALF4(x,y,z,w);

   // Success.
   return(true);
}
#endif

//==============================================================================
// convertSimTokenToFloat
//==============================================================================
bool convertSimTokenToFloat(const BSimString &token, float& v)
{
   v = token.asFloat();
   return true;
}

//=============================================================================
// convertSimTokenToVector
//=============================================================================
bool convertSimTokenToVector(const BSimString &token, BVector& v)
{
   BSimString temp;

   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToFloat(temp.getPtr(), v.x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToFloat(temp.getPtr(), v.y);
   if(!ok)
      return(false);

   // Get param3.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), v.z);
   if(!ok)
      return(false);

   // Success.
   return(true);
}


//=============================================================================
// convertSimTokenToVector2
//=============================================================================
bool convertSimTokenToVector2(const BSimString &token, BVector2& v)
{
   BSimString temp;

   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToFloat(temp.getPtr(), v.x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToFloat(temp.getPtr(), v.y);
   if(!ok)
      return(false);

   // Success.
   return(true);
}


//=============================================================================
// convertSimTokenToVector4
//=============================================================================
bool convertSimTokenToVector4(const BSimString &token, BVector4& v)
{
   BSimString temp;

   // Get param1.
   long nextPos=temp.copyTok(token, -1, -1, separators);
   if(nextPos>=0)
   {
      bool ok=convertTokenToFloat(temp.getPtr(), v.x);
      if(!ok)
         return(false);
   }

   // Get param2.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   bool ok=convertTokenToFloat(temp.getPtr(), v.y);
   if(!ok)
      return(false);

   // Get param3.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), v.z);
   if(!ok)
      return(false);

   // Get param4.
   nextPos=temp.copyTok(token, -1, nextPos, separators);
   if(nextPos<0)
      return(false);
   ok=convertTokenToFloat(temp.getPtr(), v.w);
   if(!ok)
      return(false);

   // Success.
   return(true);
}
