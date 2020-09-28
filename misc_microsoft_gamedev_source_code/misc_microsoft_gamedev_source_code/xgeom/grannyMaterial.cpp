//-------------------------------------------------------------------------------------------------------
// File: granny_material.cpp
//-------------------------------------------------------------------------------------------------------
#include "xgeom.h"
//#include <d3dx9.h>
//#include <d3dx9shader.h>

// local
#include "grannyMaterial.h"

// extlib
#include "granny.h"

#define DDX_TEXTURE_EXTENSION ".ddx"
#define XPR_TEXTURE_EXTENSION ".xpr"

namespace 
{
   void cleanFilename(const char *name, BString& cleanedName)
   {
      cleanedName.set(name);
      cleanedName.standardizePath();

      long index = cleanedName.findRight(B("\\art\\"));
      if(index != -1)
         cleanedName.crop(index + 5, cleanedName.length());
      
      BString extension;
      strPathGetExtension(cleanedName, extension);
      if ((extension != DDX_TEXTURE_EXTENSION) && (extension != XPR_TEXTURE_EXTENSION))
         strPathRemoveExtension(cleanedName);
   }   
}   

//-------------------------------------------------------------------------------------------------------
// BMaterialBuilder::findStringParam
//-------------------------------------------------------------------------------------------------------
bool BMaterialBuilder::findStringParam(const granny_material* pGrannyMaterial, const char* pName, BString& str)
{
   //str.empty();
   
//-- FIXING PREFIX BUG ID 7636
   const granny_data_type_definition* pDataType = pGrannyMaterial->ExtendedData.Type;
//--
   uint curOfs = 0;
   for ( ; GrannyEndMember != pDataType->Type; curOfs += GrannyGetMemberTypeSize(pDataType), pDataType++)
   {
      if (pDataType->Type != GrannyStringMember)
         continue;
         
      if (strcmp(pDataType->Name, pName) == 0)
      {
         const char* const* ppParamVal = reinterpret_cast<const char* const *>(static_cast<const uchar*>(pGrannyMaterial->ExtendedData.Object) + curOfs);
         if (!ppParamVal)
            continue;
         const char* pParamVal = *ppParamVal;
         if (pParamVal)
         {
            str.set(pParamVal);
            return true;
         }               
      }         
   }
   
   return false;
}

//-------------------------------------------------------------------------------------------------------
// BMaterialBuilder::findIntParam
//-------------------------------------------------------------------------------------------------------
bool BMaterialBuilder::findIntParam(const granny_material* pGrannyMaterial, const char* pName, int& val)
{
   //val = 0;

//-- FIXING PREFIX BUG ID 7637
   const granny_data_type_definition* pDataType = pGrannyMaterial->ExtendedData.Type;
//--
   uint curOfs = 0;
   for ( ; GrannyEndMember != pDataType->Type; curOfs += GrannyGetMemberTypeSize(pDataType), pDataType++)
   {
      if (pDataType->Type != GrannyInt32Member)
         continue;

      if (strcmp(pDataType->Name, pName) == 0)
      {
         const int* pParamVal = reinterpret_cast<const int*>(static_cast<const uchar*>(pGrannyMaterial->ExtendedData.Object) + curOfs);
         if (!pParamVal)
            continue;
            
         if (pParamVal)
         {
            val = *pParamVal;
            return true;
         }
      }         
   }

   return false;
}

//-------------------------------------------------------------------------------------------------------
// BMaterialBuilder::findFloatParam
//-------------------------------------------------------------------------------------------------------
bool BMaterialBuilder::findFloatParam(const granny_material* pGrannyMaterial, const char* pName, float& val)
{
   //val = 0.0f;

//-- FIXING PREFIX BUG ID 7639
   const granny_data_type_definition* pDataType = pGrannyMaterial->ExtendedData.Type;
//--
   uint curOfs = 0;
   for ( ; GrannyEndMember != pDataType->Type; curOfs += GrannyGetMemberTypeSize(pDataType), pDataType++)
   {
      if (pDataType->Type != GrannyReal32Member)
         continue;

      if (strcmp(pDataType->Name, pName) == 0)
      {
         const float* pParamVal = reinterpret_cast<const float*>(static_cast<const uchar*>(pGrannyMaterial->ExtendedData.Object) + curOfs);
         if (!pParamVal)
            continue;

         if (pParamVal)
         {
            val = *pParamVal;
            return true;
         }
      }         
   }
   
   return false;
}

