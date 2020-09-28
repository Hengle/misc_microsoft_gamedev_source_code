//==============================================================================
// HumanPlayerAITrackingData.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "humanPlayerAITrackingData.h"
#include "userprofilemanager.h"
#include "xbox.h"
#include "user.h"

// compression
#include "stream\dynamicStream.h"
#include "compressedStream.h"

//==============================================================================
//
//==============================================================================
BHumanPlayerAITrackingData::BHumanPlayerAITrackingData() :
mDirty(false),
mIsLoaded(false)
{
   setDefaultvalues();
};

//==============================================================================
// 
//==============================================================================
BHumanPlayerAITrackingData::~BHumanPlayerAITrackingData() 
{

};

//==============================================================================
// Set the managed variables to a default value
//==============================================================================
void BHumanPlayerAITrackingData::setDefaultvalues()
{
   mAutoDifficultyLevel = 40;
   mAutoDifficultyNumGamesPlayed = 0;
   mConceptIDs.clear();
   mConceptTimesReinforced.clear();
}

//==============================================================================
// Given a block of memory where the first 4 bytes of it are the size of that block
//  This will read that in, and deserialize the tracked variables from it
//==============================================================================
bool BHumanPlayerAITrackingData::loadValuesFromMemoryBlock(byte* pMemoryBlock, bool localData)
{
   if(pMemoryBlock == NULL)
      return(false);

   setDefaultvalues();
   mAIMemoryVersion = 5;
   mLocalData = localData;

   BDynamicStream* pSettingStream = new BDynamicStream();
   uint readbytes = *(reinterpret_cast<uint*>(pMemoryBlock));
   const uint* pReadDataPtr = reinterpret_cast<const uint*>(pMemoryBlock+4);

   // [5/2/2008 JRuediger] Bail on bad params.  Revert back to defaults.
   if(readbytes <= 0 || readbytes > (XPROFILE_SETTING_MAX_SIZE-sizeof(readbytes)) || pSettingStream == NULL)
   {
      delete pSettingStream;
      return(true);
   }

   // [5/1/2008 JRuediger] Read in the compressed data.
   pSettingStream->writeBytes(pReadDataPtr, readbytes);
   pSettingStream->seek(0);

   // [5/1/2008 JRuediger] Decompress it.  This can fail if we have never saved the profile because the header we read in will be malformed.
   BInflateStream* pInfStream = new BInflateStream(*pSettingStream);
   if(pInfStream == NULL)
   {
      delete pInfStream;
      delete pSettingStream;
      return(true);
   }

   if(pInfStream->errorStatus() == true)
   {
      // [5/1/2008 JRuediger] First time we have run the game, so set default values.
      pInfStream->close();
      delete pInfStream;
      delete pSettingStream;
      return(true);
   }

   // mrh - Version up to 5 -- reset older versions because we're nuking a lot of crap we don't have time to support anymore.
   *pInfStream >> mAIMemoryVersion;
   if (mAIMemoryVersion >= 5)
   {
      *pInfStream >> mAutoDifficultyLevel;
      *pInfStream >> mAutoDifficultyNumGamesPlayed;

      unsigned char numConcepts = 0;
      *pInfStream >> numConcepts;

      unsigned char value;
      for (unsigned char i=0; i<numConcepts; i++)
      {
         *pInfStream >> value;
         mConceptIDs.add(value);
      }
      for (unsigned char i=0; i<numConcepts; i++)
      {
         *pInfStream >> value;
         mConceptTimesReinforced.add(value);
      }
   }

   bool success = !pInfStream->errorStatus();
   pInfStream->close();
   delete pInfStream;
   delete pSettingStream;

   mIsLoaded = true;

   return(success);
}

//==============================================================================
// Given a stream- this will deserialize the tracking variables out to it
//==============================================================================
bool BHumanPlayerAITrackingData::saveValuesToMemoryBlock(BDynamicStream* streamPtr, bool forceWrite)
{
   if (!forceWrite)
   {
      BASSERT(mLocalData);

      if (!mLocalData)
         return(false);
   }

   mAIMemoryVersion = 5;

   // Write the AI auto difficulty stuff
   if (streamPtr->writeObj(mAIMemoryVersion) == false)
      return (false);
   if (streamPtr->writeObj(mAutoDifficultyLevel) == false)
      return (false);
   if (streamPtr->writeObj(mAutoDifficultyNumGamesPlayed) == false)
      return (false);

   //Write the hint data
   unsigned char numConcepts = (unsigned char)mConceptIDs.size();
   streamPtr->writeObj(numConcepts); 
   for(unsigned char i=0; i<numConcepts; i++)
   {
      streamPtr->writeObj(mConceptIDs.get(i));
   }
   for(unsigned char i=0; i<numConcepts; i++)
   {
      streamPtr->writeObj(mConceptTimesReinforced.get(i));
   }

   mDirty = false;
   return true;
}