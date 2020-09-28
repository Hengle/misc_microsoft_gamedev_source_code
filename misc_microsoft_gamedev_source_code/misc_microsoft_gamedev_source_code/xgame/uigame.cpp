//==============================================================================
// uigame.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "uigame.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "FontSystem2.h"
#include "gamedirectories.h"
#include "protoobject.h"
#include "prototech.h"
#include "protosquad.h"
#include "protopower.h"
#include "textvisualmanager.h"
#include "xmlreader.h"
#include "recordgame.h"
#include "soundmanager.h"
#include "uimanager.h"
#include "game.h"
#include "usermanager.h"
#include "user.h"
#include "render.h"
#include "econfigenum.h"
#include "world.h"
#include "viewportManager.h"

// Globals
BUIGame gUIGame;

//==============================================================================
// BUIGame::BUIGame
//==============================================================================
BUIGame::BUIGame() :
   mMinimapNumFlares(0),
   mMinimapFlareDuration(0),
   mMinimapFlareStartSize(0.0f),
   mMinimapFlareSmallSize(0.0f),
   mMinimapFlareBigSize(0.0f),
   mMinimapFlareStartSpeed(0.0f),
   mMinimapFlareGrowSpeed(0.0f),
   mMinimapFlareShrinkSpeed(0.0f),
   mMinimapAlertSize(0.0f),
   mMinimapNumActiveAlerts(0),
   mMinimapNumQueuedAlerts(0),
   mMinimapAlertCooldown(0),
   mMinimapPosition(),
   mMinimapBackgroundColor(1.0f, 1.0f, 1.0f),
   mMinimapBackgroundColorActive(true),
   mMinimapVisibilityColor(1.0f, 1.0f, 1.0f),
   mMinimapBorderMin(0.0f),
   mMinimapBorderMax(1.0f),
   mMinimapBorderActive(false),
   mMinimapBorderTextureHandle(cInvalidManagedTextureHandle),
   mMinimapIconTextureHandle(cInvalidManagedTextureHandle),
   mMinimapOverlayTextureHandle(cInvalidManagedTextureHandle),
   mMinimapRadarTextureHandle(cInvalidManagedTextureHandle),
   mMinimapOpacity(0.5f),
   mMinimapNumIcons(0),

   mHelpPosition(),
   mHelpFont(NULL),

   mReticleSize(30),
   mReticleSize4x3(15),

   mNumberResources(0),
   mpResourceHudIcons(NULL),
   mpResourceTextIcons(NULL),
   mpResourceFloatyIcons(NULL),
   mpResourceTextVisuals(NULL),
   mpResourceTextVisualBucketSize(NULL),
   mpResourceFlashUICostPanelFrame(NULL),
   mNumberPops(0),

   mpPopHudIcons(NULL),
   mpPopTextIcons(NULL),
   mpPopFlashUICostPanelFrame(NULL),

   mFloatyWidth(0),
   mFloatyHeight(0),

   mPlayerStatFont(NULL),
   mNumberPlayerStatLists(0),
   mpPlayerStatLists(NULL),

   mCircleMenuFadeTime(0.0f),
   mCircleMenuDisplayOnUp(false),
   mCircleMenuCount(0),

   mCircleMenuWidth(0),
   mCircleMenuItemRadius(0.0f),
   mCircleMenuItemWidth(0),

   mCircleMenuWidthSV(0),
   mCircleMenuItemRadiusSV(0.0f),
   mCircleMenuItemWidthSV(0),

   mCircleMenuWidthSH(0),
   mCircleMenuItemRadiusSH(0.0f),
   mCircleMenuItemWidthSH(0),

   mCircleMenuButtonFont(NULL),
   mCircleMenuHelpTextFont(NULL),
   mCircleMenuHelpTextIconWidth(0),
   mCircleMenuHelpTextIconHeight(0),
   mNumberCircleMenuBackgrounds(0),
   mpCircleMenuBackgrounds(NULL),

   mCircleSelectDecalHandle(-1),
   mDefaultDecalHandle(-1),
   
   mpObjectIcons(NULL),
   mpTechIcons(NULL),
   mpSquadIcons(NULL),
   mpSquadModeIcons(NULL),
   mpObjectCommandIcons(NULL),
   mHoverDecalTexture(cInvalidManagedTextureHandle)
{
   for (uint i=0; i<cGameCommandCount; i++)
   {
      mGameCommandIcons[i] = NULL;
   }
   for (uint i=0; i<cReticleCount; i++)
   {
      mReticleTextures[i] = cInvalidManagedTextureHandle;
   }
}

//==============================================================================
// BUIGame::~BUIGame
//==============================================================================
BUIGame::~BUIGame()
{
}

