//==============================================================================
// havokarchive.cpp
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "havokarchive.h"


#pragma push_macro("new")
#undef new
//==============================================================================
// Defines


//==============================================================================
// BHavokArchive::BHavokArchive
//==============================================================================
BHavokArchive::BHavokArchive(hkStreamReader *pStreambuf)  
{
   mpArchive = new hkIArchive(pStreambuf);
   BASSERTM(mpArchive, "New failed.");
}

//==============================================================================
// BHavokArchive::BHavokArchive
//==============================================================================
BHavokArchive::BHavokArchive(hkIArchive *pArchive) : mpArchive(pArchive)   
{
}

//==============================================================================
// BHavokArchive::~BHavokArchive
//==============================================================================
BHavokArchive::~BHavokArchive(void)
{
   if (mpArchive)
   {
      mpArchive->getStreamReader()->removeReference();
      if(mpArchive)
         delete mpArchive;
      mpArchive = NULL;
   }
 
} 

//==============================================================================
// BHavokArchive::getArchive
//==============================================================================
hkIArchive* BHavokArchive::getArchive( void )
{  
   return mpArchive; 
}


//==============================================================================
// BHavokArchive::getStreamBuffer
//==============================================================================
hkStreamReader* BHavokArchive::getStreamBuffer( void )
{
   if (mpArchive)
      return (NULL);

   return mpArchive->getStreamReader();
}

//==============================================================================
// BHavokArchive::isOK
//==============================================================================
bool BHavokArchive::isOK( void )                                                
{
   if (!mpArchive)
      return (false);

   return mpArchive->isOk();
}

//==============================================================================
// BHavokArchive::
//==============================================================================


#pragma pop_macro("new")


//==============================================================================
// eof: havokarchive.cpp
//==============================================================================