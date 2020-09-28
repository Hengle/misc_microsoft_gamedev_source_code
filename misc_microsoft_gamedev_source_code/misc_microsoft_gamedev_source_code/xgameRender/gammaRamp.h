// File: gammaRamp.h
#pragma once

#include "renderThread.h"

class BGammaRamp : public BRenderCommandListener
#ifdef ENABLE_RELOAD_MANAGER
   , public BEventReceiver
#endif
{
public:
   BGammaRamp();
   ~BGammaRamp();
     
   void init(void);
   void deinit(void);
   
   bool load(long dirID, const char* pFilename);
   bool save(long dirID, const char* pFilename);
   bool reload(void);
   
   // This updates the video scaler's gamma ramps. Gamma and contrast control the shape of the curve.
   // The game outputs sRGB, this curve can be used to compensate the game's analog output for crappy displays.
   void set(float gamma, float contrast);
   
   float getGamma(void) const { return mGamma; }
   float getContrast(void) const { return mContrast; }
   
   long getDirID(void) const { return mDirID; }
   const BSimString& getFilename(void) const { return mFilename; }

private:
   float mGamma;
   
   // Note contrast is actually brightness, see:
   // http://www.normankoren.com/makingfineprints1A.html#Calibration
   // Yes this is dumb but that's the way it is.
   // Luminance = Contrast * (value ^ Gamma) + Brightness
   float mContrast;
   
   long mDirID;
   BSimString mFilename;
#ifdef ENABLE_RELOAD_MANAGER
   virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);   
#endif
   virtual void initDeviceData(void);
   virtual void frameBegin(void);

   virtual void processCommand(const BRenderCommandHeader& header, const uchar* pData);
   virtual void frameEnd(void);
   virtual void deinitDeviceData(void);
};

extern BGammaRamp gGammaRamp;
