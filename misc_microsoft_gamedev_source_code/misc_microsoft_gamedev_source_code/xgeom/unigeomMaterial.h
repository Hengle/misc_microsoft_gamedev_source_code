// File: unigeom_material.h
#pragma once

#include "math\generalVector.h"
#include "containers\binaryDataTree.h"
#include "containers\binaryDataTreeReader.h"
#include "containers\nameValueMap.h"
#include "utils\packedArray.h"

#define UGX_MATERIAL_MULTIFRAME_SUBSTRING "multiframe_01"      
#define UGX_MATERIAL_MULTIFRAME_INDEX_OFS 11

namespace Unigeom
{
   typedef short BChannelIndex;
   
   class BMap : public Utils::RelativeOperators<BMap>
   {
   public:
      BMap() : mChannel(cInvalidIndex), mFlags(cAddressMode_Clamp) { }

      BMap(const char* pName, BChannelIndex channel, uint flags);
           
      void clear(void);

      bool operator== (const BMap& b) const;
      bool operator< (const BMap& b) const;
      
      void debugCheck(void) const;
            
      void log(BTextDispatcher& log) const;
      
      const BString& getName(void) const        { return mName; }
      void setName(const char* pName)           { mName.set(pName); }
      
      BChannelIndex getChannel(void) const      { return mChannel; }
      void setChannel(BChannelIndex index)      { mChannel = index; }
      
      uint getFlags(void) const                 { return mFlags; }
      void setFlags(int flags)                  { mFlags = static_cast<ushort>(flags); }
      void setWrapMode (int axis = 0)           { mFlags = static_cast<ushort>(mFlags & (~(1 << axis))); }
      void setClampMode(int axis = 0)           { mFlags = static_cast<ushort>(mFlags | (1 << axis)); } 
      bool getClampMode(int axis = 0) const     { return (mFlags & (1 << axis)) != 0;}
                  
      bool write(BBinaryDataTree::BDocumentBuilder::BNode& node) const;
            
      bool read(const BBinaryDataTree::BPackedDocumentReader::BNode& node);
      
      size_t hash(size_t prevHash) const;
            
   protected:
      enum 
      {
         cAddressMode_Wrap    = 0x00000000,
         cAddressMode_Clamp   = 0x00000007,
         cAddressMode_Clamp_U = 0x00000001,
         cAddressMode_Clamp_V = 0x00000002,
         cAddressMode_Clamp_W = 0x00000004,
      };

      BString        mName;
      BChannelIndex  mChannel;
      ushort         mFlags;
   };
      
   //---------------------------------------------------------------------------------

   class BMapArray
   {
   public:
      BMapArray() { }
     
      void clear(void) { mMaps.clear(); }

      uint getSize(void) const { return mMaps.getSize(); }

      void resize(uint size) { mMaps.resize(size); }
            
      const BMap& operator[] (uint i) const { return mMaps[i]; }
            BMap& operator[] (uint i)       { return mMaps[i]; }

      uint addMap(const BMap& map) { mMaps.pushBack(map); }

      void debugCheck(void) const { for (uint i = 0; i < mMaps.getSize(); i++) mMaps[i].debugCheck(); }
     
      void log(BTextDispatcher& log) const;
      
      bool write(BBinaryDataTree::BDocumentBuilder::BNode& node) const;
      
      bool read(const BBinaryDataTree::BPackedDocumentReader::BNode& node);
      
      size_t hash(size_t prevHash) const;

   protected:
      BDynamicArray<BMap> mMaps;
   };

   //---------------------------------------------------------------------------------

   class BMaterial : public Utils::RelativeOperators<BMaterial>
   {
   public:
      // v1 introduced distortion map
      // v2 introduced the highlight map
      // v3 introduced the modulate map
      // v4 introduced disable shadow reception flag
      enum { cMaterialVersion = 4 };
      
      enum eMapType
      {
         // If you add a new map, be sure to update BMaterial::getMapName()!
         cInvalidMapType = -1,

         cFirstMapType,

         cDiffuse = cFirstMapType,     
         cNormal,
         cGloss,
         cOpacity,
         cXForm,
         cEmissive,
         cAmbOcc,
         cEnv,
         cEnvMask,
         cEmXForm,   
         cDistortion,
         cHighlight,
         cModulate,
         
         cNumMapTypes
      };
            
      enum 
      { 
         cMaxMapsPerType = 1,
         cMaxMultiframeTextures = 4
      };
                  
      BMaterial();
      BMaterial(const char* pName);
      BMaterial(const BMaterial& other);
      
      void clear(void);
                        
      // Material name
      const BString& getName(void) const { return mName; }
      void setName(const BString& name) { mName = name; }

      // Maps
      int getNumMaps(eMapType mapType) const { return mMaps[debugRangeCheck(mapType, cNumMapTypes)].getSize(); }
      void setNumMaps(eMapType mapType, int numMaps) { mMaps[debugRangeCheck(mapType, cNumMapTypes)].resize(numMaps); }

      static const char* getMapName(eMapType mapType);
      static eMapType findMapType(const char* pName);
      
      const BMap& getMap(eMapType mapType, int slot) const { return mMaps[debugRangeCheck(mapType, cNumMapTypes)][slot]; }
            BMap& getMap(eMapType mapType, int slot)       { return mMaps[debugRangeCheck(mapType, cNumMapTypes)][slot]; }

