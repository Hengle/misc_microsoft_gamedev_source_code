//------------------------------------------------------------------------------------------------------------------------
//
//  File: ugxInstancer.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "uniGeom.h"
#include "ugxGeom.h"

//-------------------------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------------------------
class SegmentSectionTri;

enum { cUGXMaxGlobalBones = 128 };

//-------------------------------------------------------------------------------------------------------
// class BUGXGeomInstancer
//-------------------------------------------------------------------------------------------------------
class BUGXGeomInstancer
{
public:
   typedef Unigeom::BMaterial                               InputMaterialType;
      
   typedef Unigeom::BUnpackedBoneType                       BoneType;
   
   typedef BUGXGeom::BECFDataBuilder<cBigEndianNative>      DataBuilderType;
   typedef DataBuilderType::CachedDataType                  CachedDataType;
   typedef CachedDataType::SectionType                      SectionType;
   typedef CachedDataType::AccessoryType                    AccessoryType;
   typedef SectionType::UnivertPackerType                   UnivertPackerType;
         
   BUGXGeomInstancer(
      bool bigEndian, 
      const Unigeom::Geom& inputGeom, 
      BTextDispatcher* pLog = NULL, 
      bool diffuseVertexColors = false, 
      bool optimizedTexCoords = false, 
      bool supportInstancing = true);

   bool getSuccess(void) const { return mSuccess; }
   const BString& getErrorDesc(void) const { return mErrorDesc; }
   
   const CachedDataType& getUGXCachedData(void) const { return mCachedData; }
         CachedDataType& getUGXCachedData(void)       { return mCachedData; }
         
   bool packData(BECFFileBuilder& ecfBuilder);
   
   void logData(BTextDispatcher& log);
            
private:
      
   // types
   
   struct BuildSection
   {
      IntVec mSortedTriIndices;
      int mFirstOutTri;
      int mFirstOutVert;
      int mNumOutTris;
      int mNumOutVerts;
      BFixedString256 mBaseVertPackOrder;
      
      int mNumLocalBones;
      IntVec mGlobalBoneToLocalBone;            // size: mInputGeom.numBones() + 1 (first bone is the root, or -1 index)
      IntVec mLocalBoneToGlobalBone;            // size: mNumLocalBones
      
      int mOrigMaterialIndex;
      int mNewMaterialIndex;
      
      BuildSection() :
         mFirstOutTri(0),
         mFirstOutVert(0),
         mNumOutTris(0),
         mNumOutVerts(0),
         mNumLocalBones(0),
         mOrigMaterialIndex(0),
         mNewMaterialIndex(0)
      {
      }
   };
   typedef BDynamicArray<BuildSection> BuildSectionVec;
      
   // member vars

   const Unigeom::Geom&    mInputGeom;
   BTextDispatcher*        mpLog;
   
   BDynamicArray<Unigeom::BMaterial> mNewMaterials;
      
   bool                    mShadowGeom;
   bool                    mBigEndian;
   bool                    mDiffuseVertexColors;
   bool                    mOptimizeTexCoords;
   bool                    mGlobalBonesOnly;
   bool                    mSupportInstancing;
   bool                    mHasNormalMaps;
               
   AABBVec                 mBoneBounds;
   
   BuildSectionVec         mBuildSections;                 
            
   BDynamicArray<IntVec>   mNewMatUVChannelMap;          // UVChannelMap[material][outputChannel] = inputChannel
   IntVec                  mOrigToNewMatIndices;         // Maps original to new materials.
   
   UnivertAttributes       mUsedAttributes;
   IntVec                  mOutputVerts;                // vertex indices, maps new ugx indices to original unigeom indices
   IndexedTriVec           mOutputTris;                 // Output section vertex indices            
   Unigeom::TriVec         mSortedTris;                 // Tris sorted by original material indices.
   
   CachedDataType&         mCachedData;                 // This is an alias to mDataBuilder.getCachedData()
   DataBuilderType         mDataBuilder;
      
   bool                    mSuccess;
   BString                 mErrorDesc;
                                       
   // methods
   
   void setError(const char* pMsg, ...);
   
   int numSkeletonBones(void) const { return static_cast<int>(mInputGeom.numBones()); }
   const BoneType& skeletonBone(int i) const { return mInputGeom.bone(i); }
   
   void dumpInputMaterials(void);
   void dumpOutputMaterials(void);
   
   bool buildInit(void);
   
   void segmentSection(int materialIndex, int firstTri, int numTris);
   void findSections(void);
   
   void createOutputMaterials(void);
   
   void sortSections(void);
   
   void createSectionRawMesh(
      IndexedTriVec& rawTris,
      Unifier<int>& unigeomVertUnifier,
      const int sectionIndex);
      
   void createOptimizedMeshes(void);
      
   Univert createBaseOutputVertex(
      const BuildSection& buildSection,
      const SectionType& outSection,
      const int unigeomVertIndex, 
      const IntVec& uvChannelMap,
      const int numBumpMaps,
      const Unigeom::BMaterial& origMat);
      
   BFixedString256 createBaseVertexPackOrder(const SectionType& section);
         
   void createBaseOutputGeom(void);

   void findAABB(void);
   void findBoneBounds(void);

   void createOutputIndices(void);
   void createOutputGeom(void);

   typedef BDynamicArray<SegmentSectionTri> BSegmentSectionTriVec;

   void createBuildSection(
      int first, 
      int numTris, 
      int materialIndex,
      bool& allSectionsRigid,
      const BSegmentSectionTriVec& segmentSectionVec);
                     
   bool build(void);
   bool initSectionBones(void);
   void createOutputAccessories(void);
   void createOutputBones(void);
   void packOutputMaterials(void);
   void sortTrisByMaterial(void);
   void determineRigidOnlyModel(void);
   void determineAllSectionsSkinned(void);
   
   BUGXGeomInstancer(const BUGXGeomInstancer&);
   BUGXGeomInstancer& operator= (const BUGXGeomInstancer&);
};

