//==============================================================================
// gamefileheader.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Forward declarations
class BGameSettings;

//==============================================================================
// BGameFileHeader
//==============================================================================
class BGameFileHeader
{
   public:
      static const uint8 cHeaderVersion = 2;

      enum
      {
         cMaxNameSize = 32,
         cMaxNameLen = cMaxNameSize - 1,
         cMaxDescSize = 128,
         cMaxDescLen = cMaxDescSize - 1,
         // if the sizes change, then you need to change the header version and
         // account for the smaller size in the (de)serialization
      };

      BGameFileHeader();

      bool init(BGameSettings* pSettings);
      bool derive(BGameSettings* pSettings); // derive a header based on the game settings, backwards compatibility
      void reset();

      bool serialize(BStream* pStream, BSHA1Gen& sha1Gen);
      bool deserialize(BStream* pStream, BSHA1Gen& sha1Gen);

      template<typename StringType>
      bool serialize(BStream* pStream, StringType& string, int maxLen, BSHA1Gen& sha1Gen) const;

      template<typename StringType>
      bool deserialize(BStream* pStream, StringType& string, int maxLen, BSHA1Gen& sha1Gen);

      void setDate(const SYSTEMTIME& date) { mDate = date; }
      void updateLength(float length) { mLength += length; }

      uint64 getKey3() const { return mKey3; }

      void setName(const BUString& name) { mName = name; }
      void setDesc(const BUString& desc) { mDesc = desc; }
      void setAuthor(const BString& author) { mAuthor = author; }
      void setID(uint64 id) { mID = id; }
      void setXuid(XUID xuid) { mXuid = xuid; }
      void setLength(float length) { mLength = length; }
      void setSessionID(uint16 id) { mSessionID = id; }

      // I want to serialize this data out to the top of the record game
      // and then read it in when I generate the manifest
      //
      // I require a fixed size since the data resides at the beginning of the file
      // and will be modifiable
      BSHA1       mHeaderHash; // hash of the header & version DWORD that precedes the header, hopefully to prevent tampering with encrypt/compress bits
      BSHA1       mDataHash; // hash of the record game data, used to verify the validity of the recording
      BUString    mName;
      BUString    mDesc;
      BString     mAuthor;
      SYSTEMTIME  mDate;
      XUID        mXuid; // xuid of the author
      uint64      mID; // unique id for this recording
      uint64      mKey3; // random value used as the third key in the encrypt/decrypt ciper for the data payload
      uint64      mDataSize; // the size of the data payload (after compression/encryption), used to perform a quick verification of file integrity
      float       mLength; // length of the in milliseconds
      int32       mGameType;

      // this ID does not get serialized out
      LONG        mTempID; // temporary ID assigned when manifest is created, used to pick a specific recording for playback
                           // it's possible to have two identical recordings if a user copied the game file, but this ID will be different

      uint16      mSessionID; // Campaign mission session ID. Used to tie the last mission played to the save game file. The last mission is stored in the profile.

      uint8       mVersion; // version of this header
      // game type and map name will come from the game settings
};

