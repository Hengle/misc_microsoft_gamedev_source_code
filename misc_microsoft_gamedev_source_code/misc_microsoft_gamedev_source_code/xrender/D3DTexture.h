//============================================================================
//
//  File: D3DTexture.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

enum 
{ 
   cBD3DTextureIdentifier           = 0x70000000, 
   cBD3DTextureIdentifierMask       = 0xF0000000, 
   cBD3DTextureAllocatorIndexShift  = 24,
   cBD3DTextureAllocatorIndexMask   = 0x03
};

enum eDefaultTexture
{
   cDefaultTextureInvalid = -1,

   cDefaultTextureWhite = 0,
   cDefaultTextureRed,
   cDefaultTextureGreen,
   cDefaultTextureBlue,
   cDefaultTextureBlack,
   cDefaultTextureNormal,
   cDefaultTextureCheckerboard,
   cDefaultTextureTransparent,

   cDefaultTextureMax
};

enum BD3DTextureType
{
   cTTInvalid = -1,
   
   cTT2D      = 0,
   cTTCubemap = 1,
   cTTVolume  = 2,
   cTTArray   = 3,

   cTTNumTypes,

   cTTTypeForceDWORD = 0xFFFFFFFF
};

// This class must be bitwise copyable, so it can be placed directly into the command buffer.
// This class does not automatically free the D3D resource - release() must be called by the owner.
class BD3DTexture
{
public:
   BD3DTexture()
   {
      clear();
   }

   BD3DTextureType getType(void) const { return static_cast<BD3DTextureType>(mType); }
   
   enum eAllocator
   {
      cD3D,
      cXboxTextureHeap,
      cPackedTextureManager,
      
      cNumAllocators
   };
   
   bool getD3DCreated(void) const { return mAllocator == cD3D; }
   
   eAllocator getAllocator() { return static_cast<eAllocator>(mAllocator); }
   void setAllocator(eAllocator alloc) { mAllocator = alloc; updateIdentifier(); }
   
   // Associates a 24-bit value with the texture.
   void setIdentifier(uint handle);
   uint getIdentifier() const;
      
   void clear(void)
   {
      mType = 0;
      mpD3DInterface = NULL;
      mAllocator = cD3D;
   }
      
   // Get a base interface.
   IDirect3DBaseTexture9* getBaseTexture(void) const { return mpD3DInterface; }

   // Get as normal texture.
   IDirect3DTexture9* getTexture(void) const
   {
      BDEBUG_ASSERTM(mType==cTT2D, "Wrong texture type."); 
      return reinterpret_cast<IDirect3DTexture9*>(getBaseTexture());
   }

   // Get as cube texture.
   IDirect3DCubeTexture9* getCubeTexture(void) const
   {
      BDEBUG_ASSERTM(mType==cTTCubemap, "Wrong texture type."); 
      return reinterpret_cast<IDirect3DCubeTexture9*>(getBaseTexture());
   }

   // Get as volume texture.
   IDirect3DVolumeTexture9* getVolumeTexture(void) const
   {
      BDEBUG_ASSERTM(mType==cTTVolume, "Wrong texture type."); 
      return reinterpret_cast<IDirect3DVolumeTexture9*>(getBaseTexture());
   }
   
   // Get as array texture.
   IDirect3DArrayTexture9* getArrayTexture(void) const
   {
      BDEBUG_ASSERTM(mType==cTTArray, "Wrong texture type."); 
      return reinterpret_cast<IDirect3DArrayTexture9*>(getBaseTexture());
   }

   // Set as normal texture,
   void setTexture(IDirect3DTexture9* texture, eAllocator allocator)  
   {
      setD3DPtr(texture); 
      mType = cTT2D;
      mAllocator = allocator;
      updateIdentifier(); 
   }

   // Set as cube texture,
   void setCubeTexture(IDirect3DCubeTexture9* texture, eAllocator allocator) 
   {
      setD3DPtr(texture); 
      mType = cTTCubemap;
      mAllocator = allocator;
      updateIdentifier(); 
   }

   // Set as volume texture,
   void setVolumeTexture(IDirect3DVolumeTexture9* texture, eAllocator allocator) 
   {
      setD3DPtr(texture); 
      mType = cTTVolume;
      mAllocator = allocator;
      updateIdentifier(); 
   }
   
   // Set as array texture,
   void setArrayTexture(IDirect3DArrayTexture9* texture, eAllocator allocator) 
   {
      setD3DPtr(texture); 
      mType = cTTArray;
      mAllocator = allocator;
      updateIdentifier(); 
   }
      
   // Deferred release. 
   // If the resource is owned by D3D, Release() will be called on the worker thread, otherwise it will be deleted using the proper manager.
   void release(void);
   
   // Immediate release.
   void releaseImmediate(void);
   
   // Call this API to release a "wild" D3D texture pointer that originated from a BD3DTexture.
   // It will deallocate the texture from the proper heaps. 
   // It uses the GetIdentifier() API to make this work.
   // Render thread only
   static void releaseWildTexture(IDirect3DBaseTexture9* pTex);

protected:
   IDirect3DBaseTexture9*  mpD3DInterface;
   DWORD                   mType : 2;
   DWORD                   mAllocator : 2;
               
   void setD3DPtr(IDirect3DBaseTexture9* p) { mpD3DInterface = p; }
   void updateIdentifier(void); 
};
