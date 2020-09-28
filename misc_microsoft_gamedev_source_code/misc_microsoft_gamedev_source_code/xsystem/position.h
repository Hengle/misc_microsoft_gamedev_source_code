//==============================================================================
// position.h
//
// Copyright (c) 1998-2000 Ensemble Studios
//==============================================================================

#ifndef _POSITION_H_
#define _POSITION_H_

//==============================================================================
// Includes
#include "poolable.h"

//==============================================================================
// Forward declarations
class BChunkWriter;
class BChunkReader;

//==============================================================================
// BPosition
class BPosition : public IPoolable
{

   public:
      BPosition( void ) :
         mPosition(0.0f),
         mForward(0.0f),
         mUp(0.0f),
         mRight(0.0f) { }
      BPosition( BVector& p, BVector& f, BVector& u, BVector& r ) :
         mPosition(p),
         mForward(f),
         mUp(u),
         mRight(r) { }
      BPosition( BMatrix& matrix )
      {
         matrix.getTranslation(mPosition);
         matrix.getForward(mForward);
         matrix.getRight(mRight);
         matrix.getUp(mUp);
      }

         ~BPosition( void ) {}


      long                    operator==(const BPosition &v) const
      {
         return( mPosition==v.mPosition && mForward==v.mForward && mUp==v.mUp && mRight==v.mRight);
      }
     
      long                    operator!=(const BPosition &v) const
      {
         return( mPosition!=v.mPosition || mForward!=v.mForward || mUp!=v.mUp || mRight!=v.mRight);
      }

      BVector                 mPosition;
      BVector                 mForward;
      BVector                 mUp;
      BVector                 mRight;

      void                    yaw(float amount);
      void                    pitch(float amount);
      void                    roll(float amount);

      virtual void            onAcquire();
      virtual void            onRelease();

      //Save/load.
      static bool             writeVersion(BChunkWriter *chunkWriter);
      static bool             readVersion(BChunkReader *chunkReader);
      bool                    save(BChunkWriter *chunkWriter);
      bool                    load(BChunkReader *chunkReader);

      DECLARE_FREELIST(BPosition, 8);
   protected:
      //Static savegame stuff.
      static const DWORD      msSaveVersion;
      static DWORD            msLoadVersion;
}; // BPosition


//==============================================================================
#endif // _POSITION_H_

//==============================================================================
// eof: position.h
//==============================================================================
