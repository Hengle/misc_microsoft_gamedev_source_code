//==============================================================================
// textvisualeffect.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================
#include "common.h"
#include "textvisualeffect.h"
#include "textvisual.h"
#include "xmlreader.h"
#include "string\convertToken.h"


//==============================================================================
// BTextVisualEffect::BTextVisualEffect
//==============================================================================
BTextVisualEffect::BTextVisualEffect() :
   mManagedByDef(false)
{
}


//==============================================================================
// BTextVisual::~BTextVisual
//==============================================================================
BTextVisualEffect::~BTextVisualEffect()
{
}



//==============================================================================
// BTextVisualEffectPosition::BTextVisualEffectPosition
//==============================================================================
BTextVisualEffectPosition::BTextVisualEffectPosition()
{
}


//==============================================================================
// BTextVisualEffectPosition::~BTextVisualEffectPosition
//==============================================================================
BTextVisualEffectPosition::~BTextVisualEffectPosition()
{
}


//==============================================================================
// BTextVisualEffectPosition::load
//==============================================================================
bool BTextVisualEffectPosition::load(BXMLNode root)
{
   // Sanity.
   if(!root)
      return(false);

   // Temps.
   BDynamicSimArray<DWORD> times;
   BDynamicSimArray<long> xOffsets;
   BDynamicSimArray<long> yOffsets;
         
   // Run through children.
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child= root.getChild(i);
      if(!child)
         continue;
         
      // Check type.
      if(child.getName().compare("point") == 0)
      {
         // Get time.
         DWORD time = 0;
         bool ok = child.getAttribValueAsDWORD("time", time);
         if(!ok)
         {
            child.logInfo("invalid or missing time.");
            return(false);
         }
         
         // Get info.
         long x = 0;
         long y = 0;
         BSimString tempStr;
         ok = convertTokenToLongXY(child.getTextPtr(tempStr), x, y);
         if(!ok)
         {
            child.logInfo("invalid coords.");
            return(false);
         }
         
         // Add.
         times.add(time);
         xOffsets.add(x);
         yOffsets.add(y);
      }
   }
   
   // Init with the info from the file.
   init(xOffsets.getPtr(), yOffsets.getPtr(), times.getPtr(), times.getNumber());
   
   // Success.
   return(true);
}


//==============================================================================
// BTextVisualEffectPosition::init
//==============================================================================
void BTextVisualEffectPosition::init(long *xOffsets, long *yOffsets, DWORD *times, long count)
{
   // Sanity.
   if(count<=0 || !xOffsets || !yOffsets || !times)
      return;
   
   // Copy into vectors.
   BDynamicSimVectorArray pts;
   pts.setNumber(count);
   for(long i=0; i<count; i++)
   {
      pts[i].x = float(xOffsets[i]);
      pts[i].y = float(yOffsets[i]);
      pts[i].z = 0.0f;
   }
   
   // Init the curve.
   mCurve.init(pts.getPtr(), count);

   mTimeScales.setNumber(count-1);
   for(long i=0; i<count-1; i++)
   {
      // Delta from previous point to this one.
      DWORD delta = times[i+1] - times[i];
      
      // Save it.
      mTimeScales[i] = 1.0f/delta;
   }
}


//==============================================================================
// BTextVisualEffectPosition::update
//==============================================================================
void BTextVisualEffectPosition::update(DWORD elapsed, BTextVisual *visual, float &param)
{
   DWORD maxIndex = mTimeScales.getNumber()-1;
   while(elapsed)
   {
      DWORD index = DWORD(floor(param));
      if(index > maxIndex)
         index = maxIndex;
      float maxIncrement = floor(param+1.0f) - param;
      float increment = elapsed*mTimeScales[index];
      if(increment > maxIncrement)
      {
         increment = maxIncrement;
         elapsed -= DWORD(increment * 1.0f/mTimeScales[index]);
      }
      else
         elapsed = 0;
      param += increment;
   }
   
   BVector pt;
   mCurve.evaluate(param, pt);
   
   visual->setOffsetX(long(pt.x));
   visual->setOffsetY(long(pt.y));
}



//==============================================================================
// BTextVisualEffectColor::BTextVisualEffectColor
//==============================================================================
BTextVisualEffectColor::BTextVisualEffectColor()
{
}


