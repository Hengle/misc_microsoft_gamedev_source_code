// File: granny_to_unigeom.h
// Copyright (c) Microsoft Corporation. All rights reserved.
#pragma once

#include "unigeom.h"
#include "grannyMaterial.h"

struct granny_file_info;
struct granny_data_type_definition;
struct granny_model;
struct granny_mesh;
struct granny_variant;

//-------------------------------------------------------------------------------------------------------
// Converts Granny GR2 data to a single Unigeom.
// Originally, Wrench GR2 models are stored in the Unigeom as separate accessories.
// Now we only support a single model (Age3-style).
//-------------------------------------------------------------------------------------------------------
class BGrannyToUnigeom
{
public:
   BGrannyToUnigeom();
   
   BGrannyToUnigeom(granny_file_info* pFileInfo, bool allowSkinning  = true, bool allowTangentBinormal = true, BTextDispatcher* pLog = NULL);
   
   bool convert(granny_file_info* pFileInfo, bool allowSkinning  = true, bool allowTangentBinormal = true, BTextDispatcher* pLog = NULL);
   
   bool getSuccess(void) const { return mSuccess; }
   const BString& getErrorDesc(void) const { return mErrorDesc; }
        
   granny_file_info* getGrannyFile(void) const { return mpFileInfo; }
   
   const Unigeom::Geom& getGeom(void) const { return mGeom; }
                                    
private:
   granny_file_info*       mpFileInfo;
   BTextDispatcher*        mpLog;
   bool                    mAllowSkinning;
   bool                    mAllowTangentBinormal;
   
   int                     mMeshFirstVertex;
   BMaterialBuilder        mMaterialBuilder;

   Unigeom::Geom           mGeom;   
         
   bool                    mSuccess;
   BString                 mErrorDesc;
   
   struct BBoneRef
   {
      int mModelIndex;
      int mModelSkeletonBoneIndex;
      
      BBoneRef() 
      { 
      }
      
      BBoneRef(int modelIndex, int modelSkeletonBoneIndex) : mModelIndex(modelIndex), mModelSkeletonBoneIndex(modelSkeletonBoneIndex)
      {
      }
   };
   
   typedef BDynamicArray<BBoneRef> BBoneRefArray;
   BBoneRefArray           mBoneRefs;                        // mBoneRefs is for internal sanity checking
   
   struct BGrannyModelSkeleton
   {
      int mFirstBone;
      int mNumBones;
      
      BGrannyModelSkeleton()
      {
      }
      
      BGrannyModelSkeleton(int firstBone, int numBones) : mFirstBone(firstBone), mNumBones(numBones)
      {
      }
   };
   
   typedef BDynamicArray<BGrannyModelSkeleton> GrannyModelSkeletonVec;
   GrannyModelSkeletonVec  mGrannyModelSkeletons;
   
   BDynamicArray<int>      mMeshFirstBone;
   BDynamicArray<int>      mMeshLastBone;   

   void setError(const char* pMsg, ...);
   int convertVertices(const granny_model* pModel, int modelIndex, const granny_mesh* pMesh, int meshIndex);
   bool convertTopology(int modelIndex, int meshIndex, const granny_mesh* pMesh);
   bool createMesh(const granny_model* pModel, int modelIndex, int meshIndex);
   bool createMeshes(void);
   bool createSkeleton(void);
   bool createAccessories(void);
   bool createUnimodel(void);      
   bool isPostProcessorAtLeastVersion(int major, int minor);
   bool hasFastDeformerData(const granny_mesh* pMesh);
   bool findModelType(void);
   static const void* findNamedParam(const granny_variant& variant, const char* pName, uint expectedTypeSize);
   static bool findIntParam(const granny_variant& variant, const char* pName, int& val);
}; // class BGrannyToUnigeom


   
