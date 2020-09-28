//-----------------------------------------------------------------------------
// File: unigeom.cpp
//-----------------------------------------------------------------------------
#include "xgeom.h"

#include "unigeom.h"

namespace Unigeom
{
   //---------------------------------------------------------------------------------
   // Geom::Geom
   //---------------------------------------------------------------------------------
   Geom::Geom() :
      mModelType(cMTNormal),
      mVertAttributesLocked(false),
      mNumVerts(0),
      mVertSize(0)
   {
   }

   //---------------------------------------------------------------------------------
   // Geom::~Geom
   //---------------------------------------------------------------------------------
   Geom::~Geom()
   {
   }
   
   //---------------------------------------------------------------------------------
   // Geom::insertMaterial
   //---------------------------------------------------------------------------------
   MaterialIndex Geom::insertMaterial(const Unigeom::BMaterial& m) 
   { 
      return mMaterials.insert(m).first; 
   }

   //---------------------------------------------------------------------------------
   // Geom::insertTri
   //---------------------------------------------------------------------------------
   TriIndex Geom::insertTri(const Tri& t) 
   { 
      mTris.push_back(t); return numTris() - 1; 
   }
   
   //---------------------------------------------------------------------------------
   // Geom::insertMorphTarget
   //---------------------------------------------------------------------------------
   MorphTargetIndex Geom::insertMorphTarget(const MorphTarget& m)
   {
      mMorphTargets.push_back(m);
      return numMorphTargets() - 1;
   }

   //---------------------------------------------------------------------------------
   // Geom::insertBone
   //---------------------------------------------------------------------------------
   BoneIndex Geom::insertBone(const BUnpackedBoneType& b)
   {
      mBones.push_back(b);
      return numBones() - 1;
   }  

   //---------------------------------------------------------------------------------
   // Geom::insertAccessory
   //---------------------------------------------------------------------------------
   AccessoryIndex Geom::insertAccessory(const BUnpackedAccessoryType& a)      
   {
      mAccessories.push_back(a);
      return static_cast<int>(mAccessories.size()) - 1;
   }

   //---------------------------------------------------------------------------------
   // Geom::clear
   //---------------------------------------------------------------------------------
   void Geom::clear(void)
   {
      mName.clear();
      mMaterials.clear();
      mTris.clear();

      mPackedVerts.clear();
      mVertAttributes.clear();
      mVertAttributesLocked = false;
      mVertStreamer.clear();
      mVertSize = 0;
      mNumVerts = 0;
      mModelType = cMTNormal;
                     
      mMorphTargets.clear();
      mBones.clear();
      mAccessories.clear();
   }
         
   //---------------------------------------------------------------------------------
   // Geom::insertVert
   //---------------------------------------------------------------------------------
   VertIndex Geom::insertVert(const Univert& v)
   {
      if (!mVertAttributesLocked)
      {
         const UnivertAttributes vertUsedAttributes(v.usedAttributes());
         const UnivertAttributes newVertAttributes(vertUsedAttributes | mVertAttributes);
         
         if (newVertAttributes != mVertAttributes)
            changeVertAttributes(newVertAttributes);
      }
                          
      mPackedVerts.resize(mPackedVerts.size() + mVertSize);
      
      v.pack(mVertAttributes, &mPackedVerts[mPackedVerts.size() - mVertSize]);
      
      const VertIndex vertIndex = mNumVerts;
      
      mNumVerts++;
                        
      return vertIndex;
   }
   
   //---------------------------------------------------------------------------------
   // Geom::setVert
   //---------------------------------------------------------------------------------
   void Geom::setVert(VertIndex vertIndex, const Univert& v)
   {
      BDEBUG_ASSERT((uint)vertIndex < (uint)mNumVerts);
      v.pack(mVertAttributes, &mPackedVerts[vertIndex * mVertSize]);
   }
   
   //---------------------------------------------------------------------------------
   // Geom::changeVertAttributes
   //---------------------------------------------------------------------------------
   int Geom::changeVertAttributes(const UnivertAttributes& newAttributes)
   {
      const int newVertSize = newAttributes.size();
      
      UCharVec newPackedVerts(mNumVerts * newVertSize);
      
      for (uint i = 0; i < mNumVerts; i++)
         vert(i).pack(newAttributes, &newPackedVerts[i * newVertSize]);

      newPackedVerts.swap(mPackedVerts);            
      
      mVertAttributes = newAttributes;
      
      mVertStreamer.setAttributes(mVertAttributes);
      
      mVertSize = newVertSize;
      
      return newVertSize;
   }
    
   //---------------------------------------------------------------------------------
   // Geom::vert
   //---------------------------------------------------------------------------------     
   Univert Geom::vert(VertIndex i) const
   { 
      debugRangeCheck(i, numVerts());
      return Univert(mVertAttributes, &mPackedVerts[i * mVertSize]);
   }
   
