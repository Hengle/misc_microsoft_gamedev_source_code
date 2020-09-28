//==============================================================================
// buildingdamage.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "common.h"
#include "buildingdamage.h"
#include "xmlreader.h"
#include "gamedirectories.h"

BBuildingDamage gBuildingDamage;

void BBuildingDamage::init(void)
{
   deinit();

   long              i;
   BXMLReader        reader;

   if (!reader.load(cDirData, B("buildingDamage.xml")))
      BFAIL("Couldn't load buildingDamage.xml");

   const BXMLNode root(reader.getRootNode());
   BXMLNode node;
   
   const BPackedString rootName(root.getName());

   // make sure we are dealing with the right kind of file
   if (rootName.compare(B("Damage")) != 0)
      BFAIL("Invalid data in buildingDamage.xml");

   // walk the tree and respond to the various tags appropriately
   long childCount = root.getNumberChildren();
   if (childCount < 2)
      BFAIL("Invalid data in buildingDamage.xml");

   // Get bump texture
   BSimString  bumpTextureName;
   node = root.getChild(0L);
   node.getChildValue(B("Bump"), &bumpTextureName);
   // SLB: TODO send this info to the graphics system when it's ready
//   BGrannyInstance::setRevealBumpTextureIndex(getOrCreate(bumpTextureName->getPtr()));

   // loop through each tag to get damage specific data
   for (i = 1; i < childCount; i++)
   {
      BBuildingDamageInfo  damageInfo;
      BSimString  textureName;

      node = root.getChild(i);

      node.getChildValue(B("Name"), &textureName);
      damageInfo.name = textureName;

      node.getChildValue(B("Texture"), &textureName);
      //damageInfo.damageTextureID = getOrCreate(textureName.getPtr(), cTRTStatic, cInvalidEventReceiverHandle, cDefaultTextureBlack);

      node.getChildValue(B("DamageBrush"), &textureName);
      //damageInfo.damageBrushTextureID = getOrCreate(textureName.getPtr(), cTRTStatic, cInvalidEventReceiverHandle, cDefaultTextureBlack);

      node.getChildValue(B("ScorchBrush"), &textureName);
      //damageInfo.scorchBrushTextureID = getOrCreate(textureName.getPtr(), cTRTStatic, cInvalidEventReceiverHandle, cDefaultTextureBlack);

      node.getChildValue(B("DamageRadius"), damageInfo.damageRadius);

      node.getChildValue(B("ScorchRadius"), damageInfo.scorchRadius);

      node.getChildValue(B("HotIntensity"), damageInfo.hotIntensity);

      node.getChildValue(B("ColdIntensity"), damageInfo.coldIntensity);

      node.getChildValue(B("Duration"), damageInfo.duration);

      gBuildingDamage.addDamageInfo(damageInfo);
   }
}

void BBuildingDamage::deinit(void)
{
   mBuildingInfo.clear();
}

BBuildingDamage::BBuildingDamage()
{
}

BBuildingDamage::~BBuildingDamage()
{
}
