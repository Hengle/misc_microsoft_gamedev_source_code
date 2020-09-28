//============================================================================
// flashscene.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashscene.h"
#include "gamedirectories.h"
#include "string\convertToken.h"
#include "render.h"
#include "flashgateway.h"
#include "ui.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashProperties::~BFlashProperties()
{
   // clean up the mData, mDataStrings, mFlashFiles
   for (int i = 0; i < mData.getNumber(); ++i)
   {
      mData[i]->deInit();
      delete mData[i];
      mData[i]=NULL;
   }
   mData.clear();

   for (int i = 0; i < mDataStrings.getNumber(); ++i)
   {
      delete mDataStrings[i];
      mDataStrings[i]=NULL;
   }
   mDataStrings.clear();

   for (int i = 0; i < mFlashFiles.getNumber(); ++i)
   {
      delete mFlashFiles[i];
      mFlashFiles[i]=NULL;
   }
   mFlashFiles.clear();

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashPropertyHandle BFlashProperties::findHandle(const char* name) const
{
   for (int i = 0; i < mData.getNumber(); ++i)
   {
      if (mData[i]->mName.compare(name) == 0)
      {
         return i + 1;
      }
   }
   return eInvalidFlashPropertyHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashPropertyHandle BFlashProperties::findStringHandle(const char* name) const
{
   for (int i = 0; i < mDataStrings.getNumber(); ++i)
   {
      if (mDataStrings[i]->mName.compare(name) == 0)
      {
         return i + 1;
      }
   }
   return eInvalidFlashPropertyHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashPropertyHandle BFlashProperties::findFlashFileHandle(const char* name) const
{
   for (int i = 0; i < mFlashFiles.getNumber(); ++i)
   {
      if (mFlashFiles[i]->mName.compare(name) == 0)
      {
         return i + 1;
      }
   }
   return eInvalidFlashPropertyHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char* BFlashProperties::getName(BFlashPropertyHandle handle) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   return pProperty->mName.c_str();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getHalf4(BFlashPropertyHandle handle, XMHALF4& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;  // mrh 5/8/07 - Note that I removed some DEBUG_ASSERT calls elsewhere on the result of getHalf4 because it already DEBUG_ASSERTs here (the only possibly way it can return false.)

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeHalf4));

   value = *((XMHALF4*) &pProperty->mHalf4);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getHalf2(BFlashPropertyHandle handle, XMHALF2& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeHalf2));

   value = *((XMHALF2*) &pProperty->mHalf2);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getXMColor(BFlashPropertyHandle handle, XMCOLOR& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeXMColor));

   value.c = pProperty->mXMColor;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getFloat(BFlashPropertyHandle handle, float& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeFloat));

   value = pProperty->mFloat;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getInt(BFlashPropertyHandle handle, int& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeInt));

   value = pProperty->mInt;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getBool(BFlashPropertyHandle handle, bool& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeBool));

   value = pProperty->mBool;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getTextureHandle(BFlashPropertyHandle handle, BManagedTextureHandle& value) const
{
   value = cInvalidManagedTextureHandle;
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);   
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeTextureHandle));

   value = pProperty->mTextureHandle;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getString(BFlashPropertyHandle handle, BFixedString128& value) const
{
   return getPropertyStringByHandle(handle, value);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getFlashFile(BFlashPropertyHandle handle, BFixedString128& value) const
{
   return getFlashFileByHandle(handle, value);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::getLocStringID(BFlashPropertyHandle handle, int& value) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return false;

   BDEBUG_ASSERT(pProperty->validateType(BFlashPropertyVariant::eTypeStringID));

   value = pProperty->mStringID;
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashProperties::getType(BFlashPropertyHandle handle) const
{
   const BFlashPropertyVariant* pProperty = getPropertyByHandle(handle);
   BDEBUG_ASSERT(pProperty!=NULL);
   if (!pProperty)
      return BFlashPropertyVariant::eTypeInvalid;

   return pProperty->mType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashScene::BFlashScene():
   mSimEventHandle(cInvalidEventReceiverHandle),
   mpProperties(NULL)
{
   mFlags.setAll(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashScene::~BFlashScene()
{
   deinitEventHandle();

   for (int i = 0; i < mControls.getNumber(); i++)
   {
      if (mControls[i])
         delete mControls[i];
      mControls[i] = NULL;
   }
   mControls.resize(0);

   for (int i = 0; i < mASFunctions.getNumber(); i++)
   {
      if (mASFunctions[i])
         delete mASFunctions[i];
      mASFunctions[i] = NULL;
   }
   mASFunctions.resize(0);

   for (int i = 0; i < mIcons.getNumber(); i++)
   {
      if (mIcons[i])
         delete mIcons[i];
      mIcons[i] = NULL;
   }
   mIcons.resize(0);

   for (int i = 0; i < mKeyframes.getNumber(); i++)
   {
      if (mKeyframes[i])
         delete mKeyframes[i];
      mKeyframes[i] = NULL;
   }
   mKeyframes.resize(0);

   if (mpProperties)
      delete mpProperties;
   mpProperties = NULL;
   
   for (int i = 0; i < mResolutions.getNumber(); i++)
   {
      if (mResolutions[i])
         delete mResolutions[i];
      mResolutions[i] = NULL;
   }
   mResolutions.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::initEventHandle()
{
   if (mSimEventHandle != cInvalidEventReceiverHandle)
      return;

   mSimEventHandle = gEventDispatcher.addClient(this, cThreadIndexSim);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::deinitEventHandle()
{
   if (mSimEventHandle == cInvalidEventReceiverHandle)
      return;

   const BThreadIndex handleThreadIndex = gEventDispatcher.getHandleThreadIndex(mSimEventHandle);
   if (!gEventDispatcher.getThreadId(handleThreadIndex))
      gEventDispatcher.removeClientImmediate(mSimEventHandle);
   else 
   {
      // Must wait because this method guarantees that the object will no longer receive messages once this method returns.
      gEventDispatcher.removeClientDeferred(mSimEventHandle, true);
   }

   mSimEventHandle = cInvalidEventReceiverHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::initDeviceData(void)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::frameBegin(void)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::processCommand(const BRenderCommandHeader& header, const uchar* pData)
{

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::frameEnd(void)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::deinitDeviceData(void)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashScene::loadData(BXMLNode root, BXMLReader* pReader)
{
   // -- make sure we are dealing with the right kind of file
   if (root.getName()!= "FlashMovieDefinition")
      return false;

   int numChildren = root.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      BXMLNode node = root.getChild(i);
      if (node.getName().compare("Control") == 0)
      {                                    
         BFlashProtoControl* pControl = new BFlashProtoControl();
         if (!pControl->load(node, pReader))
         {
            delete pControl;
            continue;
         }
         mControls.pushBack(pControl);         
      }
      else if (node.getName().compare("ASFunction") == 0)
      {
         BFlashProtoASFunction* pFunction = new BFlashProtoASFunction();
         if (!pFunction->load(node, pReader))
         {
            delete pFunction;
            continue;
         }
         mASFunctions.pushBack(pFunction);
      }
      else if (node.getName().compare("Icon") == 0)
      {
         BFlashProtoIcon* pIcon = new BFlashProtoIcon();
         if (!pIcon->load(node, pReader))
         {
            delete pIcon;
            continue;
         }
         mIcons.pushBack(pIcon);
      }
      else if (node.getName().compare("KeyFrame") == 0)
      {
         BFlashProtoKeyFrame* pKeyFrame = new BFlashProtoKeyFrame();
         if (!pKeyFrame->load(node, pReader))
         {
            delete pKeyFrame;
            continue;
         }
         mKeyframes.pushBack(pKeyFrame);
      }
      else if (node.getName().compare("Properties") == 0)
      {
         BFlashProperties* pProperties = new BFlashProperties();
         if (!pProperties->load(node, pReader))
         {
            delete pProperties;
            continue;
         }
         mpProperties = pProperties;
      }
      else if (node.getName().compare("Resolution") == 0)
      {
         BFlashResolutionSettings* pResolution = new BFlashResolutionSettings();
         if (!pResolution->load(node, pReader))
         {
            delete pResolution;
            continue;
         }
         mResolutions.pushBack(pResolution);
      }
   }
   return true;}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashScene::loadData(const char* filename)
{
   BXMLReader reader;

   if(!reader.load(cDirProduction, filename))
      return false;

   BXMLNode root(reader.getRootNode());

   return loadData(root, &reader);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashProtoControl*  BFlashScene::getProtoControl(int index)
{
   debugRangeCheck(index, mControls.getNumber());
   return mControls[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char* BFlashScene::getProtoControlPath(int index)
{
   debugRangeCheck(index, mControls.getNumber());
   return mControls[index]->mPath.c_str();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashProtoASFunction* BFlashScene::getProtoASFunction(int index)
{
   debugRangeCheck(index, mASFunctions.getNumber());
   return mASFunctions[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char* BFlashScene::getProtoASFunctionName(int index)
{
   debugRangeCheck(index, mASFunctions.getNumber());
   return mASFunctions[index]->mName.c_str();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashProtoIcon* BFlashScene::getProtoIcon(int index)
{
   if (index < 0)
      return NULL;

   debugRangeCheck(index, mIcons.getNumber());
   return mIcons[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashProtoKeyFrame* BFlashScene::getProtoKeyFrame(int index)
{
   if (index < 0)
      return NULL;

   debugRangeCheck(index, mKeyframes.getNumber());
   return mKeyframes[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char* BFlashScene::getProtoKeyFrameName(int index)
{
   if (index < 0)
      return NULL;

   debugRangeCheck(index, mKeyframes.getNumber());
   return mKeyframes[index]->mName.c_str();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashScene::getProtoIconIndex(const BSimString& type, const BSimString& owner)
{
//-- FIXING PREFIX BUG ID 3051
   const BFlashProtoIcon* pKey = NULL;
//--
   for (int i = 0; i < mIcons.getNumber(); ++i)
   {
      pKey = getProtoIcon(i);
      BDEBUG_ASSERT(pKey!=NULL);
      if (pKey->mType.compare(type) == 0 && (pKey->mOwnerName.compare(owner) == 0))
         return i;
   }

   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashProtoIcon* BFlashScene::getProtoIconByName(const BSimString& type, const BSimString& owner)
{
   int index = getProtoIconIndex(type, owner);
   if (index == -1)
      return NULL;

   return getProtoIcon(index);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int BFlashScene::getResolutionIndex( const BSimString& name)
{
   for (int i = 0; i < mResolutions.getNumber(); ++i)
   {
      if (mResolutions[i]->mName.compare(name) == 0)
         return i;
   }

   return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashResolutionSettings* BFlashScene::getResolution(int index)
{
   if (index < 0)
      return NULL;
   debugRangeCheck(index, mResolutions.getNumber());
   return mResolutions[index];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::setControlDimension(const char* path, int x, int y, int width, int height, int xScale, int yScale)
{
   BFlashMovieInstance* pMovie = getMovie();
   if (!pMovie)
      return;

   GFxValue value[7];
   value[0].SetString(path);
   value[1].SetNumber((double) x);
   value[2].SetNumber((double) y);
   value[3].SetNumber((double) width);
   value[4].SetNumber((double) height);
   value[5].SetNumber((double) xScale);
   value[6].SetNumber((double) yScale);

   pMovie->invokeActionScript("setControlDimension", value, 7);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::setControlXY(const char* path, int x, int y)
{
   BFlashMovieInstance* pMovie = getMovie();
   if (!pMovie)
      return;

   GFxValue value[3];
   value[0].SetString(path);
   value[1].SetNumber((double) x);
   value[2].SetNumber((double) y);   
   pMovie->invokeActionScript("setControlXY", value, 3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::setControlWidthHeight(const char* path, int width, int height)
{
   BFlashMovieInstance* pMovie = getMovie();
   if (!pMovie)
      return;

   GFxValue value[3];
   value[0].SetString(path);
   value[1].SetNumber((double) width);
   value[2].SetNumber((double) height);   
   pMovie->invokeActionScript("setControlWidthHeight", value, 3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::setControlOffsetXY(const char* path, int xOffset, int yOffset)
{
   BFlashMovieInstance* pMovie = getMovie();
   if (!pMovie)
      return;

   GFxValue value[3];
   value[0].SetString(path);
   value[1].SetNumber((double) xOffset);
   value[2].SetNumber((double) yOffset);   
   pMovie->invokeActionScript("setControlOffsetXY", value, 3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::setControlScaleXY(const char* path, int xScale, int yScale)
{
   BFlashMovieInstance* pMovie = getMovie();
   if (!pMovie)
      return;

   GFxValue value[3];
   value[0].SetString(path);
   value[1].SetNumber((double) xScale);
   value[2].SetNumber((double) yScale);   
   pMovie->invokeActionScript("setControlScaleXY", value, 3);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashScene::initResolution()
{  
   float authoringDimensionWidth  = 1800.0f;
   float authoringDimensionHeight = 1300.0f;
   float authoringSafeAreaWidth   = 1400.0f;
   float authoringSafeAreaHeight  = 900.0f;   
   float backBufferSafeAreaScalar = cTitleSafeArea;
   const BFlashProperties* pProperties = getProperties();
   if (pProperties)
   {     
      authoringDimensionWidth  = (float) pProperties->mWidth;
      authoringDimensionHeight = (float) pProperties->mHeight;
      authoringSafeAreaWidth   = (float) pProperties->mSafeAreaWidth;
      authoringSafeAreaHeight  = (float) pProperties->mSafeAreaHeight; 

      bool ignoreSafeAreas = false;
      BFlashPropertyHandle dataHandle = pProperties->findHandle("IgnoreSafeAreas");
      if (dataHandle != BFlashProperties::eInvalidFlashPropertyHandle)
      {
         pProperties->getBool(dataHandle,  ignoreSafeAreas);
         if (ignoreSafeAreas)
         {
            backBufferSafeAreaScalar = 1.0f;
            authoringSafeAreaWidth = authoringDimensionWidth;
            authoringSafeAreaHeight = authoringDimensionHeight;
         }      
      }
   }      
   
   const float backBufferSafeAreaWidth = BD3D::mD3DPP.BackBufferWidth * backBufferSafeAreaScalar;
   const float backBufferSafeAreaHeight = BD3D::mD3DPP.BackBufferHeight * backBufferSafeAreaScalar;

   float scalar = 1.0f;
   BRender::BAspectRatioMode aspectRatioMode = (BRender::BAspectRatioMode) gRender.getAspectRatioMode();

   if ((aspectRatioMode==BRender::cAspectRatioMode16x9) || (authoringDimensionWidth == 1280))
      scalar = backBufferSafeAreaHeight / authoringSafeAreaHeight;
   else if (aspectRatioMode==BRender::cAspectRatioMode4x3)
      scalar = backBufferSafeAreaWidth / authoringSafeAreaWidth;

   int newWidth = Math::FloatToIntRound(scalar * authoringDimensionWidth);
   int newHeight = Math::FloatToIntRound(scalar * authoringDimensionHeight);
   int newX = Math::FloatToIntRound( 0.5f * (((float)BD3D::mD3DPP.BackBufferWidth) - newWidth));
   int newY = Math::FloatToIntRound( 0.5f * (((float)BD3D::mD3DPP.BackBufferHeight) - newHeight));

   //-- set the correct dimension of the movie
   setDimension(newX, newY, newWidth, newHeight);

   //-- fixup any control positions using normalized data
   BSimString resStr("16x9");
   if (aspectRatioMode==BRender::cAspectRatioMode4x3)
      resStr.set("4x3");
   int index = getResolutionIndex(resStr);
   BFlashResolutionSettings* pResolution = getResolution(index);
   if (pResolution)
   {      
      // Iterate over the defined controls
      for (int i = 0; i < pResolution->mControls.getNumber(); ++i)
      {
         const BFlashResolutionProtoControl* control = pResolution->mControls[i];

         if (control->mX >= cFloatCompareEpsilon || control->mY >= cFloatCompareEpsilon)
         {                      
            int newControlX = Math::FloatToIntRound(control->mX * authoringDimensionWidth);
            int newControlY = Math::FloatToIntRound(control->mY * authoringDimensionHeight);
            setControlXY(control->mPath.c_str(), newControlX, newControlY);
         }

         /*
         if (control->mOffsetX != 0 || control->mOffsetY != 0)
            setControlOffsetXY(control->mPath.c_str(), control->mOffsetX, control->mOffsetY);

         if (control->mWidth > 0 || control->mHeight > 0)
            setControlWidthHeight(control->mPath.c_str(), control->mWidth, control->mHeight);
         */

         if (control->mScaleX > cFloatCompareEpsilon || control->mScaleY > cFloatCompareEpsilon)
         {
            setControlScaleXY(control->mPath.c_str(), control->mScaleX, control->mScaleY);
         }
      }
   } 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProtoControl::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   node.getText(mPath);  
   node.getAttribValueAsString("name", mName);
   node.getAttribValueAsLong("id", mID);
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProtoASFunction::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   node.getText(mName);  
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProtoKeyFrame::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   node.getText(mName);  
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProtoIcon::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   if (!node.getAttribValueAsString("type", mType))
      mType.set("InvalidType");

   if (!node.getAttribValueAsString("name", mName))
   {
      mName.set("InvalidName");
   }

   if (!node.getAttribValueAsString("name2", mName2))
   {
      mName2.set("InvalidName2");
   }

   if (!node.getAttribValueAsString("texture", mTexture))
   {
      mTexture.set("");
   }

   float u = 0.0f;
   float v = 0.0f;
   node.getAttribValueAsFloat("U", u);
   node.getAttribValueAsFloat("V", v);
   mUV = XMHALF2(u,v);

   // Size
   float sizeX = 0.0f;
   float sizeY = 0.0f;
   float sizeZ = 0.0f;
   float sizeW = 0.0f;
   node.getAttribValueAsFloat("SizeX", sizeX);
   node.getAttribValueAsFloat("SizeY", sizeY);
   node.getAttribValueAsFloat("SizeZ", sizeZ);
   node.getAttribValueAsFloat("SizeW", sizeW);
   mSize = XMHALF4(sizeX, sizeY, sizeZ, sizeW);

   float intensity1 = 1.0f;
   float intensity2 = 1.0f;
   float intensity3 = 1.0f;
   float intensity4 = 1.0f;
   node.getAttribValueAsFloat("Intenstiy1", intensity1);
   node.getAttribValueAsFloat("Intensity2", intensity2);
   node.getAttribValueAsFloat("Intensity3", intensity3);
   node.getAttribValueAsFloat("Intensity4", intensity4);
   mIntensity = XMHALF4(intensity1, intensity2, intensity3, intensity4);

   BVector offset = XMVectorZero();
   node.getAttribValueAsVector("Offset", offset);
   XMStoreHalf4(&mOffset, *((XMVECTOR*)&offset));

   // Color
   BSimString value;
   DWORD color = cDWORDWhite;
   mColor = cDWORDWhite;
   if (node.getAttribValueAsString("Color", value))
   {
      if (convertTokenToDWORDColor(value, color))
         mColor = color;
   }

   long priority = 0;
   if (node.getAttribValueAsLong("Priority", priority))
   {
      mPriority = (BYTE) priority;
   }

   mAllowPlayerColor = true;
   node.getAttribValueAsBool("allowPlayerColor", mAllowPlayerColor);
   
   if (!node.getAttribValueAsString("owner", mOwnerName))
   {
      mOwnerName.set("InvalidOwner");
   }

   if (!node.getTextAsLong(mKeyFrame))
   {
      mKeyFrame = -1;
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashProperties::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   int count  = node.getNumberChildren();
   for (int i = 0; i < count; i++)
   {
      BXMLNode childNode = node.getChild(i);
      if (childNode.getName().compare("Size") == 0)
      {
         childNode.getAttribValueAsLong("Width", mWidth);
         childNode.getAttribValueAsLong("Height", mHeight);
         childNode.getAttribValueAsLong("SafeAreaWidth", mSafeAreaWidth);
         childNode.getAttribValueAsLong("SafeAreaHeight", mSafeAreaHeight);
      }
      else if (childNode.getName().compare("Position") == 0)
      {
         childNode.getAttribValueAsLong("X", mX);
         childNode.getAttribValueAsLong("Y", mY);
      }
      else if (childNode.getName().compare("Data") == 0)
      {
         BFixedString16 type;
         childNode.getAttribValueAsString("type", type);

         if (type.compare("float") == 0)
         {            
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            data->mType = BFlashPropertyVariant::eTypeFloat;            
            childNode.getTextAsFloat(data->mFloat);
            mData.add(data);

            //childNode.getTextAsFloat(dataFloat);
            //float dataFloat;
            //mDataFloat.add(dataFloat);
         }         
         else if (type.compare("int") == 0)
         {
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            data->mType = BFlashPropertyVariant::eTypeInt;            
            childNode.getTextAsInt(data->mInt);
            mData.add(data);
            //int dataInt;
            //childNode.getTextAsInt(dataInt);
            //mDataInt.add(dataInt);
         }
         else if (type.compare("color") == 0)
         {
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            BSimString value;
            DWORD color;
            childNode.getTextAsString(value);
            convertTokenToDWORDColor(value, color);

            data->mType = BFlashPropertyVariant::eTypeXMColor;            
            data->mXMColor = color;
            mData.add(data);

            //XMCOLOR xColor(color);
            //mDataColor.add(xColor);
         }
         else if (type.compare("bool") == 0)
         {
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            bool bBool;
            childNode.getTextAsBool(bBool);

            data->mType = BFlashPropertyVariant::eTypeBool;            
            data->mBool = bBool;
            mData.add(data);

            //mDataBool.add(bBool);
         }
         else if (type.compare("half4") == 0)
         {
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            BSimString value;
            childNode.getTextAsString(value);
            XMHALF4 v;
            convertTokenToXMHALF4(value, v);

            data->mType = BFlashPropertyVariant::eTypeHalf4;            
            *((XMHALF4*) &(data->mHalf4)) = v;
            mData.add(data);

            //mDataHalf4.add(v);
         }
         else if (type.compare("string") == 0)
         {
            BFlashPropertyString *strData = new BFlashPropertyString();
            childNode.getAttribValueAsString("name", strData->mName);
            childNode.getText(strData->mString);
            mDataStrings.add(strData);
         }
         else if (type.compare("flashfile") == 0)
         {
            BFlashPropertyString *strData = new BFlashPropertyString();
            childNode.getAttribValueAsString("name", strData->mName);
            childNode.getText(strData->mString);
            mFlashFiles.add(strData);
         }
         else if (type.compare("StringID") == 0)
         {
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            data->mType = BFlashPropertyVariant::eTypeStringID;            
            childNode.getTextAsInt(data->mStringID);
            mData.add(data);
         }
         else if (type.compare("Texture") == 0)
         {
            BFlashPropertyVariant *data = new BFlashPropertyVariant();
            childNode.getAttribValueAsString("name", data->mName);

            data->mType = BFlashPropertyVariant::eTypeTextureHandle;
            BString path;
            childNode.getText(path);
            data->mTextureHandle = gD3DTextureManager.getOrCreateHandle(path, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureWhite, true, false, "FlashScene");
            mData.add(data);
         }
      }
   }

   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashResolutionSettings::~BFlashResolutionSettings()
{
   for (int i = 0; i < mControls.getNumber(); i++)
   {
      if (mControls[i])
         delete mControls[i];
      mControls[i] = NULL;
   }
   mControls.resize(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashResolutionSettings::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   // Get the movie resolution first
   node.getAttribValueAsLong("x", mX);
   node.getAttribValueAsLong("y", mY);
   node.getAttribValueAsLong("width", mWidth);
   node.getAttribValueAsLong("height", mHeight);
   node.getAttribValueAsLong("SafeAreaWidth", mSafeAreaWidth);
   node.getAttribValueAsLong("SafeAreaHeight", mSafeAreaHeight);

   // Get the control resolution properties.
   int count  = node.getNumberChildren();
   node.getText(mName);
   for (int i = 0; i < count; i++)
   {
      BXMLNode childNode = node.getChild(i);
      if (childNode.getName().compare("Control") == 0)
      {
         BFlashResolutionProtoControl* pNewControl = new BFlashResolutionProtoControl();
         if (pNewControl->load(childNode, pReader))
         {
            mControls.pushBack(pNewControl);
         }
      }
   }
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashResolutionProtoControl::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   node.getText(mPath);  
   node.getAttribValueAsString("name", mName);
   node.getAttribValueAsLong("id", mID);
   node.getAttribValueAsFloat("x", mX);
   node.getAttribValueAsFloat("y", mY);
   node.getAttribValueAsLong("xOffset", mOffsetX);
   node.getAttribValueAsLong("yOffset", mOffsetY);
   node.getAttribValueAsLong("width", mWidth);
   node.getAttribValueAsLong("height", mHeight);
   node.getAttribValueAsFloat("xScale", mScaleX);
   node.getAttribValueAsFloat("yScale", mScaleY);

   mScaleX = Math::ClampLow(mScaleX, 1.0f);
   mScaleY = Math::ClampLow(mScaleY, 1.0f);
   return true;
}