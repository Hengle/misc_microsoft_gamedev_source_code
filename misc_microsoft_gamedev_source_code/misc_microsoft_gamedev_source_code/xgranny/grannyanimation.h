//============================================================================
// grannyanimation.h
//  
// Copyright (c) 2003-2006 Ensemble Studios
//============================================================================

#ifndef _GRANNYANIMATION_H_
#define _GRANNYANIMATION_H_

#include <granny.h>
#include "render.h"

//============================================================================
// BGrannyAnimation
//============================================================================
class BGrannyAnimation
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:

      enum BMotionExtractionMode
      {
         cNoExtraction        = GrannyNoAccumulation, 
         cConstantExtraction  = GrannyConstantExtractionAccumulation,
         cVariableExtraction  = GrannyVariableDeltaAccumulation
      };

                              BGrannyAnimation();
                              ~BGrannyAnimation();

      bool                    init(const BSimString& basePath, const BCHAR_T* pFileName, bool synced=false);
      void                    deinit();

      bool                    load(bool synced=false);
      bool                    reload(void);

      bool                    isLoaded() const   { return (mpGrannyFileInfo != NULL); }
      bool                    loadFailed() const { return mLoadFailed; }

      const BSimString&       getFilename() const { return mFilename; }
      void                    setFilename(const BCHAR_T* pFileName) { mFilename=pFileName; }

      granny_file_info*       getGrannyFileInfo() const { return mpGrannyFileInfo; }

      float                   getDuration() const;

      // Has original model skeleton that can be used for skeleton mapping
      bool                    hasModelData() const { return mHasModelData; }
      
      // Gets and sets the method to use when extracting the motion.  By default, this
      // is set to no extraction.  Constant extraction will extract the constant portion
      // of the motion from an animation.  This requires the animation to be exported
      // with the "extract accumulation" setting.  Variable motion extraction extracts all
      // root bone motion, and the anim should not be exported with "extract accumulation" enabled.
      BMotionExtractionMode   getMotionExtractionMode() const { return mMotionExtractionMode; }
      void                    setMotionExtractionMode(BMotionExtractionMode mode) { mMotionExtractionMode = mode; }

      BVector                 getTotalMotionExtraction() const { return mTotalMotionExtraction; }
      void                    setTotalMotionExtraction(BVector totalMotionExtraction) { mTotalMotionExtraction = totalMotionExtraction; }
      
      uint                    getAllocationSize() const { return mAllocationSize; }

   protected:
      
      BVector                 mTotalMotionExtraction;
      granny_file_info*       mpGrannyFileInfo;
      BMotionExtractionMode   mMotionExtractionMode;
      BSimString              mFilename;
      BSimString              mBasePath;
      
      uint                    mAllocationSize;

      bool                    mHasModelData : 1;
      bool                    mLoadFailed : 1;
#ifdef ENABLE_RELOAD_MANAGER
      virtual bool            receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
};

#endif
