//=============================================================================
// scenariolist.h
//
// Copyright (c) 2007 Ensemble Studios
//=============================================================================
#pragma once

// Includes
#include "xmlreader.h"

//=============================================================================
//=============================================================================
class BScenarioMap
{
   public:
      enum
      {
         cScenarioTypeUnknown,
         cScenarioTypeFinal,
         cScenarioTypePlaytest,
         cScenarioTypeDevelopment,
         cScenarioTypeCampaign,

         cScenarioTypeCount
      };

      BScenarioMap();
      ~BScenarioMap();

      bool              load(BXMLNode node);

      long              getMapNameStringID() const { return mNameStringID; }
      const BUString&   getMapName() const;
      const BUString&   getMapDescription() const;
      int               getMaxPlayers() const { return mMaxPlayers; }
      long              getType() const { return mType; }
      const BSimString& getFilename() const { return mFilename; }
      const BSimString& getName() const { return mName; }
      const BSimString& getLoadingScreen() const { return mLoadingScreen; }
      const BSimString& getMapKeyFrame() const { return mMapKeyFrame; }

      long              getID() const { return mID; }
      void              setID(long id) { mID = id; }


   protected:
      // <ScenarioInfo NameStringID="22308" InfoStringID="22324" MaxPlayers="2" Type="Playtest" File="skirmish\design\twinpeaks_1_swi\twinpeaks_1_swi.scn" />

      BSimString  mFilename;
      BSimString  mName; // filename sans extension
      BSimString  mLoadingScreen;
      BSimString  mMapKeyFrame;
      long        mNameStringID;
      long        mNameStringIndex;
      long        mDescriptionStringIndex;
      long        mMaxPlayers;
      long        mType;
      long        mID;
};

//=============================================================================
//=============================================================================
class BScenarioList
{
   public:
      enum
      {
         cPlayerMapType1v1,
         cPlayerMapType2v2,
         cPlayerMapType3v3,
      };

      BScenarioList();
      ~BScenarioList();

      bool                 load();
      void                 reset();

      int                  getMapCount() const { return mScenarioMaps.getNumber(); }
      const BScenarioMap*  getMapInfo(int i) const;
      const BScenarioMap*  getMapInfo(const char* pName) const;

      // maps of specific player count
      int                  getPlayerNumberMapCount(int playerCountType) const;
      const BScenarioMap*  getPlayerNumberMapInfo(int playerCountType, int i) const;

   protected:
      BSmallDynamicSimArray<BScenarioMap*> mScenarioMaps;

      BSmallDynamicArray<long> m1v1SkirmishMaps;
      BSmallDynamicArray<long> m2v2SkirmishMaps;
      BSmallDynamicArray<long> m3v3SkirmishMaps;
};
