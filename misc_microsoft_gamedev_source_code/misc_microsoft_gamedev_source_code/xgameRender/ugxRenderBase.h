//============================================================================
//
//  ugxRenderBase.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// local
#include "vertexDeclManager.h"

// xgeom
#include "ugxGeom.h"

// rg [ 5/29/06] - None of this is currently used (and may never be used).

class BUGXGeomSection
{
public:
   BUGXGeomSection();
   BUGXGeomSection(const UGXGeom::Geom& geom, const UGXGeom::Section& section);
   BUGXGeomSection(const BUGXGeomSection& b);

   BUGXGeomSection& operator= (const BUGXGeomSection& rhs);

   void createVertexDecls(void);
   
   void setVertexDecl(void) const;

   void draw(IDirect3DVertexBuffer9* pVB) const;

   const UGXGeom::Section* getSection(void) const { return mpSection; }
   BVertexDeclManager::Handle getVertexDeclHandle(void) const { return mVertexDeclHandle; }

protected:
   const UGXGeom::Section* mpSection;
   BVertexDeclManager::Handle mVertexDeclHandle;
}; // class BUGXGeomSection

class BUGXGeomRenderBase
{
public:
   enum { cMaxLocalBones = 75 };
   
   BUGXGeomRenderBase();
   ~BUGXGeomRenderBase();
      
   // Takes ownership.
   void setGeom(UGXGeom::Geom* pGeom);
   
   // Makes copy.
   void setGeom(UGXGeom::Geom& geom);
   
   bool load(BStream& stream);
   
   const UGXGeom::Geom* getGeom(void) const { return mpGeom; }

   typedef BDynamicArray<BMatrix44, 16> BAlignedMatrixArray;
   typedef BDynamicArray<BVec4, 16> BAlignedVec4Array;

   const BAlignedMatrixArray& getHier(void) const { return mHier; }
         BAlignedMatrixArray& getHier(void)       { return mHier; }

   const BAlignedVec4Array& getTHier(void) const { return mTHier; }
         BAlignedVec4Array& getTHier(void)      { return mTHier; }

   uint getNumSections(void) const { return mSections.size(); }         

   typedef BDynamicArray<BUGXGeomSection> BUGXSectionVec;
   const BUGXSectionVec& getSections(void) const { return mSections; }

   IDirect3DVertexBuffer9* getVB(void) const { return mpVB; }
   IDirect3DIndexBuffer9* getIB(void) const { return mpIB; }

   void initHierModelToWorld(const BMatrix44& modelToWorld);
   
   void initHierAccessoryMatrices(
      uint accessoryIndex,
      const BMatrix44* pMatrices, uint numMatrices,
      const BMatrix44* pLocalMatrix);
   
   void initHier(const BMatrix44& worldToView, const ushort* pAccessoryIndices = NULL, uint numAccessoryIndices = 0);
   
   void setHierGlobalConstants(uint firstRegister, const ushort* pAccessoryIndices = NULL, uint numAccessoryIndices = 0);
   
   void setHierSectionConstants(uint firstRegister, uint sectionIndex);

   uint calcSectionsIndices(ushort* pSectionIndices, const ushort* pAccessoryIndices = NULL, uint numAccessoryIndices = 0);
   
   void setIndexBuffer(void);
         
   void drawSection(uint sectionIndex);
      
protected:
   UGXGeom::Geom* mpGeom;

   BAlignedMatrixArray mHier;
   BAlignedVec4Array mTHier;

   IDirect3DVertexBuffer9* mpVB;
   IDirect3DIndexBuffer9* mpIB;

   BUGXSectionVec mSections;   

   void clear(void);
   
   void init(void);
   
   void createIB(void);
   
   void createVB(void);
   
}; // class BUGXGeomRenderBase
