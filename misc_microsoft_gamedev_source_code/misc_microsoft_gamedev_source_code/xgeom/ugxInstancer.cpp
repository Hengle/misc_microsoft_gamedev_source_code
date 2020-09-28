//------------------------------------------------------------------------------------------------------------------------
//
//  File: ugxInstancer.cpp
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#include "xgeom.h"

// local
#include "ugxInstancer.h"
#include "vertexCache.h"
#include "trilistOptimizer.h"

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::BUGXGeomInstancer
//-------------------------------------------------------------------------------------------------------
BUGXGeomInstancer::BUGXGeomInstancer(
   bool bigEndian, 
   const Unigeom::Geom& inputGeom, 
   BTextDispatcher* pLog, 
   bool diffuseVertexColors, bool optimizeTexCoords, bool supportInstancing) :
   mShadowGeom(false),
   mInputGeom(inputGeom), 
   mpLog(pLog), 
   mGlobalBonesOnly(true), 
   mBigEndian(bigEndian),
   mDiffuseVertexColors(diffuseVertexColors),
   mOptimizeTexCoords(optimizeTexCoords),
   mSupportInstancing(supportInstancing),
   mSuccess(false),
   mCachedData(mDataBuilder.getCachedData())
{
   mSuccess = build();
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::setError
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::setError(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   
   mErrorDesc.formatArgs(pMsg, args);
      
   va_end(args);  
   
   if (mpLog)
      mpLog->printf(cTCError, "BUGXGeomInstancer::setError: %s\n", mErrorDesc.getPtr());
}
      
//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::dumpInputMaterials
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::dumpInputMaterials(void)
{
   if (!mpLog)
      return;
      
   mpLog->printf("Input Materials: %i\n", mInputGeom.numMaterials());
   mpLog->indent(1);
   for (int i = 0; i < mInputGeom.numMaterials(); i++)
   {
      mpLog->printf("Input Material: %i\n", i);
      mpLog->indent(1);
      mInputGeom.material(i).log(*mpLog);
      mpLog->indent(-1);
   }
   mpLog->indent(-1);
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::dumpOutputMaterials
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::dumpOutputMaterials(void)
{
   if (!mpLog)
      return;
      
   mpLog->printf("Output Materials: %i\n", mNewMaterials.getSize());
   mpLog->indent(1);
   for (uint i = 0; i < mNewMaterials.getSize(); i++)
   {
      mpLog->printf("Output Material: %i\n", i);
      mpLog->indent(1);
      mNewMaterials[i].log(*mpLog);
      mpLog->indent(-1);
   }
   mpLog->indent(-1);
   
   mpLog->printf("Orig to new material mapping table:\n");
   for (uint i = 0; i < mOrigToNewMatIndices.size(); i++)
      mpLog->printf("%i ", mOrigToNewMatIndices[i]);
   mpLog->printf("\n");
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::determineRigidOnlyModel
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::determineRigidOnlyModel(void)
{
   int rigidBoneIndex = -1;
   int maxBones = 0;

   for (int triIndex = 0; triIndex < mInputGeom.numTris(); triIndex++)
   {
      for (int trivertIndex = 0; trivertIndex < 3; trivertIndex++)
      {
         const int numBones = mInputGeom.vert(mInputGeom.tri(triIndex)[trivertIndex]).numActualInfluences();
         maxBones = Math::Max(maxBones, numBones);

         if (1 == numBones)
         {
            const int boneIndex = mInputGeom.triVert(triIndex, trivertIndex).boneIndex(0);
            if (rigidBoneIndex == -1)
               rigidBoneIndex = boneIndex;
            else if (rigidBoneIndex != boneIndex)
               rigidBoneIndex = -2;
         }
         else
            rigidBoneIndex = -2;
      }
   }

   if (mpLog)
      mpLog->printf("Max influences: %i\n", maxBones);

   if (rigidBoneIndex >= 0)
   {
      BASSERT(1 == maxBones);
      mCachedData.header().setRigidOnly(true);

      mCachedData.header().setRigidBoneIndex((rigidBoneIndex == Univert::DefaultBoneIndex) ? -1 : rigidBoneIndex);

      if (mpLog)
         mpLog->printf("Model is rigid only, index %i\n", rigidBoneIndex);
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::buildInit
//-------------------------------------------------------------------------------------------------------
bool BUGXGeomInstancer::buildInit(void)
{
   if (mpLog)
   {
      mpLog->printf("BUGXGeomInstancer::buildInit: Unigeom stats: Type: %i, Diffuse: %i, Tris: %i, Verts: %i,\n  Ave. Verts Per Tri: %f, Materials: %i, Bones: %i, Morph Targets: %i\n",
         mInputGeom.getModelType(),
         mDiffuseVertexColors,
         mInputGeom.numTris(),
         mInputGeom.numVerts(),
         mInputGeom.numTris() ? (float(mInputGeom.numVerts()) / mInputGeom.numTris()) : 0.0f,
         mInputGeom.numMaterials(),
         numSkeletonBones(),
         mInputGeom.numMorphTargets());
   }            
   
   if (mInputGeom.numMorphTargets() != 1)
   {
      setError("Morph targets unsupported");
      return false;
   }
   
   mUsedAttributes = mInputGeom.vertAttributes();
   
   // force diffuse attribute if user wants diffuse colors
   if (mDiffuseVertexColors)
      mUsedAttributes.diffuse = true;
     
   if (mpLog)
      mUsedAttributes.log(*mpLog);
      
   mHasNormalMaps = false;
   for (int i = 0; i < mInputGeom.numMaterials(); i++)
   {
      if (mInputGeom.material(i).getNumMaps(Unigeom::BMaterial::cNormal))
      {
         mHasNormalMaps = true;
         break;
      }
   }
   
   if (!mHasNormalMaps)
   {
      mUsedAttributes.numBasis = 0; 
      
      mpLog->printf("BUGXGeomInstancer::buildInit: Model has basis vectors, but no materials have bump maps. Deleting basis vectors!");
   }
               
   dumpInputMaterials();
                 
   return true;
}

//-------------------------------------------------------------------------------------------------------
// SegmentSectionTri
//-------------------------------------------------------------------------------------------------------
class SegmentSectionTri
{
   public:
      SegmentSectionTri()
      {
      }
      
      SegmentSectionTri(int triIndex, bool rigidOnly, int rigidBoneIndex, int numBones) :
         mTriIndex(triIndex),
         mRigidOnly(rigidOnly),
         mRigidBoneIndex(rigidBoneIndex),
         mNumBones(numBones)
      {
      }
         
      int triIndex(void) const            { return mTriIndex; }
      bool rigidOnly(void) const          { return mRigidOnly; }
      int rigidBoneIndex(void) const      { return mRigidBoneIndex; }
      int numBones(void) const            { return mNumBones; }
      
      void setRigidOnly(bool rigidOnly)                  { mRigidOnly = rigidOnly; }
      void setRigidBoneIndex(int rigidBoneIndex)         { mRigidBoneIndex = rigidBoneIndex; }
      void setNumBones(int numBones)                     { mNumBones = numBones; }
      
      friend bool operator< (const SegmentSectionTri& lhs, const SegmentSectionTri& rhs)
      {
         if (static_cast<int>(lhs.mRigidOnly) < static_cast<int>(rhs.mRigidOnly))
            return true;
         else if (lhs.mRigidOnly == rhs.mRigidOnly)
         {
            if (lhs.mRigidOnly)
            {
               if (lhs.mRigidBoneIndex < rhs.mRigidBoneIndex)
                  return true;
            }
            else if (lhs.mNumBones < rhs.mNumBones)
               return true;
         }
         
         return false;                          
      }
      
      friend bool operator== (const SegmentSectionTri& lhs, const SegmentSectionTri& rhs)
      {
         return 
            (lhs.mTriIndex       == rhs.mTriIndex) && 
            (lhs.mRigidOnly      == rhs.mRigidOnly) &&
            (lhs.mRigidBoneIndex == rhs.mRigidBoneIndex) &&
            (lhs.mNumBones       == rhs.mNumBones);
      }
      
      static bool SameCategory(const SegmentSectionTri& lhs, const SegmentSectionTri& rhs)
      {
         if (lhs.mRigidOnly == rhs.mRigidOnly)
         {
            if (lhs.mRigidOnly)
            {
               return lhs.mRigidBoneIndex == rhs.mRigidBoneIndex;
            }
            else
            {
               if (lhs.mNumBones <= 2)
                  return lhs.mNumBones == rhs.mNumBones;
               else
                  return rhs.mNumBones > 2;
            }
         }
         
         return false;
      }
      
   private:
      int mTriIndex;
      bool mRigidOnly;
      int mRigidBoneIndex;
      int mNumBones;
};

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createBuildSection
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createBuildSection(
   int first, 
   int numTris,
   int origMaterialIndex,
   bool& allSectionsRigid,
   const BSegmentSectionTriVec& segmentSectionTriVec)
{
   const int last = first + numTris;
   
   BuildSection buildSection;
   for (int i = first; i < last; i++)
      buildSection.mSortedTriIndices.push_back(segmentSectionTriVec.at(i).triIndex());
   
   const int maxSectionBones = segmentSectionTriVec[first].numBones();
   
#if SKIN_ONLY           
   const bool rigidOnly = false;
   const int rigidBoneIndex = -1;
#else
   const bool rigidOnly = segmentSectionTriVec[first].rigidOnly();
   const int rigidBoneIndex = rigidOnly ? segmentSectionTriVec[first].rigidBoneIndex() : -1;
#endif   

   buildSection.mOrigMaterialIndex = origMaterialIndex;         
   buildSection.mNewMaterialIndex = mOrigToNewMatIndices[debugRangeCheck(origMaterialIndex, mOrigToNewMatIndices.size())];
   
   mBuildSections.push_back(buildSection);
   
   SectionType section;
   
   section.setMaterialIndex(buildSection.mNewMaterialIndex);
   section.setAccessoryIndex(mInputGeom.material(buildSection.mOrigMaterialIndex).getAccessoryIndex());
   section.setMaxBones(maxSectionBones);
               
   if (rigidOnly)
   { 
      BASSERT(maxSectionBones == 1);
      section.setRigidOnly(true);
                                 
      section.setRigidBoneIndex((rigidBoneIndex == Univert::DefaultBoneIndex) ? -1 : rigidBoneIndex);
   }
   else
      allSectionsRigid = false;
               
   mCachedData.sections().push_back(section);                        

   if (mpLog)
   {
      mpLog->printf("Section %i: tris: %i, origMaterial: %i, newMaterial: %i, accessory: %i, maxBones: %i, rigidOnly: %i, rigidBoneIndex: %i\n",
         mBuildSections.size(),
         buildSection.mSortedTriIndices.size(),
         buildSection.mOrigMaterialIndex,
         buildSection.mNewMaterialIndex,
         section.accessoryIndex(),
         maxSectionBones,
         rigidOnly,
         rigidBoneIndex
         );      
   }            
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::segmentSection
// FIXME: This code can output sections which are too small to be efficiently rendered.
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::segmentSection(int origMaterialIndex, int firstTri, int numTris)
{
   BASSERT(numTris > 0);
   
   if (mpLog)
      mpLog->printf("BUGXGeomInstancer::createSections: origMaterialIndex: %i, firstTri: %i, numTris: %i\n", origMaterialIndex, firstTri, numTris);

   BSegmentSectionTriVec segmentSectionTriVec;
   
   IntVec rigidHistogram(numSkeletonBones() + 1);
   int numSingleBoneSkinned = 0;
   bool hasSkinnedSections = false;
   
   for (int triIndex = firstTri; triIndex < firstTri + numTris; triIndex++)
   {
      int maxBones = 0;
      int triRigidBoneIndex = -1;
      for (int trivertIndex = 0; trivertIndex < 3; trivertIndex++)
      {
         const Univert& vert = mInputGeom.vert(mSortedTris.at(triIndex)[trivertIndex]);
         
         const int numBones = vert.numActualInfluences();
         maxBones = Math::Max(maxBones, numBones);
         
         if (1 == numBones)
         {
            const int boneIndex = vert.boneIndex(0);
            if (-1 == triRigidBoneIndex)
               triRigidBoneIndex = boneIndex;
            else if (triRigidBoneIndex != boneIndex)
               triRigidBoneIndex = -2;
         }
         else
            triRigidBoneIndex = -2;
      }
      
      segmentSectionTriVec.push_back(SegmentSectionTri(triIndex, triRigidBoneIndex >= 0, triRigidBoneIndex, maxBones));
      
      if (triRigidBoneIndex >= 0)
      {
         const int rigidBoneIndex = (Univert::DefaultBoneIndex == triRigidBoneIndex) ? 0 : (triRigidBoneIndex + 1);
         
         BDEBUG_ASSERT(rigidBoneIndex < (int)rigidHistogram.getSize());
         
         rigidHistogram.at(rigidBoneIndex) = rigidHistogram.at(rigidBoneIndex) + 1;
      }
      else if (1 == maxBones)
         numSingleBoneSkinned++;
      else
         hasSkinnedSections = true;
   }
   
   const int RigidToSkinnedThreshold = 100;

   if ((!mCachedData.header().rigidOnly()) && (hasSkinnedSections))
   {
      // rigid tris in sections with too few tris should be skinned
      for (uint i = 0; i < segmentSectionTriVec.size(); i++)
      {
         if (segmentSectionTriVec[i].rigidOnly())
         {  
            const int rigidBoneIndex = (Univert::DefaultBoneIndex == segmentSectionTriVec[i].rigidBoneIndex()) ? 0 : (segmentSectionTriVec[i].rigidBoneIndex() + 1);
            if (rigidHistogram.at(rigidBoneIndex) < RigidToSkinnedThreshold)
            {
               segmentSectionTriVec[i].setRigidOnly(false);
               segmentSectionTriVec[i].setRigidBoneIndex(-2);
               segmentSectionTriVec[i].setNumBones(1);
            }
         }
      }
   }
   
   std::sort(segmentSectionTriVec.begin(), segmentSectionTriVec.end());

   int totalTrisProcessed = 0;
   
   bool allSectionsRigid = true;
   
   int first = 0;
   for (uint cur = 1; cur <= segmentSectionTriVec.size(); cur++)
   {
      if ((cur == segmentSectionTriVec.size()) || 
          (!SegmentSectionTri::SameCategory(segmentSectionTriVec.at(first), segmentSectionTriVec.at(cur))))
      {
         int numTrisLeft = cur - first;
                                             
         while (numTrisLeft)
         {
            const int numTrisToProcess = Math::Min<int>(numTrisLeft, SectionType::MaxTris);
            
            createBuildSection(first, numTrisToProcess, origMaterialIndex, allSectionsRigid, segmentSectionTriVec);
            
            totalTrisProcessed += numTrisToProcess;
            
            first += numTrisToProcess;
            numTrisLeft -= numTrisToProcess;
         }
      }
   }
   
   if (!allSectionsRigid)
      mCachedData.header().setAllSectionsRigid(false);
                           
   BVERIFY(totalTrisProcessed == numTris);
}

//-------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------

namespace 
{
   class TriSortCompFunctor
   {
   public:
      TriSortCompFunctor(const Unigeom::Geom& geom) : mInputGeom(geom)
      {
      }
      
      bool operator() (int l, int r)
      {
         const int lMat = mInputGeom.tri(l).materialIndex();
         const int rMat = mInputGeom.tri(r).materialIndex();
         
         return lMat < rMat;
      }
      
   private:
      const Unigeom::Geom& mInputGeom;
   };
} // anonymous namespace

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::sortTrisByMaterial
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::sortTrisByMaterial(void)
{
   // tri indices, sorted by the orig (input) materials
   IntVec sortedTriIndices(mInputGeom.numTris());
   
   for (int i = 0; i < mInputGeom.numTris(); i++)
      sortedTriIndices[i] = i;
      
   std::sort(sortedTriIndices.begin(), sortedTriIndices.end(), TriSortCompFunctor(mInputGeom));
   
   mSortedTris.resize(mInputGeom.numTris());
   for (int i = 0; i < mInputGeom.numTris(); i++)
      mSortedTris[i] = mInputGeom.tri(sortedTriIndices[i]);
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::findSections
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::findSections(void)
{
   mCachedData.header().setAllSectionsRigid(true);
   
   int prevOrigMaterialIndex = -1;
   int firstTri = 0;
   
   if (mInputGeom.numTris())
   {
      for (int i = 0; i <= mInputGeom.numTris(); i++)
      {
         if (i < mInputGeom.numTris())
         {
            // assumes tris are already sorted by material indices
            BASSERT(prevOrigMaterialIndex <= mSortedTris.at(i).materialIndex());
         }
         
         int curOrigMaterialIndex = INT_MIN;
         if (i < mInputGeom.numTris())
         {
            //if (mInputGeom.shadowGeom())
            //   curOrigMaterialIndex = 0;
            //else
               curOrigMaterialIndex = mSortedTris.at(i).materialIndex();
         }
                                 
         const int curNumTris = i - firstTri;
         
         //if ((prevOrigMaterialIndex != curOrigMaterialIndex) || (curNumTris >= SectionType::MaxTris))
         if (prevOrigMaterialIndex != curOrigMaterialIndex)
         {
            if (-1 != prevOrigMaterialIndex)
            {
               segmentSection(prevOrigMaterialIndex, firstTri, checkNull(curNumTris));
               
               firstTri = i;
            }

            prevOrigMaterialIndex = curOrigMaterialIndex;
         }
      }
   }
   
   if (mpLog)
      mpLog->printf("AllSectionsRigid: %i\n", mCachedData.header().allSectionsRigid());
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::initSectionBones
//-------------------------------------------------------------------------------------------------------
bool BUGXGeomInstancer::initSectionBones(void)
{
   mGlobalBonesOnly = (numSkeletonBones() + 1) <= cUGXMaxGlobalBones;
   if (!mGlobalBonesOnly)
   {
      setError("Skeleton has too many bones");
      return false;
   }
         
   mCachedData.header().setGlobalBones(mGlobalBonesOnly);
         
   for (uint sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
   {
      BuildSection& buildSection = mBuildSections[sectionIndex];
      
      IntVec boneHist(numSkeletonBones() + 1);
      
      for (uint triIndex = 0; triIndex < buildSection.mSortedTriIndices.size(); triIndex++)
      {
         const int sortedTriIndex = buildSection.mSortedTriIndices[triIndex];
                     
         for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
         {
            const Univert& vert = mInputGeom.vert(mSortedTris[sortedTriIndex][triVertIndex]);
            
            for (int influenceIndex = 0; influenceIndex < Univert::MaxInfluences; influenceIndex++)
            {
               int boneIndex = vert.boneIndex(influenceIndex);
               float boneWeight = vert.boneWeight(influenceIndex);
                                 
               if (boneWeight > 0.0f)
               {
                  if (Univert::DefaultBoneIndex == boneIndex)
                     boneHist.at(0) = boneHist.at(0) + 1;
                  else
                     boneHist.at(boneIndex + 1) = boneHist.at(boneIndex + 1) + 1;
               }        
            }
         }
      }
      
      int numUsedBones = 0;
      for (int i = 0; i < numSkeletonBones() + 1; i++)
         if (boneHist[i])
            numUsedBones++;
      
      if (mpLog)
      {
         mpLog->printf("Section: %i Num Used Bones: %i\n", sectionIndex, numUsedBones);
         for (int i = 0; i < numSkeletonBones() + 1; i++)
         {
            if (boneHist[i])
               mpLog->printf("[%03i: %03i] ", i, boneHist[i]);
         }
         mpLog->printf("\n");
      }            
      
      buildSection.mNumLocalBones = numUsedBones;
      buildSection.mGlobalBoneToLocalBone.resize(numSkeletonBones() + 1);
      buildSection.mLocalBoneToGlobalBone.resize(numUsedBones);
      
      int nextLocalBoneIndex = 0;
      for (int i = 0; i < numSkeletonBones() + 1; i++)
      {
         if (boneHist[i])
         {
            buildSection.mGlobalBoneToLocalBone.at(i) = nextLocalBoneIndex;
            buildSection.mLocalBoneToGlobalBone.at(nextLocalBoneIndex) = i;
            nextLocalBoneIndex++;
         }
         else
         {
            // store invalid local bone index for debugging purposes
            buildSection.mGlobalBoneToLocalBone.at(i) = -1; 
         }
      }
   }
   
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createOutputMaterials
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createOutputMaterials(void)
{
   // UV channel map, maps new to old 
   mNewMatUVChannelMap.clear();
   
   typedef BArrayUnifier<Unigeom::BMaterial> MaterialUnifier;
   MaterialUnifier newMatUnifier;
         
   mOrigToNewMatIndices.clear();
   
   for (int origMaterialIndex = 0; origMaterialIndex < mInputGeom.numMaterials(); origMaterialIndex++)
   {
      const Unigeom::BMaterial& origMat = mInputGeom.material(origMaterialIndex);
      
      Unigeom::BMaterial newMat(origMat);
      
      // Set the new material's accessory index to 0, because we're using a separate per-section accessory index in the UGX data.
      newMat.setAccessoryIndex(0);
      
      for (int mapType = 0; mapType < Unigeom::BMaterial::cNumMapTypes; mapType++)
      {
         for (int mapIndex = 0; mapIndex < newMat.getNumMaps((Unigeom::BMaterial::eMapType)mapType); mapIndex++)
         {
            Unigeom::BMap& map = newMat.getMap((Unigeom::BMaterial::eMapType)mapType, mapIndex);
            const int uvChannel = map.getChannel();

            bool badUVMapping = false;
            if (!mUsedAttributes.numUVSets)
            {
               if (0 != uvChannel)
                  badUVMapping = true;
            }
            else
            {
               if (uvChannel >= mUsedAttributes.numUVSets)
                  badUVMapping = true;
            }

            if (badUVMapping)
            {
               mpLog->printf(cTCWarning, 
                  "BUGXGeomInstancer::buildInit: mat %i mapType %i mapIndex %i uses an invalid UV channel", 
                  origMaterialIndex,
                  mapType,
                  mapIndex);

               map.setChannel(0);   
            }               
         }
      }
      
      // --- find unique UV channels                                                            
      IntVec materialUVChannelMap;      
            
      if (mShadowGeom)
      {
         for (int i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
            newMat.setNumMaps((Unigeom::BMaterial::eMapType)i, 0);
      }
      else
      {
         // limit the max # of maps per type
         for (int i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
         {
            int maxMaps = 1;
            switch (i)
            {
               case Unigeom::BMaterial::cOpacity: 
               case Unigeom::BMaterial::cEnv: 
               case Unigeom::BMaterial::cNormal: 
               case Unigeom::BMaterial::cEmissive:
               {
                  maxMaps = 1;//(newMat.numMaps(Unigeom::Material::eObjBump) > 0) ? 1 : 2;
                                                      
                  break;
               }
               default:
               {
                  maxMaps = Unigeom::BMaterial::cMaxMapsPerType;
                  break;
               }
            }
            if (newMat.getNumMaps((Unigeom::BMaterial::eMapType)i) > maxMaps)
            {
               newMat.setNumMaps((Unigeom::BMaterial::eMapType)i, maxMaps);
               
               if (mpLog)
               {
                  mpLog->printf(cTCWarning,
                     "BUGXGeomInstancer::createOutputMaterials: Material \"%s\": Truncating map type %i (%s) down to %i textures!\n",
                     origMat.getName().getPtr(),
                     i,
                     Unigeom::BMaterial::getMapName((Unigeom::BMaterial::eMapType)i), 
                     maxMaps);
               }                        
            }
         }
         
         // Check for maps that use invalid UV channels and delete them
         for (int i = 0; i < Unigeom::BMaterial::cNumMapTypes; i++)
         {
            const int numMaps = newMat.getNumMaps((Unigeom::BMaterial::eMapType)i);
            for (int j = numMaps - 1; j >= 0; j--)
            {
               const int uvChannel = newMat.getMap((Unigeom::BMaterial::eMapType)i, j).getChannel();
               
               bool deleteMap = false;
               if (uvChannel >= mUsedAttributes.numUVSets)
                  deleteMap = true;
               else if (i == Unigeom::BMaterial::cNormal)
               {
                  // rg [10/26/06] - I'm disabling this check for now, assume the UV 0 basis vectors will work.
                  // Assumes the input basis vectors directly correspond to input UV channels.
                  //if (uvChannel >= mUsedAttributes.numBasis)
                  //   deleteMap = true;
               }
               
               if (deleteMap)
               {
                  if (mpLog)
                  {
                     mpLog->printf(cTCWarning, 
                        "BUGXGeomInstancer::createOutputMaterials: Material \"%s\": Map \"%s\" references UV channel or basis vector set %i, but this UV channel/basis vector set is not present in the mesh! Deleting all maps present in this category.\n",
                        origMat.getName().getPtr(),
                        newMat.getMap((Unigeom::BMaterial::eMapType)i, j).getName().getPtr(),
                        uvChannel);
                  }                           
                                       
                  newMat.setMap((Unigeom::BMaterial::eMapType)i, j, Unigeom::BMap());
                  newMat.setNumMaps((Unigeom::BMaterial::eMapType)i, 0);
                  break;
               }
            }
         }
         
         // Ensure bump maps are contiguous.
         for (int i = 0; i < newMat.getNumMaps(Unigeom::BMaterial::cNormal); i++)
         {
            if (newMat.getMap(Unigeom::BMaterial::cNormal, i).getName().isEmpty())
            {
               if (mpLog)
                  mpLog->printf(cTCWarning, "Truncating material \"%s\" to %i bump maps!\n", newMat.getName().getPtr(), i);
               newMat.setNumMaps(Unigeom::BMaterial::cNormal, i);
               break;
            }
         }
         
         typedef BArrayUnifier<int> IntMapUnifier;
         IntMapUnifier uvChannelUnifier;
                                     
         for (int mapType = 0; mapType < Unigeom::BMaterial::cNumMapTypes; mapType++)
         {
            for (int mapIndex = 0; mapIndex < newMat.getNumMaps(static_cast<Unigeom::BMaterial::eMapType>(mapType)); mapIndex++)
            {
               Unigeom::BMap& map = newMat.getMap(static_cast<Unigeom::BMaterial::eMapType>(mapType), mapIndex);
               
               if (Unigeom::BMaterial::cEnv == mapType)
               {
                  map.setChannel(0);
               }
               else if (mOptimizeTexCoords)
               {
                  const int oldUVChannel = map.getChannel();
                  
                  const uint newUVChannel = uvChannelUnifier.insert(oldUVChannel).first;

                  if (newUVChannel >= materialUVChannelMap.size())
                     materialUVChannelMap.resize(newUVChannel + 1);
                     
                  materialUVChannelMap.at(newUVChannel) = oldUVChannel;

                  map.setChannel(static_cast<Unigeom::BChannelIndex>(newUVChannel));
               }
            }
         }
      }
      
      const std::pair<int, bool> insertRes = newMatUnifier.insert(newMat);
      
      mOrigToNewMatIndices.push_back(insertRes.first);
      
      if (insertRes.second)
      {
         if (!mOptimizeTexCoords)
         {
            materialUVChannelMap.resize(mUsedAttributes.numUVSets);
            for (int i = 0; i < mUsedAttributes.numUVSets; i++)
               materialUVChannelMap[i] = i;
         }
         
         mNewMatUVChannelMap.push_back(materialUVChannelMap);
      }
   }
   
   BASSERT(mNewMatUVChannelMap.size() == newMatUnifier.size());
   
   mNewMaterials.resize(0);
   for (uint i = 0; i < newMatUnifier.size(); i++)
      mNewMaterials.push_back(newMatUnifier[i]);
   
   dumpOutputMaterials();
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::sortSections
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::sortSections(void)
{
   for (uint sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
   {
      BuildSection& buildSection = mBuildSections[sectionIndex];
      const SectionType& section = mCachedData.section(sectionIndex);
      
      buildSection.mBaseVertPackOrder = createBaseVertexPackOrder(section);
   }

   // Sort by shadow reception flag, material blend type, then base vert pack order.
   for (int i = 0; i < static_cast<int>(mBuildSections.size() - 1); i++)
   {
      BuildSection& buildSectionA = mBuildSections[i];
      SectionType& sectionA = mCachedData.section(i);
      const Unigeom::BMaterial& materialA = mInputGeom.material(buildSectionA.mOrigMaterialIndex);
         
      for (uint j = i + 1; j < mBuildSections.size(); j++)
      {
         BuildSection& buildSectionB = mBuildSections[j];
         SectionType& sectionB = mCachedData.section(j);
         const Unigeom::BMaterial& materialB = mInputGeom.material(buildSectionB.mOrigMaterialIndex);
         
         bool swap = false;
         
         if (materialA.getFlag(Unigeom::BMaterial::cFlagDisableShadowReception) && !materialB.getFlag(Unigeom::BMaterial::cFlagDisableShadowReception))
            swap = true;
         else if (materialA.getFlag(Unigeom::BMaterial::cFlagDisableShadowReception) == materialB.getFlag(Unigeom::BMaterial::cFlagDisableShadowReception))
         {
            if (materialA.getBlendType() > materialB.getBlendType())
               swap = true;
            else if (materialA.getBlendType() == materialB.getBlendType())
            {
               if (buildSectionA.mBaseVertPackOrder > buildSectionB.mBaseVertPackOrder)
                  swap = true;
            }               
         }            
                           
         if (swap)
         {
            std::swap(buildSectionA, buildSectionB);
            std::swap(sectionA, sectionB);
         }
      }
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createSectionRawMesh
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createSectionRawMesh(
   IndexedTriVec& rawTris,
   Unifier<int>& unigeomVertUnifier,
   const int sectionIndex)
{
   BuildSection& buildSection = mBuildSections[sectionIndex];
   //SectionType& section = mCachedData.section(sectionIndex);
           
   if (mpLog)            
      mpLog->printf("createSectionRawMesh: Remerging verts in section %i\n", sectionIndex);
   
   rawTris.resize(buildSection.mSortedTriIndices.size());

   const int numTris = static_cast<int>(buildSection.mSortedTriIndices.size());
   
   for (int triIndex = 0; triIndex < numTris; triIndex++)
   {
      const int sortedTriIndex = buildSection.mSortedTriIndices[triIndex];
      const IndexedTri& unigeomTri = mSortedTris.at(sortedTriIndex).indexedTri();
      
      IndexedTri outTri;
                        
      for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
      {
         const int unigeomVertIndex = unigeomTri[triVertIndex];
                           
         outTri[triVertIndex] = unigeomVertUnifier.insert(unigeomVertIndex).first;
      }

      rawTris[triIndex] = outTri.canonicalize();
   }  
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createOptimizedMeshes
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createOptimizedMeshes(void)
{
   for (uint sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
   {
      BuildSection& buildSection = mBuildSections[sectionIndex];
      //SectionType& section = mCachedData.section(sectionIndex);
               
      IndexedTriVec rawTris;
      Unifier<int> unigeomVertUnifier;
      
      createSectionRawMesh(rawTris, unigeomVertUnifier, sectionIndex);
                        
      const int numTris = static_cast<int>(buildSection.mSortedTriIndices.size());
      const int numVerts = unigeomVertUnifier.size();
      
      buildSection.mFirstOutTri = static_cast<int>(mOutputTris.size());
      buildSection.mFirstOutVert = static_cast<int>(mOutputVerts.size());
           
      if (mpLog)                     
         mpLog->printf("  Optimizing %i tris\n", rawTris.size());
      
      TriListOptimizer listOptimizer(rawTris);
      
      if (mpLog)
      {
         mpLog->printf("  Total Simulated Loads: %i Verts, Average Loads Per Tri: %f\n", 
            listOptimizer.totalCost(),
            listOptimizer.totalCost() / float(rawTris.size()) );
      }               
      
      const IntVec& optimizedTriOrder = listOptimizer.triOrder();
      
      IntVec oldToNew(numVerts);
      std::fill(oldToNew.begin(), oldToNew.end(), -1);
      
      for (int triIndex = 0; triIndex < listOptimizer.numTris(); triIndex++)
      {
         const int reorderedTriIndex = debugRangeCheck(optimizedTriOrder[triIndex], numTris);
         
         IndexedTri newTri;
         for (int triVertIndex = 0; triVertIndex < 3; triVertIndex++)
         {
            const int oldVertIndex = rawTris[reorderedTriIndex][triVertIndex];
            const int unigeomVertIndex = unigeomVertUnifier[oldVertIndex];
            
            int newVertIndex = oldToNew[debugRangeCheck(oldVertIndex, oldToNew.size())];
            if (-1 == newVertIndex)
            {
               newVertIndex = static_cast<int>(mOutputVerts.size()) - buildSection.mFirstOutVert;
               oldToNew[oldVertIndex] = newVertIndex;
               
               mOutputVerts.push_back(unigeomVertIndex);
            }
                                          
            newTri[triVertIndex] = newVertIndex;
         }
      
         mOutputTris.push_back(newTri);   
      }
      
      BASSERT((mOutputVerts.size() - buildSection.mFirstOutVert) == (uint)numVerts);
      
      buildSection.mNumOutTris = listOptimizer.numTris();
      buildSection.mNumOutVerts = static_cast<int>(mOutputVerts.size()) - buildSection.mFirstOutVert;
      BASSERT((uint)buildSection.mNumOutVerts <= buildSection.mSortedTriIndices.size() * 3);
           
      if (mpLog)
      {                              
         mpLog->printf("  Input Tris: %i\n", buildSection.mSortedTriIndices.size());
         mpLog->printf("  Input Verts: %i\n", unigeomVertUnifier.size());
         mpLog->printf("  Output Verts: %i\n", buildSection.mNumOutVerts);
         mpLog->printf("  Input/Output Vertex Ratio: %f\n", float(buildSection.mNumOutVerts) / unigeomVertUnifier.size());
         mpLog->printf("  Average Output Verts Per Tri: %f\n",
            buildSection.mNumOutVerts / float(buildSection.mSortedTriIndices.size()) );
      }               
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createBaseVertexPackOrder
//-------------------------------------------------------------------------------------------------------
BFixedString256 BUGXGeomInstancer::createBaseVertexPackOrder(const SectionType& section)
{
   //const Unigeom::Material& newMat = mCachedData.material(section.materialIndex());
   const IntVec& uvChannelMap = mNewMatUVChannelMap[section.materialIndex()];
      
   BFixedString256 packOrder;
   
   packOrder += "P";
   
   if (mShadowGeom)
   {
      if (!section.rigidOnly())
         packOrder += "S";
   }
   else
   {
      if (mUsedAttributes.norm)
      {
         packOrder += "N";
                           
         for (int i = 0; i < mUsedAttributes.numBasis; i++)
            packOrder += BFixedString256(cVarArg, "A%i", i, i);
      }
         
      if (!section.rigidOnly())
         packOrder += "S";
      
      if (mDiffuseVertexColors)
         packOrder += "D";
         
      const int numUVCoords = static_cast<int>(uvChannelMap.size());
      for (int i = 0; i < numUVCoords; i++)
         packOrder += BFixedString256(cVarArg, "T%i", i);
   }      
      
   return packOrder;
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createBaseOutputVertex
//-------------------------------------------------------------------------------------------------------
Univert BUGXGeomInstancer::createBaseOutputVertex(
   const BuildSection& buildSection,
   const SectionType& outSection,
   const int unigeomVertIndex, 
   const IntVec& uvChannelMap,
   const int numBumpMaps,
   const Unigeom::BMaterial& origMat)
{
   numBumpMaps;
   
   origMat;
   
   const Univert& src = mInputGeom.vert(unigeomVertIndex);

   Univert dst(src);

   for (uint dstUVChannel = 0; dstUVChannel < uvChannelMap.size(); dstUVChannel++)
   {
      const int srcUVChannel = uvChannelMap[dstUVChannel];
      dst.texcoord(dstUVChannel) = src.texcoord(srcUVChannel);
   }

   for (int bumpIndex = 0; bumpIndex < mUsedAttributes.numBasis; bumpIndex++)
   {
      // This assumes the basis vectors where derived from each input texcoord. 
      // If the UV's have been remapped, so have the basis vectors.
      // The logic here is weak.
      int srcUVChannel = ((uint)bumpIndex < uvChannelMap.size()) ? uvChannelMap[bumpIndex] : 0;

      if (srcUVChannel >= mUsedAttributes.numBasis)
         srcUVChannel = 0;

      dst.tangent(bumpIndex) = src.tangent(srcUVChannel);
      dst.binormal(bumpIndex) = src.binormal(srcUVChannel);
   }

   for (int i = 0; i < Univert::MaxInfluences; i++)
   {
      int boneIndex = dst.boneIndex(i);
      float boneWeight = dst.boneWeight(i);

      if (boneWeight > 0.0f)
      {
         if (boneIndex == Univert::DefaultBoneIndex)
            boneIndex = -1;

         if ((mGlobalBonesOnly) || (0 == outSection.numLocalBones()))
         {
            // store global bone index
            dst.boneIndex(i) = static_cast<uint8>(boneIndex + 1);
         }
         else 
         {
            // remap to local bone index
            dst.boneIndex(i) = static_cast<uint8>(debugRangeCheck(buildSection.mGlobalBoneToLocalBone.at(boneIndex + 1), buildSection.mNumLocalBones));
         }
      }
   }

   return dst;
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createBaseOutputGeom
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createBaseOutputGeom(void)
{
   mDataBuilder.getVB().reserve(1024*1024*8);
   
   UnivertPackerType vertPacker;
   vertPacker.setPos(VertexElement::eHALFFLOAT4);
   vertPacker.setNorm(VertexElement::eDEC3N);
   vertPacker.setBasis(VertexElement::eDEC3N);
   vertPacker.setBasisScales(VertexElement::eHALFFLOAT2);
   vertPacker.setTangent(VertexElement::eDEC3N);
   vertPacker.setIndices(VertexElement::eUBYTE4);
   vertPacker.setWeights(VertexElement::eUBYTE4N);
   vertPacker.setDiffuse(VertexElement::eD3DCOLOR);
   vertPacker.setUV(VertexElement::eHALFFLOAT2);
      
   for (uint sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
   {
      const BuildSection& buildSection = mBuildSections[sectionIndex];
      SectionType& outSection = mCachedData.section(sectionIndex);
      
      const Unigeom::BMaterial& origMat = mInputGeom.material(buildSection.mOrigMaterialIndex);
      const Unigeom::BMaterial& newMat = mNewMaterials[buildSection.mNewMaterialIndex];
      
      const IntVec& uvChannelMap = mNewMatUVChannelMap[buildSection.mNewMaterialIndex];

      const int numBaseBumpMaps = mShadowGeom ? 0 : newMat.getNumMaps(Unigeom::BMaterial::cNormal);
            
      const BFixedString256& packOrder = buildSection.mBaseVertPackOrder;
      
      vertPacker.setPackOrder(packOrder);
      
      outSection.setBaseVertPacker(vertPacker);
                              
      const int outVertexSize = vertPacker.size();
      const int outVertexBytes = outVertexSize * buildSection.mNumOutVerts;
      
      if (mpLog)
      {
         mpLog->printf("createBaseOutputGeom: Section %i, Used Elements \"%s\", Vertex Size: %i, VB Size: %i\n", 
            sectionIndex, 
            packOrder.c_str(),
            outVertexSize,
            outVertexBytes);
         
         vertPacker.log(*mpLog);
      }               
                          
      const int cPackedVertexBufferSize = 512;
      BASSERT(outVertexSize <= cPackedVertexBufferSize);
      uchar packedVertexBuf[cPackedVertexBufferSize];
      memset(packedVertexBuf, 0xCC, sizeof(packedVertexBuf));
      
      outSection.setVBOfs(static_cast<int>(mDataBuilder.getVB().size()));
      outSection.setVBBytes(outVertexBytes);
      outSection.setVertSize(outVertexSize);
      outSection.setNumVerts(buildSection.mNumOutVerts);
      
      if ((!mGlobalBonesOnly) && (!mCachedData.header().rigidOnly()) && (!outSection.rigidOnly()))
      {
         outSection.localToGlobalBoneRemap().resize(buildSection.mNumLocalBones);
         for (int i = 0; i < buildSection.mNumLocalBones; i++)
            outSection.localToGlobalBoneRemap().at(i) = buildSection.mLocalBoneToGlobalBone.at(i);             
      }
            
      for (int outVertIndex = 0; outVertIndex < buildSection.mNumOutVerts; outVertIndex++)
      {
         Univert outVert(createBaseOutputVertex(
            buildSection,
            outSection,
            mOutputVerts.at(buildSection.mFirstOutVert + outVertIndex),
            uvChannelMap,
            numBaseBumpMaps,
            origMat));
            
         // Packer will skip unnecessary outVert elements.
         vertPacker.pack(packedVertexBuf, outVert);

         mDataBuilder.getVB().pushBack(&packedVertexBuf[0], outVertexSize);
      }
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::findAABB
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::findAABB(void)
{
   AABB bounds(AABB::eInitExpand);

   for (int keyFrameIndex = 0; keyFrameIndex < mInputGeom.numMorphTargets(); keyFrameIndex++)        
   {
      for (uint vertIndex = 0; vertIndex < mOutputVerts.size(); vertIndex++)
      {
         const int origUnigeomVert = mOutputVerts[vertIndex];
         
         const Unigeom::MorphVert& morphVert = mInputGeom.morphTarget(keyFrameIndex)[origUnigeomVert];
         
         bounds.expand(morphVert.p);
      }
   }
   
   mCachedData.header().setBounds(bounds);

   if (mpLog)
   {
      mpLog->printf("Overall bounds: (%f %f %f) - (%f %f %f)\n",
         bounds[0][0], bounds[0][1], bounds[0][2],
         bounds[1][0], bounds[1][1], bounds[1][2]);
   }            
      
   mCachedData.header().setBoundingSphere(Sphere(bounds.center(), bounds.diagonal().len() * .5f));
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::findBoneBounds
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::findBoneBounds(void)
{
   if (!numSkeletonBones())
      return;
      
   if ((int)mBoneBounds.size() != numSkeletonBones())
   {
      mBoneBounds.resize(numSkeletonBones());
   
      for (int i = 0; i < numSkeletonBones(); i++)
         mBoneBounds[i] = AABB(AABB::eInitExpand);
   }
         
   for (int keyFrameIndex = 0; keyFrameIndex < mInputGeom.numMorphTargets(); keyFrameIndex++)
   {
      for (uint vertIndex = 0; vertIndex < mOutputVerts.size(); vertIndex++)
      {
         const int origUnigeomVert = mOutputVerts[vertIndex];
         
         const Univert& baseVert = mInputGeom.vert(origUnigeomVert);
         const Unigeom::MorphVert& morphVert = mInputGeom.morphTarget(keyFrameIndex)[origUnigeomVert];
         
         const float MinWeight = 1.0f/64.0f;
         
         for (int i = 0; i < baseVert.numActualInfluences(); i++)
         {
            const int boneIndex = baseVert.boneIndex(i);
            const float boneWeight = baseVert.boneWeight(i);
            
            if ((Univert::DefaultBoneIndex == boneIndex) || (boneWeight < MinWeight))
               continue;
               
            BASSERT(boneIndex >= 0);
            BASSERT(boneIndex < numSkeletonBones());
            
            //const BMatrix44 bindToBone(skeletonBone(boneIndex).modelToBone().getMatrix());
            //mBoneBounds[boneIndex].expand(BVec3(BVec4(morphVert.p, 1.0f) * bindToBone));
            
            mBoneBounds[boneIndex].expand(morphVert.p);
         }
      }
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createOutputIndices
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createOutputIndices(void)
{
   uint maxIndex = 0;
   for (uint triIndex = 0; triIndex < mOutputTris.getSize(); triIndex++)
   {
      const IndexedTri& outTri = mOutputTris[triIndex];
      maxIndex = Math::Max(maxIndex, outTri.maxVertIndex());
   }

   uint instanceIndexMultiplier = maxIndex + 1;
   if (!Math::IsPow2(instanceIndexMultiplier))
      instanceIndexMultiplier = Math::NextPowerOf2(instanceIndexMultiplier);

   uint maxInstances = 1;
   
   if (mSupportInstancing)
   {
      const uint cInstancingTriCountThresh = 2048;
      if (mOutputTris.getSize() <= cInstancingTriCountThresh)
      {
         maxInstances = 65534 / instanceIndexMultiplier;
         maxInstances = Math::IsPow2(maxInstances) ? maxInstances : (Math::NextPowerOf2(maxInstances) >> 1);
         maxInstances = Math::Clamp<uint>(maxInstances, 1U, BUGXGeom::cMaxInstances);
      }
   }
   
   if (maxInstances * instanceIndexMultiplier >= 65535)
   {
      maxInstances = Math::Max(1U, maxInstances >> 1U);
      BDEBUG_ASSERT(maxInstances * instanceIndexMultiplier < 65535);
   }

   mCachedData.header().setMaxInstances(maxInstances);
   mCachedData.header().setInstanceIndexMultiplier(instanceIndexMultiplier);
         
   DataBuilderType::IBArrayType& ibArray = mDataBuilder.getIB();

   ibArray.resize(0);
   
   for (uint sectionIndex = 0; sectionIndex < mBuildSections.size(); sectionIndex++)
   {
      const BuildSection& buildSection = mBuildSections[sectionIndex];
      
      SectionType& section = mCachedData.section(sectionIndex);
   
      section.setNumTris(buildSection.mNumOutTris);
      section.setIBOfs(ibArray.getSize());
      
      for (uint instanceIndex = 0; instanceIndex < maxInstances; instanceIndex++)
      {
         const uint instanceIndexOffset = instanceIndexMultiplier * instanceIndex;

         for (int sectionTriIndex = 0; sectionTriIndex < buildSection.mNumOutTris; sectionTriIndex++)
         {
            const IndexedTri& tri = mOutputTris[buildSection.mFirstOutTri + sectionTriIndex];

            for (uint triVertIndex = 0; triVertIndex < 3; triVertIndex++)
            {
               ibArray.pushBack( static_cast<DataBuilderType::IndexType>(tri[triVertIndex] + instanceIndexOffset) );
               
               BDEBUG_ASSERT(tri[triVertIndex] < (uint)buildSection.mNumOutVerts);
            }
         }
      }
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createOutputAccessories
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createOutputAccessories(void)
{
   Unigeom::AccessoryVec outAccessories(mInputGeom.accessories());
   
   for (uint i = 0; i < outAccessories.getSize(); i++)
      outAccessories[i].objectIndices().resize(0);

   // For each accessory (mesh), create a list of section indices, so we know what sections to render given an accessory.
   for (int sectionIndex = 0; sectionIndex < mCachedData.numSections(); sectionIndex++)
   {
      const int accessoryIndex = mCachedData.section(sectionIndex).accessoryIndex();
      if (accessoryIndex >= (int)outAccessories.getSize())
         outAccessories.resize(accessoryIndex + 1);
         
      outAccessories.at(accessoryIndex).objectIndices().push_back(sectionIndex);
   }

   for (uint i = 0; i < outAccessories.size(); i++)
   {
      std::sort(outAccessories[i].objectIndices().begin(), outAccessories[i].objectIndices().end());
   }
   
   mCachedData.accessories().resize(outAccessories.getSize());
   for (uint i = 0; i < outAccessories.getSize(); i++)
      mCachedData.accessories()[i] = outAccessories[i];

   // create an array of valid (used) accessories
   for (int accessoryIndex = 0; accessoryIndex < mCachedData.numAccessories(); accessoryIndex++)
   {
      if (mCachedData.accessory(accessoryIndex).numObjectIndices() > 0)
      {
         mCachedData.validAccessories().push_back(accessoryIndex);
      }
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createOutputBones
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createOutputBones(void)
{
   for (int i = 0; i < mInputGeom.numBones(); i++)
   {
      mCachedData.bones().push_back(mInputGeom.bone(i));
      mCachedData.boneBoundsLowArray().push_back(mBoneBounds[debugRangeCheck(i, mBoneBounds.size())][0]);
      mCachedData.boneBoundsHighArray().push_back(mBoneBounds[debugRangeCheck(i, mBoneBounds.size())][1]);
   }
}      

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::determineAllSectionsSkinned
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::determineAllSectionsSkinned(void)
{
   if (!mCachedData.header().globalBones())
      return;

   if ((mCachedData.header().allSectionsRigid()) || (mCachedData.header().rigidOnly()))
      return;

   for (int sectionIndex = 0; sectionIndex < mCachedData.numSections(); sectionIndex++)
   {
//-- FIXING PREFIX BUG ID 7656
      const SectionType& section = mCachedData.section(sectionIndex);
//--
      
      if (section.rigidOnly())
         return;
   }

   // At this point, we know:
   //  Model uses global bones
   //  There are no rigid sections
   // Therefore, all sections are skinned with global bones.
   mCachedData.header().setAllSectionsSkinned(true);
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::packOutputMaterials(void)
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::packOutputMaterials(void)
{
   BBinaryDataTree::BDocumentBuilder docBuilder;
   docBuilder.getRootNode().setName("Materials");
         
   for (uint i = 0; i < mNewMaterials.getSize(); i++)
   {
      const Unigeom::BMaterial& material = mNewMaterials[i];
      
      BBinaryDataTree::BDocumentBuilder::BNode node(docBuilder.getRootNode().addChild("Material"));
      
      bool success = material.write(node);
      BVERIFY(success);
   }
   
   if (mpLog)
   {
      mpLog->printf("********************* BUGXGeomInstancer::packOutputMaterials:\n");
      docBuilder.dumpToXML(*mpLog, true);
   }
   
   BByteArray packedData;
   bool success = docBuilder.serialize(packedData, mBigEndian);
   BVERIFY(success);
   
   mDataBuilder.getMaterialData().pushBack(packedData.getPtr(), packedData.getSizeInBytes());
}
                        
//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::createOutputGeom
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::createOutputGeom(void)
{
   createBaseOutputGeom();
   
   findAABB();
   
   findBoneBounds();
         
   createOutputIndices();
   
   createOutputBones();
   
   createOutputAccessories();
   
   packOutputMaterials();
   
   determineAllSectionsSkinned();
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::build
//-------------------------------------------------------------------------------------------------------
bool BUGXGeomInstancer::build(void)
{
   if (mpLog)
      mpLog->printf("BUGXGeomInstancer::build: Started\n");
      
   if (!buildInit())
      return false;
      
   determineRigidOnlyModel();
              
   createOutputMaterials();      
   
   sortTrisByMaterial();
   
   findSections();
   
   if (!initSectionBones())
      return false;
     
   sortSections();
   
   createOptimizedMeshes();
   
   createOutputGeom();

   if (mpLog)
      mpLog->printf("BUGXGeomInstancer::build: Finished\n");
      
   if (mpLog)
      mDataBuilder.log(*mpLog);
      
   gConsoleOutput.printf("Max instances: %u\n", mDataBuilder.getCachedData().header().getMaxInstances());
   gConsoleOutput.printf("Total sections: %u\n", mDataBuilder.getCachedData().numSections());
   
   return true;
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::packData
//-------------------------------------------------------------------------------------------------------
bool BUGXGeomInstancer::packData(BECFFileBuilder& ecfBuilder)
{
   if (cBigEndianNative == mBigEndian)
   {
      return mDataBuilder.createPackedData(ecfBuilder);
   }
   else
   {
      typedef BUGXGeom::BECFDataBuilder<!cBigEndianNative> OppEndianDataBuilderType;
      
      OppEndianDataBuilderType oppEndianDataBuilder(mDataBuilder);
      
      return oppEndianDataBuilder.createPackedData(ecfBuilder);
   }
}

//-------------------------------------------------------------------------------------------------------
// BUGXGeomInstancer::logData
//-------------------------------------------------------------------------------------------------------
void BUGXGeomInstancer::logData(BTextDispatcher& log)
{
   log.printf("BUGXGeomInstancer::logData:\n");
   
   mDataBuilder.log(log);
}














