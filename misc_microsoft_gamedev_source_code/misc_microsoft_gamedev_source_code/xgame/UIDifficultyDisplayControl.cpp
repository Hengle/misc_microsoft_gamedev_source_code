//============================================================================
// UIDifficultyDisplayControl.cpp
// Ensemble Studios (c) 2007
//============================================================================

#include "common.h"
#include "UIDifficultyDisplayControl.h"
#include "campaignmanager.h"
#include "database.h"

//==============================================================================
//==============================================================================
BUIDifficultyDisplayControl::BUIDifficultyDisplayControl() : 
mCurrentDifficulty(-1)
{
}

//==============================================================================
//==============================================================================
BUIDifficultyDisplayControl::~BUIDifficultyDisplayControl()
{

}


//==============================================================================
// BUIDifficultyDisplayControl::setText
//==============================================================================
void BUIDifficultyDisplayControl::setText(const BUString& t)
{
   GFxValue value;
   value.SetStringW(t.getPtr());

   invokeActionScript("setText", &value, 1);
}


//==============================================================================
// BUIDifficultyDisplayControl::setState
//==============================================================================
void BUIDifficultyDisplayControl::setState(uint s)
{
   GFxValue value;
   value.SetNumber(s);

   invokeActionScript("setState", &value, 1);
}


//==============================================================================
// BUIDifficultyDisplayControl::update
//==============================================================================
void BUIDifficultyDisplayControl::update(long difficulty)
{
   // don't update unless there is a change
   if (mCurrentDifficulty == difficulty)
      return;

   mCurrentDifficulty = difficulty;

   BUString diffString;
   switch (mCurrentDifficulty)
   {
      case DifficultyType::cEasy:
         diffString.locFormat(gDatabase.getLocStringFromID(25624).getPtr(), gDatabase.getLocStringFromID(25618).getPtr(), gDatabase.getLocStringFromID(25623).getPtr() );
         break;
      case DifficultyType::cNormal:
         diffString.locFormat(gDatabase.getLocStringFromID(25624).getPtr(), gDatabase.getLocStringFromID(25619).getPtr(), gDatabase.getLocStringFromID(25623).getPtr() );
         break;
      case DifficultyType::cHard:
         diffString.locFormat(gDatabase.getLocStringFromID(25624).getPtr(), gDatabase.getLocStringFromID(25620).getPtr(), gDatabase.getLocStringFromID(25623).getPtr() );
         break;
      case DifficultyType::cLegendary:
         diffString.locFormat(gDatabase.getLocStringFromID(25624).getPtr(), gDatabase.getLocStringFromID(25621).getPtr(), gDatabase.getLocStringFromID(25623).getPtr() );
         break;
      case DifficultyType::cAutomatic:
         diffString.locFormat(gDatabase.getLocStringFromID(25624).getPtr(), gDatabase.getLocStringFromID(25622).getPtr(), gDatabase.getLocStringFromID(25623).getPtr() );
         break;
   }

   setState(mCurrentDifficulty+1);
   setText(diffString);
}