//==============================================================================
// BUIGame::init
//==============================================================================
bool BUIGame::init()
{
   SCOPEDSAMPLE(BUIGame_init)
   bool splitScreen = gGame.isSplitScreen();

   mpSquadModeIcons=new BManagedTextureHandle[BSquadAI::cNumberModes];
   if(!mpSquadModeIcons)
      return false;
   for(long i=0; i<BSquadAI::cNumberModes; i++)
   {
      mpSquadModeIcons[i]=cInvalidManagedTextureHandle;
      if (splitScreen)
         getSquadModeIcon(i);
   }

   mpObjectCommandIcons=new BManagedTextureHandle[BProtoObjectCommand::cNumberTypes];
   if(!mpObjectCommandIcons)
      return false;
   for(long i=0; i<BProtoObjectCommand::cNumberTypes; i++)
   {
      mpObjectCommandIcons[i]=cInvalidManagedTextureHandle;
      if (splitScreen)
         getObjectCommandIcon(i);
   }

   for(uint i=0; i<cGameCommandCount; i++)
   {
      mGameCommandIcons[i]=NULL;
      if (splitScreen)
         getGameCommandIcon(i);
   }

   BXMLReader reader;
   {
      SCOPEDSAMPLE(Load_UIGAME_XML)
      if(!reader.load(cDirArt, "ui\\game\\uigame.xml"))
         return false;
   }

   const BXMLNode root(reader.getRootNode());
   long nodeCount=root.getNumberChildren();

   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName());
      if(nodeName=="Minimap")
         loadMinimap(node);
      else if(nodeName=="Help")
         loadHelp(node);
      else if(nodeName=="Reticles")
         loadReticles(node);
      else if(nodeName=="Resources")
         loadResources(node);
      else if(nodeName=="Pops")
         loadPops(node);
      else if(nodeName=="Floaty")
         loadFloaty(node);
      else if(nodeName=="PlayerStats")
         loadPlayerStats(node);
      else if(nodeName=="CircleMenu")
         loadCircleMenu(node);
      else if(nodeName=="SquadModes")
         loadSquadModes(node);
      else if(nodeName=="ObjectCommands")
         loadObjectCommands(node);      
      else if(nodeName=="GameCommands")
         loadGameCommands(node);
      else if(nodeName=="FlashResources")
         loadFlashUI(node);
      else if(nodeName=="UnitStats")
         loadUnitStats(node);
      else if(nodeName=="Tribute")
         loadTribute(node);
      else if(nodeName=="HoverDecal")
         loadHoverDecal(node);
   }

   long protoObjectCount=gDatabase.getNumberProtoObjects();
   mpObjectIcons=new BManagedTextureHandle[protoObjectCount];
   if(!mpObjectIcons)
      return false;
   for(long i=0; i<protoObjectCount; i++)
   {
      mpObjectIcons[i]=cInvalidManagedTextureHandle;
      if (splitScreen)
         getObjectIcon(i, -1);
   }
      
   long protoTechCount=gDatabase.getNumberProtoTechs();
   mpTechIcons=new BManagedTextureHandle[protoTechCount];
   if(!mpTechIcons)
      return false;
   for(long i=0; i<protoTechCount; i++)
   {
      mpTechIcons[i]=cInvalidManagedTextureHandle;
      if (splitScreen)
         getTechIcon(i);
   }

   long protoSquadCount=gDatabase.getNumberProtoSquads();
   mpSquadIcons=new BManagedTextureHandle[protoSquadCount];
   if(!mpSquadIcons)
      return false;
   for(long i=0; i<protoSquadCount; i++)
   {
      mpSquadIcons[i]=cInvalidManagedTextureHandle;
      if (splitScreen)
         getSquadIcon(i, -1);
   }

   

   return true;
}

//==============================================================================
// BUIGame::deinit
//==============================================================================
void BUIGame::deinit()
{
   if(mpObjectCommandIcons)
   {
      for(long i=0; i<BProtoObjectCommand::cNumberTypes; i++)
         gUI.unloadTexture(mpObjectCommandIcons[i]);
      delete[] mpObjectCommandIcons;
      mpObjectCommandIcons=NULL;
   }

   if(mpSquadModeIcons)
   {
      for(long i=0; i<BSquadAI::cNumberModes; i++)
         gUI.unloadTexture(mpSquadModeIcons[i]);
      delete[] mpSquadModeIcons;
      mpSquadModeIcons=NULL;
   }

   if(mpSquadIcons)
   {
      long protoSquadCount=gDatabase.getNumberProtoSquads();
      for(long i=0; i<protoSquadCount; i++)
         gUI.unloadTexture(mpSquadIcons[i]);
      delete[] mpSquadIcons;
      mpSquadIcons=NULL;
   }

   if(mpTechIcons)
   {
      long protoTechCount=gDatabase.getNumberProtoTechs();
      for(long i=0; i<protoTechCount; i++)
         gUI.unloadTexture(mpTechIcons[i]);
      delete[] mpTechIcons;
      mpTechIcons=NULL;
   }

   if(mpObjectIcons)
   {
      long protoObjectCount=gDatabase.getNumberProtoObjects();
      for(long i=0; i<protoObjectCount; i++)
         gUI.unloadTexture(mpObjectIcons[i]);
      delete[] mpObjectIcons;
      mpObjectIcons=NULL;
   }   
   
   if(mpCircleMenuBackgrounds)
   {
      for(long i=0; i<mNumberCircleMenuBackgrounds; i++)
         gUI.unloadTexture(mpCircleMenuBackgrounds[i]);
      delete[] mpCircleMenuBackgrounds;
      mpCircleMenuBackgrounds=NULL;
   }

   if(mpPlayerStatLists)
   {
      delete[] mpPlayerStatLists;
      mpPlayerStatLists=NULL;
   }

   // Unload pop bucket textures
   if(mpPopHudIcons)
   {
      for(long i=0; i<mNumberPops; i++)
         gUI.unloadTexture(mpPopHudIcons[i]);
      delete[] mpPopHudIcons;
      mpPopHudIcons=NULL;
   }

   if(mpPopTextIcons)
   {
      for(long i=0; i<mNumberPops; i++)
         gUI.unloadTexture(mpPopTextIcons[i]);
      delete[] mpPopTextIcons;
      mpPopTextIcons=NULL;
   }

   if(mpPopFlashUICostPanelFrame)
   {
      delete[] mpPopFlashUICostPanelFrame;
      mpPopFlashUICostPanelFrame=NULL;
   }

   // Unload resource textures
   if(mpResourceHudIcons)
   {
      for(long i=0; i<mNumberResources; i++)
         gUI.unloadTexture(mpResourceHudIcons[i]);
      delete[] mpResourceHudIcons;
      mpResourceHudIcons=NULL;
   }

   if(mpResourceTextIcons)
   {
      for(long i=0; i<mNumberResources; i++)
         gUI.unloadTexture(mpResourceTextIcons[i]);
      delete[] mpResourceTextIcons;
      mpResourceTextIcons=NULL;
   }

   if(mpResourceFloatyIcons)
   {
      for(long i=0; i<mNumberResources; i++)
         gUI.unloadTexture(mpResourceFloatyIcons[i]);
      delete[] mpResourceFloatyIcons;
      mpResourceFloatyIcons=NULL;
   }

   if(mpResourceTextVisuals)
   {
      delete[] mpResourceTextVisuals;
      mpResourceTextVisuals=NULL;
   }

   if(mpResourceTextVisualBucketSize)
   {
      delete[] mpResourceTextVisualBucketSize;
      mpResourceTextVisualBucketSize=NULL;
   }

   if(mpResourceFlashUICostPanelFrame)
   {
      delete[] mpResourceFlashUICostPanelFrame;
      mpResourceFlashUICostPanelFrame=NULL;
   }
   
   // Unload reticle textures
   for(long i=0; i<cReticleCount; i++)
   {
      gUI.unloadTexture(mReticleTextures[i]);
      mReticleTextures[i]=cInvalidManagedTextureHandle;
   }
   
   // Unload minimap textures
   if(mMinimapBorderTextureHandle!=cInvalidManagedTextureHandle)
   {
      gD3DTextureManager.unloadManagedTextureByHandle(mMinimapBorderTextureHandle);
      mMinimapBorderTextureHandle=cInvalidManagedTextureHandle;
   }

   if(mHoverDecalTexture!=cInvalidManagedTextureHandle)
   {
      gD3DTextureManager.unloadManagedTextureByHandle(mHoverDecalTexture);
      mHoverDecalTexture=cInvalidManagedTextureHandle;
   }

   mMinimapIcon.clear();
   mUnitStats.clear();
   mTributes.clear();

   mFlashUIReticles.clear();
   mFlashUIHUDs.clear();
   mFlashUIMinimaps.clear();
   mFlashUIDecals.clear();
}

