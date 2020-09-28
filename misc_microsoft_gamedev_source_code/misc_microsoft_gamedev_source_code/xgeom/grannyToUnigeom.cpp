// File: granny_to_unigeom.cpp
// Copyright (c) Microsoft Corporation. All rights reserved.
#include "xgeom.h"

// local
#include "grannyToUnigeom.h"
#include "grannyVertexConverter.h"
#include "fastDeformer.h"

// extlib
#include "granny.h"

#define POST_GRANNY_EXPORT_TOOL_NAME "ESPostExport"

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::BGrannyToUnigeom
//-------------------------------------------------------------------------------------------------------
BGrannyToUnigeom::BGrannyToUnigeom() :
   mSuccess(false),
   mAllowSkinning(false),
   mAllowTangentBinormal(false),
   mpFileInfo(NULL),
   mpLog(NULL),
   mMeshFirstVertex(0)
{
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::BGrannyToUnigeom
//-------------------------------------------------------------------------------------------------------
BGrannyToUnigeom::BGrannyToUnigeom(granny_file_info* pFileInfo, bool allowSkinning, bool allowTangentBinormal, BTextDispatcher* pLog) :
   mSuccess(false),
   mAllowSkinning(allowSkinning),
   mAllowTangentBinormal(allowTangentBinormal),
   mpFileInfo(checkNull(pFileInfo)),
   mpLog(pLog),
   mMeshFirstVertex(0)
{
   createUnimodel();
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::setError
//-------------------------------------------------------------------------------------------------------
void BGrannyToUnigeom::setError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   
   mErrorDesc.formatArgs(pMsg, args);
      
   va_end(args);  
   
   if (mpLog)
      mpLog->printf(cTCError, "BGrannyToUnigeom::setError: %s\n", mErrorDesc.getPtr());
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::convert
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::convert(granny_file_info* pFileInfo, bool allowSkinning, bool allowTangentBinormal, BTextDispatcher* pLog)
{
   mSuccess = false;
   mAllowSkinning = allowSkinning;
   mAllowTangentBinormal = allowTangentBinormal;
   mpFileInfo = checkNull(pFileInfo);
   mpLog = pLog;
   mMeshFirstVertex = 0;
      
   return createUnimodel();
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::isPostProcessorAtLeastVersion
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::isPostProcessorAtLeastVersion(int major, int minor)
{
   if (strcmp(mpFileInfo->ArtToolInfo->FromArtToolName, POST_GRANNY_EXPORT_TOOL_NAME) != 0)
      return false;

   if (mpFileInfo->ArtToolInfo->ArtToolMajorRevision < major)
      return false;
   else if (mpFileInfo->ArtToolInfo->ArtToolMajorRevision > major)
      return true;

   if (mpFileInfo->ArtToolInfo->ArtToolMinorRevision >= minor)
      return true;
      
   return false;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::hasFastDeformerData
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::hasFastDeformerData(const granny_mesh* pMesh)
{
   if (!isPostProcessorAtLeastVersion(1, 5))
      return false;
      
   granny_data_type_definition GrannyFastDeformByteArrayDef[] =
   {
      {GrannyUInt8Member, "FD"},
      {GrannyEndMember},
   };

   return 0 != GrannyDataTypesAreEqual(GrannyGetMeshVertexType(pMesh), GrannyFastDeformByteArrayDef);
}   

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::convertVertices
//-------------------------------------------------------------------------------------------------------
int BGrannyToUnigeom::convertVertices(const granny_model* pModel, int modelIndex, const granny_mesh* pMesh, int meshIndex)
{
   granny_mesh_binding* pBinding = GrannyNewMeshBinding(const_cast<granny_mesh*>(pMesh), pModel->Skeleton, pModel->Skeleton);

   const int* pToBoneIndices = GrannyGetMeshBindingToBoneIndices(pBinding);
   BVERIFY(pToBoneIndices);
   
   const int boneBindingCount = pMesh->BoneBindingCount;
         
   const granny_vertex_data* pVertData = pMesh->PrimaryVertexData;

   BGrannyVertexConverter vertConverter;
   
   const bool hasFastDeformerData = this->hasFastDeformerData(pMesh);
   BFastMeshDeformer::BDeformer* pFastDeformer = NULL;
   BDynamicArray<uchar, 16> tempFastDeformerData;
   
   int vertexCount = 0;
   
   if (hasFastDeformerData)
   {
      pFastDeformer = new BFastMeshDeformer::BDeformer;

      bool success = pFastDeformer->crcCheck(pVertData->Vertices, pVertData->VertexCount);
      if (success)
      {
         tempFastDeformerData.pushBack(reinterpret_cast<const uchar*>(pMesh->PrimaryVertexData->Vertices), pMesh->PrimaryVertexData->VertexCount);
         
#ifdef XBOX
         pFastDeformer->endianSwap(tempFastDeformerData.getPtr(), tempFastDeformerData.size());
#endif

         success = pFastDeformer->setData(tempFastDeformerData.getPtr(), tempFastDeformerData.size());
         if (success)
         {
            vertexCount = pFastDeformer->getHeader()->mNumVerts;
         }
      }

      if (!success)
      {
         setError("Bad fast deformer data!");
         delete pFastDeformer;
         return -1;
      }
   }
   else
   {
      vertexCount = pMesh->PrimaryVertexData->VertexCount;
      
      if (!vertConverter.init(pVertData->VertexType, mAllowSkinning, mAllowTangentBinormal))
         return -1;
   }         
               
   Unigeom::MorphTarget& morphTarget = mGeom.morphTarget(0);
   
   mMeshFirstVertex = morphTarget.size();
   
   for (int vertIndex = 0; vertIndex < vertexCount; vertIndex++)
   {
      Univert vert;
      vert.clear();
      
      if (hasFastDeformerData)
      {
         BFastMeshDeformer::BGrannyBumpVert bumpVert;
         
         Utils::ClearObj(bumpVert);
         pFastDeformer->getVertex(bumpVert, vertIndex);
         
         for (uint i = 0; i < 3; i++)
         {
            vert.p[i] = bumpVert.Position[i];
            vert.n[i] = bumpVert.Normal[i];
            vert.t[0][i] = bumpVert.Tangent[i];
         }
                  
         vert.uv[0][0] = bumpVert.UV[0];
         vert.uv[0][1] = bumpVert.UV[1];
         
         for (uint i = 0; i < 4; i++)
         {
            vert.indices[i] = bumpVert.BoneIndices[i];
            vert.weights[i] = bumpVert.BoneWeights[i] / 255.0f;
            if (0 == vert.weights[i])
               vert.indices[i] = 0;
         }         
                     
         const float tangentLen = vert.t[0].len();
         
         // rg [1/31/06] - Due to quantization, the vector may be very close to zero length, but not quite.
         if (tangentLen < .125f)
            vert.t[0].setZero();

#if 0            
         else if (tangentLen < .8f)
         {
            trace("%f\n", tangentLen);
         }            
#endif         
      }
      else
      {
         vert = vertConverter.getVert(pVertData->Vertices, vertIndex);
      }
      
      // Must negate tangent, I think this has to do with the Max->Game coord system conversion.
      vert.tangent(0) = -vert.tangent(0);
      vert.binormal(0) = -vert.binormal(0);
      
      const float tangentLen = vert.t[0].len();
      if (tangentLen)
      {
         BVec3 recoveredBinormal(BVec3(vert.tangent(0)) % vert.norm());
         
         if ((recoveredBinormal * BVec3(vert.binormal(0))) < 0.0f)
         {
            vert.tangent(0) *= .5f;
         }
      }
      
      if ( (boneBindingCount) && ((hasFastDeformerData) || (vertConverter.getHasIndices())) )
      {
         for (int infIndex = 0; infIndex < Univert::MaxInfluences; infIndex++)
         {
            if (vert.weights[infIndex] > 0.0f)
            {
               // translate granny bone index to internal bone index
               if (vert.indices[infIndex] >= boneBindingCount)
               {
                  delete pFastDeformer;
                  return -1;
               }
               
               const int grannyBoneIndex = pToBoneIndices[debugRangeCheck(vert.indices[infIndex], boneBindingCount)];
               
               const int internalBoneIndex = 
                  mGrannyModelSkeletons[
                     debugRangeCheck(modelIndex, mGrannyModelSkeletons.size())  ].mFirstBone + grannyBoneIndex;
               
               BASSERT(mBoneRefs.at(internalBoneIndex).mModelIndex == modelIndex);
               vert.indices[infIndex] = static_cast<uint8>(internalBoneIndex);
               
               mMeshFirstBone[meshIndex] = Math::Min(mMeshFirstBone[meshIndex], internalBoneIndex);
               mMeshLastBone[meshIndex] = Math::Max(mMeshLastBone[meshIndex], internalBoneIndex);
            }
         }
      }
      else if (mAllowSkinning)
      {
         if (boneBindingCount != 1)
         {
            delete pFastDeformer;
            return -1;
         }
                     
         const int grannyBoneIndex = pToBoneIndices[0];
         const int internalBoneIndex = mGrannyModelSkeletons[debugRangeCheck(modelIndex, mGrannyModelSkeletons.size())].mFirstBone + grannyBoneIndex;
         
         BASSERT(mBoneRefs.at(internalBoneIndex).mModelIndex == modelIndex);
         vert.indices[0] = static_cast<uint8>(internalBoneIndex);
         
         mMeshFirstBone[meshIndex] = Math::Min(mMeshFirstBone[meshIndex], internalBoneIndex);
         mMeshLastBone[meshIndex] = Math::Max(mMeshLastBone[meshIndex], internalBoneIndex);
      }
                                 
      mGeom.insertVert(vert);
      
      Unigeom::MorphVert morphVert;
      
      morphVert.p = vert.p;
      morphVert.n = vert.n;
      morphTarget.insertVert(morphVert);
   }
         
   GrannyFreeMeshBinding(pBinding);
   
   delete pFastDeformer;
   return vertexCount;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::convertTopology
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::convertTopology(int modelIndex, int meshIndex, const granny_mesh* pMesh)
{
   modelIndex;
   
   IntVec meshOrigToNewMaterialIndices;
   meshOrigToNewMaterialIndices.resize(Math::Max(1, pMesh->MaterialBindingCount));
   std::fill(meshOrigToNewMaterialIndices.begin(), meshOrigToNewMaterialIndices.end(), cInvalidIndex);
         
//-- FIXING PREFIX BUG ID 7630
   const granny_tri_topology* pTopo = pMesh->PrimaryTopology;
//--
   
   if ((pTopo->Indices == NULL) && (pTopo->Indices16 == NULL))
      return false;
   
   for (int groupIndex = 0; groupIndex < pTopo->GroupCount; groupIndex++)
   {
      const granny_tri_material_group* pGroup = &pTopo->Groups[groupIndex];
      
      const int grannyMaterialIndex = pGroup->MaterialIndex;
            
      int newMaterialIndex = meshOrigToNewMaterialIndices[debugRangeCheck(grannyMaterialIndex, meshOrigToNewMaterialIndices.size())];
      if (cInvalidIndex == newMaterialIndex)
      {
         const granny_material* pGrannyMaterial = pMesh->MaterialBindings ? pMesh->MaterialBindings[grannyMaterialIndex].Material : NULL;
         
         Unigeom::BMaterial material;
         
         // We only support rendering by Granny mesh if the model is destructible.
         if (!mMaterialBuilder.create(material, (mGeom.getModelType() == Unigeom::Geom::cMTDestructable) ? meshIndex : 0, pGrannyMaterial, mpLog))
            return false;

         newMaterialIndex = meshOrigToNewMaterialIndices[grannyMaterialIndex] = mGeom.insertMaterial(material);
      }
      
      Unigeom::Tri tri(newMaterialIndex);
      
      for (int groupTriIndex = 0; groupTriIndex < pGroup->TriCount; groupTriIndex++)
      {
         const int triIndex = pGroup->TriFirst + groupTriIndex;

         for (int vertIndex = 0; vertIndex < 3; vertIndex++)
         {
            if (pTopo->IndexCount)
            {
               BASSERT(pTopo->Indices);
               tri[vertIndex] = pTopo->Indices[triIndex * 3 + vertIndex] + mMeshFirstVertex;
            }
            else 
            {  
#if GrannyProductMinorVersion <= 6
               BASSERT(pTopo->IndexCount16 && pTopo->Indices16);
#else            
               BASSERT(pTopo->Index16Count && pTopo->Indices16);
#endif               
               tri[vertIndex] = pTopo->Indices16[triIndex * 3 + vertIndex] + mMeshFirstVertex;
            }
         }
         
         mGeom.insertTri(tri);         
      }
   }
   
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::createMesh
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::createMesh(const granny_model* pModel, int modelIndex, int meshIndex)
{
   const granny_mesh* pMesh = pModel->MeshBindings[meshIndex].Mesh;
   
   // pMesh->PrimaryVertexData->VertexCount will not be the mesh's true vertex count if the mesh has fast deformer data!
   if ((!pMesh->PrimaryVertexData) || (!pMesh->PrimaryTopology) || (!pMesh->PrimaryVertexData->VertexCount))
   {
      setError("BGrannyToUnigeom::createMesh: Model \"%s\", mesh \"%s\" has no vertices!", pModel->Name, pMesh->Name);
      return false;
   }
         
   const int numVerts = convertVertices(pModel, modelIndex, pMesh, meshIndex);
   if (numVerts < 0)
   {
      setError("BGrannyToUnigeom::createMesh: Model \"%s\", mesh \"%s\": Failed converting vertices!", pModel->Name, pMesh->Name);
      return false;
   }
   
   if (!convertTopology(modelIndex, meshIndex, pMesh))
   {
      setError("BGrannyToUnigeom::createMesh: Model \"%s\", mesh \"%s\": Failed converting topology!", pModel->Name, pMesh->Name);
      return false;
   }
   
   mMeshFirstVertex += numVerts;
   
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::createMeshes
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::createMeshes(void)
{
   Unigeom::MorphTarget morphTarget;
   mGeom.insertMorphTarget(morphTarget);

   const int modelIndex = 0;
   const granny_model* pModel = checkNull(mpFileInfo->Models[modelIndex]);

   // Was this for instancing in Wrench?
   //if ((mLevelGeom) && (pModel->Name) && (pModel->Name[0] == '!'))
   //   continue;
   
   //tracenocrlf("BGrannyToUnigeom::createMeshes: Processing model %i\n", modelIndex);
   
   const int numMeshes = pModel->MeshBindingCount;
   mMeshFirstBone.resize(numMeshes);
   mMeshLastBone.resize(numMeshes);
   mMeshFirstBone.setAll(INT_MAX);
   mMeshLastBone.setAll(INT_MIN);
      
   for (int meshIndex = 0; meshIndex < pModel->MeshBindingCount; meshIndex++)
   {
      if (!createMesh(pModel, modelIndex, meshIndex))
         return false;
   }
   
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::createAccessories
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::createAccessories(void)
{
   // We only support rendering by Granny mesh if the model is destructible.
   if (mGeom.getModelType() != Unigeom::Geom::cMTDestructable)
   {
      Unigeom::BUnpackedAccessoryType accessory;
      accessory.setFirstBone(0);
      accessory.setNumBones(0);
      
      if (!mGrannyModelSkeletons.isEmpty())
      {
         accessory.setFirstBone(mGrannyModelSkeletons[0].mFirstBone);
         accessory.setNumBones(mGrannyModelSkeletons[0].mNumBones);
      }
      
      mGeom.insertAccessory(accessory);
   }
   else
   {
      // Output 1 accessory per input mesh if the model is destructible.
      for (uint i = 0; i < mMeshFirstBone.size(); i++)
      {
         const int firstBone = mMeshFirstBone[i];
         const int lastBone = mMeshLastBone[i];
         Unigeom::BUnpackedAccessoryType accessory;
         
         if (lastBone < firstBone)
         {
            accessory.setFirstBone(0);
            accessory.setNumBones(0);
         }
         else
         {
            accessory.setFirstBone(firstBone);
            accessory.setNumBones(lastBone - firstBone + 1);
         }
         
         mGeom.insertAccessory(accessory);
      }   
   }      
   
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::createSkeleton
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::createSkeleton(void)
{
   if (!mAllowSkinning)
      return true;
      
   const int modelIndex = 0;
   
   const granny_model* pModel = checkNull(mpFileInfo->Models[modelIndex]);
//-- FIXING PREFIX BUG ID 7631
   const granny_skeleton* pSkeleton = checkNull(pModel->Skeleton);
//--
         
   // Filter out attachment-only models
   if (!pModel->MeshBindingCount)
   {
      setError("BGrannyToUnigeom::createSkeleton: GrannyModel %i (\"%s\"): Attachment only", modelIndex, pModel->Name);
      mGrannyModelSkeletons.push_back(BGrannyModelSkeleton(cInvalidIndex, 0));
      return false;
   }
   
   const int firstBoneIndex = 0;
   
   // store per-skeleton information
   mGrannyModelSkeletons.push_back(BGrannyModelSkeleton(firstBoneIndex, pSkeleton->BoneCount));
   
   if (mpLog)
      mpLog->printf(cTCInfo, "BGrannyToUnigeom::createSkeleton: GrannyModel %i (\"%s\"): FirstUGFBone: %i, NumBones: %i\n",
         modelIndex, pModel->Name, firstBoneIndex, pSkeleton->BoneCount);
                                       
   for (int boneIndex = 0; boneIndex < pSkeleton->BoneCount; boneIndex++)
   {
      const granny_bone* pGrannyBone = &pSkeleton->Bones[boneIndex];
      
      Unigeom::BUnpackedBoneType bone;
      bone.setName(pGrannyBone->Name);
               
      if (GrannyNoParentBone == pGrannyBone->ParentIndex)
         bone.setParentBoneIndex(cInvalidIndex);
      else
         bone.setParentBoneIndex(pGrannyBone->ParentIndex + firstBoneIndex);
      
      bone.setModelToBone(
         Unigeom::BUnpackedTransformType(
            BVecN<4>(pGrannyBone->InverseWorld4x4[0][0], pGrannyBone->InverseWorld4x4[0][1], pGrannyBone->InverseWorld4x4[0][2], pGrannyBone->InverseWorld4x4[0][3]),
            BVecN<4>(pGrannyBone->InverseWorld4x4[1][0], pGrannyBone->InverseWorld4x4[1][1], pGrannyBone->InverseWorld4x4[1][2], pGrannyBone->InverseWorld4x4[1][3]),
            BVecN<4>(pGrannyBone->InverseWorld4x4[2][0], pGrannyBone->InverseWorld4x4[2][1], pGrannyBone->InverseWorld4x4[2][2], pGrannyBone->InverseWorld4x4[2][3]),
            BVecN<4>(pGrannyBone->InverseWorld4x4[3][0], pGrannyBone->InverseWorld4x4[3][1], pGrannyBone->InverseWorld4x4[3][2], pGrannyBone->InverseWorld4x4[3][3])
         )
      );
            
      if (mpLog)                        
         mpLog->printf(cTCInfo, "UGFBone %i: Name: %s, Model: %i, GrannyBoneIndex: %i\n", mGeom.numBones(), pGrannyBone->Name, modelIndex, boneIndex);
      
      mGeom.insertBone(bone);
            
      // store per-bone information
      mBoneRefs.push_back(BBoneRef(modelIndex, boneIndex));
   }
      
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::findNamedParam
//-------------------------------------------------------------------------------------------------------
const void* BGrannyToUnigeom::findNamedParam(const granny_variant& variant, const char* pName, uint expectedTypeSize)
{
//-- FIXING PREFIX BUG ID 7625
   const granny_data_type_definition* pDataType = variant.Type;
//--
   uint curOfs = 0;
   for ( ; GrannyEndMember != pDataType->Type; curOfs += GrannyGetMemberTypeSize(pDataType), pDataType++)
   {
      if (_stricmp(pDataType->Name, pName) != 0)
         continue;
      if (pDataType->Type != GrannyReferenceMember)
         continue;
            
      const uint typeSize = GrannyGetTotalObjectSize(pDataType->ReferenceType);
      if (typeSize != expectedTypeSize)
         continue;

      const void* pParamVal = *reinterpret_cast<const void* const*>(static_cast<const uchar*>(variant.Object) + curOfs);

      if (pParamVal)
         return pParamVal;
   }

   return false;
}

//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::findIntParam
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::findIntParam(const granny_variant& variant, const char* pName, int& val)
{
//-- FIXING PREFIX BUG ID 7627
   const granny_data_type_definition* pDataType = variant.Type;
//--
   uint curOfs = 0;
   for ( ; GrannyEndMember != pDataType->Type; curOfs += GrannyGetMemberTypeSize(pDataType), pDataType++)
   {
      if (pDataType->Type != GrannyInt32Member)
         continue;

      if (_stricmp(pDataType->Name, pName) == 0)
      {
         const int* pParamVal = reinterpret_cast<const int*>(static_cast<const uchar*>(variant.Object) + curOfs);
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
// BGrannyToUnigeom::createUnimodel
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::findModelType(void)
{
   mGeom.setModelType(Unigeom::Geom::cMTNormal);
   
   int foundModelType = -1;
   
   const granny_model* pModel = checkNull(mpFileInfo->Models[0]);
   if (pModel)
   {
      for (int meshIndex = 0; meshIndex < pModel->MeshBindingCount; meshIndex++)
      {
         const granny_mesh* pMesh = pModel->MeshBindings[meshIndex].Mesh;
         
         int modelType = -1;
         
         // FIXME: We should validate the exact reference type here! We're assuming 2 ints, with the 2nd int being the model type.
         const int* pMeshSizer = (const int*)findNamedParam(pMesh->ExtendedData, "MeshSizer", sizeof(uint) * 2);
         if (pMeshSizer)
            modelType = pMeshSizer[1];
         
         if (modelType < 1)
         {
            if (!findIntParam(pMesh->ExtendedData, "ESModelType", modelType))
               continue;
         }
         
         if (modelType < 1)
            continue;
            
         modelType -= 1;
                           
         if ((modelType < Unigeom::Geom::cMTNormal) || (modelType > Unigeom::Geom::cMTLarge))
         {
            setError("Invalid ESModelType parameter! (%i)", modelType);
            return false;
         }
         
         if (foundModelType < 0)
            foundModelType = modelType;
         else if (foundModelType != modelType)
         {
            setError("Inconsistent ESModelType parameter! (%i and %i)", foundModelType, modelType);
            return false;
         }
      }
   }      
   
   if (foundModelType >= 0)
   {
      gConsoleOutput.printf("Found model type: %u (0=normal,1=destructable, 2=large)\n", foundModelType);
      mGeom.setModelType((Unigeom::Geom::eModelType)foundModelType);
   }
      
   return true;
}
                  
//-------------------------------------------------------------------------------------------------------
// BGrannyToUnigeom::createUnimodel
//-------------------------------------------------------------------------------------------------------
bool BGrannyToUnigeom::createUnimodel(void)
{
   mSuccess = false;
   
   mGeom.clear();
   
   if (mpFileInfo->ModelCount < 1)
   {
      setError("Granny file has no models!");
      return false;
   }
   else if (mpFileInfo->ModelCount > 1)
   {
      if (mpLog)
         mpLog->printf(cTCWarning, "Granny file has %i models, only processing first model!\n", mpFileInfo->ModelCount);
   }
   
   if (!findModelType())
   {
      mGeom.clear();
      return false;  
   }
               
   if (!createSkeleton())
   {
      mGeom.clear();
      return false;
   }
     
   if (!createMeshes())
   {
      mGeom.clear();
      return false;
   }
   
   if (!createAccessories())
   {
      mGeom.clear();
      return false;
   }
        
   mGeom.name() = mpFileInfo->FromFileName;

   mSuccess = true;               
   
   return true;
}

