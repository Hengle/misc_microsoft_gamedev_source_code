//==============================================================================
// gamefileheader.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "gamefileheader.h"
#include "database.h"
#include "gamesettings.h"
#include "ScenarioList.h"
#include "user.h"
#include "usermanager.h"

//==============================================================================
// 
//==============================================================================
BGameFileHeader::BGameFileHeader() :
   mXuid(0),
   mID(0),
   mLength(0.0f),
   mSessionID(0),
   mVersion(cHeaderVersion)
{
   Utils::FastMemSet(&mDate, 0, sizeof(mDate));

   XNetRandom((BYTE*)&mKey3, sizeof(mKey3));
}

//==============================================================================
// 
//==============================================================================
bool BGameFileHeader::init(BGameSettings* pSettings)
{
   if (!derive(pSettings))
      return false;

   // grab the primary user
   // doesn't matter whether they're signed into Live, Local, not at all
   // we still need a BUser to grab basic settings
//-- FIXING PREFIX BUG ID 2227
   const BUser* pUser = gUserManager.getPrimaryUser();
//--
   if (!pUser)
      return false;

   GetSystemTime(&mDate);

   mAuthor = pUser->getName();

   mXuid = pUser->getXuid();

   mLength = 0.0f;
   mSessionID = 0;

   mVersion = cHeaderVersion;

   return true;
}

//==============================================================================
// For older record games, we're going to derive a header
//==============================================================================
bool BGameFileHeader::derive(BGameSettings* pSettings)
{
   // create a temporary name/desc based on the game settings from
   // the existing recording
   BFixedStringMaxPath mapName;
   pSettings->getString(BGameSettings::cMapName, mapName, MAX_PATH);

   // look up the scenario info, if not found, stick with the mapName
   // otherwise, lookup the NameStringID and get the fancy description
   const BScenarioList& scenarioList = gDatabase.getScenarioList();

   long stringID = 0;

   const BScenarioMap* pMap = scenarioList.getMapInfo(mapName);
   if (pMap)
      stringID = pMap->getMapNameStringID();

   if (stringID > 0)
   {
      // lookup the localized name
      mName = gDatabase.getLocStringFromID(stringID);
   }
   else
   {
      mName = mapName;
   }

   BFixedString<256> fullBuf;

   // FIXME DPM 10/16/07 dump the player names into the desc for now
   long gameType = 0;
   pSettings->getLong(BGameSettings::cGameType, gameType);
   switch (gameType)
   {
      case BGameSettings::cGameTypeSkirmish:
         {
            fullBuf.formatAppend("Skirmish - ");
            break;
         }
      case BGameSettings::cGameTypeCampaign:
         {
            fullBuf.formatAppend("Campaign - ");
            break;
         }
      case BGameSettings::cGameTypeScenario:
         {
            fullBuf.formatAppend("Scenario - ");
            break;
         }
      default:
         {
            fullBuf.formatAppend("- ");
            break;
         }
   }

   mGameType = gameType;

   //fullBuf.formatAppend("%04d/%02d/%02d %02d:%02d:%02d - ", mDate.wYear, mDate.wMonth, mDate.wDay, mDate.wHour, mDate.wMinute, mDate.wSecond);

   const char* pFmt1 = "%s, ";
   const char* pFmt2 = "%s";
   long playerCount = 0;
   pSettings->getLong(BGameSettings::cPlayerCount, playerCount);
   for (long i=1; i <= playerCount; ++i)
   {
      BString playerName;
      if (pSettings->getString(PSINDEX(i, BGameSettings::cPlayerName), playerName))
         fullBuf.formatAppend((i == playerCount ? pFmt2 : pFmt1), playerName.getPtr());
   }

   mDesc.set(fullBuf);

   mAuthor = "[Unknown]";

   mXuid = 0;

   XNetRandom((BYTE*)&mID, sizeof(mID));

   mLength = 0.0f;
   mSessionID = 0;
   mVersion = cHeaderVersion;

   return true;
}

//==============================================================================
// 
//==============================================================================
void BGameFileHeader::reset()
{
   mXuid = 0;
   mID = 0;
   mLength = 0.0f;
   mSessionID = 0;
   mVersion = cHeaderVersion;

   mName.empty();
   mDesc.empty();
   mAuthor.empty();
   Utils::FastMemSet(&mDate, 0, sizeof(mDate));
}

//==============================================================================
// 
//==============================================================================
template<typename StringType>
bool BGameFileHeader::serialize(BStream* pStream, StringType& string, int maxLen, BSHA1Gen& sha1Gen) const
{
   int32 count = maxLen+1;

   StringType::charType* pStr = new StringType::charType[count];

   pStr[0] = 0;
   pStr[maxLen] = 0;

   int32 len = Math::Min<int32>(maxLen, string.length());

   Utils::FastMemSet(pStr, 0, sizeof(StringType::charType)*count);
   Utils::FastMemCpy(pStr, string.getPtr(), len*sizeof(StringType::charType));

   if (!pStream->writeBytes(pStr, sizeof(StringType::charType)*count))
   {
      delete[] pStr;
      return false;
   }

   sha1Gen.update(pStr, count*sizeof(StringType::charType));

   delete[] pStr;

   return true;
}

