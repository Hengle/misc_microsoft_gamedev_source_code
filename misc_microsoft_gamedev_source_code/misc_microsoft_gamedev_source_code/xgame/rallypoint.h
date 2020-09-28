//==============================================================================
// rallypoint.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once


//==============================================================================
// class BRallyPoint
// Holds a rally point position and entity ID.  Used to save space in BUnit.
//==============================================================================
class BRallyPoint
{
public:
   void init()
   {
      x = 0.0f;
      y = 0.0f;
      z = 0.0f;
      mEntityID = cInvalidObjectID;
   }

   float x;
   float y;
   float z;
   BEntityID mEntityID;
};