//==============================================================================
// BTextVisualEffectColor::~BTextVisualEffectColor
//==============================================================================
BTextVisualEffectColor::~BTextVisualEffectColor()
{
}


//==============================================================================
// BTextVisualEffectColor::load
//==============================================================================
bool BTextVisualEffectColor::load(BXMLNode root)
{
   // Sanity.
   if(!root)
      return(false);

   // Temps.
   BDynamicSimArray<DWORD> times;
   BDynamicSimArray<BColor> colors;
         
   // Run through children.
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
      if(!child)
         continue;
         
      // Check type.
      if(child.getName().compare("point") == 0)
      {
         // Get time.
         DWORD time = 0;
         bool ok = child.getAttribValueAsDWORD("time", time);
         if(!ok)
         {
            child.logInfo("invalid or missing time.");
            return(false);
         }
         
         // Get info.
         DWORD color = 0;
         BSimString tempStr;
         ok = convertTokenToDWORDColor(child.getTextPtr(tempStr), color);
         if(!ok)
         {
            child.logInfo("invalid color.");
            return(false);
         }
         
         // Add.
         times.add(time);
         colors.add(BColor(color));
      }
   }
   
   // Init with the info from the file.
   init(colors.getPtr(), times.getPtr(), times.getNumber());
   
   // Success.
   return(true);
}


//==============================================================================
// BTextVisualEffectColor::init
//==============================================================================
void BTextVisualEffectColor::init(BColor *colors, DWORD *times, long count)
{
   // Sanity.
   if(count<=0 || !colors || !times)
      return;
   
   // Copy into vectors.
   BDynamicSimVectorArray pts;
   pts.setNumber(count);
   for(long i=0; i<count; i++)
   {
      pts[i].x = colors[i].r;
      pts[i].y = colors[i].g;
      pts[i].z = colors[i].b;
   }
   
   // Init the curve.
   mCurve.init(pts.getPtr(), count);

   mTimeScales.setNumber(count-1);
   for(long i=0; i<count-1; i++)
   {
      // Delta from previous point to this one.
      DWORD delta = times[i+1] - times[i];
      
      // Save it.
      mTimeScales[i] = 1.0f/delta;
   }
}


//==============================================================================
// BTextVisualEffectColor::update
//==============================================================================
void BTextVisualEffectColor::update(DWORD elapsed, BTextVisual *visual, float &param)
{
   DWORD maxIndex = mTimeScales.getNumber()-1;
   while(elapsed)
   {
      DWORD index = DWORD(floor(param));
      if(index > maxIndex)
         index = maxIndex;
      float maxIncrement = floor(param+1.0f) - param;
      float increment = elapsed*mTimeScales[index];
      if(increment > maxIncrement)
      {
         increment = maxIncrement;
         elapsed -= DWORD(increment * 1.0f/mTimeScales[index]);
      }
      else
         elapsed = 0;
      param += increment;
   }

   BVector pt;
   mCurve.evaluate(param, pt);

   visual->setColor(BColor(pt.x, pt.y, pt.z));   
}


//==============================================================================
// BTextVisualEffectAlpha::BTextVisualEffectAlpha
//==============================================================================
BTextVisualEffectAlpha::BTextVisualEffectAlpha()
{
}


//==============================================================================
// BTextVisualEffectAlpha::~BTextVisualEffectAlpha
//==============================================================================
BTextVisualEffectAlpha::~BTextVisualEffectAlpha()
{
}


//==============================================================================
// BTextVisualEffectAlpha::load
//==============================================================================
bool BTextVisualEffectAlpha::load(BXMLNode root)
{
   // Sanity.
   if(!root)
      return(false);

   // Temps.
   BDynamicSimArray<DWORD> times;
   BDynamicSimArray<float> alphas;
         
   // Run through children.
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(root.getChild(i));
      if(!child)
         continue;
         
      // Check type.
      if(child.getName().compare("point") == 0)
      {
         // Get time.
         DWORD time = 0;
         bool ok = child.getAttribValueAsDWORD("time", time);
         if(!ok)
         {
            child.logInfo("invalid or missing time.");
            return(false);
         }
         
         // Get info.
         DWORD alpha = 0;
         BSimString tempStr;
         ok = convertTokenToDWORD(child.getTextPtr(tempStr), alpha);
         if(!ok)
         {
            child.logInfo("invalid alpha.");
            return(false);
         }
         
         // Clamp alpha         
         if(alpha>255)
            alpha = 255;
         
         // Add.
         times.add(time);
         alphas.add(cOneOver255*alpha);
      }
   }
   
   // Init with the info from the file.
   init(alphas.getPtr(), times.getPtr(), times.getNumber());
   
   // Success.
   return(true);
}


