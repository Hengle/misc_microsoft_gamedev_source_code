//==============================================================================
// HPBar.cpp
//
// The HP bar renderer.
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "common.h"
#include "HPBar.h"
#include "renderDraw.h"
#include "render.h"
#include "unit.h"
#include "protoobject.h"
#include "usermanager.h"
#include "user.h"
#include "world.h"
#include "reloadManager.h"
#include "configsgame.h"
#include "visual.h"
#include "protosquad.h"
#include "math\VMXUtils.h"
#include "gamedirectories.h"
#include "D3DTextureLoader.h"
#include "string\converttoken.h"
#include "uimanager.h"
#include "scenario.h"
#include "database.h"
#include "ability.h"
#include "game.h"

//==============================================================================
// Defines
//==============================================================================
const float cHPBarMinHeight = 0.25f;
const float cHPBarMaxHeight = 0.75f;
const float cHPBarMinWidth = 2.0f;
const float cHPBarMaxWidth = 20.0f;
const float cHPBarMinWidthHeightRatio = 0.3f;
const float cHPBarWidthMultiplier = 0.002f;

//==============================================================================
// Globals
//==============================================================================
BHPBar gHPBar;

//==============================================================================
// Prototypes
//==============================================================================

//==============================================================================
// Constructors/Destructors
//==============================================================================
BHPBar::BHPBar() :
   mBPTCVertexDecl(NULL),
   mSquadVertexDecl(NULL),
   BEventReceiver()
{
}

//==============================================================================
//==============================================================================
BHPBar::~BHPBar()
{
}

//==============================================================================
// Initializer/Deinitializer
//==============================================================================
void BHPBar::init(void)
{
   SCOPEDSAMPLE(BHPBar_init)
   ASSERT_MAIN_THREAD

   deinit();

   // init event receiver and command listener
   eventReceiverInit(cThreadIndexRender);
   commandListenerInit();
}

//==============================================================================
// BHPBar::initScenario
//==============================================================================
void BHPBar::initScenario(void)
{
   if (mEventHandle == cInvalidEventReceiverHandle)
      return;
      
   // rg [1/8/08] - FIXME - This code is loading hpbar textures from the scenario archive, this should be in the root archive!
   bool bResult = loadTextures();
   if (!bResult)
   {
      gConsoleOutput.error("BHPBar::initScenario: BHPBar::loadTextures() failed!\n");
   }

   bResult = loadVeterancyBarTextures();   
   if (!bResult)
   {
      gConsoleOutput.error("BHPBar::initScenario: BHPBar::loadVeterancyBarTextures() failed!\n");
   }

   bResult = loadAbilityRecoveryBarTextures();
   if (!bResult)
   {
      gConsoleOutput.error("BHPBar::initScenario: BHPBar::loadAbilityRecoveryBarTextures() failed!\n");
   }

   bResult = loadBobbleHeadTextures();
   if (!bResult)
   {
      gConsoleOutput.error("BHPBar::initScenario: BHPBar::loadBobbleHeadTextures() failed!\n");
   }

   bResult = loadBuildingStrengthTextures();
   if (!bResult)
   {
      gConsoleOutput.error("BHPBar::initScenario: BHPBar::loadBuildingStrengthTextures() failed!\n");
   }

   mColorStageID[eColorStageIDDefault] = gDatabase.getProtoHPColorStageID("DEFAULT");
}

//==============================================================================
// BHPBar::deinitScenario()
//==============================================================================
void BHPBar::deinitScenario()
{
   unloadTextures();
   unloadVeterancyBarTextures();
   unloadAbilityRecoveryBarTextures();
   unloadBobbleHeadTextures();
   unloadBuildingStrengthTextures();
}

//==============================================================================
// BHPBar::deinit
//==============================================================================
void BHPBar::deinit(void)
{
//   ASSERT_MAIN_THREAD

   if (gRenderThread.getHasD3DOwnership())
      gRenderThread.blockUntilGPUIdle();         

   mMaskTexture.release();
   mDiffuseTexture.release();

   commandListenerDeinit();
   eventReceiverDeinit();

   if (gRenderThread.getHasD3DOwnership())
      gRenderThread.blockUntilGPUIdle();
}

//==============================================================================
// BHPBar::loadTextures()
//==============================================================================
bool BHPBar::loadTextures()
{
      SCOPEDSAMPLE(BHPBar_loadTextures)
   int count = gDatabase.getNumberProtoHPBars();
   if (count < 1)
      return false;

   BD3DTextureLoader diffuseTextureLoader;
   BD3DTextureLoader maskTextureLoader;
   BD3DTextureLoader::BCreateParams textureCreateParams;
   textureCreateParams.mArraySize   = (uchar) count;
   textureCreateParams.mBigEndian   = true;
         
   BString filename;
   for(int i=0;i<count;i++)
   {
      BProtoHPBar* pProtoHPBar = gDatabase.getProtoHPBar(i);

      BFile file;  
      filename= pProtoHPBar->mDiffuseTexture;
      filename.removeExtension();
      filename.append(".ddx");
      
      textureCreateParams.mName = filename;

      if (!file.openReadOnly(cDirArt, filename))
      {
         gConsoleOutput.error("BHPBar::init: Unable to open file: %s\n", filename.getPtr());
         return false;
      }

      unsigned long fileSize;
      if (!file.getSize(fileSize))
      {
         gConsoleOutput.error("BHPBar::init: Unable to read file: %s\n", filename.getPtr());
         return false;
      }

      BDynamicArray<uchar> data(fileSize);
      if (!file.read(data.getPtr(), fileSize))
      {
         gConsoleOutput.error("BHPBar::init: Unable to read file: %s\n", filename.getPtr());
         return false;
      }

      textureCreateParams.mArrayIndex = (uchar)i;    
      textureCreateParams.mManager = "HPBar";  
      textureCreateParams.mName = filename;

      if (!diffuseTextureLoader.createFromDDXFileInMemory(data.getPtr(), fileSize, textureCreateParams))
      {
         gConsoleOutput.error("BHPBar::init: Unable to read file: %s\n", filename.getPtr());
         return false;
      }

      pProtoHPBar->mDiffuseIndex = (BYTE) i;
   }

   mDiffuseTexture.release();
   mDiffuseTexture = diffuseTextureLoader.getD3DTexture();


   //-- load the mask textures
   for(int i=0;i<count;i++)
   {
      BFile file;  
      BProtoHPBar* pProtoHPBar = gDatabase.getProtoHPBar(i);

      filename= pProtoHPBar->mMaskTexture;
      filename.removeExtension();
      filename.append(".ddx");

      if (!file.openReadOnly(cDirArt, filename))
      {
         gConsoleOutput.error("BHPBar::init: Unable to open file: %s\n", filename.getPtr());
         return false;
      }

      unsigned long fileSize;
      if (!file.getSize(fileSize))
      {
         gConsoleOutput.error("BHPBar::init: Unable to read file: %s\n", filename.getPtr());
         return false;
      }

      BDynamicArray<uchar> data(fileSize);
      if (!file.read(data.getPtr(), fileSize))
      {
         gConsoleOutput.error("BHPBar::init: Unable to read file: %s\n", filename.getPtr());
         return false;
      }

      textureCreateParams.mArrayIndex = (uchar)i;      

      if (!maskTextureLoader.createFromDDXFileInMemory(data.getPtr(), fileSize, textureCreateParams))
      {
         gConsoleOutput.error("BHPBar::init: Unable to read file: %s\n", filename.getPtr());
         return false;
      }

      pProtoHPBar->mMaskIndex = (BYTE) i;
   }
   
   for(int i=0;i<count;i++)
   {
      BProtoHPBar* pProtoHPBar = gDatabase.getProtoHPBar(i);
      filename= pProtoHPBar->mDiffuseTexture;
      filename.removeExtension();
      filename.append(".ddx");
      gFileManager.discardFiles(cDirArt, filename, false);
   }
   
   for(int i=0;i<count;i++)
   {
      BProtoHPBar* pProtoHPBar = gDatabase.getProtoHPBar(i);
      filename= pProtoHPBar->mMaskTexture;
      filename.removeExtension();
      filename.append(".ddx");
      gFileManager.discardFiles(cDirArt, filename);
   }

   mMaskTexture.release();
   mMaskTexture = maskTextureLoader.getD3DTexture();

   diffuseTextureLoader.releaseOwnership();
   maskTextureLoader.releaseOwnership();

   return true;
}

//==============================================================================
// BHPBar::unloadTextures
//==============================================================================
void BHPBar::unloadTextures()
{
   mDiffuseTexture.release();
   mMaskTexture.release();   
}