   //---------------------------------------------------------------------------------
   // Geom::packedVertPtr
   //---------------------------------------------------------------------------------
   const uchar* Geom::packedVertPtr(VertIndex i) const
   {
      debugRangeCheck(i, numVerts());
      return &mPackedVerts[mVertSize * i];
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertPos
   //---------------------------------------------------------------------------------
   BVec3 Geom::vertPos(VertIndex i) const
   {
      BASSERT(-1 != mVertStreamer.posOfs());
      return *reinterpret_cast<const BVec3*>( packedVertPtr(i) + mVertStreamer.posOfs());
   }

   //---------------------------------------------------------------------------------
   // Geom::vertNorm
   //---------------------------------------------------------------------------------          
   BVec3 Geom::vertNorm(VertIndex i) const
   {
      BASSERT(-1 != mVertStreamer.normOfs());
      return *reinterpret_cast<const BVec3*>( packedVertPtr(i) + mVertStreamer.normOfs());
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertS
   //---------------------------------------------------------------------------------
   BVec3 Geom::vertS(VertIndex i, uint set) const
   {
      BASSERT(-1 != mVertStreamer.basisOfs());
      BASSERT(set < (uint)Univert::MaxBasisVecs && set < (uint)mVertAttributes.numBasis);
      return *reinterpret_cast<const BVec3*>( packedVertPtr(i) + mVertStreamer.basisOfs() + sizeof(BVec3) * 2 * set) ;
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertT
   //---------------------------------------------------------------------------------
   BVec3 Geom::vertT(VertIndex i, uint set) const
   {
      BASSERT(-1 != mVertStreamer.basisOfs());
      BASSERT(set < (uint)Univert::MaxBasisVecs && set < (uint)mVertAttributes.numBasis);
      return *reinterpret_cast<const BVec3*>( packedVertPtr(i) + mVertStreamer.basisOfs() + sizeof(BVec3) * 2 * set + sizeof(BVec3));
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertUV
   //---------------------------------------------------------------------------------
   BVec2 Geom::vertUV(VertIndex i, uint set) const
   {
      BASSERT(-1 != mVertStreamer.uvOfs());
      BASSERT(set < (uint)Univert::MaxUVCoords && set < (uint)mVertAttributes.numUVSets);
      return *reinterpret_cast<const BVec2*>( packedVertPtr(i) + mVertStreamer.uvOfs() + sizeof(BVec2) * set);
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertIndices
   //---------------------------------------------------------------------------------
   void Geom::vertIndices(uint8* pIndices, VertIndex i) const
   {
      BASSERT(pIndices);
      BASSERT(-1 != mVertStreamer.indicesOfs());
      
      const uchar* pSrcIndices = packedVertPtr(i) + mVertStreamer.indicesOfs();
      
      Utils::Copy(pSrcIndices, pSrcIndices + Univert::MaxInfluences, pIndices);
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertWeights
   //---------------------------------------------------------------------------------
   BVecN<Univert::MaxInfluences> Geom::vertWeights(VertIndex i) const
   {
      BASSERT(-1 != mVertStreamer.weightsOfs());
      return *reinterpret_cast<const BVecN<Univert::MaxInfluences>*>( packedVertPtr(i) + mVertStreamer.weightsOfs() );
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertDiffuse
   //---------------------------------------------------------------------------------
   BVec4 Geom::vertDiffuse(VertIndex i) const
   {
      BASSERT(-1 != mVertStreamer.diffuseOfs());
      return *reinterpret_cast<const BVec4*>( packedVertPtr(i) + mVertStreamer.diffuseOfs() );
   }
   
   //---------------------------------------------------------------------------------
   // Geom::vertIndex
   //---------------------------------------------------------------------------------
   int Geom::vertIndex(VertIndex i) const
   {
      BASSERT(-1 != mVertStreamer.indexOfs());
      return *reinterpret_cast<const int*>( packedVertPtr(i) + mVertStreamer.indicesOfs() );
   }
   
   //---------------------------------------------------------------------------------
   // Geom::log
   //---------------------------------------------------------------------------------      
   void Geom::log(BTextDispatcher& log) const      
   {
      log.printf("Geom Name: \"%s\"\n", mName.c_str());
      log.printf("ModelType: %u\n", mModelType);

      log.printf("Tris: %i, Verts: %i, Morph Targets: %i\n",
         mTris.size(),
         mNumVerts,
         mMorphTargets.size());
      
      BFixedString256 str;
      mVertAttributes.getDescStr(str);
      log.printf("Bytes per vert: %i, Total vert Bytes: %i, Desc: %s\n", 
         mVertSize,
         mPackedVerts.size(),
         str.c_str());
                  
      log.printf("Verts per tri: %f\n", mTris.size() ? mNumVerts / float(mTris.size()) : 0.0f);

      log.printf("Total materials: %i\n", mMaterials.size());
      log.indent(1);
      for (uint i = 0; i < mMaterials.size(); i++)
      {
         log.printf("Material %i:\n", i);
         mMaterials[i].log(log);
      }
      log.indent(-1);
   }

} // namespace Unigom


