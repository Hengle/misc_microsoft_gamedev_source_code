//==============================================================================
// BackgroundMovies.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "BackgroundMovies.h"
#include "gamedirectories.h"


//==============================================================================
//==============================================================================
BBackgroundMovies::BBackgroundMovies()
{
}

//==============================================================================
//==============================================================================
BBackgroundMovies::~BBackgroundMovies()
{
   removeData();
}


//==============================================================================
//==============================================================================
const BBackgroundMovieData* BBackgroundMovies::getData(int i)
{
   if ( (i<0) || (i>=mData.getNumber()) )
      return NULL;

   return mData[i];
}


//==============================================================================
//==============================================================================
bool BBackgroundMovies::loadEvents()
{
   BXMLReader reader;
   if (!reader.load(cDirData, "BackgroundMovies.xml"))
      return(false);   

   removeData();

   BXMLNode rootNode(reader.getRootNode());

   BASSERT(rootNode.getName() == "BackgroundMovies");

   //Create an achievement rule for each entry.
   int numMovies = rootNode.getNumberChildren();
   for (int j = 0; j < numMovies; j++)
   {
      BXMLNode movieNode = rootNode.getChild(j);
      if (movieNode.getName() != "Movie")
         continue;

      BSimString movieName;

      long unlockScenario;
      movieNode.getText(movieName);
      movieNode.getAttribValueAsLong("unlocksAt", unlockScenario);

      BBackgroundMovieData* pData = new BBackgroundMovieData();
      pData->mUnlocksAt = unlockScenario;
      pData->mMovieName = movieName;

      mData.add(pData);
   }

   return(true);
}

//=============================================================================
//=============================================================================
void BBackgroundMovies::removeData(void)
{
   for(int i=0; i<mData.getNumber(); i++)
      delete mData[i];
   mData.setNumber(0);
}

