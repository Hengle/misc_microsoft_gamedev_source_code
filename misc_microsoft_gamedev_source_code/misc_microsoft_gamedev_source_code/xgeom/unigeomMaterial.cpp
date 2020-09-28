// File: unigeomMaterial.cpp
#include "xgeom.h"

#include "math\generalVector.h"
#include "unigeomMaterial.h"

namespace Unigeom
{

BMap::BMap(const char* pName, BChannelIndex channel, uint flags) : 
   mChannel(channel), 
   mFlags(static_cast<ushort>(flags))
{
   mName.set(pName);
   mName.toLower();
}

void BMap::clear(void)
{
   mName.empty();
   mChannel = 0;
   mFlags = cAddressMode_Clamp;
}

bool BMap::operator== (const BMap& b) const
{
   return ((mFlags == b.mFlags) && (mChannel == b.mChannel) && (mName == b.mName));
}

bool BMap::operator< (const BMap& b) const
{
   if (mFlags < b.mFlags)
      return true;
   else if (mFlags == b.mFlags)
   {
      if (mChannel < b.mChannel)
         return true;
      else if (mChannel == b.mChannel)
      {
         if (mName < b.mName)
            return true;
      }
   }
   return false;
}

void BMap::debugCheck(void) const
{
   BDEBUG_ASSERT(strlen(mName) <= MAX_PATH);
   BDEBUG_ASSERT(mChannel <= 16);
}

void BMap::log(BTextDispatcher& log) const
{
   log.printf("Name: \"%s\", BChannelIndex: %u, Flags: %u\n",
      getName().getPtr(),
      (uint)getChannel(),
      (uint)getFlags());
}

bool BMap::write(BBinaryDataTree::BDocumentBuilder::BNode& node) const
{
   node.addAttribute("Name", mName);
   node.addAttribute("Channel", mChannel);
   node.addAttribute("Flags", mFlags);
   return true;
}

bool BMap::read(const BBinaryDataTree::BPackedDocumentReader::BNode& node)
{
   node.getAttributeValue("Name", mName);
   node.getAttributeValue("Channel", mChannel);
   node.getAttributeValue("Flags", mFlags);
   return true;
}

size_t BMap::hash(size_t prevHash) const
{
   prevHash = mName.hash(prevHash);
   prevHash = hashFast(&mChannel, sizeof(mChannel), prevHash);
   prevHash = hashFast(&mFlags, sizeof(mFlags), prevHash);
   return prevHash;
}

void BMapArray::log(BTextDispatcher& log) const
{
   log.printf("Num Textures: %u\n", mMaps.getSize());
   log.indent(1);
   for (uint i = 0; i < mMaps.getSize(); i++)
   {
      log.printf("Texture: %i\n", i);
      mMaps[i].log(log);
   }
   log.indent(-1);
}

bool BMapArray::write(BBinaryDataTree::BDocumentBuilder::BNode& node) const
{
   for (uint i = 0; i < mMaps.getSize(); i++)
   {
      BBinaryDataTree::BDocumentBuilder::BNode mapNode(node.addChild("Map"));
      mMaps[i].write(mapNode);
   }

   return true;
}

bool BMapArray::read(const BBinaryDataTree::BPackedDocumentReader::BNode& node)
{
   mMaps.resize(0);

   for (uint i = 0; i < node.getNumChildren(); i++)
   {
      BBinaryDataTree::BPackedDocumentReader::BNode childNode(node.getChild(i));
      if (childNode.compareName("Map") == 0)
      {
         BMap& map = mMaps.grow();
         if (!map.read(childNode))
            return false;
      }
   }

   return true;
}

size_t BMapArray::hash(size_t prevHash) const
{
   const uint size = mMaps.getSize();
   prevHash = hashFast(&size, sizeof(size), prevHash);

   for (uint i = 0; i < mMaps.getSize(); i++)
      prevHash = mMaps[i].hash(prevHash);

   return prevHash;
}

BMaterial::BMaterial()
{
   mNameValueMap.setKeepSorted(true);
   clear();
}

BMaterial::BMaterial(const char* pName)
{
   mNameValueMap.setKeepSorted(true);
   
   clear();
   
   mName = pName;
}

BMaterial::BMaterial(const BMaterial& other) :
   mName(other.mName),
   mNameValueMap(other.mNameValueMap)
{
   for (uint i = 0; i < cNumMapTypes; i++)
   {
      mMaps[i] = other.mMaps[i];
      mUVWVelocity[i] = other.mUVWVelocity[i];
   }
}

void BMaterial::clear(void)
{
   mName.empty();

   for (int i = 0; i < BMaterial::cNumMapTypes; i++)
      mMaps[i].clear();
   
   Utils::ClearObj(mUVWVelocity);
         
   mNameValueMap.clear();
   mNameValueMap.set("SpecPower", 10.0f);
   mNameValueMap.set("SpecColorR", 1.0f);
   mNameValueMap.set("SpecColorG", 1.0f);
   mNameValueMap.set("SpecColorB", 1.0f);
   mNameValueMap.set("EnvReflectivity", 1.0f);
   mNameValueMap.set("EnvSharpness", 0.0f);
   mNameValueMap.set("EnvFresnel", .5f);
   mNameValueMap.set("EnvFresnelPower", 4.0f);
   mNameValueMap.set("AccessoryIndex", (uint32)0);
   mNameValueMap.set("Flags", (uint32)0);
   mNameValueMap.set("BlendType", (uint8)cBlendAlphaToCoverage);
   mNameValueMap.set("Opacity", (uint8)255);
}

const char* BMaterial::getMapName(eMapType mapType) 
{
   switch (mapType)
   {
      case cDiffuse:    return "diffuse";
      case cNormal:     return "normal";
      case cGloss:      return "gloss";
      case cOpacity:    return "opacity";
      case cXForm:      return "xform";
      case cEmissive:   return "emissive"; 
      case cAmbOcc:     return "ao";
      case cEnv:        return "env";
      case cEnvMask:    return "envmask";
      case cEmXForm:    return "emxform";
      case cDistortion: return "distortion";
      case cHighlight:  return "highlight";
      case cModulate:   return "modulate";
   }
   return "";
}

BMaterial::eMapType BMaterial::findMapType(const char* pName) 
{
   for (uint i = 0; i < cNumMapTypes; i++)
      if (_stricmp(pName, getMapName((eMapType)i)) == 0)
         return (eMapType)i;
   
   return BMaterial::cInvalidMapType;
}
   
bool BMaterial::operator== (const BMaterial& b) const
{
   if (mName != b.mName)
      return false;

   for (uint i = 0; i < cNumMapTypes; i++)
   {
      if (mMaps[i].getSize() != b.mMaps[i].getSize())
         return false;

      for (uint j = 0; j < mMaps[i].getSize(); j++)
         if (mMaps[i][j] != b.mMaps[i][j])
            return false;
            
      if (mUVWVelocity[i] != b.mUVWVelocity[i])
         return false;               
   }
   
   return mNameValueMap == b.mNameValueMap;
}

bool BMaterial::operator< (const BMaterial& b) const
{
   const uint lhsAccessoryIndex = getAccessoryIndex();
   const uint rhsAccessoryIndex = b.getAccessoryIndex();
   
   if (lhsAccessoryIndex != rhsAccessoryIndex)
      return lhsAccessoryIndex < rhsAccessoryIndex;
   
#define COMP(a) do { if (a < b.a) return true; else if (a != b.a) return false; } while(0)
   for (int i = 0; i < cNumMapTypes; i++)
   {
      if (mMaps[i].getSize() < b.mMaps[i].getSize())
         return true;
      else if (mMaps[i].getSize() != b.mMaps[i].getSize())
         return false;

      for (uint j = 0; j < mMaps[i].getSize(); j++)
      {
         if (mMaps[i][j] < b.mMaps[i][j])
            return true;
         else if (mMaps[i][j] != b.mMaps[i][j])
            return false;
      }
      
      if (mUVWVelocity[i] < b.mUVWVelocity[i])
         return true;
      else if (mUVWVelocity[i] != b.mUVWVelocity[i])
         return false;
   }
#undef COMP            
   
   if (mName < b.mName)
      return true;
   else if (mName != b.mName)
      return false;

   if (mNameValueMap < b.mNameValueMap)
      return true;
      
   return false;
}

void BMaterial::log(BTextDispatcher& log) const
{
   log.printf("BMaterial Name: \"%s\"\n", mName.getPtr());

   log.printf("AccessoryIndex: 0x%X\n", (uint)getAccessoryIndex());
               
   for (int i = 0; i < BMaterial::cNumMapTypes; i++)
   {
      log.printf("Map %i (%s) (UVW Vel: %f,%f,%f):\n", i, getMapName(static_cast<eMapType>(i)), (float)mUVWVelocity[i][0], (float)mUVWVelocity[i][1], (float)mUVWVelocity[i][2]);
      log.indent(1);
      mMaps[i].log(log);
      log.indent(-1);
   }

   BString str;
   for (uint i = 0; i < mNameValueMap.getNumNameValues(); i++)
      log.printf("%s: %s\n", mNameValueMap.getName(i).getPtr(), mNameValueMap.convertToString(i, str).getPtr());
   
   log.printf("FlagColorGloss: %u\n", getFlag(cFlagColorGloss));
   log.printf("FlagTwoSided: %u\n", getFlag(cFlagTwoSided));
   log.printf("DisableShadows: %u\n", getFlag(cFlagDisableShadows));
   log.printf("GlobalEnv: %u\n", getFlag(cFlagGlobalEnv));
   log.printf("TerrainConform: %u\n", getFlag(cFlagTerrainConform));
   log.printf("LocalReflection: %u\n", getFlag(cFlagLocalReflection));
   log.printf("DisableShadowReception: %u\n", getFlag(cFlagDisableShadowReception));
}

void BMaterial::debugCheck(void) const
{
   for (int i = 0; i < BMaterial::cNumMapTypes; i++)
   {
      mMaps[i].debugCheck();
      mUVWVelocity[i].debugCheck();
   }
}

BMaterial::operator size_t() const
{
   size_t hash = (size_t)mNameValueMap;
   hash = mName.hash(hash);
   hash = hashFast(&mUVWVelocity, sizeof(mUVWVelocity), hash);
   
   for (uint i = 0; i < cNumMapTypes; i++)
      hash = mMaps[i].hash(hash);
   
   return hash;
}

bool BMaterial::write(BBinaryDataTree::BDocumentBuilder::BNode& node) const
{
   node.addAttribute("Name", mName);
   node.addAttribute("Ver", (uint32)cMaterialVersion);
      
   BBinaryDataTree::BDocumentBuilder::BNode nameValueNode(node.addChild("NameValues"));
   if (!nameValueNode.addNameValueMap(mNameValueMap, false))
      return false;
   
   BBinaryDataTree::BDocumentBuilder::BNode mapsNode(node.addChild("Maps"));
      
   for (uint i = 0; i < cNumMapTypes; i++)
   {
      BBinaryDataTree::BDocumentBuilder::BNode mapNode(mapsNode.addChild(getMapName((eMapType)i)));
      
      if (!mMaps[i].write(mapNode))
         return false;
        
      mapNode.addAttribute("UVWVel", &mUVWVelocity[i], sizeof(mUVWVelocity[i]), BBinaryDataTree::gFloatTypeDesc, 3);
   }  
      
   return true;
}

bool BMaterial::read(const BBinaryDataTree::BPackedDocumentReader::BNode& node)
{
   clear();
   
   if ((!node.doesAttributeExist("Name")) || (!node.doesAttributeExist("Ver")))
      return false;
   
   node.getAttributeValue("Name", mName);
   
   uint version = 0;
   node.getAttributeValue("Ver", version);
   
   BBinaryDataTree::BPackedDocumentReader::BNode nameValueNode;
   if (node.findChild("NameValues", nameValueNode))
   {
      if (!nameValueNode.getNameValueMap(mNameValueMap, false))
      {
         clear();
         return false;
      }
   }
   
   BBinaryDataTree::BPackedDocumentReader::BNode mapsNode;
   if (node.findChild("Maps", mapsNode))
   {  
      for (uint i = 0; i < mapsNode.getNumChildren(); i++)
      {
         BBinaryDataTree::BPackedDocumentReader::BNode mapNode(mapsNode.getChild(i));
         const char* pMapNodeName = mapNode.getName();
         
         const eMapType mapType = findMapType(pMapNodeName);
         
         if (mapType < BMaterial::cInvalidMapType)
            continue;
            
         if (!mMaps[mapType].read(mapNode))
         {
            clear();
            return false;
         }
                           
         BBinaryDataTree::BPackedDocumentReader::BValue value;
         if (mapNode.findAttribute("UVWVel", value))
         {
            if (value.getArraySize() >= 3)
               mUVWVelocity[mapType].set(value.asFloat(0), value.asFloat(1), value.asFloat(2));
         }
      }
      
      return true;
   }
 
   return true;  
}

} // namespace Unigeom






