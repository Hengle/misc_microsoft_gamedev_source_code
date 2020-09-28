//============================================================================
// flashscene.h
// Ensemble Studios (c) 2006
//============================================================================

#pragma once 

#include "renderThread.h"
#include "threading\eventDispatcher.h"
#include "inputsystem.h"
#include "bitvector.h"
#include "D3DTextureManager.h"

// forward dec
class BFlashMovieInstance;

//============================================================================
//============================================================================
class BFlashPropertyVariant
{
   public:
      BFlashPropertyVariant() : mTextureHandle(cInvalidManagedTextureHandle) {};
      ~BFlashPropertyVariant() {} 

      void BFlashPropertyVariant::deInit()
      {
         if ( (mType==eTypeTextureHandle) && gD3DTextureManager.isValidManagedTextureHandle(mTextureHandle) )
         {
            gD3DTextureManager.releaseManagedTextureByHandle(mTextureHandle);
            mTextureHandle = cInvalidManagedTextureHandle;
         }
      };

      enum BFlashPropertyType
      {
         eTypeInvalid,
         eTypeFloat,
         eTypeInt,
         eTypeXMColor,
         eTypeBool,
         eTypeHalf4,
         eTypeHalf2,
         eTypeStringID,
         eTypeTextureHandle,
         eTypeTotal,
      };

      bool validateType(BFlashPropertyType type) const { return type == mType; }

      BFixedString64 mName;
      union
      {
         uint16      mHalf4[4];
         uint16      mHalf2[2];
         BManagedTextureHandle mTextureHandle;
         float       mFloat;
         int         mInt;
         int         mStringID;
         DWORD       mXMColor;
         bool        mBool;                  
      };
      int            mType;
};

//============================================================================
//============================================================================
class BFlashPropertyString
{
public:
    BFlashPropertyString() {};
   ~BFlashPropertyString() {};

   BFixedString64  mName;
   BFixedString128 mString;
};

//============================================================================
//============================================================================
typedef uint32 BFlashPropertyHandle;
class BFlashProperties
{
   public:      
      enum 
      {
         eInvalidFlashPropertyHandle = 0,
      };

      BFlashProperties() : mWidth(0), mHeight(0), mSafeAreaHeight(0), mSafeAreaWidth(0) {};
     ~BFlashProperties();
     
      bool load(BXMLNode node, BXMLReader* pReader);

      BFlashPropertyHandle findHandle(const char* name) const;      
      BFlashPropertyHandle findStringHandle(const char* name) const;
      BFlashPropertyHandle findFlashFileHandle(const char* name) const;

      const char* getName(BFlashPropertyHandle handle) const;
      bool  getHalf4(BFlashPropertyHandle handle, XMHALF4& value) const;
      bool  getHalf2(BFlashPropertyHandle handle, XMHALF2& value) const;
      bool  getXMColor(BFlashPropertyHandle handle, XMCOLOR& value) const;      
      bool  getFloat(BFlashPropertyHandle handle, float& value) const;
      bool  getInt(BFlashPropertyHandle handle, int& value) const;
      bool  getBool(BFlashPropertyHandle handle, bool& value) const;
      bool  getString(BFlashPropertyHandle handle, BFixedString128& value) const;
      bool  getLocStringID(BFlashPropertyHandle handle, int& stringID) const;
      bool  getFlashFile(BFlashPropertyHandle handle, BFixedString128& value) const;
      bool  getTextureHandle(BFlashPropertyHandle handle, BManagedTextureHandle& value) const;
      
      bool  getFlashFileByIndex(int index, BFixedString128& value) const { return getFlashFile(index+1, value); }
      int getFlashFileCount() const { return mFlashFiles.size(); };

      int getType(const char* name)const;
      int getType(BFlashPropertyHandle handle) const;

      long mWidth;
      long mHeight;
      long mSafeAreaWidth;
      long mSafeAreaHeight;
      long mX;
      long mY;      

