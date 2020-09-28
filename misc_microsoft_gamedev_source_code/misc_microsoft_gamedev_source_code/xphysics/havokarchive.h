//==============================================================================
// havokarchive.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================



#ifndef _HAVOK_ARCHIVE_
#define _HAVOK_ARCHIVE_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations
class hkIArchive;


//==============================================================================
// Const declarations


//==============================================================================
//==============================================================================
class BHavokArchive
{
public:

   BHavokArchive(hkStreamReader *pStreambuf); 
   BHavokArchive(hkIArchive *pArchive); 
   virtual ~BHavokArchive( void );                                
   
   hkIArchive*      getArchive( void );
   hkStreamReader*     getStreamBuffer( void );
   bool             isOK( void );                                                
  

protected:
   hkIArchive *mpArchive;
};

//==============================================================================
#endif // _HAVOK_ARCHIVE_

//==============================================================================
// eof: havokarchive.h
//==============================================================================