//==============================================================================
// BHPBar::loadVeterancyBarTextures
//==============================================================================
bool BHPBar::loadVeterancyBarTextures()
{
   int count = gDatabase.getNumberProtoVeterancyBars();
   if (count < 1)
      return false;

   for (int i = 0; i < count; ++i)
   {
      BProtoVeterancyBar* pProto = gDatabase.getProtoVeterancyBar(i);
      pProto->mTextureHandle = gD3DTextureManager.getOrCreateHandle(pProto->mTexture, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
   }
   return true;
}

//==============================================================================
// BHPBar::unloadVeterancyBarTextures
//==============================================================================
void BHPBar::unloadVeterancyBarTextures()
{
   int count = gDatabase.getNumberProtoVeterancyBars();   
   for (int i = 0; i < count; ++i)
   {
      BProtoVeterancyBar* pProto = gDatabase.getProtoVeterancyBar(i);
      if (!pProto)
         continue;

      if (pProto->mTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mTextureHandle);
      pProto->mTextureHandle = cInvalidManagedTextureHandle;
   }
}

//==============================================================================
// BHPBar::loadAbilityRecoveryBarTextures
//==============================================================================
bool BHPBar::loadAbilityRecoveryBarTextures()
{
   int count = gDatabase.getNumberProtoPieProgressBars();
   if (count < 1)
      return false;

   for (int i = 0; i < count; ++i)
   {
      BProtoPieProgress* pProto = gDatabase.getProtoPieProgressBar(i);
      if (!pProto)
         continue;

      pProto->mBackgroundTextureHandle = gD3DTextureManager.getOrCreateHandle(pProto->mBackgroundTexture, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
      pProto->mMaskTextureHandle = gD3DTextureManager.getOrCreateHandle(pProto->mMaskTexture, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
   }
   return true;
}

//==============================================================================
// BHPBar::unloadAbilityRecoveryBarTextures
//==============================================================================
void BHPBar::unloadAbilityRecoveryBarTextures()
{
   int count = gDatabase.getNumberProtoPieProgressBars();   
   for (int i = 0; i < count; ++i)
   {
      BProtoPieProgress* pProto = gDatabase.getProtoPieProgressBar(i);
      if (!pProto)
         continue;

      if (pProto->mBackgroundTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mBackgroundTextureHandle);
      pProto->mBackgroundTextureHandle = cInvalidManagedTextureHandle;

      if (pProto->mMaskTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mMaskTextureHandle);     
      pProto->mMaskTextureHandle = cInvalidManagedTextureHandle; 
   }   
}

//==============================================================================
// BHPBar::loadBobbleHeadTextures
//==============================================================================
bool BHPBar::loadBobbleHeadTextures()
{
   int count = gDatabase.getNumberProtoBobbleHeads();
   if (count < 1)
      return false;

   for (int i = 0; i < count; ++i)
   {
      BProtoBobbleHead* pProto = gDatabase.getProtoBobbleHead(i);
      if (!pProto)
         continue;

      pProto->mBackgroundTextureHandle = gD3DTextureManager.getOrCreateHandle(pProto->mBackgroundTexture, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
      pProto->mPortraitTextureHandle = gD3DTextureManager.getOrCreateHandle(pProto->mPortraitTexture, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
      pProto->mVetTextureHandle = gD3DTextureManager.getOrCreateHandle(pProto->mVetTexture, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
   }
   return true;
}

//==============================================================================
// BHPBar::unloadBobbleHeadTextures
//==============================================================================
void BHPBar::unloadBobbleHeadTextures()
{
   int count = gDatabase.getNumberProtoBobbleHeads();   
   for (int i = 0; i < count; ++i)
   {
      BProtoBobbleHead* pProto = gDatabase.getProtoBobbleHead(i);
      if (!pProto)
         continue;

      if (pProto->mBackgroundTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mBackgroundTextureHandle);
      pProto->mBackgroundTextureHandle = cInvalidManagedTextureHandle;

      if (pProto->mPortraitTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mPortraitTextureHandle);            
      pProto->mPortraitTextureHandle = cInvalidManagedTextureHandle;

      if (pProto->mVetTextureHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mVetTextureHandle);
      pProto->mVetTextureHandle = cInvalidManagedTextureHandle; 
   }
}

//==============================================================================
// BHPBar::loadBuildingStrengthTextures
//==============================================================================
bool BHPBar::loadBuildingStrengthTextures()
{
   int count = gDatabase.getNumberProtoBuildingStrength();
   if (count < 1)
      return false;

   for (int i = 0; i < count; ++i)
   {
      BProtoBuildingStrength* pProto = gDatabase.getProtoBuildingStrength(i);
      if (!pProto)
         continue;

      pProto->mTextureOnHandle = gD3DTextureManager.getOrCreateHandle(pProto->mTextureOn, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
      pProto->mTextureOffHandle = gD3DTextureManager.getOrCreateHandle(pProto->mTextureOff, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BHPBar");
   }
   return true;
}

//==============================================================================
// BHPBar::unloadBuildingStrengthTextures
//==============================================================================
void BHPBar::unloadBuildingStrengthTextures()
{
   int count = gDatabase.getNumberProtoBuildingStrength();
   for (int i = 0; i < count; ++i)
   {
      BProtoBuildingStrength* pProto = gDatabase.getProtoBuildingStrength(i);
      if (!pProto)
         continue;

      if (pProto->mTextureOnHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mTextureOnHandle);
      pProto->mTextureOnHandle = cInvalidManagedTextureHandle;

      if (pProto->mTextureOffHandle != cInvalidManagedTextureHandle)
         gD3DTextureManager.releaseManagedTextureByHandle(pProto->mTextureOffHandle);
      pProto->mTextureOffHandle = cInvalidManagedTextureHandle; 
   }
}

//==============================================================================
// BHPBarNode::load
//==============================================================================
bool BProtoHPBar::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   clear();

   node.getAttribValueAsString("name", mName);
   int childCount = node.getNumberChildren();
   for (int i = 0; i < childCount; ++i)
   {
      BXMLNode child(node.getChild(i));
      if (!child)
         continue;
      const BPackedString childName(child.getName());
      if (childName=="Texture")
      {
         child.getText(mDiffuseTexture);
      }
      else if (childName=="MaskTexture")
      {
         child.getText(mMaskTexture);
      }
      else if (childName == "SizeX")
      {
         child.getTextAsFloat(mSizeX);
      }
      else if (childName == "SizeY")
      {
         child.getTextAsFloat(mSizeY);
      }
      else if (childName == "HitpointUV")
      {
         float u1=0,u2=0,v1=0,v2=0;
         child.getAttribValueAsFloat("u1", u1);
         child.getAttribValueAsFloat("u2", u2);
         child.getAttribValueAsFloat("v1", v1);
         child.getAttribValueAsFloat("v2", v2);
         mHPUV = XMVectorSet(u1,v1,u2,v2);
      }
      else if (childName == "ShieldUV")
      {
         float u1=0,u2=0,v1=0,v2=0;
         child.getAttribValueAsFloat("u1", u1);
         child.getAttribValueAsFloat("u2", u2);
         child.getAttribValueAsFloat("v1", v1);
         child.getAttribValueAsFloat("v2", v2);
         mShieldUV = XMVectorSet(u1,v1,u2,v2);
      }
      else if (childName == "AmmoUV")
      {
         float u1=0,u2=0,v1=0,v2=0;
         child.getAttribValueAsFloat("u1", u1);
         child.getAttribValueAsFloat("u2", u2);
         child.getAttribValueAsFloat("v1", v1);
         child.getAttribValueAsFloat("v2", v2);
         mAmmoUV = XMVectorSet(u1,v1,u2,v2);
      }
   }
   return true;
}

//==============================================================================
// BProtoPieProgress::load
//==============================================================================
bool BProtoPieProgress::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   clear();

   node.getAttribValueAsString("name", mName);
   int childCount = node.getNumberChildren();
   for (int i = 0; i < childCount; ++i)
   {
      BXMLNode child(node.getChild(i));
      if (!child)
         continue;
      const BPackedString childName(child.getName());
      if (childName=="BackgroundTexture")
      {
         child.getText(mBackgroundTexture);
      }
      else if (childName=="MaskTexture")
      {
         child.getText(mMaskTexture);
      }
      else if (childName == "SizeX")
      {
         child.getTextAsFloat(mSizeX);
      }
      else if (childName == "SizeY")
      {
         child.getTextAsFloat(mSizeY);
      }
      else if (childName == "OffsetX")
      {
         child.getTextAsFloat(mOffsetX);
      }
      else if (childName == "OffsetY")
      {
         child.getTextAsFloat(mOffsetY);
      }
      else if (childName == "Fill")
      {
         child.getTextAsBool(mFill);
      }
      else if (childName == "Clockwise")
      {
         child.getTextAsBool(mClockwise);
      }      
      else if (childName == "BackgroundSizeX")
      {
         child.getTextAsFloat(mBackgroundSizeX);
      }
      else if (childName == "BackgroundSizeY")
      {
         child.getTextAsFloat(mBackgroundSizeY);
      }
      else if (childName == "BackgroundOffsetX")
      {
         child.getTextAsFloat(mBackgroundOffsetX);
      }
      else if (childName == "BackgroundOffsetY")
      {
         child.getTextAsFloat(mBackgroundOffsetY);
      }
   }
   return true;
}

//==============================================================================
// BProtoVeterancyBar::load
//==============================================================================
bool BProtoVeterancyBar::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   clear();

   node.getAttribValueAsString("name", mName);
   int childCount = node.getNumberChildren();
   for (int i = 0; i < childCount; ++i)
   {
      BXMLNode child(node.getChild(i));
      if (!child)
         continue;
      const BPackedString childName(child.getName());
      if (childName=="Texture")
      {
         child.getText(mTexture);
      }      
      else if (childName == "SizeX")
      {
         child.getTextAsFloat(mSizeX);
      }
      else if (childName == "SizeY")
      {
         child.getTextAsFloat(mSizeY);
      }
      else if (childName == "OffsetX")
      {
         child.getTextAsFloat(mOffsetX);
      }
      else if (childName == "OffsetY")
      {
         child.getTextAsFloat(mOffsetY);
      }            
   }
   return true;
}

//==============================================================================
// BProtoAnchor::load
//==============================================================================
bool BProtoAnchor::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   const BPackedString name(node.getName());

   if (!node.getAttribValueAsFloat("offsetX", mOffsetX))
      return false;
   if (!node.getAttribValueAsFloat("offsetY", mOffsetY))
      return false;

   return true;
}

//==============================================================================
// BProtoBobbleHead::load
//==============================================================================
bool BProtoBobbleHead::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   clear();

   node.getAttribValueAsString("name", mName);
   int childCount = node.getNumberChildren();
   for (int i = 0; i < childCount; ++i)
   {
      BXMLNode child(node.getChild(i));
      if (!child)
         continue;
      const BPackedString childName(child.getName());
      if (childName=="BackgroundTexture")
      {
         child.getText(mBackgroundTexture);
      }
      else if (childName=="VetTexture")
      {
         child.getText(mVetTexture);
      }
      else if (childName=="PortraitTexture")
      {
         child.getText(mPortraitTexture);
      }
      else if (childName == "PortraitSizeX")
      {
         child.getTextAsFloat(mPortraitSizeX);
      }
      else if (childName == "PortraitSizeY")
      {
         child.getTextAsFloat(mPortraitSizeY);
      }
      else if (childName == "PortraitOffsetX")
      {
         child.getTextAsFloat(mPortraitOffsetX);
      }
      else if (childName == "PortraitOffsetY")
      {
         child.getTextAsFloat(mPortraitOffsetY);
      }      
      else if (childName == "BackgroundSizeX")
      {
         child.getTextAsFloat(mBackgroundSizeX);
      }
      else if (childName == "BackgroundSizeY")
      {
         child.getTextAsFloat(mBackgroundSizeY);
      }
      else if (childName == "BackgroundOffsetX")
      {
         child.getTextAsFloat(mBackgroundOffsetX);
      }
      else if (childName == "BackgroundOffsetY")
      {
         child.getTextAsFloat(mBackgroundOffsetY);
      }
      else if (childName == "VetRadius")
      {
         child.getTextAsFloat(mVetRadius);
      }
      else if (childName == "VetAngleRange")
      {
         child.getTextAsFloat(mVetAngleRange);
      }
      else if (childName == "VetOffsetX")
      {
         child.getTextAsFloat(mVetOffsetX);
      }
      else if (childName == "VetOffsetY")
      {
         child.getTextAsFloat(mVetOffsetY);
      }
      else if (childName == "VetScaleX")
      {
         child.getTextAsFloat(mVetScaleX);
      }
      else if (childName == "VetScaleY")
      {
         child.getTextAsFloat(mVetScaleY);
      }
      else if (childName == "VetSkewX")
      {
         child.getTextAsFloat(mVetSkewX);
      }
      else if (childName == "VetSkewY")
      {
         child.getTextAsFloat(mVetSkewY);
      }      
      else if (childName == "VetAnchor")
      {
         int grandChildrenCount = child.getNumberChildren();
         mVetAnchors.resize(0);          
         for (int j = 0; j < grandChildrenCount; ++j)
         {
            BXMLNode grandchild(child.getChild(j));
            if (!grandchild)
               continue;

            BProtoAnchor anchor;           
            if (!anchor.load(grandchild, pReader))
               return false;

            mVetAnchors.pushBack(anchor);
         }
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoBuildingStrength::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   clear();

   node.getAttribValueAsString("name", mName);
   int childCount = node.getNumberChildren();
   for (int i = 0; i < childCount; ++i)
   {
      BXMLNode child(node.getChild(i));
      if (!child)
         continue;
      const BPackedString childName(child.getName());
      if (childName=="TextureOn")
      {
         child.getText(mTextureOn);
      }
      else if (childName=="TextureOff")
      {
         child.getText(mTextureOff);
      }
      else if (childName=="SizeX")
      {
         child.getTextAsFloat(mSizeX);
      }
      else if (childName=="SizeY")
      {
         child.getTextAsFloat(mSizeY);
      }
      else if (childName=="OffsetX")
      {
         child.getTextAsFloat(mOffsetX);
      }
      else if (childName=="OffsetY")
      {
         child.getTextAsFloat(mOffsetY);
      }
      else if (childName=="Padding")
      {
         child.getTextAsFloat(mPadding);
      }
      else if (childName == "Anchors")
      {
         int grandChildrenCount = child.getNumberChildren();
         mAnchors.resize(0);          
         for (int j = 0; j < grandChildrenCount; ++j)
         {
            BXMLNode grandchild(child.getChild(j));
            if (!grandchild)
               continue;

            BProtoAnchor anchor;           
            if (!anchor.load(grandchild, pReader))
               return false;

            mAnchors.pushBack(anchor);
         }
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BProtoHPBarColorStage::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;
  
   clear();
   
   BSimString text;
   if (node.getTextAsString(text))
      convertTokenToDWORDColor(text, mColor, 255);

   node.getAttribValueAsFloat("intensity", mIntensity);
   node.getAttribValueAsFloat("value", mPct);   

   return true;
}

//==============================================================================
//==============================================================================
bool BProtoHPBarColorStages::load(BXMLNode node, BXMLReader* pReader)
{
   if (!pReader)
      return false;

   if (!node.getAttribValueAsString("name", mName))
      return false;

   int childCount = node.getNumberChildren();
   for (int i = 0; i < childCount; ++i)
   {
      BXMLNode child(node.getChild(i));
      if (!child)
         continue;

      const BPackedString childName(child.getName());      
      if (childName=="Hitpoint")
      {         
         int numGrandChildren = child.getNumberChildren();
         BProtoHPBarColorStage stage;
         for (int i = 0; i < numGrandChildren; ++i)
         {
            BXMLNode grandChild(child.getChild(i));
            if (!stage.load(grandChild, pReader))
               return false;

            mHP.add(stage);
         }
      }
      else if (childName=="Ammo")
      {         
         int numGrandChildren = child.getNumberChildren();
         BProtoHPBarColorStage stage;
         for (int i = 0; i < numGrandChildren; ++i)
         {
            BXMLNode grandChild(child.getChild(i));
            if (!stage.load(grandChild, pReader))
               return false;

            mAmmo.add(stage);
         }
      }
      else if (childName == "Shield")
      {      
         int numGrandChildren = child.getNumberChildren();
         BProtoHPBarColorStage stage;
         for (int i = 0; i < numGrandChildren; ++i)
         {
            BXMLNode grandChild(child.getChild(i));
            if (!stage.load(grandChild, pReader))
               return false;

            mShield.add(stage);
         }
      }   
   }
   return true;
}

//==============================================================================
// BProtoHPBarColorStages::getHPStage
//==============================================================================
const BProtoHPBarColorStage* BProtoHPBarColorStages::getHPStage(float pct)
{
   int bestID = -1;
   for (int i = 0; i < mHP.getNumber(); i++)
   {  
      bestID = i;
      if (mHP[i].mPct >= pct)
         break;      
   }
   
   if (bestID == -1)
      return NULL;

   debugRangeCheck(bestID, mHP.getNumber());
   return &mHP[bestID];
}

//==============================================================================
// BProtoHPBarColorStages::getAmmoStage
//==============================================================================
const BProtoHPBarColorStage* BProtoHPBarColorStages::getAmmoStage(float pct)
{
   int bestID = -1;
   for (int i = 0; i < mAmmo.getNumber(); i++)
   {
      bestID = i;
      if (mAmmo[i].mPct >= pct)
         break;
   }

   if (bestID == -1)
      return NULL;

   debugRangeCheck(bestID, mAmmo.getNumber());
   return &mAmmo[bestID];
}

//==============================================================================
// BProtoHPBarColorStages::getShieldStage
//==============================================================================
const BProtoHPBarColorStage* BProtoHPBarColorStages::getShieldStage(float pct)
{
   int bestID = -1;
   for (int i = 0; i < mShield.getNumber(); i++)
   {
      bestID = i;
      if (mShield[i].mPct >= pct)
         break;
   }

   if (bestID == -1)
      return NULL;

   debugRangeCheck(bestID, mShield.getNumber());
   return &mShield[bestID];
}



//==============================================================================
// BHPBar::getHPBarPosition()
//==============================================================================
void BHPBar::getHPBarPosition(const BEntity &entity, BVector &position, float &width, float &height)
{
   // ajl 9/26/06 - Use the sim bounding box instead because the visual bounding box isn't very good for HP bar positions.
   // Also changed the width calculation to be based on the number of hitpoints instead of the size of the unit.
   // Also added min/max for both width and height.

   const BObject*  pObject = entity.getObject();
   if(!pObject)
   {
      position = cInvalidVector;
      width=0.0f;
      height=0.0f;
      return;
   }

   const BProtoObject* pProtoObject=pObject->getProtoObject();

   // Get width and height
   width = Math::Max(Math::Min(pProtoObject->getHitpoints() * cHPBarWidthMultiplier, cHPBarMaxWidth), cHPBarMinWidth);
   height = Math::Max(Math::Min(width * cHPBarMinWidthHeightRatio, cHPBarMaxHeight), cHPBarMinHeight);

   // Halwes - 1/26/2007 - This needs to be flushed out better.
   const BUnit* pUnit  = entity.getUnit();
   long   target = -1;
   // If unit does NOT have a targeted hit zone with a valid position
   if( !pUnit || !pUnit->getTargetedHitZone( target ) || !pUnit->getHitZonePosition( target, position ) ) 
   {
      // Get the position
      position = pObject->getSimCenter();
      position.y += pObject->getProtoObject()->getObstructionRadiusY() * 2.0f;

//-- FIXING PREFIX BUG ID 3001
      const BVisual* pVisual = pObject->getVisual();
//--
      long hpBarPoint = (pVisual ? pVisual->getPointHandle(cActionAnimationTrack, cVisualPointHitpointBar) : -1);
      if (hpBarPoint != -1)
      {
         if(pVisual->getPointPosition(cActionAnimationTrack, hpBarPoint, position))
            position += pObject->getInterpolatedPosition();
      }      
   }
}

//==============================================================================
// BHPBar::getSquadHPBarPosition()
//==============================================================================
void BHPBar::getSquadHPBarPosition(BSquad* pSquad, BVector& pos)
{   
   if (!pSquad)
   {
      pos = cInvalidVector;      
      return;
   }

   XMVECTOR avgPos = XMVectorZero();
   uint count = pSquad->getNumberChildren();
   for (uint i = 0; i < count; ++i)
   {
//-- FIXING PREFIX BUG ID 3002
      const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
//--
      if (!pUnit)
         continue;

      avgPos += pUnit->getInterpolatedPosition(); 
   }

   if (count > 1)
   {
      avgPos /= (float) count;
      avgPos.w = 1.0f;
   }

   pos = avgPos;

   const BProtoSquad* pProto = pSquad->getProtoSquad();
   if (pProto)
   {
      pos += pProto->getHPBarOffset();
   }
   else
   {
      pos.y += 5.0f;
   }
}

//==============================================================================
// BHPBar::getSquadBarStatus()
//==============================================================================
void BHPBar::getSquadBarStatus(BSquad* pSquad, float& hpPct, float& shieldPct, float& ammoPct)
{   
   hpPct = 1.0f;
   shieldPct = 1.0f;
   ammoPct = 1.0f;

   if (!pSquad)
      return;

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;

   BPlayer* pPlayer = pSquad->getPlayer();
   if (!pPlayer)
      return;

   float hpTotal = 0.0f;
   float shieldTotal = 0.0f;
   float ammoTotal = 0.0f;
   for (int i = 0 ; i < pProtoSquad->getNumberUnitNodes(); i++)
   {
      const BProtoSquadUnitNode& unitNode = pProtoSquad->getUnitNode(i);
      long unitCount = unitNode.mUnitCount;

//-- FIXING PREFIX BUG ID 3004
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(unitNode.mUnitType);
//--
      shieldTotal+= pProtoObject->getShieldpoints() * unitCount;
      hpTotal    += pProtoObject->getHitpoints() * unitCount;
      ammoTotal  += pProtoObject->getMaxAmmo() * unitCount;            
   }
         
   //-- compute the current HP's for the entire squad (the current life of the squad)
   //-- compute the current Total HP for the entire squad (the total HPs the squad has undamaaged)
   hpPct = 0.0f;
   shieldPct = 0.0f;
   ammoPct = 0.0f;
   
   float curHP   = 0.0f;
   float curShield = 0.0f;
   float curAmmo = 0.0f;
   
   int unitCount = pSquad->getNumberChildren();
   const BProtoObject* pProtoObject = NULL;
   for (int j = 0; j < unitCount; ++j)
   {
      BUnit *pUnit = gWorld->getUnit(pSquad->getChild(j));
      if (!pUnit)
         continue;

      pProtoObject = pUnit->getProtoObject();

      curHP += pUnit->getHitpoints();

      // Compute shield point pct      
      if(pUnit->getFlagHasShield())
      {                    
         curShield += pUnit->getShieldpoints();         
      }

      if (pUnit->getFlagUsesAmmo())
      {                
         curAmmo += pUnit->getAmmunition();         
      }
   }

   if (hpTotal >= cFloatCompareEpsilon)
      hpPct = curHP / hpTotal;

   if (shieldTotal >= cFloatCompareEpsilon)
      shieldPct = curShield / shieldTotal;

   if (ammoTotal >= cFloatCompareEpsilon)
      ammoPct = curAmmo / ammoTotal;
}

//==============================================================================
// Display a unit's HP
//==============================================================================
void BHPBar::getHPBarStatus(BUnit& unit, float& hpPct, float& shieldPct, float& ammoPct)
{
   const BProtoObject* pProto = unit.getProtoObject();

   // Get HP percentage
   float hpTotal      = 0.0f;
   long hitZoneIndex      = -1;
   bool hitZoneTarget     = unit.getTargetedHitZone( hitZoneIndex );
   bool hitZoneHasShields = false;
   if( hitZoneTarget )
   {
      hpTotal = unit.getHitZoneList()->get( hitZoneIndex ).getHitpoints();
      hpPct   = hpTotal / unit.getMaxHitZoneHitpoints( hitZoneIndex );         
      hitZoneHasShields = unit.getHitZoneList()->get( hitZoneIndex ).getHasShields();
   }
   else
   {
      long numEntityRefs = unit.getNumberEntityRefs();
      for (long i = numEntityRefs - 1; i >= 0; i--)
      {
         BEntityRef *pEntityRef = unit.getEntityRefByIndex(i);
         if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
         {
            BUnit *pUnit = gWorld->getUnit(pEntityRef->mID);
            if (pUnit)
               hpTotal += pUnit->getHitpoints();
         }
      }
      if (hpTotal > 0.0f)
         hpPct = hpTotal / unit.getMaxHPContained();
      else
         hpPct = unit.getHitpoints() / pProto->getHitpoints();
   }
      
   // Show shield points   
   if( unit.getFlagHasShield() || hitZoneHasShields )
   {        
      if( hitZoneHasShields )
      {
         shieldPct = unit.getHitZoneList()->get( hitZoneIndex ).getShieldpoints() / unit.getMaxHitZoneShieldpoints( hitZoneIndex );            
      }
      else
      {
         shieldPct = unit.getShieldpoints() / pProto->getShieldpoints();
      }      
   }

   if (unit.getFlagUsesAmmo())
   {      
      ammoPct = unit.getAmmunition() / unit.getProtoObject()->getMaxAmmo();      
   }
}

//==============================================================================
//==============================================================================
void BHPBar::getBarColors(int playerID, float hpPct, float shieldPct, float ammoPct, DWORD& hpColor, DWORD& shieldColor, DWORD& ammoColor)
{
   hpColor = cDWORDBlack;
   shieldColor = cDWORDBlack;
   ammoColor = cDWORDBlack;

//-- FIXING PREFIX BUG ID 3008
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   pPlayer;
   BProtoHPBarColorStages* pStages = gDatabase.getProtoHPColorStage(mColorStageID[eColorStageIDDefault]);
   if (!pStages)
      return;

   const BProtoHPBarColorStage* pHPStage = pStages->getHPStage(hpPct);
   const BProtoHPBarColorStage* pShieldStage = pStages->getShieldStage(shieldPct);
   const BProtoHPBarColorStage* pAmmoStage = pStages->getAmmoStage(ammoPct);
   if (pHPStage)
      hpColor = pHPStage->mColor;
   if (pShieldStage)
      shieldColor = pShieldStage->mColor;
   if (pAmmoStage)
      ammoColor = pAmmoStage->mColor;   
}

//==============================================================================
// Display a unit's HP
//==============================================================================
void BHPBar::displayHP(BUnit &unit)
{   
   if (gGame.isSplitScreen())
   {
      if ((!unit.isVisible(gUserManager.getPrimaryUser()->getTeamID())) && (!unit.isVisible(gUserManager.getSecondaryUser()->getTeamID())))
         return;
   }
   else if (!unit.isVisible(gUserManager.getPrimaryUser()->getTeamID()))
      return;

   const BProtoObject* pProto = unit.getProtoObject();           

   int colorPlayerID = unit.getColorPlayerID();
   BSquadHPVertex vertex;   
   D3DCOLOR playerColor = gWorld->getPlayerColor(colorPlayerID, BWorld::cPlayerColorContextSelection);

   BVector position;
   float width, height;
   getHPBarPosition(unit, position, width, height);

   float hpPct, shieldPct, ammoPct;
   getHPBarStatus(unit, hpPct, shieldPct, ammoPct);

   D3DCOLOR hpColor, shieldColor, ammoColor;
   getBarColors(colorPlayerID, hpPct, shieldPct, ammoPct, hpColor, shieldColor, ammoColor);

   //-- compute the actual uv offset for the status in regards to the visual width of the bars   
   XMVECTOR statusHP;
   XMVECTOR statusShield;
   XMVECTOR statusAmmo;
//-- FIXING PREFIX BUG ID 3009
   const BProtoHPBar* hpProto = gDatabase.getProtoHPBar(pProto->getHPBarID());
//--
   if (!hpProto)
      return;

   // this assumes that each hp bar has always the same number of masks and diffuse textures
   float oneOverStageCount = 1.0f / gDatabase.getNumberProtoHPBars();
   float textureArrayZ = (0.49999f * oneOverStageCount) + (hpProto->mDiffuseIndex * oneOverStageCount);

   //-- store the texture array lookup in the z component of the position
   position.w = textureArrayZ;

   statusHP = XMVectorLerp(XMLoadScalar(&hpProto->mHPUV.x), XMLoadScalar(&hpProto->mHPUV.z), hpPct);
   statusShield = XMVectorLerp(XMLoadScalar(&hpProto->mShieldUV.x), XMLoadScalar(&hpProto->mShieldUV.z), shieldPct);
   statusAmmo = XMVectorLerp(XMLoadScalar(&hpProto->mAmmoUV.x), XMLoadScalar(&hpProto->mAmmoUV.z), ammoPct);

   XMVECTOR status = XMVectorZero();
   status = __vrlimi(status, statusHP,     VRLIMI_CONST(1,0,0,0), 0); // store in x
   status = __vrlimi(status, statusShield, VRLIMI_CONST(0,1,0,0), 3); // store in y
   status = __vrlimi(status, statusAmmo,   VRLIMI_CONST(0,0,1,0), 2); // store in z

   XMStoreFloat4NC(&vertex.mPos, (XMVECTOR) position);
   XMStoreHalf4(&vertex.mHPDimension, (XMVECTOR) hpProto->mHPUV);
   XMStoreHalf4(&vertex.mShieldDimension, (XMVECTOR) hpProto->mShieldUV);
   XMStoreHalf4(&vertex.mAmmoDimension, (XMVECTOR) hpProto->mAmmoUV);
   XMStoreHalf4(&vertex.mStatus, status);

   float sizeX = pProto->getHPBarSizeX() > cFloatCompareEpsilon ? pProto->getHPBarSizeX() : hpProto->mSizeX;
   float sizeY = pProto->getHPBarSizeY() > cFloatCompareEpsilon ? pProto->getHPBarSizeY() : hpProto->mSizeY;   
   vertex.mSize  = XMHALF2(sizeX, sizeY);
   vertex.mColor = playerColor;
   vertex.mHPColor = hpColor;
   vertex.mShieldColor = shieldColor;
   vertex.mAmmoColor = ammoColor;

   mSquadBarArray.pushBack(vertex); 
}
//==============================================================================
// BHPBar::displayBuildingStrength()
//==============================================================================
void BHPBar::displayBuildingStrength(BSquad* pSquad)
{
   if (!pSquad)
      return;

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;

   int buildingStrengthID = pProtoSquad->getBuildingStrengthID();
   if (buildingStrengthID == -1)
      return;

   BProtoBuildingStrength* pProtoBuildingStrength = gDatabase.getProtoBuildingStrength(buildingStrengthID);
   if (!pProtoBuildingStrength)
      return;

   if (pProtoBuildingStrength->mTextureOffHandle == cInvalidManagedTextureHandle)
      return;

   if (pProtoBuildingStrength->mTextureOnHandle == cInvalidManagedTextureHandle)
      return;   

//-- FIXING PREFIX BUG ID 3005
   const BUnit* pUnit = pSquad->getLeaderUnit();
//--
   if (!pUnit)
      return;

   int aliveCount = 0;
   uint numRefs = pUnit->getNumberEntityRefs();
   for (uint i=0; i<numRefs; i++)
   {
      BEntityRef* pRef = pUnit->getEntityRefByIndex(i);
      if (pRef->mType == BEntityRef::cTypeAssociatedBuilding)
      {
         BUnit* pChild = gWorld->getUnit(pRef->mID);
         if (pChild && pChild->isAlive() && pChild->getFlagBuilt() && pChild->getProtoObject()->getFlagChildForDamageTakenScalar())
         {
            aliveCount++;            
         }
      }
   }            

   BVector position;
   BMatrix matrix;   
   matrix = XMMatrixIdentity();
   getSquadHPBarPosition(pSquad, position);   
   DWORD color = gWorld->getPlayerColor(pUnit->getPlayerID(), BWorld::cPlayerColorContextSelection);

   static DWORD customColor = 0xFFFFFFFF;
   static bool bUseCustomColor = false;
   if (bUseCustomColor)
      color = customColor;
   
   const int maxSprites = 7;      

   

   float offsetX = pProtoBuildingStrength->mOffsetX;
   float offsetY = pProtoBuildingStrength->mOffsetY;
   float sizeX   = pProtoBuildingStrength->mSizeX; 
   float sizeY   = pProtoBuildingStrength->mSizeY;   
   float padding = pProtoBuildingStrength->mPadding;
   float highlightScalarX = 0.7f;
   float highlightScalarY = 0.7f;

#if 0
   static bool bUseStatics = false;
   static float staticOffsetX = -10.25;
   static float staticOffsetY = 1.75;
   static float staticSizeX = 4.25;
   static float staticSizeY = 4.25;
   static float staticPadding = 2.25f;
   if (bUseStatics)
   {
      offsetX = staticOffsetX;
      offsetY = staticOffsetY;
      sizeX = staticSizeX;
      sizeY = staticSizeY;
      padding = staticPadding;
   }
#endif

   // Calculate how many child buildings we have
   BManagedTextureHandle tex1 = pProtoBuildingStrength->mTextureOnHandle;
   BManagedTextureHandle tex2 = pProtoBuildingStrength->mTextureOffHandle;
   BManagedTextureHandle textureHandle = cInvalidManagedTextureHandle;

   for (int j = 0; j < maxSprites; ++j)
   {            
      matrix.r[3] = position;

      if (j<aliveCount)
      {
         textureHandle = tex1;
         gUIManager->addSprite(matrix, textureHandle, color, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, sizeX, sizeY, offsetX, offsetY, 250, -1.0f);
         gUIManager->addSprite(matrix, textureHandle, cDWORDWhite, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, sizeX*highlightScalarX, sizeY*highlightScalarY, offsetX, offsetY, 255, -1.0f);
         
      }
      else
      {
         textureHandle = tex2;
         gUIManager->addSprite(matrix, textureHandle, color, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, sizeX, sizeY, offsetX, offsetY, 255, -1.0f);
      }         
      
      offsetX += padding;
   }   
}

//==============================================================================
// BHPBar::displaySquadHP()
//==============================================================================
void BHPBar::displaySquadHP(BSquad* pSquad)
{   
   if (!pSquad)
      return;
      
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;

   BSquadHPVertex vertex;   
   int colorPlayerID = pSquad->getColorPlayerID();
   D3DCOLOR playerColor      = gWorld->getPlayerColor(colorPlayerID, BWorld::cPlayerColorContextSelection);
   BVector position;      
   getSquadHPBarPosition(pSquad, position);   
   float hpPct, shieldPct, ammoPct;
   getSquadBarStatus(pSquad, hpPct, shieldPct, ammoPct);

   D3DCOLOR hpColor, shieldColor, ammoColor;
   getBarColors(colorPlayerID, hpPct, shieldPct, ammoPct, hpColor, shieldColor, ammoColor);
   
   //-- compute the actual uv offset for the status in regards to the visual width of the bars   
   XMVECTOR statusHP;
   XMVECTOR statusShield;
   XMVECTOR statusAmmo;
//-- FIXING PREFIX BUG ID 3010
   const BProtoHPBar* hpProto = gDatabase.getProtoHPBar(pProtoSquad->getHPBarID());
//--
   if (!hpProto)
      return;
   
   // this assumes that each hp bar has always the same number of masks and diffuse textures
   float oneOverStageCount = 1.0f / gDatabase.getNumberProtoHPBars();
   float textureArrayZ = (0.49999f * oneOverStageCount) + (hpProto->mDiffuseIndex * oneOverStageCount);

   //-- store the texture array lookup in the z component of the position
   position.w = textureArrayZ;

   statusHP = XMVectorLerp(XMLoadScalar(&hpProto->mHPUV.x), XMLoadScalar(&hpProto->mHPUV.z), hpPct);
   statusShield = XMVectorLerp(XMLoadScalar(&hpProto->mShieldUV.x), XMLoadScalar(&hpProto->mShieldUV.z), shieldPct);
   statusAmmo = XMVectorLerp(XMLoadScalar(&hpProto->mAmmoUV.x), XMLoadScalar(&hpProto->mAmmoUV.z), ammoPct);

   XMVECTOR status = XMVectorZero();
   status = __vrlimi(status, statusHP,     VRLIMI_CONST(1,0,0,0), 0); // store in x
   status = __vrlimi(status, statusShield, VRLIMI_CONST(0,1,0,0), 3); // store in y
   status = __vrlimi(status, statusAmmo,   VRLIMI_CONST(0,0,1,0), 2); // store in z
   
   XMStoreFloat4NC(&vertex.mPos, (XMVECTOR) position);
   XMStoreHalf4(&vertex.mHPDimension, (XMVECTOR) hpProto->mHPUV);
   XMStoreHalf4(&vertex.mShieldDimension, (XMVECTOR) hpProto->mShieldUV);
   XMStoreHalf4(&vertex.mAmmoDimension, (XMVECTOR) hpProto->mAmmoUV);
   XMStoreHalf4(&vertex.mStatus, status);

   float sizeX = pProtoSquad->getHPBarSizeX() > cFloatCompareEpsilon ? pProtoSquad->getHPBarSizeX() : hpProto->mSizeX;
   float sizeY = pProtoSquad->getHPBarSizeY() > cFloatCompareEpsilon ? pProtoSquad->getHPBarSizeY() : hpProto->mSizeY;   
   vertex.mSize  = XMHALF2(sizeX, sizeY);   
   vertex.mColor = playerColor;
   vertex.mHPColor = hpColor;
   vertex.mShieldColor = shieldColor;
   vertex.mAmmoColor = ammoColor;

   mSquadBarArray.pushBack(vertex);    

   displayBuildingStrength(pSquad);
}

//==============================================================================
// void BHPBar::displayVeterancy()
//==============================================================================
void BHPBar::displayVeterancy(BSquad* pSquad, bool bCentered)
{
   if (!pSquad)
      return;
      
//-- FIXING PREFIX BUG ID 3006
   const BUnit* pUnit=pSquad->getLeaderUnit();
//--
   if (!pUnit)
      return;
         
   if (pSquad->getVetLevel() <= 0)
      return;

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;

   int vetID = pProtoSquad->getVeterancyBarID();

   if (bCentered)
      vetID = pProtoSquad->getVeterancyCenteredBarID();


   if (vetID < 0)
      return;

//-- FIXING PREFIX BUG ID 3012
   const BProtoVeterancyBar* pProtoVetBar = gDatabase.getProtoVeterancyBar(vetID);
//--
   if (!pProtoVetBar)
      return;
            
   BVector position;
   BMatrix matrix;
   matrix = XMMatrixIdentity();
   getSquadHPBarPosition(pSquad, position);         
   DWORD color = gWorld->getPlayerColor(pSquad->getPlayerID(), BWorld::cPlayerColorContextSelection);
   
   static float startOffsetX = 0.0f;
   static float startOffsetY = 1.5f;
   static float padding = 1.2f;

   float offsetX = pProtoVetBar->mOffsetX;
   float offsetY = pProtoVetBar->mOffsetY;   
   float sizeX   = pProtoVetBar->mSizeX;
   float sizeY   = pProtoVetBar->mSizeY;
   if (!pSquad->getFlagCloaked() || pSquad->getFlagCloakDetected())
   {        
      int count = pSquad->getVetLevel();
      for (int i = 0; i < count; ++i)
      {
         matrix.r[3] = position;
         
         gUIManager->addSprite(matrix, pProtoVetBar->mTextureHandle, color, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, sizeX, sizeY, offsetX, offsetY, 0, -1.0f);
         offsetX += padding;
      }         
    }
}

//==============================================================================
//==============================================================================
void BHPBar::displayRechargeProgress(BSquad* pSquad, bool bCentered)
{
   if (!pSquad)
      return;

   if (!pSquad->getFlagRecovering())
      return;

   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;

   int recoveryID = pProtoSquad->getAbilityRecoveryBarID();

   if (bCentered)
      recoveryID = pProtoSquad->getAbilityRecoveryCenteredBarID();
   
   if (recoveryID < 0)
      return;

//-- FIXING PREFIX BUG ID 3013
   const BProtoPieProgress* pProtoPie = gDatabase.getProtoPieProgressBar(recoveryID);
//--
   if (!pProtoPie)
      return;

   BMatrix matrix;
   
   BVector position;      
   getSquadHPBarPosition(pSquad, position);         
   matrix = XMMatrixIdentity();
   matrix.r[3] = position;

//-- FIXING PREFIX BUG ID 3014
   const BUnit* pLeader = pSquad->getLeaderUnit();
//--
   if (!pLeader)
      return;

   const BProtoObject* pProtoObject = pLeader->getProtoObject();
   if (!pProtoObject)
      return;
   
   int abilityID = pProtoObject->getAbilityCommand();
//-- FIXING PREFIX BUG ID 3015
   const BAbility* pAbility = gDatabase.getAbilityFromID(abilityID);
//--
   if (!pAbility)
      return;

   float duration = pAbility->getRecoverTime();
   if (duration <= cFloatCompareEpsilon)
      return;

   float curRecoverTime = pSquad->getRecoverTime();   
   float alpha = curRecoverTime / duration;
   alpha = Math::Clamp(alpha, 0.0f, 1.0f);
   
   float backgroundOffsetX = pProtoPie->mBackgroundOffsetX;
   float backgroundOffsetY = pProtoPie->mBackgroundOffsetY;
   float backgroundScale   = pProtoPie->mBackgroundSizeX;
   
   bool bFill = pProtoPie->mFill;
   bool bClockwise = pProtoPie->mClockwise;
   float pieOffsetX = pProtoPie->mOffsetX;
   float pieOffsetY = pProtoPie->mOffsetY;
   float pieScale   = pProtoPie->mSizeX;

   DWORD playercolor = gWorld->getPlayerColor(pSquad->getPlayerID(), BWorld::cPlayerColorContextSelection);

   gUIManager->addSprite(matrix, pProtoPie->mBackgroundTextureHandle, playercolor, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, backgroundScale, backgroundScale, backgroundOffsetX, backgroundOffsetY, 0, -1.0f);
   gUIManager->addPieProgress(matrix, pProtoPie->mMaskTextureHandle, cDWORDYellow, alpha, pieScale, pieScale, pieOffsetX, -pieOffsetY, bFill, bClockwise, 1, -1.0f);
}

//==============================================================================
//==============================================================================
void BHPBar::displayBobbleHead(BSquad* pSquad)
{
   if (!pSquad)
      return;   
      
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;
  
   int bobbleID = pProtoSquad->getBobbleHeadID();       
   if (bobbleID < 0)
      return;

   BProtoBobbleHead* pProtoBobble = gDatabase.getProtoBobbleHead(bobbleID);
   if (!pProtoBobble)
      return;

   BMatrix matrix;
   BVector position;      
   getSquadHPBarPosition(pSquad, position);
   int squadLevel = pSquad->getTechPlusVetLevel();      

   // if the squad is spartan that is contained in another unit then we need to get the tech and vet level from the
   // the unit that contains him
   if (pSquad->getFlagContainedSpartan())
   {
      BUnit* pContainer = pSquad->getSpartanContainerUnit();
      if (pContainer)
      {
         BSquad* pContainerSquad = pContainer->getParentSquad();
         if (pContainerSquad)
         {
            squadLevel = pContainerSquad->getTechPlusVetLevel();
            getSquadHPBarPosition(pContainerSquad, position);
         }
      }
   }
   
   matrix = XMMatrixIdentity();
   matrix.r[3] = position;

   DWORD playercolor = gWorld->getPlayerColor(pSquad->getPlayerID(), BWorld::cPlayerColorContextSelection);
      
   float backgroundOffsetX = pProtoBobble->mBackgroundOffsetX;
   float backgroundOffsetY = pProtoBobble->mBackgroundOffsetY;
   float backgroundScaleX  = pProtoBobble->mBackgroundSizeX;
   float backgroundScaleY  = pProtoBobble->mBackgroundSizeY;
      
   float portraitOffsetX = pProtoBobble->mPortraitOffsetX;
   float portraitOffsetY = pProtoBobble->mPortraitOffsetY;
   float portraitScaleX  = pProtoBobble->mPortraitSizeX;
   float portraitScaleY  = pProtoBobble->mPortraitSizeY;
   
   if (!pSquad->getFlagCloaked() || pSquad->getFlagCloakDetected())
   {  
      const int cMaxStarsCount = 14;
      const int cMaxHalfStarsCount = cMaxStarsCount / 2;
                              
      float starScaleX = pProtoBobble->mVetScaleX;
      float starScaleY = pProtoBobble->mVetScaleY;
      float overallOffsetX = pProtoBobble->mVetOffsetX;
      float overallOffsetY = pProtoBobble->mVetOffsetY;
      
      float skewScaleX = pProtoBobble->mVetSkewX;
      float skewScaleY = pProtoBobble->mVetSkewY;      
      float radius  = pProtoBobble->mVetRadius; //0.75f;          
      float angleRange = pProtoBobble->mVetAngleRange; //90.0;
      float angleIncrement = (angleRange / cMaxHalfStarsCount);
      float angleStartOffset = ((180.0f - angleRange) * 0.5f) + (angleIncrement * 0.5f);

      float offsetX = 0.0f;
      float offsetY = 0.0f;
      float angleRadians = angleIncrement * cRadiansPerDegree;
      float angleStartOffsetRadians = angleStartOffset * cRadiansPerDegree;            
      float sinT, cosT;
      float curAngle = -XM_PI;
      for (int i = 0; i < squadLevel; ++i)
      {
         matrix.r[3] = position;         
         
         if (i >= cMaxHalfStarsCount)
            curAngle = angleStartOffsetRadians + ((i-cMaxHalfStarsCount) * angleRadians);
         else
            curAngle = -XM_PI + angleStartOffsetRadians + (i * angleRadians);

         curAngle = Math::Clamp(curAngle, -XM_PI, XM_PI-cFloatCompareEpsilon);

         XMScalarSinCosEst(&sinT, &cosT, curAngle);

         offsetX = overallOffsetX + (sinT * radius * skewScaleX);
         offsetY = overallOffsetY + (cosT * radius * skewScaleY);                  
         gUIManager->addSprite(matrix, pProtoBobble->mVetTextureHandle, playercolor, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, starScaleX, starScaleY, offsetX, offsetY, 1, -1.0f);         
      }         
    }
   
   // add background
   gUIManager->addSprite(matrix, pProtoBobble->mBackgroundTextureHandle, playercolor, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, backgroundScaleX, backgroundScaleY, backgroundOffsetX, backgroundOffsetY, 0, -1.0f);   
   gUIManager->addSprite(matrix, pProtoBobble->mPortraitTextureHandle, cDWORDWhite, BRender2DPrimitiveUtility::eRender2DBlendMode_Alphablend, portraitScaleX, portraitScaleY, portraitOffsetX, portraitOffsetY, 2, -1.0f);
}

//==============================================================================
// Prep for next game update
//==============================================================================
//-- FIXING PREFIX BUG ID 3011
void BHPBar::displayProgress(const BUnit* pUnit, float value)
//--
{
   if (!pUnit)
      return;   

   const BProtoObject* pProto = pUnit->getProtoObject();

   int colorPlayerID = pUnit->getColorPlayerID();
   BSquadHPVertex vertex;   
   D3DCOLOR playerColor = gWorld->getPlayerColor(colorPlayerID, BWorld::cPlayerColorContextSelection);

   BVector position;
   float width, height;
   getHPBarPosition(*pUnit, position, width, height);

   float hpPct = value;
   float shieldPct = 0.0f; 
   float ammoPct = 0.0f;
   
   D3DCOLOR hpColor, shieldColor, ammoColor;
   getBarColors(colorPlayerID, hpPct, shieldPct, ammoPct, hpColor, shieldColor, ammoColor);

   //-- compute the actual uv offset for the status in regards to the visual width of the bars   
   XMVECTOR statusHP;
   XMVECTOR statusShield;
   XMVECTOR statusAmmo;
//-- FIXING PREFIX BUG ID 3017
   const BProtoHPBar* hpProto = gDatabase.getProtoHPBar(pProto->getHPBarID());
//--
   if (!hpProto)
      return;

   // this assumes that each hp bar has always the same number of masks and diffuse textures
   float oneOverStageCount = 1.0f / gDatabase.getNumberProtoHPBars();
   float textureArrayZ = (0.49999f * oneOverStageCount) + (hpProto->mDiffuseIndex * oneOverStageCount);

   //-- store the texture array lookup in the z component of the position
   position.w = textureArrayZ;

   statusHP = XMVectorLerp(XMLoadScalar(&hpProto->mHPUV.x), XMLoadScalar(&hpProto->mHPUV.z), hpPct);
   statusShield = XMVectorLerp(XMLoadScalar(&hpProto->mShieldUV.x), XMLoadScalar(&hpProto->mShieldUV.z), shieldPct);
   statusAmmo = XMVectorLerp(XMLoadScalar(&hpProto->mAmmoUV.x), XMLoadScalar(&hpProto->mAmmoUV.z), ammoPct);

   XMVECTOR status = XMVectorZero();
   status = __vrlimi(status, statusHP,     VRLIMI_CONST(1,0,0,0), 0); // store in x
   status = __vrlimi(status, statusShield, VRLIMI_CONST(0,1,0,0), 3); // store in y
   status = __vrlimi(status, statusAmmo,   VRLIMI_CONST(0,0,1,0), 2); // store in z

   XMStoreFloat4NC(&vertex.mPos, (XMVECTOR) position);
   XMStoreHalf4(&vertex.mHPDimension, (XMVECTOR) hpProto->mHPUV);
   XMStoreHalf4(&vertex.mShieldDimension, (XMVECTOR) hpProto->mShieldUV);
   XMStoreHalf4(&vertex.mAmmoDimension, (XMVECTOR) hpProto->mAmmoUV);
   XMStoreHalf4(&vertex.mStatus, status);

   float sizeX = pProto->getHPBarSizeX() > cFloatCompareEpsilon ? pProto->getHPBarSizeX() : hpProto->mSizeX;
   float sizeY = pProto->getHPBarSizeY() > cFloatCompareEpsilon ? pProto->getHPBarSizeY() : hpProto->mSizeY;   
   vertex.mSize  = XMHALF2(sizeX, sizeY);
   vertex.mColor = playerColor;
   vertex.mHPColor = hpColor;
   vertex.mShieldColor = shieldColor;
   vertex.mAmmoColor = ammoColor;

   mSquadBarArray.pushBack(vertex); 
}

//==============================================================================
// Prep for next game update
//==============================================================================
void BHPBar::displayProgress(BSquad* pSquad, float value)
{
   if (!pSquad)
      return;   
   
   const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
   if (!pProtoSquad)
      return;

   BSquadHPVertex vertex;   
   int colorPlayerID = pSquad->getColorPlayerID();
   D3DCOLOR playerColor      = gWorld->getPlayerColor(colorPlayerID, BWorld::cPlayerColorContextSelection);
   BVector position;      
   getSquadHPBarPosition(pSquad, position);   

   float hpPct = value;
   float shieldPct = 0.0f; 
   float ammoPct = 0.0f;   

   D3DCOLOR hpColor, shieldColor, ammoColor;
   getBarColors(colorPlayerID, hpPct, shieldPct, ammoPct, hpColor, shieldColor, ammoColor);
   
   //-- compute the actual uv offset for the status in regards to the visual width of the bars   
   XMVECTOR statusHP;
   XMVECTOR statusShield;
   XMVECTOR statusAmmo;
//-- FIXING PREFIX BUG ID 3018
   const BProtoHPBar* hpProto = gDatabase.getProtoHPBar(pProtoSquad->getHPBarID());
//--
   if (!hpProto)
      return;

   // this assumes that each hp bar has always the same number of masks and diffuse textures
   float oneOverStageCount = 1.0f / gDatabase.getNumberProtoHPBars();
   float textureArrayZ = (0.49999f * oneOverStageCount) + (hpProto->mDiffuseIndex * oneOverStageCount);

   //-- store the texture array lookup in the z component of the position
   position.w = textureArrayZ;

   statusHP = XMVectorLerp(XMLoadScalar(&hpProto->mHPUV.x), XMLoadScalar(&hpProto->mHPUV.z), hpPct);
   statusShield = XMVectorLerp(XMLoadScalar(&hpProto->mShieldUV.x), XMLoadScalar(&hpProto->mShieldUV.z), shieldPct);
   statusAmmo = XMVectorLerp(XMLoadScalar(&hpProto->mAmmoUV.x), XMLoadScalar(&hpProto->mAmmoUV.z), ammoPct);

   XMVECTOR status = XMVectorZero();
   status = __vrlimi(status, statusHP,     VRLIMI_CONST(1,0,0,0), 0); // store in x
   status = __vrlimi(status, statusShield, VRLIMI_CONST(0,1,0,0), 3); // store in y
   status = __vrlimi(status, statusAmmo,   VRLIMI_CONST(0,0,1,0), 2); // store in z
   
   XMStoreFloat4NC(&vertex.mPos, (XMVECTOR) position);
   XMStoreHalf4(&vertex.mHPDimension, (XMVECTOR) hpProto->mHPUV);
   XMStoreHalf4(&vertex.mShieldDimension, (XMVECTOR) hpProto->mShieldUV);
   XMStoreHalf4(&vertex.mAmmoDimension, (XMVECTOR) hpProto->mAmmoUV);
   XMStoreHalf4(&vertex.mStatus, status);

   float sizeX = pProtoSquad->getHPBarSizeX() > cFloatCompareEpsilon ? pProtoSquad->getHPBarSizeX() : hpProto->mSizeX;
   float sizeY = pProtoSquad->getHPBarSizeY() > cFloatCompareEpsilon ? pProtoSquad->getHPBarSizeY() : hpProto->mSizeY;   
   vertex.mSize  = XMHALF2(sizeX, sizeY);   
   vertex.mColor = playerColor;
   vertex.mHPColor = hpColor;
   vertex.mShieldColor = shieldColor;
   vertex.mAmmoColor = ammoColor;

   mSquadBarArray.pushBack(vertex); 
}

//==============================================================================
// Prep for next game update
//==============================================================================
void BHPBar::prep(void)
{
   ASSERT_MAIN_THREAD
   SCOPEDSAMPLE(HPBarPrep);

   mHPBarArray.clear();
   mSquadBarArray.clear();
}

//==============================================================================
// Render world visibility
//==============================================================================
void BHPBar::render(int viewPortIndex, bool bSplitScreen)
{
   ASSERT_MAIN_THREAD

   long numPrimitives = mHPBarArray.size();

   if (numPrimitives && mBPTCVertexDecl && !gConfig.isDefined(cConfigNoHPBar))
   {
      BPTCVertex  *HPBarVB = reinterpret_cast<BPTCVertex *>(gRenderDraw.lockDynamicVB(numPrimitives, sizeof(BPTCVertex)));
         memcpy(HPBarVB, mHPBarArray.getData(), numPrimitives * sizeof(BPTCVertex));
         IDirect3DVertexBuffer9* pD3DVB = gRenderDraw.getDynamicVB();
      gRenderDraw.unlockDynamicVB();

      // Begin packet
      BRenderHPBarPacket *packet = reinterpret_cast<BRenderHPBarPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandRender, sizeof(BRenderHPBarPacket)));
         // Set packet
         packet->numPrimitives = numPrimitives;
         packet->HPBarVB = pD3DVB;
         packet->mViewportIndex = viewPortIndex;
         packet->mSplitScreen = bSplitScreen;
      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BRenderHPBarPacket));
   }

   int numSquadBars = mSquadBarArray.size();
   if ((numSquadBars > 0) && mSquadVertexDecl && !gConfig.isDefined(cConfigNoHPBar))
   {
      BSquadHPVertex* pVB = reinterpret_cast<BSquadHPVertex *>(gRenderDraw.lockDynamicVB(numSquadBars, sizeof(BSquadHPVertex)));
      memcpy(pVB, mSquadBarArray.getData(), numSquadBars * sizeof(BSquadHPVertex));
      IDirect3DVertexBuffer9* pD3DVB = gRenderDraw.getDynamicVB();
      gRenderDraw.unlockDynamicVB();

      // Begin packet
      BRenderSquadBarPacket *packet = reinterpret_cast<BRenderSquadBarPacket *>(gRenderThread.submitCommandBegin(mCommandHandle, cMMCommandRenderSquadBars, sizeof(BRenderSquadBarPacket)));
      // Set packet
      packet->mSquadBarVB = pD3DVB;
      packet->numPrimitives = numSquadBars;
      packet->mViewportIndex = viewPortIndex;
      packet->mSplitScreen = bSplitScreen;

      // Send packet
      gRenderThread.submitCommandEnd(sizeof(BRenderSquadBarPacket));
   }
}

//==============================================================================
// Process a worker thread command
//==============================================================================
void BHPBar::processCommand(const BRenderCommandHeader &header, const uchar *pData)
{
   ASSERT_RENDER_THREAD

   switch (header.mType)
   {
      case cMMCommandRender:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderHPBarPacket));
         workerRender(reinterpret_cast<const BRenderHPBarPacket *>(pData));
         break;
      }

      case cMMCommandRenderSquadBars:
      {
         BDEBUG_ASSERT(header.mLen == sizeof(BRenderSquadBarPacket));
         workerRender(reinterpret_cast<const BRenderSquadBarPacket *>(pData));
         break;
      }
   }
}

