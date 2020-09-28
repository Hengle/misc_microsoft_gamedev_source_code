//==============================================================================
// textvisualeffect.h
//
// Copyright (c) 2005-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// Includes
#include "bezier.h"
#include "xmlreader.h"


//==============================================================================
// Forward declarations
class BTextVisual;


//==============================================================================
// Defines.



//==============================================================================
class BTextVisualEffect
{
   public:
                              BTextVisualEffect();
      virtual                 ~BTextVisualEffect();
                              
      virtual void            update(DWORD elapsed, BTextVisual *visual, float &param) = 0;
      virtual bool            load(BXMLNode root) = 0;

      bool                    getManagedByDef(void) const {return(mManagedByDef);}
      void                    setManagedByDef(bool managed) {mManagedByDef = managed;}

   protected:
      bool                    mManagedByDef;
};


// jce [11/10/2005] -- TODO: these bezier lines are overkill in several cases as they are 3D 

//==============================================================================
class BTextVisualEffectPosition : public BTextVisualEffect
{
   public:
                              BTextVisualEffectPosition();
      virtual                 ~BTextVisualEffectPosition();

      void                    init(long *xOffsets, long *yOffsets, DWORD *times, long count);
                                    
      virtual void            update(DWORD elapsed, BTextVisual *visual, float &param);
      virtual bool            load(BXMLNode root);

   protected:
      BBezierLine             mCurve;
      BDynamicSimArray<float> mTimeScales;
};


//==============================================================================
class BTextVisualEffectColor : public BTextVisualEffect
{
   public:
                              BTextVisualEffectColor();
      virtual                 ~BTextVisualEffectColor();

      void                    init(BColor *colors, DWORD *times, long count);
                                    
      virtual void            update(DWORD elapsed, BTextVisual *visual, float &param);
      virtual bool            load(BXMLNode root);

   protected:
      BBezierLine             mCurve;
      BDynamicSimArray<float> mTimeScales;
};


//==============================================================================
class BTextVisualEffectAlpha : public BTextVisualEffect
{
   public:
                              BTextVisualEffectAlpha();
      virtual                 ~BTextVisualEffectAlpha();

      void                    init(float *alphas, DWORD *times, long count);
                                    
      virtual void            update(DWORD elapsed, BTextVisual *visual, float &param);
      virtual bool            load(BXMLNode root);

   protected:
      BBezierLine             mCurve;
      BDynamicSimArray<float> mTimeScales;
};

//==============================================================================
class BTextVisualEffectIcon : public BTextVisualEffect
{
   public:
      enum
      {
         cIconSpacing = 7
      };

                               BTextVisualEffectIcon();
      virtual                 ~BTextVisualEffectIcon();

      void                    init();

      virtual void            update(DWORD elapsed, BTextVisual *visual, float &param);
      virtual bool            load(BXMLNode root);

   protected:
      BSimString                 mIconPath;
      long                       mIconWidth;
      long                       mIconHeight;
};