   private:

      const BFlashPropertyVariant* getPropertyByHandle(BFlashPropertyHandle handle) const
      {
         if (handle == eInvalidFlashPropertyHandle)
            return NULL;
         
         int index = handle - 1;
         debugRangeCheck(index, mData.getNumber());
         return mData[index];
      };      

      bool getPropertyStringByHandle(BFlashPropertyHandle handle, BFixedString128& value) const
      {
         if (handle == eInvalidFlashPropertyHandle)
            return false;

         int index = handle - 1;
         debugRangeCheck(index, mDataStrings.getNumber());
         value = mDataStrings[index]->mString;
         return true;
      };      

      bool getFlashFileByHandle(BFlashPropertyHandle handle, BFixedString128& value) const
      {
         if (handle == eInvalidFlashPropertyHandle)
            return false;

         int index = handle - 1;
         debugRangeCheck(index, mFlashFiles.getNumber());
         value = mFlashFiles[index]->mString;
         return true;
      }
      
      BDynamicSimArray<BFlashPropertyVariant*> mData;
      BDynamicSimArray<BFlashPropertyString*>  mDataStrings;
      BDynamicSimArray<BFlashPropertyString*>  mFlashFiles;
            
};

//============================================================================
//============================================================================
class BFlashProtoControl
{
   public:
      BFlashProtoControl(){};
     ~BFlashProtoControl() {};
      bool load(BXMLNode node, BXMLReader* pReader);
      
      BFixedString256  mName;
      BFixedString256  mPath;
      long        mID;
};


//============================================================================
//============================================================================
class BFlashResolutionProtoControl
{
   public:
      BFlashResolutionProtoControl() : mX(-1), mY(-1), mOffsetX(0), mOffsetY(0), mWidth(0), mHeight(0), mScaleX(100), mScaleY(100) {};
     ~BFlashResolutionProtoControl() {};
      bool load(BXMLNode node, BXMLReader* pReader);
      
      BFixedString256  mName;
      BFixedString256  mPath;
      long             mID;
      float            mX;
      float            mY;
      long             mOffsetX;
      long             mOffsetY;
      long             mWidth;
      long             mHeight;
      float            mScaleX;
      float            mScaleY;      
};

//============================================================================
//============================================================================
class BFlashResolutionSettings
{
   public:
      BFlashResolutionSettings() : mX(-1), mY(-1), mWidth(-1), mHeight(-1), mSafeAreaWidth(-1), mSafeAreaHeight(-1) {};
     ~BFlashResolutionSettings();
      bool load(BXMLNode node, BXMLReader* pReader);

      BFixedString256 mName;
      long            mID;

      long            mX;
      long            mY;
      long            mWidth;
      long            mHeight;
      long            mSafeAreaWidth;
      long            mSafeAreaHeight;
      BDynamicSimArray<BFlashResolutionProtoControl*> mControls;
};

//============================================================================
//============================================================================
class BFlashProtoASFunction
{
   public:
      BFlashProtoASFunction(){};
     ~BFlashProtoASFunction(){};
      bool load(BXMLNode node, BXMLReader* pReader);

      BFixedString128 mName;
};

//============================================================================
//============================================================================
class BFlashProtoKeyFrame
{
public:
   BFlashProtoKeyFrame(){};
   ~BFlashProtoKeyFrame(){};
   bool load(BXMLNode node, BXMLReader* pReader);

   BFixedString128 mName;
};

//============================================================================
//============================================================================
class BFlashProtoIcon
{
   public:
      BFlashProtoIcon(){};
     ~BFlashProtoIcon(){};
      bool load(BXMLNode node, BXMLReader* pReader);