//-------------------------------------------------------------------------------------------------------
// gMaterialParams
//-------------------------------------------------------------------------------------------------------
static struct 
{
   const char* pUVIndexName;
   const char* pUScrollName;
   const char* pVScrollName;
   const char* pWScrollName;
   Unigeom::BMaterial::eMapType mapType;
} gMaterialParams[] = 
{
   "ESEffectParamValue13", "ESEffectParamValue14", "ESEffectParamValue15", "ESEffectParamValue17", Unigeom::BMaterial::cDiffuse,
   "ESEffectParamValue22", "ESEffectParamValue23", "ESEffectParamValue24", "ESEffectParamValue26", Unigeom::BMaterial::cNormal,
   "ESEffectParamValue35", "ESEffectParamValue36", "ESEffectParamValue37", "", Unigeom::BMaterial::cGloss,
   "ESEffectParamValue52", "ESEffectParamValue53", "ESEffectParamValue54", "", Unigeom::BMaterial::cOpacity,
   "ESEffectParamValue62", "ESEffectParamValue63", "ESEffectParamValue64", "", Unigeom::BMaterial::cXForm,
   "ESEffectParamValue72", "ESEffectParamValue73", "ESEffectParamValue74", "", Unigeom::BMaterial::cEmissive,
   "ESEffectParamValue82", "ESEffectParamValue83", "ESEffectParamValue84", "", Unigeom::BMaterial::cAmbOcc,
   "ESEffectParamValue94", "ESEffectParamValue95", "ESEffectParamValue96", "", Unigeom::BMaterial::cEnv,
   "ESEffectParamValue102", "ESEffectParamValue103", "ESEffectParamValue104", "", Unigeom::BMaterial::cEnvMask,
   "ESEffectParamValue152", "ESEffectParamValue153", "ESEffectParamValue154", "", Unigeom::BMaterial::cEmXForm,
   "ESEffectParamValue192", "ESEffectParamValue193", "ESEffectParamValue194", "", Unigeom::BMaterial::cDistortion,
   "ESEffectParamValue202", "ESEffectParamValue203", "ESEffectParamValue204", "", Unigeom::BMaterial::cHighlight,
   "ESEffectParamValue212", "ESEffectParamValue213", "ESEffectParamValue214", "", Unigeom::BMaterial::cModulate,
};

const uint cNumMaterialParams = sizeof(gMaterialParams)/sizeof(gMaterialParams[0]);

