//============================================================================
//
//  ugxRenderBase.cpp
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xgameRender.h"

// local
#include "ugxRenderBase.h"
#include "BD3D.h"

// rg [ 5/29/06] - None of this is currently used (and may never be used).

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::BUGXGeomSection
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomSection::BUGXGeomSection()
{
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::BUGXGeomSection
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomSection::BUGXGeomSection(const UGXGeom::Geom& geom, const UGXGeom::Section& section) :
   mpSection(&section)
{
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::BUGXGeomSection
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomSection::BUGXGeomSection(const BUGXGeomSection& b)
{
   *this = b;
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::operator=
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomSection& BUGXGeomSection::operator= (const BUGXGeomSection& rhs)
{
   if (this == &rhs)
      return *this;

   mpSection = rhs.mpSection;
   mVertexDeclHandle = rhs.mVertexDeclHandle;

   return *this;
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::createVertexDecls
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomSection::createVertexDecls(void)
{
   BUnivertPackerArray packers;

   const UnivertPacker::ElementStats baseStats(mpSection->baseVertPacker().getStats());
   const uint MAX_PASS0_TEXCOORDS = 5;


   BASSERT(1 == baseStats.numPos);

   UnivertPacker basePacker(mpSection->baseVertPacker());
      
   IntVec basisMap;
   IntVec basisScalesMap;
   IntVec uvMap;

   BASSERT(baseStats.numTexCoords <= MAX_PASS0_TEXCOORDS);
   for (int i = 0; i < MAX_PASS0_TEXCOORDS; i++)
      uvMap.push_back(i);

   BASSERT(baseStats.numBasis <= 2);
   BASSERT(baseStats.numBasisScales <= 2);
   for (int i = 0; i < 2; i++)
   {
      basisMap.push_back(i);
      const uint FIRST_BASIS_SCALE_TEXCOORD_INDEX = 0;
      basisScalesMap.push_back(FIRST_BASIS_SCALE_TEXCOORD_INDEX + i);
   }

   basePacker.setDeclOrder(0, basisMap, basisScalesMap, 0, uvMap);

   packers.push_back(basePacker);
   
   mVertexDeclHandle = gVertexDeclManager.create(packers);     
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::setVertexDecl
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomSection::setVertexDecl(void) const
{
   gVertexDeclManager.setToDevice(mVertexDeclHandle);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomSection::draw
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomSection::draw(IDirect3DVertexBuffer9* pVB) const
{              
   BD3D::mpDev->SetStreamSource(0, pVB, mpSection->VBOfs(), mpSection->vertSize());

   BD3D::mpDev->DrawIndexedPrimitive(
      D3DPT_TRIANGLELIST,
      0,       // BaseVertexIndex
      0,       // MinIndex
      mpSection->numVerts(),     // NumVertices
      mpSection->IBOfs(),           // StartIndex
      mpSection->numTris());     // PrimitiveCount
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::BUGXGeomRenderBase
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomRenderBase::BUGXGeomRenderBase() : 
   mpGeom(NULL),
   mpVB(NULL),
   mpIB(NULL)
{
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::~BUGXGeomRenderBase
//------------------------------------------------------------------------------------------------------------------------------
BUGXGeomRenderBase::~BUGXGeomRenderBase()
{
   clear();
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::setGeom
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::setGeom(UGXGeom::Geom* pGeom)
{
   clear();
   
   mpGeom = pGeom;
   
   init();
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::setGeom
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::setGeom(UGXGeom::Geom& geom)
{
   clear();

   mpGeom = new UGXGeom::Geom(geom);

   init();
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::load
//------------------------------------------------------------------------------------------------------------------------------
bool BUGXGeomRenderBase::load(BStream& stream)
{
   clear();
   
   mpGeom = new UGXGeom::Geom;

   if (!mpGeom->read(stream))
   {
      clear();
      
      return false;
   }
   
   init();
   
   return true;
}
                        
//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::initHierModelToWorld
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::initHierModelToWorld(const BMatrix44& modelToWorld)
{
   mHier[0] = modelToWorld;
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::initHierAccessoryMatrices
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::initHierAccessoryMatrices(
   uint accessoryIndex,
   const BMatrix44* pMatrices, uint numMatrices,
   const BMatrix44* pLocalMatrix)
{
   debugRangeCheck(accessoryIndex, mpGeom->numAccessories());
   BASSERT(numMatrices == mpGeom->accessory(accessoryIndex).numBones());

   if (numMatrices)
   {
      checkNull(pMatrices);

      const int firstInternalMatrix = 1 + mpGeom->accessory(accessoryIndex).firstBone();
      const int lastInternalMatrix = firstInternalMatrix + numMatrices - 1;
      
      BMatrix44* pDest = &mHier[0];
      if (pLocalMatrix)
      {
         for (int i = 0; i < numMatrices; i++)
            pDest[i + firstInternalMatrix] = pMatrices[i] * (*pLocalMatrix);
      }
      else
      {
         for (int i = 0; i < numMatrices; i++)
            pDest[i + firstInternalMatrix] = pMatrices[i];
      }
   }
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::initHier
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::initHier(const BMatrix44& worldToView, const ushort* pAccessoryIndices, uint numAccessoryIndices)
{
   // mHier now has model->View
   BMatrix44& modelToView = mHier[0];
   modelToView *= worldToView;
   
   BVec4* pBoneMatrices = mTHier.begin();

   // first is special because it's the "root" node (bone index -1)
   pBoneMatrices[0] = modelToView.getColumn(0);
   pBoneMatrices[1] = modelToView.getColumn(1);
   pBoneMatrices[2] = modelToView.getColumn(2);
   
   if (mpGeom->header().levelGeom())
      return;
   
   // Concat the model->view matrix to all active bind->model matrices.
   const uint numActiveAccessoryIndices = pAccessoryIndices ? numAccessoryIndices : mpGeom->numAccessories();
   for (int activeAccessoryIndex = 0; activeAccessoryIndex < numActiveAccessoryIndices; activeAccessoryIndex++)
   {
      const int accessoryIndex = pAccessoryIndices ? pAccessoryIndices[activeAccessoryIndex] : activeAccessoryIndex;
      const int firstInternalBoneIndex = mpGeom->accessory(accessoryIndex).firstBone() + 1;

      BVec4* pDst = pBoneMatrices + firstInternalBoneIndex * 3;
      
      for (int boneIndex = 0; boneIndex < mpGeom->accessory(accessoryIndex).numBones(); boneIndex++)
      {
         BMatrix44& matrix = mHier[firstInternalBoneIndex + boneIndex];
         matrix = matrix * modelToView;
         
         pDst[0] = matrix.getColumn(0);
         pDst[1] = matrix.getColumn(1);
         pDst[2] = matrix.getColumn(2);
         pDst += 3;
      }
   }
}         

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::setHierGlobalConstants
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::setHierGlobalConstants(uint firstRegister, const ushort* pAccessoryIndices, uint numAccessoryIndices)
{
   // skip if the model is rigid but each section may use a separate matrix,
   // in this case we're going to upload as we go
   if (mpGeom->header().allSectionsRigid())
   {
      if (!mpGeom->header().rigidOnly())
         return;
   }

   BVec4* pBoneMatrices = mTHier.begin();
   
   // all sections use the same matrix
   if (mpGeom->header().rigidOnly())
   {
      const int matrixIndex = debugRangeCheck(mpGeom->header().rigidBoneIndex() + 1, mpGeom->numBones() + 1);

      const BVec4* pMatrix = &pBoneMatrices[matrixIndex * 3];         

      BD3D::mpDev->SetVertexShaderConstantF(
         firstRegister, 
         reinterpret_cast<const float*>(pMatrix), 
         3);
   }
   // global bones, upload the constants used by the active models
   else if (mpGeom->header().globalBones())
   {
      BD3D::mpDev->SetVertexShaderConstantF(
         firstRegister, 
         reinterpret_cast<const float*>(pBoneMatrices), 
         3);

      const uint numActiveModelIndices = pAccessoryIndices ? numAccessoryIndices : mpGeom->numAccessories();
      for (int activeModelIndex = 0; activeModelIndex < numActiveModelIndices; activeModelIndex++)
      {
         const int accessoryIndex = pAccessoryIndices ? pAccessoryIndices[activeModelIndex] : activeModelIndex;
         const int firstInternalBoneIndex = mpGeom->accessory(accessoryIndex).firstBone() + 1;
         const int numInternalBones = mpGeom->accessory(accessoryIndex).numBones();

         const int numShaderBones = numInternalBones;
         const int numShaderBoneVectors = numShaderBones * 3;

         BVec4* pSrc = pBoneMatrices + firstInternalBoneIndex * 3;

         BD3D::mpDev->SetVertexShaderConstantF(
            firstRegister + firstInternalBoneIndex * 3,
            reinterpret_cast<const float*>(pSrc), 
            numShaderBoneVectors);
      }
   }
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::setHierSectionConstants
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::setHierSectionConstants(uint firstRegister, uint sectionIndex)
{
   if (mpGeom->header().rigidOnly())
      return;

   const UGXGeom::Section& UGXSection = mpGeom->section(sectionIndex);
            
   if (UGXSection.rigidOnly())
   {
      // Rigid only - set this section's matrix.
      const int numShaderBones = 1 + mpGeom->numBones();
      const int matrixIndex = debugRangeCheck(UGXSection.rigidBoneIndex() + 1, numShaderBones);

      const BMatrix44 temp(mHier[matrixIndex]);

      BVec4 boneMatrices[3];
      boneMatrices[0] = temp.getColumn(0);
      boneMatrices[1] = temp.getColumn(1);
      boneMatrices[2] = temp.getColumn(2);

      BD3D::mpDev->SetVertexShaderConstantF(
         firstRegister, 
         reinterpret_cast<const float*>(boneMatrices), 
         3);
   }
   else if (!mpGeom->header().globalBones())
   {
      // Local bones - set this section's matrices.
      BASSERT(UGXSection.numLocalBones() > 0);
      BVERIFY((UGXSection.numLocalBones() <= cMaxLocalBones) && "Too many local bones!");

      const BVec4* pSrcMatrices = mTHier.begin();
      BVec4 tempMatrices[cMaxLocalBones * 3];
      BVec4* pDstMatrices = tempMatrices;

      for (int i = 0; i < UGXSection.numLocalBones(); i++)
      {
         const int globalBoneIndex = UGXSection.localToGlobalBone(i);
         debugRangeCheck(globalBoneIndex, mpGeom->numBones() + 1);

         pDstMatrices[0] = pSrcMatrices[globalBoneIndex * 3 + 0];
         pDstMatrices[1] = pSrcMatrices[globalBoneIndex * 3 + 1];
         pDstMatrices[2] = pSrcMatrices[globalBoneIndex * 3 + 2];
         pDstMatrices += 3;
      }

      BD3D::mpDev->SetVertexShaderConstantF(firstRegister, reinterpret_cast<const float*>(tempMatrices), UGXSection.numLocalBones() * 3);          
   }
}
      
//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::calcSectionsIndices
//------------------------------------------------------------------------------------------------------------------------------
uint BUGXGeomRenderBase::calcSectionsIndices(ushort* pSectionIndices, const ushort* pAccessoryIndices, uint numAccessoryIndices)
{
   int numSectionsToRender = 0;

   if ((!pAccessoryIndices) || (numAccessoryIndices == mpGeom->validAccessories().size()))
   {
      numSectionsToRender = mpGeom->numSections();
      for (int i = 0; i < numSectionsToRender; i++)
         pSectionIndices[i] = i;
   }
   else
   {
      for (int activeAccessoryIndex = 0; activeAccessoryIndex < numAccessoryIndices; activeAccessoryIndex++)
      {
         const int accessoryIndex = pAccessoryIndices[activeAccessoryIndex];
         const Unigeom::Accessory& accessory = mpGeom->accessory(accessoryIndex);

         for (int modelSectionIndex = 0; modelSectionIndex < accessory.numObjectIndices(); modelSectionIndex++)
         {
            const int sectionIndex = accessory.objectIndex(modelSectionIndex);

            pSectionIndices[numSectionsToRender++] = sectionIndex;
         }
      }

      std::sort(pSectionIndices, pSectionIndices + numSectionsToRender);
   }

   return numSectionsToRender;      
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::setIndexBuffer
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::setIndexBuffer(void)
{
   BD3D::mpDev->SetIndices(mpIB);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::drawSection
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::drawSection(uint sectionIndex)
{
   BUGXGeomSection& section = mSections[sectionIndex];
   
   section.setVertexDecl();
   section.draw(mpVB);
}
               
//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::clear
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::clear(void)
{
   if (mpGeom)
   {
      delete mpGeom;
      mpGeom = NULL;
   }
   
   BD3D::safeRelease(mpVB);
   BD3D::safeRelease(mpIB);
   
   mSections.resize(0);
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::init
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::init(void)
{
   createIB();
   createVB();
   
   mSections.resize(0);
   for (int sectionIndex = 0; sectionIndex < mpGeom->numSections(); sectionIndex++)
      mSections.push_back(BUGXGeomSection(*mpGeom, mpGeom->section(sectionIndex)));
   
   for (int sectionIndex = 0; sectionIndex < mpGeom->numSections(); sectionIndex++)
      mSections[sectionIndex].createVertexDecls();
      
   mHier.resize(mpGeom->numBones());
   mTHier.resize(3 * mpGeom->numBones());
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::createIB
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::createIB(void)
{
   BD3D::mpDev->SetIndices(NULL);
   BD3D::safeRelease(mpIB);
   
   const int IBSize = mpGeom->IB().size() * sizeof(mpGeom->IB()[0]);
   if (IBSize)
   {
      BCOMPILETIMEASSERT(sizeof(UGXGeom::Geom::IndexType) == sizeof(WORD));

      const D3DFORMAT IndexFormat = D3DFMT_INDEX16;
      
      BD3D::checkHResult(BD3D::mpDev->CreateIndexBuffer(
         IBSize,
         D3DUSAGE_WRITEONLY, 
         IndexFormat, 
         D3DPOOL_DEFAULT,
         &mpIB, 
         NULL));

      BYTE* pDst;
      mpIB->Lock(0, 0, reinterpret_cast<void**>(&pDst), 0);

      const BYTE* pSrc = reinterpret_cast<const BYTE*>(&mpGeom->IB()[0]);

      memcpy(pDst, pSrc, IBSize);

      mpIB->Unlock();
   }         
}

//------------------------------------------------------------------------------------------------------------------------------
// BUGXGeomRenderBase::createVB
//------------------------------------------------------------------------------------------------------------------------------
void BUGXGeomRenderBase::createVB(void)
{
   BD3D::mpDev->SetStreamSource(0, NULL, 0, 0);
   BD3D::mpDev->SetStreamSource(1, NULL, 0, 0);
   BD3D::safeRelease(mpVB);

   const int VBSize = mpGeom->VB().size();
   if (VBSize)
   {
      BD3D::checkHResult(BD3D::mpDev->CreateVertexBuffer(
         VBSize, 
         D3DUSAGE_WRITEONLY, 
         0, 
         D3DPOOL_DEFAULT,
         &mpVB,
         NULL));

      BYTE* pDst;
      mpVB->Lock(0, 0, reinterpret_cast<void**>(&pDst), 0);

      const BYTE* pSrc = reinterpret_cast<const BYTE*>(&mpGeom->VB()[0]);

      memcpy(pDst, pSrc, VBSize);

      mpVB->Unlock();
   }
}