      void setMap(eMapType mapType, int slot, const BMap& map) { mMaps[debugRangeCheck(mapType, cNumMapTypes)][slot] = map; }

      const BMapArray& getMapArray(eMapType mapType) const   { return mMaps[debugRangeCheck(mapType, cNumMapTypes)]; }
            BMapArray& getMapArray(eMapType mapType)         { return mMaps[debugRangeCheck(mapType, cNumMapTypes)]; }
            
      const BVec3& getUVWVelocity(eMapType mapType) const { return mUVWVelocity[debugRangeCheck(mapType, cNumMapTypes)]; }            
      void setUVWVelocity(eMapType mapType, const BVec3& velocity) { mUVWVelocity[debugRangeCheck(mapType, cNumMapTypes)] = velocity; }            
      
      // Flags
      enum 
      {
         cFlagColorGloss             = 0,
         cFlagOpacityValid           = 1,
         cFlagTwoSided               = 2,
         cFlagDisableShadows         = 3,
         cFlagGlobalEnv              = 4,
         cFlagTerrainConform         = 5,
         cFlagLocalReflection        = 6,
         cFlagDisableShadowReception = 7
      };
            
      // Name-Values pairs
      const BNameValueMap& getNameValueMap() const { return mNameValueMap; }
            BNameValueMap& getNameValueMap()       { return mNameValueMap; }

      DWORD getFlagDWORD() const { return mNameValueMap.getUInt32("Flags", 0); }
      void setFlagDWORD(DWORD d) { mNameValueMap.set("Flags", (uint32)d); }

      bool getFlag(uint flagIndex) const { return (getFlagDWORD() & (1U << flagIndex)) != 0; }
      void setFlag(uint flagIndex, bool val) { if (val) setFlagDWORD(getFlagDWORD() | (1U << flagIndex)); else setFlagDWORD(getFlagDWORD() & (~(1U << flagIndex))); }
            
      float getSpecPower(void) const               { return mNameValueMap.getFloat("SpecPower", 10.0f); }
      void setSpecPower(float power)               { mNameValueMap.set("SpecPower", power); }
      
      BVec3 getSpecColor(void) const               { return BVec3(mNameValueMap.getFloat("SpecColorR", 1.0f), mNameValueMap.getFloat("SpecColorG", 1.0f), mNameValueMap.getFloat("SpecColorB", 1.0f)); }
      void setSpecColor(const BVec3& color)        { mNameValueMap.set("SpecColorR", color[0]); mNameValueMap.set("SpecColorG", color[1]); mNameValueMap.set("SpecColorB", color[2]); }
      
      float getEnvReflectivity(void) const         { return mNameValueMap.getFloat("EnvReflectivity", 1.0f); }
      void setEnvReflectivity(float reflectivity)  { mNameValueMap.set("EnvReflectivity", reflectivity); }
      
      float getEnvSharpness(void) const            { return mNameValueMap.getFloat("EnvSharpness", 0.0f); }
      void setEnvSharpness(float sharpness)        { mNameValueMap.set("EnvSharpness", sharpness); }
      
      float getEnvFresnel(void) const              { return mNameValueMap.getFloat("EnvFresnel", .5f); }
      void setEnvFresnel(float fresnel)            { mNameValueMap.set("EnvFresnel", fresnel); }
      
      float getEnvFresnelPower(void) const         { return mNameValueMap.getFloat("EnvFresnelPower", 4.0f); }
      void setEnvFresnelPower(float fresnelPower)  { mNameValueMap.set("EnvFresnelPower", fresnelPower); }
      
      uint getAccessoryIndex(void) const           { return mNameValueMap.getUInt32("AccessoryIndex", 0); }
      void setAccessoryIndex(uint i)               { mNameValueMap.set("AccessoryIndex", i); }
                        
      enum 
      {
         // Alpha to coverage
         cBlendAlphaToCoverage,
         // Additive
         cBlendAdditive,
         // Over operator
         cBlendOver,
         // Alphatest
         cBlendAlphaTest
      };
      
      uint getBlendType(void) const { return mNameValueMap.getUInt8("BlendType", cBlendAlphaToCoverage); }
      void setBlendType(uint blendType) { mNameValueMap.set("BlendType", (uint8)blendType); }
      
      uint getOpacity(void) const { return mNameValueMap.getUInt8("Opacity", (uint8)255); }
      void setOpacity(uint opacity) { mNameValueMap.set("Opacity", (uint8)opacity); }
                              
      bool operator== (const BMaterial& b) const;
      bool operator< (const BMaterial& b) const;
      
      operator size_t() const;
                                          
      void log(BTextDispatcher& log) const;
      
      void debugCheck(void) const;
      
      bool write(BBinaryDataTree::BDocumentBuilder::BNode& node) const;
      bool read(const BBinaryDataTree::BPackedDocumentReader::BNode& node);
                  
   protected:
      BString        mName;
            
      BMapArray      mMaps[cNumMapTypes];
      BVec3          mUVWVelocity[cNumMapTypes];
            
      BNameValueMap  mNameValueMap;
   };

} // namespace Unigeom

#include "unigeomMaterial.inl"