      BFixedString128 mName;
      BFixedString128 mName2;  // second name for special states
      BFixedString128 mOwnerName;
      BFixedString128 mType;
      BFixedString128 mTexture; // path to texture
      XMHALF4    mSize;
      XMHALF4    mOffset;
      XMHALF4    mIntensity;
      XMHALF2    mUV;      
      long       mKeyFrame;
      long       mID;
      XMCOLOR    mColor;
      BYTE       mPriority;
      bool       mAllowPlayerColor;
};

//============================================================================
// BFlashScene
//============================================================================
class BFlashScene : public BRenderCommandListener, public BEventReceiverInterface
{
   public:
               BFlashScene();
      virtual ~BFlashScene();

      void                 initEventHandle();
      void                 deinitEventHandle();

      virtual bool         init(const char* filename, const char* dataFile) = 0;      
      virtual void         deinit()= 0;      
      virtual void         enter() = 0;
      virtual void         leave() = 0;
      virtual void         update(float elapsedTime) = 0;
      virtual void         renderBegin() = 0;
      virtual void         render()= 0;
      virtual void         renderEnd()= 0;
      virtual bool         handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail) = 0;
      virtual bool         receiveEvent(const BEvent& event, BThreadIndex threadIndex) {return true;}
      virtual void         setDimension(int x, int y, int width, int height) {};

      bool                 getFlag(long n) const { return(mFlags.isSet(n)!=0); }
      void                 setFlag(long n, bool v) { if(v) mFlags.set(n); else mFlags.unset(n); }
      
      bool                 loadData(const char* filename);      
      bool                 loadData(BXMLNode root, BXMLReader* pReader);

      int                    getProtoControlCount() const { return mControls.getNumber(); };
      BFlashProtoControl*    getProtoControl(int index);
      const char*            getProtoControlPath(int index);

      int                    getProtoASFunctionCount() const {return mASFunctions.getNumber(); };
      BFlashProtoASFunction* getProtoASFunction(int index);
      const char*            getProtoASFunctionName(int index);

      int                    getProtoIconCount() const {return mIcons.getNumber(); };
      int                    getProtoIconIndex(const BSimString& type, const BSimString& owner);
      BFlashProtoIcon*       getProtoIcon(int index);
      BFlashProtoIcon*       getProtoIconByName(const BSimString& type, const BSimString& owner);

      int                    getProtoKeyFrameCount() const {return mKeyframes.getNumber(); };
      BFlashProtoKeyFrame*   getProtoKeyFrame(int index);
      const char*            getProtoKeyFrameName(int index);

      const BFlashProperties* getProperties() const { return mpProperties; }


      int                       getResolutionCount() const { return mResolutions.getNumber(); }
      int                       getResolutionIndex( const BSimString& name);
      BFlashResolutionSettings* getResolution(int index);

      virtual void         initDeviceData(void);
      virtual void         frameBegin(void);
      virtual void         processCommand(const BRenderCommandHeader& header, const uchar* pData);
      virtual void         frameEnd(void);
      virtual void         deinitDeviceData(void);
      virtual BFlashMovieInstance*  getMovie()=0;


      // scaling helpers
      void initResolution();
      void setControlDimension(const char* path, int x, int y, int width, int height, int xScale, int yScale);
      void setControlXY(const char* path, int x, int y);
      void setControlWidthHeight(const char* path, int width, int height);
      void setControlOffsetXY(const char* path, int xOffset, int yOffset);
      void setControlScaleXY(const char* path, int xScale, int yScale);


   protected:

      BEventReceiverHandle mSimEventHandle;
      UTBitVector<32>      mFlags;
      BDynamicSimArray<BFlashProtoControl*> mControls;
      BDynamicSimArray<BFlashProtoASFunction*> mASFunctions;
      BDynamicSimArray<BFlashProtoIcon*> mIcons;
      BDynamicSimArray<BFlashProtoKeyFrame*> mKeyframes;
      BDynamicSimArray<BFlashResolutionSettings*> mResolutions;
      BFlashProperties* mpProperties;
};
   
      