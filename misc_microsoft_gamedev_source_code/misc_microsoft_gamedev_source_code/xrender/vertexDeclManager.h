//============================================================================
//
//  vertexDeclManager.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include "math/generalVector.h"
#include "univertPacker.h"

#include "BD3D.h"
   
typedef BDynamicArray<BUnpackedUnivertPackerType, ALIGN_OF(BUnpackedUnivertPackerType), BDynamicArrayRenderHeapAllocator> BUnivertPackerArray;
      
class BVertexDecl
{
public:
   enum { MaxStreams = 8 };
                           
   BVertexDecl();
   BVertexDecl(const BUnivertPackerArray& packers);
   BVertexDecl(const BVertexDecl& rhs);
      
   virtual ~BVertexDecl();
                        
   BVertexDecl& operator= (const BVertexDecl& rhs);
   
   bool operator== (const BVertexDecl& rhs) const;
   bool operator== (const BUnivertPackerArray& packers) const;
      
   IDirect3DVertexDeclaration9* getDecl(void) const;
      
   void setToDevice(IDirect3DDevice9* pDev) const;
                           
private:
   IDirect3DVertexDeclaration9* mpDecl;
   BUnivertPackerArray mUnivertPackers;
   
   void clear(void);
      
   void create(void);
         
   static DWORD convertType(VertexElement::EType type);
}; // class BVertexDecl

class BVertexDeclManager
{
public:
   BVertexDeclManager();
   virtual ~BVertexDeclManager();

   void clear(void);
                        
   typedef int Handle;     
   const Handle create(const BUnivertPackerArray& packers);
   void setToDevice(Handle handle, IDirect3DDevice9* pDev) const;
   
   const BVertexDecl& get(Handle handle) const { return mVertexDeclCache[handle]; }
                        
private:
   BDynamicArray<BVertexDecl, ALIGN_OF(BVertexDecl), BDynamicArrayRenderHeapAllocator> mVertexDeclCache;
            
   BVertexDeclManager(const BVertexDeclManager&);
   BVertexDeclManager& operator= (const BVertexDeclManager&);
}; // class BVertexDeclManager

extern BVertexDeclManager gVertexDeclManager;