//==============================================================================
// 
//==============================================================================
template<typename StringType>
bool BGameFileHeader::deserialize(BStream* pStream, StringType& string, int maxLen, BSHA1Gen& sha1Gen)
{
   int32 count = maxLen+1;

   StringType::charType* pStr = new StringType::charType[count];

   pStr[0] = 0;
   pStr[maxLen] = 0;

   if (pStream->readBytes(pStr, count*sizeof(StringType::charType)) != (count*sizeof(StringType::charType)))
   {
      delete[] pStr;
      return false;
   }

   sha1Gen.update(pStr, count*sizeof(StringType::charType));

   string = pStr;

   delete[] pStr;

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFileHeader::serialize(BStream* pStream, BSHA1Gen& sha1Gen)
{
   BDEBUG_ASSERT(pStream);

   sha1Gen.update(mVersion);
   if (pStream->writeBytes(&mVersion, sizeof(mVersion)) != sizeof(mVersion))
      return false;

   sha1Gen.update64(mID);
   if (pStream->writeBytes(&mID, sizeof(mID)) != sizeof(mID))
      return false;

   if (!serialize(pStream, mName, cMaxNameLen, sha1Gen))
      return false;
   if (!serialize(pStream, mDesc, cMaxDescLen, sha1Gen))
      return false;
   if (!serialize(pStream, mAuthor, XUSER_MAX_NAME_LENGTH, sha1Gen))
      return false;

   sha1Gen.update(&mDate, sizeof(mDate));
   if (pStream->writeBytes(&mDate, sizeof(mDate)) != sizeof(mDate))
      return false;

   sha1Gen.update64(mXuid);
   if (pStream->writeBytes(&mXuid, sizeof(mXuid)) != sizeof(mXuid))
      return false;

   sha1Gen.update(&mLength, sizeof(float));
   if (pStream->writeBytes(&mLength, sizeof(mLength)) != sizeof(mLength))
      return false;

   sha1Gen.update(&mSessionID, sizeof(uint16));
   if (pStream->writeBytes(&mSessionID, sizeof(mSessionID)) != sizeof(mSessionID))
      return false;

   sha1Gen.update(&mGameType, sizeof(int32));
   if (pStream->writeBytes(&mGameType, sizeof(mGameType)) != sizeof(mGameType))
      return false;

   sha1Gen.update64(mKey3);
   if (pStream->writeBytes(&mKey3, sizeof(mKey3)) != sizeof(mKey3))
      return false;

   // include the data hash in the header hash
   sha1Gen.update(mDataHash);
   if (pStream->writeBytes(&mDataHash, sizeof(BSHA1)) != sizeof(BSHA1))
      return false;

   sha1Gen.update64(mDataSize);
   if (pStream->writeBytes(&mDataSize, sizeof(mDataSize)) != sizeof(mDataSize))
      return false;

   mHeaderHash = sha1Gen.finalize();

   if (pStream->writeBytes(&mHeaderHash, sizeof(BSHA1)) != sizeof(BSHA1))
      return false;

   return true;
}

//==============================================================================
// 
//==============================================================================
bool BGameFileHeader::deserialize(BStream* pStream, BSHA1Gen& sha1Gen)
{
   // assume the stream is pointing to the beginning of the header
   // read the version number and compare to our current known value
   BDEBUG_ASSERT(pStream);

   if (pStream->readBytes(&mVersion, sizeof(mVersion)) != sizeof(mVersion) || mVersion > cHeaderVersion)
      return false;

   sha1Gen.update(mVersion);

   if (pStream->readBytes(&mID, sizeof(mID)) != sizeof(mID))
      return false;

   sha1Gen.update64(mID);

   if (!deserialize(pStream, mName, cMaxNameLen, sha1Gen))
      return false;
   if (!deserialize(pStream, mDesc, cMaxDescLen, sha1Gen))
      return false;
   if (!deserialize(pStream, mAuthor, XUSER_MAX_NAME_LENGTH, sha1Gen))
      return false;

   if (pStream->readBytes(&mDate, sizeof(mDate)) != sizeof(mDate))
      return false;

   sha1Gen.update(&mDate, sizeof(mDate));

   if (pStream->readBytes(&mXuid, sizeof(mXuid)) != sizeof(mXuid))
      return false;

   sha1Gen.update64(mXuid);

   if (pStream->readBytes(&mLength, sizeof(mLength)) != sizeof(mLength))
      return false;

   sha1Gen.update(&mLength, sizeof(int32));

   if (mVersion >= 2)
   {
      if (pStream->readBytes(&mSessionID, sizeof(mSessionID)) != sizeof(mSessionID))
         return false;

      sha1Gen.update(&mSessionID, sizeof(uint16));
   }

   if (pStream->readBytes(&mGameType, sizeof(mGameType)) != sizeof(mGameType))
      return false;

   sha1Gen.update(&mGameType, sizeof(float));

   if (pStream->readBytes(&mKey3, sizeof(mKey3)) != sizeof(mKey3))
      return false;

   sha1Gen.update64(mKey3);

   if (pStream->readBytes(&mDataHash, sizeof(BSHA1)) != sizeof(BSHA1))
      return false;

   sha1Gen.update(mDataHash);

   if (pStream->readBytes(&mDataSize, sizeof(mDataSize)) != sizeof(mDataSize))
      return false;

   sha1Gen.update64(mDataSize);

   if (pStream->readBytes(&mHeaderHash, sizeof(BSHA1)) != sizeof(BSHA1))
      return false;

   BSHA1 sha1 = sha1Gen.finalize();

   if (sha1 != mHeaderHash)
      return false;

   return true;
}