//-------------------------------------------------------------------------------------------------------
// gSupportedESEffectMaps
//-------------------------------------------------------------------------------------------------------
static const struct    
{
   const char*                   pName;
   Unigeom::BMaterial::eMapType  mapType;
} gSupportedESEffectMaps[] = 
{
   { "diffuse",            Unigeom::BMaterial::cDiffuse     },
   { "normal",             Unigeom::BMaterial::cNormal      },
   { "gloss",              Unigeom::BMaterial::cGloss       },
   { "opacity",            Unigeom::BMaterial::cOpacity     },
   { "pixelxformColor",    Unigeom::BMaterial::cXForm       },
   { "emissive",           Unigeom::BMaterial::cEmissive    },
   { "AO",                 Unigeom::BMaterial::cAmbOcc      },
   { "environment",        Unigeom::BMaterial::cEnv         },
   { "enviornment",        Unigeom::BMaterial::cEnv         },
   { "reflectionMask",     Unigeom::BMaterial::cEnvMask     },
   { "bump",			      Unigeom::BMaterial::cNormal      },
   { "emPixelxformColor",  Unigeom::BMaterial::cEmXForm     },
   { "distortion",         Unigeom::BMaterial::cDistortion  },
   { "Highlite",           Unigeom::BMaterial::cHighlight   },
   { "Modulate",           Unigeom::BMaterial::cModulate    },
};
const int cNumSupportedESEffectMaps = sizeof(gSupportedESEffectMaps) / sizeof(gSupportedESEffectMaps[0]);
//-------------------------------------------------------------------------------------------------------
// BMaterialBuilder::create
// rg [12/14/07] - This is such a fucking mess
//-------------------------------------------------------------------------------------------------------
bool BMaterialBuilder::create(
   Unigeom::BMaterial& dstMaterial,
   int accessoryIndex, 
   const granny_material* pGrannyMaterial,
   BTextDispatcher* pLog)
{
   pLog;
      
   dstMaterial.setName(pGrannyMaterial ? pGrannyMaterial->Name : "NA");
   dstMaterial.setAccessoryIndex(accessoryIndex);
   
   if (!pGrannyMaterial)
      return true;
   
   int matVersion = -1;
   findIntParam(pGrannyMaterial, "ESEffectVersion", matVersion);
   
   if (pLog)
   {
      pLog->printf("ESEffectVersion %i\n", matVersion);
   }
   
   bool foundNewMaterial = false;
   
   const char* pEffectName = "ESEffectName";
   
   const char* pEffectParamName = "ESEffectParamName";
   uint effectParamNameLen = strlen(pEffectParamName);
      
//-- FIXING PREFIX BUG ID 7644
   const granny_data_type_definition* pDataType = pGrannyMaterial->ExtendedData.Type;
//--
   uint curOfs = 0;
   for ( ; GrannyEndMember != pDataType->Type; curOfs += GrannyGetMemberTypeSize(pDataType), pDataType++)
   {
      if (pDataType->Type != GrannyStringMember)
         continue;
      
      if (strcmp(pDataType->Name, pEffectName) == 0)
      {
         const char* const* ppParamVal = reinterpret_cast<const char* const *>(static_cast<const uchar*>(pGrannyMaterial->ExtendedData.Object) + curOfs);
         if (!ppParamVal)
            continue;
         const char* pParamVal = *ppParamVal;
         if (pParamVal)
         {
            BString matName(pParamVal);
            matName.toLower();
            dstMaterial.setName(matName);
         }
            
         continue;
      }
         
      if (strncmp(pDataType->Name, pEffectParamName, effectParamNameLen) != 0)
         continue;

      const int paramVal = atoi(pDataType->Name + effectParamNameLen);
      if ((paramVal < 1) || (paramVal > 32))
         continue;
         
      const char* const* ppParamName = reinterpret_cast<const char* const *>(static_cast<const uchar*>(pGrannyMaterial->ExtendedData.Object) + curOfs);
      if (!ppParamName)
         continue;
      const char* pParamName = *ppParamName;

      enum
      {
         cSpecPowerIndex = -2,
         cInvalidIndex = -1 
      };           
      int supportedMapTypeIndex = cInvalidIndex;
      
      if (_stricmp("SpecularPower", pParamName) == 0)
         supportedMapTypeIndex = cSpecPowerIndex;
      else
      {
         for (supportedMapTypeIndex = 0; supportedMapTypeIndex < cNumSupportedESEffectMaps; supportedMapTypeIndex++)
            if (_stricmp(gSupportedESEffectMaps[supportedMapTypeIndex].pName, pParamName) == 0)
               break;
         if (supportedMapTypeIndex >= cNumSupportedESEffectMaps)               
            continue;
      }            
      
      const uint mapType = gSupportedESEffectMaps[supportedMapTypeIndex].mapType;
      
      for (uint k = 1; k <= 8; k++)
      {
         BFixedString256 value(cVarArg, "ESEffectParamValue%i%i", paramVal, k);

#if GrannyProductMinorVersion <= 6
         bool findResult = true;
         granny_variant valueResult = GrannyFindMatchingMember(
            pGrannyMaterial->ExtendedData.Type,
            pGrannyMaterial->ExtendedData.Object,
            value.c_str());
#else
         granny_variant valueResult;
         bool findResult = GrannyFindMatchingMember(
            pGrannyMaterial->ExtendedData.Type,
            pGrannyMaterial->ExtendedData.Object,
            value.c_str(), 
            &valueResult);
#endif            

         if ((!findResult) || (!valueResult.Object))
            break;

         if (valueResult.Type->Type == GrannyReal32Member)
         {
            const float* pValue = static_cast<const float*>(valueResult.Object);
            if (pValue)
            {
               switch (mapType)
               {
                  case Unigeom::BMaterial::cEnv:
                  {
                     switch (k)
                     {
                        case 2: 
                        {
                           dstMaterial.setEnvReflectivity(*pValue);
                           break;                            
                        }
                     }
                     break;
                  }
                  case Unigeom::BMaterial::cGloss:
                  {
                     switch (k)
                     {
                        case 4: 
                        {
                           if (valueResult.Type->ArrayWidth == 3)
                           {
                              BVec3 c(pValue);
                              c.clampComponents(0.0f, 1.0f);
                              dstMaterial.setSpecColor(c);
                           }  
                           break;                            
                        }
                     }
                  }                     
               }           
            }
         }
         else if (valueResult.Type->Type == GrannyInt32Member)
         {
            const int* pValue = static_cast<const int*>(valueResult.Object);
            if (pValue)
            {
               int val = *pValue;
               
               if (supportedMapTypeIndex == cSpecPowerIndex)
               {
                  if (val < 3)
                     val = 3;
                  else if (val > 500)
                     val = 500;
                  dstMaterial.setSpecPower(static_cast<float>(val));
                  break;
               }
               else if (mapType == Unigeom::BMaterial::cGloss)
               {
                  switch (k)
                  {
                     case 1: 
                     {
                        dstMaterial.setFlag(Unigeom::BMaterial::cFlagColorGloss, val != 0);
                        break;
                     }
                  }
               }
            }
         }
         else if (valueResult.Type->Type == GrannyStringMember)
         {
            if (supportedMapTypeIndex >= 0)
            {
               const char* const* ppValueName = static_cast<const char* const*>(valueResult.Object);

               if (ppValueName)
               {
                  const char* pValueName = *ppValueName;

                  if ((strlen(pValueName) == 0) || (strstr(pValueName, "UserDefined") != NULL))
                     continue;
                                    
                  BFixedString256 textureName(pValueName);

                  BString cleanedTextureName;
                  cleanFilename(textureName.c_str(), cleanedTextureName);

                  Unigeom::BMap map;      
                  map.setName(cleanedTextureName.getPtr());
                  map.setChannel(0);

                  dstMaterial.setNumMaps((Unigeom::BMaterial::eMapType)mapType, 1);
                  dstMaterial.setMap((Unigeom::BMaterial::eMapType)mapType, 0, map);

                  foundNewMaterial = true;
               }
            }               
         }                  
      }            
   }
   
   if (matVersion >= 9)
   {
      for (uint i = 0; i < cNumMaterialParams; i++)
      {
         const Unigeom::BMaterial::eMapType mapType = gMaterialParams[i].mapType;
      
         int uvIndex = 0;
         if (findIntParam(pGrannyMaterial, gMaterialParams[i].pUVIndexName, uvIndex))
         {
            const uint cMaxMapUVIndex = 3;
            uvIndex = Math::Clamp<int>(uvIndex, 0, cMaxMapUVIndex);
            
            if (dstMaterial.getNumMaps(mapType) > 0)
            {
               dstMaterial.getMap(mapType, 0).setChannel(static_cast<Unigeom::BChannelIndex>(uvIndex));
            }
         }
         
         float uVel = 0.0f;
         const bool foundUVel = findFloatParam(pGrannyMaterial, gMaterialParams[i].pUScrollName, uVel);
         
         float vVel = 0.0f;
         const bool foundVVel = findFloatParam(pGrannyMaterial, gMaterialParams[i].pVScrollName, vVel);

         float wVel = 0.0f;
         const bool foundWVel = findFloatParam(pGrannyMaterial, gMaterialParams[i].pWScrollName, wVel);

         if (foundUVel || foundVVel || foundWVel)
         {
            dstMaterial.setUVWVelocity(mapType, BVec3(uVel, vVel, wVel));
         }
      }
      
      int globalEnv = 0;
      findIntParam(pGrannyMaterial, "ESEffectParamValue95", globalEnv);
      dstMaterial.setFlag(Unigeom::BMaterial::cFlagGlobalEnv, globalEnv != 0);
      
      float envFresnel = 0.5f;
      if (findFloatParam(pGrannyMaterial, "ESEffectParamValue94", envFresnel))
      {
         envFresnel = Math::Clamp(envFresnel, 0.0f, 1.0f);
         dstMaterial.setEnvFresnel(envFresnel);
      }
      
      float envFresnelPower = 4.0f;
      if (findFloatParam(pGrannyMaterial, "ESEffectParamValue96", envFresnelPower))
      {
         envFresnelPower = Math::Clamp(envFresnelPower, 0.0f, 100.0f);
         dstMaterial.setEnvFresnelPower(envFresnelPower);
      }
      
      float envSharpness = 0.0f;
      if (findFloatParam(pGrannyMaterial, "ESEffectParamValue93", envSharpness))
      {
         envSharpness = Math::Clamp(envSharpness, -13.0f, 13.0f);
         dstMaterial.setEnvSharpness(envSharpness);
      }
      
      int shadowCaster = 1;
      if (findIntParam(pGrannyMaterial, "ESEffectParamValue141", shadowCaster))
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagDisableShadows, shadowCaster ? false : true);

      int disableShadowReception = 0;
      if (findIntParam(pGrannyMaterial, "ESEffectParamName221", disableShadowReception))
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagDisableShadowReception, disableShadowReception ? true : false);         
         
      int additiveBlending = 0;
      findIntParam(pGrannyMaterial, "ESEffectParamValue12", additiveBlending);
      
      int alphaBlendType = 0; // 1 - alpha-to-coverage, 2 - alpha blend, 3 - alpha test
      if (matVersion < 17)
      {
         int alphaBlending = 0;
         findIntParam(pGrannyMaterial, "ESEffectParamValue55", alphaBlending);
         if (alphaBlending == 0)
            alphaBlendType = 1;
         else
            alphaBlendType = 2;
      }
      else
      {
         findIntParam(pGrannyMaterial, "ESEffectParamValue57", alphaBlendType);
      }
      
      if (additiveBlending)
         dstMaterial.setBlendType(Unigeom::BMaterial::cBlendAdditive);
      else if (alphaBlendType == 1)
         dstMaterial.setBlendType(Unigeom::BMaterial::cBlendAlphaToCoverage);
      else if (alphaBlendType == 2)
         dstMaterial.setBlendType(Unigeom::BMaterial::cBlendOver);
      else if (alphaBlendType == 3)
         dstMaterial.setBlendType(Unigeom::BMaterial::cBlendAlphaTest);
         
      int twoSided = 0;
      findIntParam(pGrannyMaterial, "ESEffectParamValue131", twoSided);
      if (twoSided)
      {
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagTwoSided, true);
      }
      
      int opacityValid = 0;
      findIntParam(pGrannyMaterial, "ESEffectParamValue121", opacityValid);
      
      int opacity = 100;
      findIntParam(pGrannyMaterial, "ESEffectParamValue122", opacity);
      
      if (opacityValid)
      {
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagOpacityValid, true);
         
         opacity = Math::Clamp(opacity, 0, 100);
         dstMaterial.setOpacity((opacity * 255 + 50) / 100);
      }
      
      int terrainConform = 0;
      findIntParam(pGrannyMaterial, "ESEffectParamValue161", terrainConform);
      if (terrainConform)
      {
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagTerrainConform, true);
         // Geometry conformed to the terrain shouldn't cast shadows
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagDisableShadows, true);
      }

      int localReflection = 0;
      findIntParam(pGrannyMaterial, "ESEffectParamValue171", localReflection);
      if (localReflection)
      {
         dstMaterial.setFlag(Unigeom::BMaterial::cFlagLocalReflection, true);
      }
      
      struct 
      {
         Unigeom::BMaterial::eMapType mapType;
         const char* pName;
      } uvWrapParams[] =
      {
         { Unigeom::BMaterial::cDiffuse,  "ESEffectParamValue16" },
         { Unigeom::BMaterial::cNormal,   "ESEffectParamValue25" },
         { Unigeom::BMaterial::cGloss,    "ESEffectParamValue48" },
         { Unigeom::BMaterial::cOpacity,  "ESEffectParamValue56" },
         { Unigeom::BMaterial::cXForm,    "ESEffectParamValue65" },
         { Unigeom::BMaterial::cEmissive, "ESEffectParamValue75" },
         { Unigeom::BMaterial::cAmbOcc,   "ESEffectParamValue85" },
         { Unigeom::BMaterial::cEnvMask,  "ESEffectParamValue105" },
         { Unigeom::BMaterial::cEmXForm,  "ESEffectParamValue155" },
         { Unigeom::BMaterial::cDistortion, "ESEffectParamValue195" },
         { Unigeom::BMaterial::cHighlight, "ESEffectParamValue205" },
         { Unigeom::BMaterial::cModulate, "ESEffectParamValue215" }
      };         
      
      const uint cNumUVWrapParams = sizeof(uvWrapParams)/sizeof(uvWrapParams[0]);
      
      for (uint i = 0; i < cNumUVWrapParams; i++)
      {
         Unigeom::BMaterial::eMapType mapType = uvWrapParams[i].mapType;
         
         if (dstMaterial.getNumMaps(mapType) > 0)
         {
            int uvWrap = 0;
            findIntParam(pGrannyMaterial, uvWrapParams[i].pName, uvWrap);
            
            if (uvWrap)
            {
               dstMaterial.getMap(mapType, 0).setWrapMode(0);
               dstMaterial.getMap(mapType, 0).setWrapMode(1);
               dstMaterial.getMap(mapType, 0).setWrapMode(2);
            }
            else
            {
               dstMaterial.getMap(mapType, 0).setClampMode(0);
               dstMaterial.getMap(mapType, 0).setClampMode(1);
               dstMaterial.getMap(mapType, 0).setClampMode(2);
            }
         }
      }
   }

   if (!foundNewMaterial)
   {
      for (uint i = 0; i < 32; i++)
      {
         BFixedString256 name(cVarArg, "ESEffectParamName%i", i + 1);      
         BFixedString256 value(cVarArg, "ESEffectParamValue%i", i + 1);      

#if GrannyProductMinorVersion <= 6
         bool findResult = true;
         granny_variant nameResult = GrannyFindMatchingMember(
            pGrannyMaterial->ExtendedData.Type,
            pGrannyMaterial->ExtendedData.Object,
            name.c_str());
#else
         granny_variant nameResult;
         bool findResult = GrannyFindMatchingMember(
            pGrannyMaterial->ExtendedData.Type,
            pGrannyMaterial->ExtendedData.Object,
            name.c_str(),
            &nameResult);
#endif
                  
         if ((!findResult) || (!nameResult.Object))
            break;

#if GrannyProductMinorVersion <= 6
         granny_variant valueResult = GrannyFindMatchingMember(
            pGrannyMaterial->ExtendedData.Type,
            pGrannyMaterial->ExtendedData.Object,
            value.c_str());
#else
         granny_variant valueResult;
         findResult = GrannyFindMatchingMember(
            pGrannyMaterial->ExtendedData.Type,
            pGrannyMaterial->ExtendedData.Object,
            value.c_str(),
            &valueResult);
#endif
         
         if ((!findResult) || (!valueResult.Object))
            break;
            
         if ((!nameResult.Object) || (!valueResult.Object))
            continue;
            
         if ((nameResult.Type->Type == GrannyStringMember) && 
             (valueResult.Type->Type == GrannyStringMember))
         {
            const char* const* ppParamName = static_cast<const char* const*>(nameResult.Object);
            const char* const* ppValueName = static_cast<const char* const*>(valueResult.Object);
            
            if ((ppParamName) && (ppValueName))
            {
               const char* pParamName = *ppParamName;
               const char* pValueName = *ppValueName;
               
               if ((_stricmp(pParamName, "pixelxformColor") == 0) && (_stricmp(pValueName, "playerColor") == 0))
                  continue;
               
               if ((strlen(pValueName) == 0) || (strstr(pValueName, "UserDefined") != NULL))
                  continue;
            
               uint j;
               for (j = 0; j < cNumSupportedESEffectMaps; j++)
                  if (_stricmp(gSupportedESEffectMaps[j].pName, pParamName) == 0)
                     break;

               if (j < cNumSupportedESEffectMaps)   
               {
                  const uint mapType = gSupportedESEffectMaps[j].mapType;
                  BFixedString256 textureName(pValueName);

                  BString cleanedTextureName;
                  cleanFilename(textureName.c_str(), cleanedTextureName);

                  Unigeom::BMap map;      
                  map.setName(cleanedTextureName.getPtr());
                  map.setChannel(0);
                  
                  dstMaterial.setNumMaps((Unigeom::BMaterial::eMapType)mapType, 1);
                  dstMaterial.setMap((Unigeom::BMaterial::eMapType)mapType, 0, map);
                  
                  foundNewMaterial = true;
               }            
            }            
         }         
      } 
   }      
   
   if (!foundNewMaterial)
   {
      // rg [5/29/06] - Temporary (until all meshes are converted to new material.
      
      for (int pass = 0; pass < 2; pass++)
      {
         for (int i = 0; i < pGrannyMaterial->MapCount; i++)
         {
            const granny_material_map& grannyMap = pGrannyMaterial->Maps[i];

            Unigeom::BMaterial::eMapType mapType = Unigeom::BMaterial::cInvalidMapType;
            int mapIndex = 0;

            static const struct 
            {
               char* pMapName;
               Unigeom::BMaterial::eMapType mapType;
               int channel;
               int pass;
            } supportedMaps[] = 
            {
               // -- for backwards compatibility
               { "Diffuse Color",         Unigeom::BMaterial::cDiffuse,  0, 0 },
               { "Specular Color",        Unigeom::BMaterial::cGloss,    0, 0 },
               { "Bump",                  Unigeom::BMaterial::cNormal,   0, 0 },
               { "NormalMap",             Unigeom::BMaterial::cNormal,   0, 1 },
               { "Reflection",            Unigeom::BMaterial::cEnv,      0, 0 },
               { "ReflectionMap",         Unigeom::BMaterial::cEnv,      0, 1 },
               { "EmissiveMap",           Unigeom::BMaterial::cEmissive, 0, 1 },
            };
            const int NumSupportedMaps = sizeof(supportedMaps) / sizeof(supportedMaps[0]);

            int j;
            for (j = 0; j < NumSupportedMaps; j++)
            {
               if (_stricmp(grannyMap.Usage, supportedMaps[j].pMapName) == 0)
               {
                  if (pass == supportedMaps[j].pass)
                  {
                     mapType = supportedMaps[j].mapType;
                     mapIndex = supportedMaps[j].channel;
                     break;
                  }
               }
            }

            if (NumSupportedMaps == j)
               continue;

            if (grannyMap.Material->Texture == NULL)
               continue;
               
            BFixedString256 name(grannyMap.Material->Texture->FromFileName);
                              
            BString cleanedName;
            cleanFilename(name.c_str(), cleanedName);
            
            name.set(cleanedName.getPtr());

            const granny_data_type_definition channelDataTypes[] =
            {
               {GrannyInt32Member, "UVWChannel"},
               {GrannyInt32Member, "UVWTiling"},
               {GrannyEndMember}
            };

            struct 
            {
               int channel;
               int tiling;
            } uvwChannel = { 1 , 3 };

            GrannyMergeSingleObject(grannyMap.Material->ExtendedData.Type, grannyMap.Material->ExtendedData.Object, channelDataTypes, &uvwChannel);

            Unigeom::BMap map;
            map.setName(name.c_str());
            map.setChannel(static_cast<Unigeom::BChannelIndex>(Math::Max(0, uvwChannel.channel - 1)));

            //-- enable clamping if necessary
            if (uvwChannel.tiling == 0)
            {
               map.setClampMode(0);
               map.setClampMode(1);
            }
            else if (uvwChannel.tiling == 1)
            {
               map.setClampMode(0);
            }
            else if (uvwChannel.tiling == 2)
            {
               map.setClampMode(1);
            }

            if (!dstMaterial.getNumMaps(mapType))
            {
               dstMaterial.setNumMaps(mapType, Math::Max(dstMaterial.getNumMaps(mapType), mapIndex + 1));
               dstMaterial.setMap(mapType, mapIndex, map);
            }
         }
      }
   }      
   
   return true;
}

