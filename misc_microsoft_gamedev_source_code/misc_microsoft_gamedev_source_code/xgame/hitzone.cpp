//==============================================================================
// hitzone.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "hitzone.h"
#include "gamefilemacros.h"

//==============================================================================
// BHitZone::BHitZone
//==============================================================================
BHitZone::BHitZone() :
   mAttachmentHandle( -1 ),
   mHitpoints( -1.0f ),
   mShieldpoints( -1.0f ),
   mFlags( 0 )
{
   mAttachmentName.format( "" );
}

//==============================================================================
// BHitZone::BHitZone operator =
//==============================================================================
BHitZone& BHitZone::operator = ( const BHitZone& cSrcHZ )
{
   mAttachmentName   = ( (BHitZone)cSrcHZ ).getAttachmentName();
   mAttachmentHandle = ( (BHitZone)cSrcHZ ).getAttachmentHandle();
   mHitpoints        = ( (BHitZone)cSrcHZ ).getHitpoints();
   mShieldpoints     = ( (BHitZone)cSrcHZ ).getShieldpoints();
   mFlags            = ( (BHitZone)cSrcHZ ).getFlags();
   
   return( *this );
}

//==============================================================================
// BHitZone::operator ==
//==============================================================================
bool BHitZone::operator == ( const BHitZone& cSrcHZ ) const
{
   if( mAttachmentName != ( (BHitZone)cSrcHZ ).getAttachmentName() )
   {
      return( false );
   }

   if( mAttachmentHandle != ( (BHitZone)cSrcHZ ).getAttachmentHandle() )
   {
      return( false );
   }

   if( mHitpoints != ( (BHitZone)cSrcHZ ).getHitpoints() )
   {
      return( false );
   }

   if( mShieldpoints != ( (BHitZone)cSrcHZ ).getShieldpoints() )
   {
      return( false );
   }

   if( mFlags != ( (BHitZone)cSrcHZ ).getFlags() )
   {
      return( false );
   }

   return( true );
}

//==============================================================================
//==============================================================================
bool BHitZone::save(BStream* pStream, int saveType) const
{
   GFWRITESTRING(pStream, BSimString, mAttachmentName, 100);
   //long mAttachmentHandle;
   GFWRITEVAR(pStream, float, mHitpoints);
   GFWRITEVAR(pStream, float, mShieldpoints);
   GFWRITEVAR(pStream, DWORD, mFlags);
   return true;
}

//==============================================================================
//==============================================================================
bool BHitZone::load(BStream* pStream, int saveType)
{
   GFREADSTRING(pStream, BSimString, mAttachmentName, 100);
   //long mAttachmentHandle;
   GFREADVAR(pStream, float, mHitpoints);
   GFREADVAR(pStream, float, mShieldpoints);
   GFREADVAR(pStream, DWORD, mFlags);
   return true;
}
