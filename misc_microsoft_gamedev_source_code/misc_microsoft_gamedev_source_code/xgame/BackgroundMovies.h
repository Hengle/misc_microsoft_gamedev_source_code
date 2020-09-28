//==============================================================================
// BackgroundMovies.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once


class BBackgroundMovieData
{
public:
   int   mUnlocksAt;
   BString mMovieName;
};


//==============================================================================
// BBackgroundMovies
//==============================================================================
class BBackgroundMovies
{
public:
   BBackgroundMovies();
   ~BBackgroundMovies();

   const BBackgroundMovieData* getData(int index);
   const int getNumber() const { return mData.getNumber(); }
   bool loadEvents();

protected:
   void removeData(void);

   BDynamicSimArray<BBackgroundMovieData*>   mData;
};