//==============================================================================
// BTextVisualEffectAlpha::init
//==============================================================================
void BTextVisualEffectAlpha::init(float *alphas, DWORD *times, long count)
{
   // Sanity.
   if(count<=0 || !alphas || !times)
      return;
   
   // Copy into vectors.
   BDynamicSimVectorArray pts;   
   pts.setNumber(count);
   for(long i=0; i<count; i++)
   {
      pts[i].x = alphas[i];
      pts[i].y = 0.0f;
      pts[i].z = 0.0f;
   }
   
   // Init the curve.
   mCurve.init(pts.getPtr(), count);

   mTimeScales.setNumber(count-1);
   for(long i=0; i<count-1; i++)
   {
      // Delta from previous point to this one.
      DWORD delta = times[i+1] - times[i];
      
      // Save it.
      mTimeScales[i] = 1.0f/delta;
   }
}


//==============================================================================
// BTextVisualEffectAlpha::update
//==============================================================================
void BTextVisualEffectAlpha::update(DWORD elapsed, BTextVisual *visual, float &param)
{
   DWORD maxIndex = mTimeScales.getNumber()-1;
   while(elapsed)
   {
      DWORD index = DWORD(floor(param));
      if(index > maxIndex)
         index = maxIndex;
      float maxIncrement = floor(param+1.0f) - param;
      float increment = elapsed*mTimeScales[index];
      if(increment > maxIncrement)
      {
         increment = maxIncrement;
         elapsed -= DWORD(increment * 1.0f/mTimeScales[index]);
      }
      else
         elapsed = 0;
      param += increment;
   }

   BVector pt;
   mCurve.evaluate(param, pt);

   visual->setAlpha(pt.x);
}

//==============================================================================
// BTextVisualEffectIcon::BTextVisualEffectIcon
//==============================================================================
BTextVisualEffectIcon::BTextVisualEffectIcon() : mIconWidth(-1), mIconHeight(-1)
{
}


//==============================================================================
// BTextVisualEffectIcon::~BTextVisualEffectIcon
//==============================================================================
BTextVisualEffectIcon::~BTextVisualEffectIcon()
{      
}

//==============================================================================
// BTextVisualEffectIcon::load
//==============================================================================
bool BTextVisualEffectIcon::load(BXMLNode root)
{
   // Sanity.
   if(!root)
      return(false);

   BString iconPath;

   long numChildren = root.getNumberChildren();
   for(long i = 0; i < numChildren; i++)
   {
      BXMLNode node = root.getChild(i);
      
      if(node.getName().compare("IconPath") == 0)
      {
         node.getText(mIconPath);
      }
      else if(node.getName().compare("IconSize") == 0)
      {
         BSimString tempStr;   
         sscanf_s(node.getTextPtr(tempStr), "%d %d", &mIconWidth, &mIconHeight);
      }
   }

   // Success.
   return(true);
}


//==============================================================================
// BTextVisualEffectIcon::init
//==============================================================================
void BTextVisualEffectIcon::init(void)
{
}

//==============================================================================
// BTextVisualEffectIcon::update
//==============================================================================
void BTextVisualEffectIcon::update(DWORD elapsed, BTextVisual *visual, float &param)
{
   param; elapsed;

   if(!visual)
      return;
   
   //Tell the visual the icon handle so it can render it.   
   if(visual->getIcon() == NULL)
   {      
      BUIElement* pIcon = visual->createIcon();
      pIcon->setTexture(mIconPath, true);      
      pIcon->setSize(mIconWidth, mIconHeight);
   }
}

//==============================================================================
// eof: textvisual.cpp
//==============================================================================