//==============================================================================
// BUIGame::loadMinimap
//==============================================================================
void BUIGame::loadMinimap(const BXMLNode  root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName());
      BSimString tempStr;
      if(nodeName=="NumFlares")
         node.getTextAsLong(mMinimapNumFlares);
      else if(nodeName=="FlareDuration")
         node.getTextAsDWORD(mMinimapFlareDuration);
      else if(nodeName=="FlareStartSize")
         node.getTextAsFloat(mMinimapFlareStartSize);
      else if(nodeName=="FlareSmallSize")
         node.getTextAsFloat(mMinimapFlareSmallSize);
      else if(nodeName=="FlareBigSize")
         node.getTextAsFloat(mMinimapFlareBigSize);
      else if(nodeName=="FlareStartSpeed")
         node.getTextAsFloat(mMinimapFlareStartSpeed);
      else if(nodeName=="FlareGrowSpeed")
         node.getTextAsFloat(mMinimapFlareGrowSpeed);
      else if(nodeName=="FlareShrinkSpeed")
         node.getTextAsFloat(mMinimapFlareShrinkSpeed);
      else if(nodeName=="AlertSize")
         node.getTextAsFloat(mMinimapAlertSize);
      else if(nodeName=="NumActiveAlerts")
         node.getTextAsLong(mMinimapNumActiveAlerts);
      else if(nodeName=="NumQueuedAlerts")
         node.getTextAsLong(mMinimapNumQueuedAlerts);
      else if(nodeName=="AlertCooldown")
         node.getTextAsDWORD(mMinimapAlertCooldown);
      else if(nodeName=="Position")
         gUI.loadPosition(node, &mMinimapPosition);
      else if(nodeName=="BackgroundColor")
         node.getTextAsVector(mMinimapBackgroundColor);
      else if(nodeName=="BackgroundColorActive")
         node.getTextAsBool(mMinimapBackgroundColorActive);
      else if(nodeName=="VisibilityColor")
         node.getTextAsVector(mMinimapVisibilityColor);
      else if(nodeName=="BorderMin")
         node.getTextAsFloat(mMinimapBorderMin);
      else if(nodeName=="BorderMax")
         node.getTextAsFloat(mMinimapBorderMax);
      else if(nodeName=="BorderActive")
         node.getTextAsBool(mMinimapBorderActive);
      else if(nodeName=="BorderTexture")
      {
         if(!gConfig.isDefined(cConfigDisableOldUITextures))
         {
            mMinimapBorderTextureHandle=gD3DTextureManager.getOrCreateHandle(node.getText(tempStr), BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "UIGame");
         }
      }
      else if(nodeName=="IconTexture")
      {
         if(!gConfig.isDefined(cConfigDisableOldUITextures))
         {
            mMinimapIconTextureHandle=gD3DTextureManager.getOrCreateHandle(node.getText(tempStr), BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "UIGame");
         }
      }
      else if(nodeName=="RadarTexture")
      {
         if(!gConfig.isDefined(cConfigDisableOldUITextures))
         {
            mMinimapRadarTextureHandle=gD3DTextureManager.getOrCreateHandle(node.getText(tempStr), BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "UIGame");
         }
      }
      else if(nodeName=="OverlayTexture")
      {
         if(!gConfig.isDefined(cConfigDisableOldUITextures))
         {
            mMinimapOverlayTextureHandle=gD3DTextureManager.getOrCreateHandle(node.getText(tempStr), BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "UIGame");
         }
      }
      else if(nodeName=="Opacity")
         node.getTextAsFloat(mMinimapOpacity);
      else if(nodeName=="Icons")
      {
         mMinimapNumIcons = node.getNumberChildren();
         for (long i = 0; i < mMinimapNumIcons; i++)
         {
            const BXMLNode iconNode(node.getChild(i));

            BMinimapIcon icon;
            icon.mU = 0.0f;
            icon.mV = 0.0f;
            icon.mSize = 0;
            iconNode.getAttribValueAsFloat("u", icon.mU);
            iconNode.getAttribValueAsFloat("v", icon.mV);
            iconNode.getAttribValueAsFloat("size", icon.mSize);
            iconNode.getText(icon.mName);

            mMinimapIcon.add(icon);
         }
      }
   }
}

//==============================================================================
// BUIGame::loadHelp
//==============================================================================
long BUIGame::getMinimapIconIndex(const BSimString &name) const
{
   for (long i = 0; i < mMinimapNumIcons; i++)
   {
      const BMinimapIcon& icon = mMinimapIcon[i];
      
      if (icon.mName == name)
         return i;
   }

   return -1;
}

//==============================================================================
// BUIGame::loadHelp
//==============================================================================
void BUIGame::loadHelp(const BXMLNode  root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName());
      BSimString tempStr;
      if(nodeName=="Font")
         mHelpFont=gFontManager.findFont(node.getTextPtr(tempStr));
      else if(nodeName=="Position")
         gUI.loadPosition(node, &mHelpPosition);
   }
}

