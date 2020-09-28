//==============================================================================
// tilespan.h
//
// Copyright (c) 2000-2007, Ensemble Studios
//==============================================================================
#pragma once

#include "memory\alignedAlloc.h"

//==============================================================================
// BTileSpan
//==============================================================================
class BTileSpan
{
   public:

      //Constructors and Destructor.
      BTileSpan( long x, long z, long n);
      BTileSpan( void );
      BTileSpan( const BTileSpan &s ) { *this = s; }
      ~BTileSpan( void );

      //Gets/Sets.
      long                    getStartX( void ) const { return(mStartX); }
      void                    setStartX( long v ) { BDEBUG_ASSERT((v>=SHRT_MIN) && (v<=SHRT_MAX)); mStartX=static_cast<short>(v); }
      long                    getStartZ( void ) const { return(mStartZ); }
      void                    setStartZ( long v ) { BDEBUG_ASSERT((v>=SHRT_MIN) && (v<=SHRT_MAX)); mStartZ=static_cast<short>(v); }
      long                    getNumberTiles( void ) const { return(mNumberTiles); }
      void                    setNumberTiles( long v );

      //Operators.
      bool                    operator==( const BTileSpan& t ) const;
      BTileSpan&                  operator=( const BTileSpan& t);

      //Logical methods.
      void                    reset( void );

   protected:
      short                   mStartX;
      short                   mStartZ;
      short                   mNumberTiles;
      short                   mUnused;

};

//==============================================================================
// BTileSpan::BTileSpan
//==============================================================================
BTileSpan::BTileSpan( long x, long z, long n) :
   mStartX(static_cast<short>(x)), 
   mStartZ(static_cast<short>(z)), 
   mNumberTiles(static_cast<short>(n)),
   mUnused(0)
{ 
   BDEBUG_ASSERT((x>=SHRT_MIN) && (x<=SHRT_MAX));
   BDEBUG_ASSERT((z>=SHRT_MIN) && (z<=SHRT_MAX));
   BDEBUG_ASSERT((n>=SHRT_MIN) && (n<=SHRT_MAX));
};

BTileSpan::BTileSpan( void ) : 
   mStartX(0), 
   mStartZ(0), 
   mNumberTiles(0),
   mUnused(0)
{ 
};

//==============================================================================
// BTileSpan::~BTileSpan
//==============================================================================
BTileSpan::~BTileSpan(void)
{
};

//==============================================================================
// BTileSpan::operator==
//==============================================================================
bool BTileSpan::operator==( const BTileSpan& t ) const 
{ 
   if ((mStartX == t.mStartX) && (mStartZ == t.mStartZ) && (mNumberTiles == t.mNumberTiles)) 
      return(true); 
   return(false); 
};

//==============================================================================
// BTileSpan::operator=
//==============================================================================
BTileSpan& BTileSpan::operator=( const BTileSpan& t) 
{ 
   if (this == &t)
   {
      // if you are hitting this, you are trying to copy to yourself!!
      BASSERT(0);
      return(*this);
   }

   reset();

   mStartX=t.mStartX; 
   mStartZ=t.mStartZ; 
   mNumberTiles=t.mNumberTiles; 

   return(*this); 
};

//==============================================================================
// BTileSpan::reset
//==============================================================================
void BTileSpan::reset( void )
{
   mStartX=mStartZ=mNumberTiles=0;
};

//==============================================================================
// BTileSpan::setNumberTiles
//==============================================================================
void BTileSpan::setNumberTiles( long v ) 
{ 
   BDEBUG_ASSERT((v>=SHRT_MIN) && (v<=SHRT_MAX));
   mNumberTiles=static_cast<short>(v); 
};