//==============================================================================
// Process frameBegin
//==============================================================================
void BHPBar::frameBegin(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// Process frameEnd
//==============================================================================
void BHPBar::frameEnd(void)
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// Process deinitDeviceData
//==============================================================================
void BHPBar::deinitDeviceData()
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// Process initDeviceData
//==============================================================================
void BHPBar::initDeviceData()
{
   ASSERT_RENDER_THREAD
}

//==============================================================================
// Process received events
//==============================================================================
bool BHPBar::receiveEvent(const BEvent &event, BThreadIndex threadIndex)
{
   switch (event.mEventClass)
   {
      case cEventClassClientAdded:
      {
         ASSERT_RENDER_THREAD
         loadEffect();

         BString filename;
         eFileManagerError result = gFileManager.getDirListEntry(filename, gRender.getEffectCompilerDefaultDirID());
         BVERIFY(cFME_SUCCESS == result);
         strPathAddBackSlash(filename);
         filename += "HPBar\\HPBar.bin";

         BReloadManager::BPathArray paths;
         paths.pushBack(filename);
         gReloadManager.registerClient(paths, BReloadManager::cFlagSynchronous, mEventHandle, eEventClassReloadEffects);

         break;
      }

      case cEventClassClientRemove:
      {
         ASSERT_RENDER_THREAD
         killEffect();
         gReloadManager.deregisterClient(mEventHandle);

         break;
      }

      case cEventClassAsyncFile:
      {
         ASSERT_RENDER_THREAD
         killEffect();
         setupEffect(event);

         break;
      }

      case eEventClassReloadEffects:
      {
         ASSERT_RENDER_THREAD
         loadEffect();

         break;
      }
   }

   return false;
}

//==============================================================================
// Load effect
//==============================================================================
void BHPBar::loadEffect(void)
{
   ASSERT_RENDER_THREAD

   BAsyncFileManager::BRequestPacket   *packet = gAsyncFileManager.newRequestPacket();
   packet->setDirID(gRender.getEffectCompilerDefaultDirID());
   packet->setFilename("HPBar\\HPBar.bin");
   packet->setReceiverHandle(mEventHandle);
   packet->setSynchronousReply(true);
   packet->setDiscardOnClose(true);
   gAsyncFileManager.submitRequest(packet);
}

//==============================================================================
// Initialize effect
//==============================================================================
void BHPBar::setupEffect(const BEvent &event)
{
   ASSERT_RENDER_THREAD

   BAsyncFileManager::BRequestPacket *packet = reinterpret_cast<BAsyncFileManager::BRequestPacket *>(event.mpPayload);

   if (!packet->getSucceeded())
   {
      gConsoleOutput.output(cMsgError, "BHPBar::receiveEvent: Async load of file %s failed", packet->getFilename().c_str());
   }
   else
   {
      // rg [6/7/06] - Turn off param validation, for now
      HRESULT hres = mEffect.createFromCompiledData(BD3D::mpDev, packet->getData(), NULL, false);
      if (FAILED(hres))
      {
         gConsoleOutput.output(cMsgError, "BHPBar::receiveEvent: Effect creation of file %s failed", packet->getFilename().c_str());
      }

      // Get techniques and params
      mRenderHPBarTechnique = mEffect.getTechnique("renderHPBar");

      // Create vertex decl
      const D3DVERTEXELEMENT9 PTC_vertexElements[] =
      {
         { 0, 0,  D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
         { 0, 16, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
         { 0, 20, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0 },
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(PTC_vertexElements, &mBPTCVertexDecl);


      mRenderSquadBarTechnique = mEffect.getTechnique("renderSquadHPBar");      
      BASSERT(mRenderSquadBarTechnique.getValid());

      // Create vertex decl
      const D3DVERTEXELEMENT9 Squad_VertexElements[] =
      {
         { 0, 0,  D3DDECLTYPE_FLOAT4,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 }, //Pos
         { 0, 16, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 }, //HP Dim
         { 0, 24, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1 }, //Shield Dim
         { 0, 32, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 2 }, //Ammo Dim
         { 0, 40, D3DDECLTYPE_FLOAT16_4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 3 }, //Status
         { 0, 48, D3DDECLTYPE_FLOAT16_2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 4 }, //Size         
         { 0, 52, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 }, //Color
         { 0, 56, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    1 }, //HPColor
         { 0, 60, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    2 }, //Shield Color
         { 0, 64, D3DDECLTYPE_D3DCOLOR,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    3 }, //Ammo Color
         D3DDECL_END()
      };
      BD3D::mpDev->CreateVertexDeclaration(Squad_VertexElements, &mSquadVertexDecl);

      mDiffuseTextureParam = mEffect("gDiffuseSampler");
      mMaskTextureParam    = mEffect("gMaskSampler");
      mAmmoBarColor        = mEffect("gAmmoBarColor");
      mShieldBarColor      = mEffect("gShieldBarColor");      

   }
}

//==============================================================================
// Kill effect
//==============================================================================
void BHPBar::killEffect(void)
{
   ASSERT_RENDER_THREAD

   if (mBPTCVertexDecl)
   {
      mBPTCVertexDecl->Release();
      mBPTCVertexDecl = NULL;
   }

   if (mSquadVertexDecl)
   {
      mSquadVertexDecl->Release();
      mSquadVertexDecl=NULL;
   }

   mRenderHPBarTechnique.clear();
   mRenderSquadBarTechnique.clear();
   mEffect.clear();
}

//==============================================================================
// Render
//==============================================================================
void BHPBar::workerRender(const BRenderHPBarPacket *HPBarData)
{
   ASSERT_RENDER_THREAD
      
   // Set params
   BD3D::mpDev->SetVertexDeclaration(mBPTCVertexDecl);
   BD3D::mpDev->SetStreamSource(0, HPBarData->HPBarVB, 0, sizeof(BPTCVertex));
   mEffect.updateIntrinsicParams();

   // Draw
   mRenderHPBarTechnique.beginRestoreDefaultState();

      mRenderHPBarTechnique.beginPass(0);
         mRenderHPBarTechnique.commit();
         BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, HPBarData->numPrimitives * 4);
      mRenderHPBarTechnique.endPass();

   mRenderHPBarTechnique.end();

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);   
}

//==============================================================================
// Render
//==============================================================================
void BHPBar::workerRender(const BRenderSquadBarPacket* pPacket)
{
   ASSERT_RENDER_THREAD

   // Set params
   BD3D::mpDev->SetVertexDeclaration(mSquadVertexDecl);
   BD3D::mpDev->SetStreamSource(0, pPacket->mSquadBarVB, 0, sizeof(BSquadHPVertex));
   mEffect.updateIntrinsicParams();
   
   // Draw
   mRenderSquadBarTechnique.beginRestoreDefaultState();

   XMCOLOR shieldColor = XMCOLOR(gDatabase.getShieldBarColor());
   XMCOLOR ammoColor   = XMCOLOR(gDatabase.getAmmoBarColor());

   mRenderSquadBarTechnique.beginPass(0);
      mDiffuseTextureParam = mDiffuseTexture.getBaseTexture();
      mMaskTextureParam = mMaskTexture.getBaseTexture();
      mShieldBarColor   = XMLoadColor(&shieldColor);
      mAmmoBarColor     = XMLoadColor(&ammoColor);
      mRenderSquadBarTechnique.commit();
         BD3D::mpDev->DrawVertices(D3DPT_QUADLIST, 0, pPacket->numPrimitives * 4);
      mRenderSquadBarTechnique.endPass();
   mRenderSquadBarTechnique.end();

   // Reset a few things
   BD3D::mpDev->SetVertexDeclaration(NULL);
}