//==============================================================================
// BUIGame::loadReticles
//==============================================================================
void BUIGame::loadReticles(const BXMLNode  root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode  node(root.getChild(i));
      if(node.getName()=="Size")
      {
         node.getTextAsLong(mReticleSize);
         node.getAttribValueAsLong("size4x3", mReticleSize4x3);
      }
      else if(node.getName()=="Reticle")
      {
         BSimString  typeStr;
         if (node.getAttribValue("Type", &typeStr))
         {
            long type=-1;
            if(typeStr=="Normal")
               type=cReticleNormal;
            else if(typeStr=="Attack")
               type=cReticleAttack;
            else if(typeStr=="Garrison")
               type=cReticleGarrison;
            else if (typeStr == "Hitch")
               type = cReticleHitch;
            else if(typeStr=="Capture")
               type=cReticleCapture;
            else if(typeStr=="Repair")
               type=cReticleRepair;
            else if(typeStr=="PowerValid")
               type=cReticlePowerValid;
            else if(typeStr=="PowerInvalid")
               type=cReticlePowerInvalid;
            else if(typeStr=="Base")
               type=cReticleBase;
            else if(typeStr=="TransportFull")
               type=cReticleTransportFull;
            if(type!=-1)
            {               
               int childCount =node.getNumberChildren();
               for (long j = 0; j < childCount; j++)
               {
                  const BXMLNode child(node.getChild(j));
                  const BPackedString name(child.getName());
                  BSimString tempStr;
                  if (name == "icon")
                  {
                     mReticleTextures[type]=gUI.loadTexture(child.getTextPtr(tempStr));
                  }
                  else if (name == "flashkeyframe")
                  {                     
                     BSimString text;
                     child.getText(text);
                     mReticleFlashKeyframes[type].set(text);
                  }
               }                           
            }
         }
      }
   }
}

