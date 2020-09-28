//==============================================================================
// hitzone.h
//
// A targetable zone on a unit defined by the geometries attachment visual.
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

class BStream;

//==============================================================================
// BHitZone
//==============================================================================
class BHitZone
{
   public:

      enum
      {
         cHitZoneActive     = ( 1 << 0 ),
         cHitZoneTargeted   = ( 1 << 1 ),
         cHitZoneHasShields = ( 1 << 2 ),
      };

      BHitZone();
      ~BHitZone(){}

      // Assignment operator.
      BHitZone& operator = ( const BHitZone& cSrcHZ );

      // Equality operator
      bool operator == ( const BHitZone& cSrcHZ ) const;      

      // Set/Get attachment name
      inline void        setAttachmentName( BSimString name ){ mAttachmentName = name; }
      inline BSimString& getAttachmentName(){ return( mAttachmentName ); }

      // Set/Get attachment handle
      inline void setAttachmentHandle( long handle ){ mAttachmentHandle = handle; }
      inline long getAttachmentHandle(){ return( mAttachmentHandle ); }

      // Set/Get hitpoints
      inline void  setHitpoints( float hp ){ mHitpoints = hp; }
      inline float getHitpoints(){ return( mHitpoints ); }

      // Set/Get shieldpoints
      inline void  setShieldpoints( float sp ){ mShieldpoints = sp; }
      inline float getShieldpoints(){ return( mShieldpoints ); }

      // Flag functions
      inline void  addFlag( long flag ){ mFlags |= flag; }
      inline void  removeFlag( long flag ){ mFlags &= ~flag; }
      inline bool  hasFlag( long flag ){ return( ( mFlags & flag ) ? true : false ); }
      inline DWORD getFlags(){ return( mFlags ); }
      inline void  setFlags(DWORD flags) { mFlags=flags; }

      // Set/Get active flag
      inline void setActive( bool active ){ mFlags = active ? ( mFlags | cHitZoneActive ) : ( mFlags & ~cHitZoneActive ); }
      inline bool getActive() const { return( ( mFlags & cHitZoneActive ) ? true : false ); }      

      // Set/Get targeted flag
      inline void setTargeted( bool targeted ){ mFlags = targeted ? ( mFlags | cHitZoneTargeted ) : ( mFlags & ~cHitZoneTargeted ); }
      inline bool getTargeted(){ return( ( mFlags & cHitZoneTargeted ) ? true : false ); }

      // Set/Get has shields flag
      inline void setHasShields( bool shields ){ mFlags = shields ? ( mFlags | cHitZoneHasShields ) : ( mFlags & ~cHitZoneHasShields ); }
      inline bool getHasShields(){ return( ( mFlags & cHitZoneHasShields ) ? true : false ); }

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:

      BSimString mAttachmentName;
      long       mAttachmentHandle;
      float      mHitpoints;
      float      mShieldpoints;
      DWORD      mFlags;
};

typedef BSmallDynamicSimArray<BHitZone> BHitZoneArray;