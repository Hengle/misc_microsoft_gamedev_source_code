//==============================================================================
// tilecoordinates.h
//
// Copyright (c) 1997-2003, Ensemble Studios
//==============================================================================

#pragma once
#ifndef _TILECOORDINATES_H_
#define _TILECOORDINATES_H_

//=============================================================================
// class BTileCoordinates
//=============================================================================
class BTileCoordinates
{
   public:
                              BTileCoordinates(long nx=-1, long nz=-1) {x=nx; z=nz;}
      void                    set(long nx=-1, long nz=-1) {x=nx; z=nz;}


      long x, z;

      long operator==(const BTileCoordinates &tc2) const {return( (x==tc2.x) && (z==tc2.z) );}
};
typedef BDynamicArray<BTileCoordinates> BTileCoordinatesArray;
typedef BDynamicArray<BTileCoordinates> BSimpleTileCoordinateArray;

#endif


//=============================================================================
// eof: tilecoordinates.h
//=============================================================================