//==============================================================================
// BUIGame::loadResources
//==============================================================================
void BUIGame::loadResources(const BXMLNode  root)
{
   // Init resource texture arrays
   mNumberResources=gDatabase.getNumberResources();

   mpResourceHudIcons=new BManagedTextureHandle[mNumberResources];
   if(!mpResourceHudIcons)
      return;
   for(long i=0; i<mNumberResources; i++)
      mpResourceHudIcons[i]=cInvalidManagedTextureHandle;

   mpResourceTextIcons=new BManagedTextureHandle[mNumberResources];
   if(!mpResourceTextIcons)
      return;
   for(long i=0; i<mNumberResources; i++)
      mpResourceTextIcons[i]=cInvalidManagedTextureHandle;

   mpResourceFloatyIcons=new BManagedTextureHandle[mNumberResources];
   if(!mpResourceFloatyIcons)
      return;
   for(long i=0; i<mNumberResources; i++)
      mpResourceFloatyIcons[i]=cInvalidManagedTextureHandle;

   mpResourceTextVisuals=new long[mNumberResources];
   if(!mpResourceTextVisuals)
      return;
   for(long i=0; i<mNumberResources; i++)
      mpResourceTextVisuals[i]=-1;

   mpResourceTextVisualBucketSize=new long[mNumberResources];
   if(!mpResourceTextVisualBucketSize)
      return;
   for(long i=0; i<mNumberResources; i++)
      mpResourceTextVisualBucketSize[i]=-1;

   mpResourceFlashUICostPanelFrame=new int[mNumberResources];
   if(!mpResourceFlashUICostPanelFrame)
      return;
   for(long i=0; i<mNumberResources; i++)
      mpResourceFlashUICostPanelFrame[i]=-1;

   // Load the resource data
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode  node(root.getChild(i));
      if(node.getName()=="Resource")
      {
         BSimString tempStr;
         long resourceID=gDatabase.getResource(node.getTextPtr(tempStr));
         if(resourceID!=-1)
         {
            long childCount=node.getNumberChildren();
            for(long j=0; j<childCount; j++)
            {
               const BXMLNode  child(node.getChild(j));
               const BPackedString childName(child.getName());
               if(childName=="HudIcon")
                  mpResourceHudIcons[resourceID]=gUI.loadTexture(child.getTextPtr(tempStr));
               else if(childName=="TextIcon")
                  mpResourceTextIcons[resourceID]=gUI.loadTexture(child.getTextPtr(tempStr));
               else if(childName=="FloatyIcon")
                  mpResourceFloatyIcons[resourceID]=gUI.loadTexture(child.getTextPtr(tempStr));
               else if(childName=="Reticle")
               {
                  if(resourceID<cGatherReticleCount)
                  {
                     long type=cFirstGatherReticle+resourceID;
                     mReticleTextures[type]=gUI.loadTexture(child.getTextPtr(tempStr));

                     BSimString tempStr;
                     child.getAttribValueAsString("flashkeyframe", tempStr);
                     mReticleFlashKeyframes[type] = tempStr;
                  }
               }
               else if(childName=="TextVisual")
                  mpResourceTextVisuals[resourceID]=gTextVisualManager.getOrCreateVisualDef(child.getTextPtr(tempStr));
               else if(childName=="TextVisualBucketSize")
                  child.getTextAsLong(mpResourceTextVisualBucketSize[resourceID]);
               else if(childName=="FlashUI")
               {
                  int grandChildCount = child.getNumberChildren();
                  for (int k = 0; k < grandChildCount; ++k)
                  {
                     const BXMLNode grandChild(child.getChild(k));
                     const BPackedString grandChildName(grandChild.getName());
                     if (grandChildName=="CostPanelFrame")
                        grandChild.getTextAsInt(mpResourceFlashUICostPanelFrame[resourceID]);
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
// BUIGame::loadPops
//==============================================================================
void BUIGame::loadPops(const BXMLNode  root)
{
   // Init pop bucket texture arrays
   mNumberPops=gDatabase.getNumberPops();

   mpPopHudIcons=new BManagedTextureHandle[mNumberPops];
   if(!mpPopHudIcons)
      return;
   for(long i=0; i<mNumberPops; i++)
      mpPopHudIcons[i]=cInvalidManagedTextureHandle;

   mpPopTextIcons=new BManagedTextureHandle[mNumberPops];
   if(!mpPopTextIcons)
      return;
   for(long i=0; i<mNumberPops; i++)
      mpPopTextIcons[i]=cInvalidManagedTextureHandle;

   mpPopFlashUICostPanelFrame=new int[mNumberPops];
   if(!mpPopFlashUICostPanelFrame)
      return;
   for(long i=0; i<mNumberPops; i++)
      mpPopFlashUICostPanelFrame[i]=-1;

   // Load the pop bucket data
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      if(node.getName()=="Pop")
      {
         BSimString tempStr;
         long popID=gDatabase.getPop(node.getTextPtr(tempStr));
         if(popID!=-1)
         {
            long childCount=node.getNumberChildren();
            for(long j=0; j<childCount; j++)
            {
               const BXMLNode child(node.getChild(j));
               const BPackedString childName(child.getName());
               if(childName=="HudIcon")
                  mpPopHudIcons[popID]=gUI.loadTexture(child.getTextPtr(tempStr));
               else if(childName=="TextIcon")
                  mpPopTextIcons[popID]=gUI.loadTexture(child.getTextPtr(tempStr));
               else if(childName=="FlashUI")
               {
                  int grandChildCount = child.getNumberChildren();
                  for (int k = 0; k < grandChildCount; ++k)
                  {
                     const BXMLNode grandChild(child.getChild(k));
                     const BPackedString grandChildName(grandChild.getName());
                     if (grandChildName=="CostPanelFrame")
                        grandChild.getTextAsInt(mpPopFlashUICostPanelFrame[popID]);
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
// BUIGame::loadFloaty
//==============================================================================
void BUIGame::loadFloaty(const BXMLNode  root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      if(node.getName()=="IconSize")
         gUI.loadSize(node, &mFloatyWidth, &mFloatyHeight);
   }
}

//==============================================================================
// BUIGame::loadPlayerStats
//==============================================================================
void BUIGame::loadPlayerStats(const BXMLNode  root)
{
   // Init player stat arrays
   mNumberPlayerStatLists=gDatabase.getNumberCivs();
   mpPlayerStatLists=new BUIGamePlayerStatArray[mNumberPlayerStatLists];
   if(!mpPlayerStatLists)
      return;

   // Load the player stat data
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName());
      BSimString tempStr;
      if(nodeName=="Font")
         mPlayerStatFont=gFontManager.findFont(node.getTextPtr(tempStr));
      else if(nodeName=="Stats")
      {
         long civID=gDatabase.getCivID(node.getTextPtr(tempStr));
         if(civID!=-1)
         {
            long childCount=node.getNumberChildren();
            for(long j=0; j<childCount; j++)
            {
               const BXMLNode child(node.getChild(j));
               if(child.getName()=="Stat")
               {
                  BSimString typeStr;
                  if (child.getAttribValue("Type", &typeStr))
                  {
                     long leaderID=-1;
                     BSimString leaderStr;
                     if (child.getAttribValue("Leader", &leaderStr))
                        leaderID=gDatabase.getLeaderID(leaderStr);

                     if(typeStr=="Pop")
                     {
                        long id=gDatabase.getPop(child.getTextPtr(tempStr));
                        if(id!=-1)
                        {
                           BUIGamePlayerStat stat;
                           stat.mType=BUIGamePlayerStat::cTypePop;
                           stat.mID=id;
                           stat.mLeaderID=leaderID;
                           for(long k=0; k<child.getNumberChildren(); k++)
                           {
                              const BXMLNode child2(child.getChild(k));
                              if(child2.getName()=="IconPosition")
                                 gUI.loadPosition(child2, &stat.mIconPosition);
                              else if(child2.getName()=="ValuePosition")
                                 gUI.loadPosition(child2, &stat.mValuePosition);
                              else if(child2.getName()=="IconPositionSV1")
                                 gUI.loadPosition(child2, &stat.mIconPositionSV1);
                              else if(child2.getName()=="ValuePositionSV1")
                                 gUI.loadPosition(child2, &stat.mValuePositionSV1);
                              else if(child2.getName()=="IconPositionSV2")
                                 gUI.loadPosition(child2, &stat.mIconPositionSV2);
                              else if(child2.getName()=="ValuePositionSV2")
                                 gUI.loadPosition(child2, &stat.mValuePositionSV2);
                              else if(child2.getName()=="IconPositionSH1")
                                 gUI.loadPosition(child2, &stat.mIconPositionSH1);
                              else if(child2.getName()=="ValuePositionSH1")
                                 gUI.loadPosition(child2, &stat.mValuePositionSH1);
                              else if(child2.getName()=="IconPositionSH2")
                                 gUI.loadPosition(child2, &stat.mIconPositionSH2);
                              else if(child2.getName()=="ValuePositionSH2")
                                 gUI.loadPosition(child2, &stat.mValuePositionSH2);
                           }
                           mpPlayerStatLists[civID].add(stat);
                        }
                     }
                     else if(typeStr=="Resource")
                     {
                        long id=gDatabase.getResource(child.getTextPtr(tempStr));
                        if(id!=-1)
                        {
                           BUIGamePlayerStat stat;
                           stat.mType=BUIGamePlayerStat::cTypeResource;
                           stat.mID=id;
                           stat.mID2=-1;
                           stat.mLeaderID=leaderID;
                           for(long k=0; k<child.getNumberChildren(); k++)
                           {
                              const BXMLNode child2(child.getChild(k));
                              if(child2.getName()=="IconPosition")
                                 gUI.loadPosition(child2, &stat.mIconPosition);
                              else if(child2.getName()=="ValuePosition")
                                 gUI.loadPosition(child2, &stat.mValuePosition);
                              else if(child2.getName()=="IconPositionSV1")
                                 gUI.loadPosition(child2, &stat.mIconPositionSV1);
                              else if(child2.getName()=="ValuePositionSV1")
                                 gUI.loadPosition(child2, &stat.mValuePositionSV1);
                              else if(child2.getName()=="IconPositionSV2")
                                 gUI.loadPosition(child2, &stat.mIconPositionSV2);
                              else if(child2.getName()=="ValuePositionSV2")
                                 gUI.loadPosition(child2, &stat.mValuePositionSV2);
                              else if(child2.getName()=="IconPositionSH1")
                                 gUI.loadPosition(child2, &stat.mIconPositionSH1);
                              else if(child2.getName()=="ValuePositionSH1")
                                 gUI.loadPosition(child2, &stat.mValuePositionSH1);
                              else if(child2.getName()=="IconPositionSH2")
                                 gUI.loadPosition(child2, &stat.mIconPositionSH2);
                              else if(child2.getName()=="ValuePositionSH2")
                                 gUI.loadPosition(child2, &stat.mValuePositionSH2);
                              else if(child2.getName()=="Rate")
                                 stat.mID2=gDatabase.getRate(child2.getTextPtr(tempStr));
                           }
                           mpPlayerStatLists[civID].add(stat);
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

//==============================================================================
// BUIGame::loadCircleMenu
//==============================================================================
void BUIGame::loadCircleMenu(const BXMLNode  root)
{
   // Init circle menu background array
   mNumberCircleMenuBackgrounds=gDatabase.getNumberCivs();

   mpCircleMenuBackgrounds=new BManagedTextureHandle[mNumberCircleMenuBackgrounds];
   if(!mpCircleMenuBackgrounds)
      return;
   for(long i=0; i<mNumberCircleMenuBackgrounds; i++)
      mpCircleMenuBackgrounds[i]=cInvalidManagedTextureHandle;

   // Load circle menu data
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      const BPackedString nodeName(node.getName());
      BSimString tempStr;
      if(nodeName=="FadeTime")
         node.getTextAsFloat(mCircleMenuFadeTime);
      else if(nodeName=="DisplayOnUp")
         node.getTextAsBool(mCircleMenuDisplayOnUp);
      else if(nodeName=="CircleCount")
         node.getTextAsLong(mCircleMenuCount);
      else if(nodeName=="CircleWidth")
         node.getTextAsLong(mCircleMenuWidth);
      else if(nodeName=="ItemRadius")
         node.getTextAsFloat(mCircleMenuItemRadius);
      else if(nodeName=="ItemWidth")
         node.getTextAsLong(mCircleMenuItemWidth);
      else if(nodeName=="CircleWidthSV")
         node.getTextAsLong(mCircleMenuWidthSV);
      else if(nodeName=="ItemRadiusSV")
         node.getTextAsFloat(mCircleMenuItemRadiusSV);
      else if(nodeName=="ItemWidthSV")
         node.getTextAsLong(mCircleMenuItemWidthSV);
      else if(nodeName=="CircleWidthSH")
         node.getTextAsLong(mCircleMenuWidthSH);
      else if(nodeName=="ItemRadiusSH")
         node.getTextAsFloat(mCircleMenuItemRadiusSH);
      else if(nodeName=="ItemWidthSH")
         node.getTextAsLong(mCircleMenuItemWidthSH);
      else if(nodeName=="ButtonFont")
         mCircleMenuButtonFont=gFontManager.findFont(node.getTextPtr(tempStr));
      else if(nodeName=="HelpTextFont")
         mCircleMenuHelpTextFont=gFontManager.findFont(node.getTextPtr(tempStr));
      else if(nodeName=="HelpTextIconSize")
         gUI.loadSize(node, &mCircleMenuHelpTextIconWidth, &mCircleMenuHelpTextIconHeight);
      else if(nodeName=="Backgrounds")
      {
         long childCount=node.getNumberChildren();
         for(long j=0; j<childCount; j++)
         {
            const BXMLNode child(node.getChild(j));
            if(child.getName()=="Background")
            {
               BSimString civ;
               if (child.getAttribValue("Civ", &civ))
               {
                  long civID=gDatabase.getCivID(civ);
                  if(civID!=-1)
                     mpCircleMenuBackgrounds[civID]=gUI.loadTexture(child.getTextPtr(tempStr));
               }
            }
         }
      }
   }
}

//==============================================================================
// BUIGame::loadSquadModes
//==============================================================================
void BUIGame::loadSquadModes(const BXMLNode  root)
{
   int nodeCount = root.getNumberChildren();
   for (int i = 0; i < nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      BSimString typeStr;
      if (node.getAttribValue("Type", &typeStr))
      {
         int squadMode = gDatabase.getSquadMode(typeStr);
         if (squadMode != -1)
         {
            int childCount = node.getNumberChildren();
            for (int j = 0; j < childCount; j++)
            {
               const BXMLNode child(node.getChild(j));
               const BPackedString name(child.getName());
               if (name == "icon")
               {
                  BSimString tempStr;
                  mpSquadModeIcons[squadMode] = gUI.loadTexture(child.getTextPtr(tempStr));
               }
            }            
         }
      }
   }
}

//==============================================================================
// BUIGame::loadObjectCommands
//==============================================================================
void BUIGame::loadObjectCommands(const BXMLNode  root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      BSimString  typeStr;
      if (node.getAttribValue("Type", &typeStr))
      {
         long type=gDatabase.getProtoObjectCommandType(typeStr);
         if(type != -1)
         {
            int childCount =node.getNumberChildren();
            for (long j = 0; j < childCount; j++)
            {
               const BXMLNode child(node.getChild(j));
               const BPackedString name(child.getName());
               BSimString tempStr;
               
               if (name == "icon")
               {
                  mpObjectCommandIcons[type] = gUI.loadTexture(child.getTextPtr(tempStr));
               }
            }                        
         }
      }
   }
}

//==============================================================================
// BUIGame::loadGameCommands
//==============================================================================
void BUIGame::loadGameCommands(const BXMLNode  root)
{
   long nodeCount=root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      const BXMLNode node(root.getChild(i));
      BSimString tempStr;
      if (node.getAttribValue("Type", &tempStr))
      {
         long type=getGameCommandID(tempStr);
         if(type != -1)
         {
            node.getText(tempStr);
            mGameCommandIcons[type] = gUI.loadTexture(tempStr);
         }
      }
   }
}

//==============================================================================
// BUIGame::loadFlashUI
//==============================================================================
void BUIGame::loadFlashUI(const BXMLNode root)
{
   long civCount = gDatabase.getNumberCivs();
   if (civCount < 1)
      return;

   mFlashUIReticles.resize(civCount);
   mFlashUIHUDs.resize(civCount);   
   mFlashUIMinimaps.resize(civCount);
   mFlashUIDecals.resize(civCount);
   // mFlashUIObjectives.resize(civCount);

   long nodeCount=root.getNumberChildren();
   for (int i = 0; i < nodeCount; ++i)
   {
      const BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      if (name=="Reticle")
      {  
         loadFlashUINode(eFlashUIReticle, node);
      }
      else if (name=="HUD")
      {
         loadFlashUINode(eFlashUIHUD, node);
      }
      else if (name=="Minimap")
      {
         loadFlashUINode(eFlashUIMinimap, node);
      }
      else if (name=="Decals")
      {         
         loadFlashUINode(eFlashUIDecals, node);
      }
      else if (name=="Callouts")
      {
         loadFlashUINode(eFlashUICallouts, node);
      }
      else if (name=="Hints")
      {
         loadFlashUINode(eFlashUIHints, node);
      }
      else if (name=="Widgets")
      {         
         loadFlashUINode(eFlashUIWidgets, node);
      }
   }
}

//==============================================================================
// BUIGame::loadFlashUINode
//==============================================================================
void BUIGame::loadFlashUINode(int resourceType, const BXMLNode node)
{
   if (!node)
      return;

   BFlashUIDataNode dataNode;
   BSimString type;

   node.getAttribValueAsString("Type", type);
   long civID=gDatabase.getCivID(BStrConv::toA(type));

   // The type might be "All"
   //if (civID == -1)
   //   return;
   
   long nodeCount=node.getNumberChildren();
   for (int i = 0; i < nodeCount; ++i)
   {
      
      const BXMLNode child(node.getChild(i));
      const BPackedString name(child.getName());
      BSimString tempStr;
      if (name=="Flash")
      {
         child.getAttribValueAsString("VerticalSplitScreen", dataNode.mVSplitScreenFlashFile);
         child.getText(dataNode.mFlashFile);
      }
      else if (name=="Data")
      {
         child.getAttribValueAsString("VerticalSplitScreen", dataNode.mVSplitScreenDataFile);
         child.getText(dataNode.mDataFile);
      }
   }
   addFlashUIDataNode(civID, resourceType, dataNode);
}

//==============================================================================
// BUIGame::addFlashUIDataNode
//==============================================================================
void BUIGame::addFlashUIDataNode(int civID, int resourceType, const BFlashUIDataNode& data)
{
   long civCount = gDatabase.getNumberCivs();
   
   switch (resourceType)
   {
      // don't need to check the civ type for these
      case eFlashUIHints:
      case eFlashUICallouts:
      case eFlashUIWidgets:
         break;

      default:
         // for those types that require a CIV ID, check its bounds
         if ( (civID < 0) || (civID >= civCount) )
            return;
         break;
   }

   switch (resourceType)
   {
      case eFlashUICallouts:
         mFlashUICallouts = data;
         break;
      case eFlashUIHints:  
         mFlashUIHints = data;
         break;
      case eFlashUIWidgets:
         mFlashUIWidgets = data;
         break;
      case eFlashUIReticle: 
         mFlashUIReticles[civID] = data; 
         break;    
      case eFlashUIMinimap: 
         mFlashUIMinimaps[civID] = data;
         break;
      case eFlashUIHUD: 
         mFlashUIHUDs[civID] = data;
         break;
      case eFlashUIDecals: 
         mFlashUIDecals[civID] = data;
         break;
   };
}

//==============================================================================
// BUIGame::loadUnitStats
//==============================================================================
void BUIGame::loadUnitStats(const BXMLNode node)
{
   long nodeCount=node.getNumberChildren();
   for (int i = 0; i < nodeCount; ++i)
   {
      const BXMLNode child(node.getChild(i));
      const BPackedString name(child.getName());
      if (name=="Stat")
      {
         
         BSimString tempStr;
         if (child.getAttribValue("type", &tempStr))
         {
            BUIGameUnitStat stat;
            if (tempStr=="AttackRating")
            {
               stat.mStatType=BUIGameUnitStat::cTypeAttackRating;
               long damageType=gDatabase.getDamageType(child.getText(tempStr));
               if (damageType==-1)
                  continue;
               stat.mStatData=damageType;
            }
            else if (tempStr=="DefenseRating")
               stat.mStatType=BUIGameUnitStat::cTypeDefenseRating;
            else if (tempStr=="AttackGrade")
            {
               stat.mStatType=BUIGameUnitStat::cTypeAttackGrade;
               long damageType=gDatabase.getDamageType(child.getText(tempStr));
               if (damageType==-1)
                  continue;
               stat.mStatData=damageType;
            }
            else
               continue;
            long stringID=-1;
            if (child.getAttribValueAsLong("stringID", stringID))
               stat.mDisplayNameIndex=gDatabase.getLocStringIndex(stringID);

            child.getText(stat.mName);
            
            mUnitStats.add(stat);
         }
      }
   }
}

//==============================================================================
// BUIGame::loadTribute
//==============================================================================
void BUIGame::loadTribute(const BXMLNode node)
{
   long nodeCount=node.getNumberChildren();
   for (int i = 0; i < nodeCount; ++i)
   {
      const BXMLNode child(node.getChild(i));
      const BPackedString name(child.getName());
      if (name=="Resource")
      {
         BUIGameTribute tribute;
         BSimString tempStr;
         tribute.mResourceID=gDatabase.getResource(child.getText(tempStr));
         if (tribute.mResourceID!=-1)
         {
            int val=-1;
            tribute.mItemStringIndex = (child.getAttribValueAsInt("ItemStringID", val) ? gDatabase.getLocStringIndex(val) : -1);
            tribute.mDetailStringIndex = (child.getAttribValueAsInt("DetailStringID", val) ? gDatabase.getLocStringIndex(val) : -1);
            tribute.mReceivedStringIndex = (child.getAttribValueAsInt("ReceivedStringID", val) ? gDatabase.getLocStringIndex(val) : -1);
            tribute.mItemLoc[0] = (child.getAttribValueAsInt("ItemLoc1", val) ? val : -1);
            tribute.mItemLoc[1] = (child.getAttribValueAsInt("ItemLoc2", val) ? val : -1);
            tribute.mItemLoc[2] = (child.getAttribValueAsInt("ItemLoc3", val) ? val : -1);
            mTributes.add(tribute);
         }
      }
   }
}

//==============================================================================
// BUIGame::loadHoverDecal
//==============================================================================
void BUIGame::loadHoverDecal(const BXMLNode node)
{
   BSimString tempStr;
   mHoverDecalTexture=gD3DTextureManager.getOrCreateHandle(node.getText(tempStr), BFILE_OPEN_DISCARD_ON_CLOSE, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "UIGame");
}

//==============================================================================
// BUIGame::getObjectIcon
//==============================================================================
BManagedTextureHandle BUIGame::getObjectIcon(long protoObjectID, long playerID)
{
   if(protoObjectID==-1)
      return cInvalidManagedTextureHandle;
   if ((protoObjectID < gDatabase.getNumberProtoObjects()) && (mpObjectIcons[protoObjectID]!=cInvalidManagedTextureHandle))
      return mpObjectIcons[protoObjectID];

//-- FIXING PREFIX BUG ID 2668
   const BProtoObject* pPO = NULL;
//--
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPO = pPlayer->getProtoObject(protoObjectID);
   else
      pPO = gDatabase.getGenericProtoObject(protoObjectID);

   if (pPO)
   {
      mpObjectIcons[pPO->getBaseType()]=gUI.loadTexture(pPO->getIcon());
      return mpObjectIcons[pPO->getBaseType()];
   }

   return cInvalidManagedTextureHandle;
}

//==============================================================================
// BUIGame::getTechIcon
//==============================================================================
BManagedTextureHandle BUIGame::getTechIcon(long protoTechID)
{
   if(protoTechID==-1)
      return cInvalidManagedTextureHandle;
   if(mpTechIcons[protoTechID]!=cInvalidManagedTextureHandle)
      return mpTechIcons[protoTechID];
   mpTechIcons[protoTechID]=gUI.loadTexture(gDatabase.getProtoTech(protoTechID)->getIcon());
   return mpTechIcons[protoTechID];
}

//==============================================================================
// BUIGame::getSquadIcon
//==============================================================================
BManagedTextureHandle BUIGame::getSquadIcon(long protoSquadID, long playerID)
{
   if(protoSquadID==-1)
      return cInvalidManagedTextureHandle;
   if ((protoSquadID < gDatabase.getNumberProtoSquads()) && (mpSquadIcons[protoSquadID]!=cInvalidManagedTextureHandle))
      return mpSquadIcons[protoSquadID];

//-- FIXING PREFIX BUG ID 2669
   const BProtoSquad* pPS = NULL;
//--
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
      pPS = pPlayer->getProtoSquad(protoSquadID);
   else
      pPS = gDatabase.getGenericProtoSquad(protoSquadID);

   if (pPS)
   {
      mpSquadIcons[pPS->getBaseType()]=gUI.loadTexture(pPS->getIcon());
      return mpSquadIcons[pPS->getBaseType()];
   }

   return cInvalidManagedTextureHandle;
}

//==============================================================================
// BUIGame::getSquadModeIcon
//==============================================================================
BManagedTextureHandle BUIGame::getSquadModeIcon(long squadMode)
{
   if(squadMode<0 || squadMode>=BSquadAI::cNumberModes)
      return cInvalidManagedTextureHandle;
   return mpSquadModeIcons[squadMode];
}

//==============================================================================
// BUIGame::getObjectCommandIcon
//==============================================================================
BManagedTextureHandle BUIGame::getObjectCommandIcon(long commandType)
{
   if(commandType<0 || commandType>=BProtoObjectCommand::cNumberTypes)
      return cInvalidManagedTextureHandle;
   return mpObjectCommandIcons[commandType];
}

//==============================================================================
// BUIGame::getGameCommandIcon
//==============================================================================
BManagedTextureHandle BUIGame::getGameCommandIcon(long commandType)
{
   if(commandType<0 || commandType>=cGameCommandCount)
      return cInvalidManagedTextureHandle;
   return mGameCommandIcons[commandType];
}

//==============================================================================
// BUIGame::getGameCommandID
//==============================================================================
long BUIGame::getGameCommandID(const char* pCommandName)
{
   if(stricmp(pCommandName, "GlobalRallyPoint")==0)
      return cGameCommandRallyPoint;
   else if(stricmp(pCommandName, "SelectPower")==0)
      return cGameCommandSelectPower;
   else if(stricmp(pCommandName, "Repair")==0)
      return cGameCommandRepair;
   return -1;
}

//==============================================================================
// BUIGame::getReticleSize
//==============================================================================
long BUIGame::getReticleSize() const
{
   if (gRender.getAspectRatioMode()==BRender::cAspectRatioMode4x3)
      return mReticleSize4x3;

   return mReticleSize;
}

//==============================================================================
// BUIGame::playSound
//==============================================================================
void BUIGame::playSound(uint soundType, long civID, bool record)
{
   BCueIndex cueIndex = cInvalidCueIndex;
   cueIndex = gSoundManager.getCueIndexByEnum(soundType);

   if (cueIndex!=cInvalidCueIndex)
   {
      gSoundManager.playCue(cueIndex);
      if (record)
         gRecordGame.recordUIUserSound(cueIndex);
   }
}

//==============================================================================
// BUIGame::getUnitStatIndex
//==============================================================================
long BUIGame::getUnitStatIndex(const char* name)
{
   for (int i = 0; i < mUnitStats.getNumber(); ++i)
   {
      if (mUnitStats[i].mName.compare(name) == 0)
         return i;
   }
   return -1;
}

//==============================================================================
// BUIGame::getViewCenter
//==============================================================================
void BUIGame::getViewCenter(BUser* pUser, long& x, long& y)
{
   int viewportIndex;
   if (pUser == gUserManager.getUser(BUserManager::cPrimaryUser))
      viewportIndex = gViewportManager.getUserViewportIndex(BUserManager::cPrimaryUser);
   else 
      viewportIndex = gViewportManager.getUserViewportIndex(BUserManager::cSecondaryUser);
   
   if (viewportIndex == -1)
   {
      const D3DVIEWPORT9 mv = gViewportManager.getMasterViewport();
      int halfWidth  = mv.Width / 2;
      int halfHeight = mv.Height / 2;
      x = mv.X + halfWidth;
      y = mv.Y + halfHeight; 
      return;
   }
   
   const D3DVIEWPORT9 vp = gViewportManager.getSceneViewport(viewportIndex);
   int halfWidth  = vp.Width / 2;
   int halfHeight = vp.Height / 2;
   x = vp.X + halfWidth;
   y = vp.Y + halfHeight;            
}

//==============================================================================
// BUIGame::getCircleMenuWidth
//==============================================================================
long BUIGame::getCircleMenuWidth(BUser* pUser)
{
   if (gGame.isSplitScreen())
   {
      if (gGame.isVerticalSplit())
         return mCircleMenuWidthSV;
      else
         return mCircleMenuWidthSH;
   }
   else
      return mCircleMenuWidth;
}

//==============================================================================
// BUIGame::getCircleMenuItemRadius
//==============================================================================
float BUIGame::getCircleMenuItemRadius(BUser* pUser)
{
   if (gGame.isSplitScreen())
   {
      if (gGame.isVerticalSplit())
         return mCircleMenuItemRadiusSV;
      else
         return mCircleMenuItemRadiusSH;
   }
   else
      return mCircleMenuItemRadius;
}

//==============================================================================
// BUIGame::getCircleMenuItemWidth
//==============================================================================
long BUIGame::getCircleMenuItemWidth(BUser* pUser)
{
   if (gGame.isSplitScreen())
   {
      if (gGame.isVerticalSplit())
         return mCircleMenuItemWidthSV;
      else
         return mCircleMenuItemWidthSH;
   }
   else
      return mCircleMenuItemWidth;
}
