//============================================================================
//
//  BGameStrings.h
//
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================

#pragma once 

//----------------------------------------------------------------------------
//  Class BGameStrings
//----------------------------------------------------------------------------
class BGameStrings
{

public:
   //-- Construction/Destruction
   BGameStrings();
   ~BGameStrings();

   //-- String accessor
   const BSimString& getString  (long stringID); 
   bool hasString(long stringID);

   //-- String table methods
   bool loadStringFile(long dirID, const BSimString &filename);


   //-- Property methods
   const BSimString& getFilename() { return mFilename; };
   const BSimString& getLanguage() { return mLanguage; };
   long  getNumber() { return mStrings.getNumber(); }
   const BDynamicSimArray<BSimString>& getStringArray() { return mStrings; }

private:
   //-- Private Data
   BSimString mFilename;
   BSimString mLanguage;


   //-- String table array.
   BDynamicSimArray<BSimString> mStrings;     // actual strings
   BDynamicSimLongArray      mIndexTable;  // list of indexes (should be optimized)


};


