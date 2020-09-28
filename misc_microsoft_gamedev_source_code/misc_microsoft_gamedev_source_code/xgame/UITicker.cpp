//============================================================================
// UITicker.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITicker.h"
#include "lspManager.h"
#include "user.h"
#include "userprofilemanager.h"
#include "skullmanager.h"
#include "achievementmanager.h"

#define TICKER_TEXT_UPDATE_INTERVAL 10000
//#define TICKERTEST 1

namespace
{
   BUITicker * sInstance = 0;
}

//============================================================================
//============================================================================
BUITicker::BUITicker( void )
{
   sInstance = this;

   mTipsSetup = false;
   mProfileSetup = false;
   mNextUpdate = 0;
   mEnabled = false;
}

//============================================================================
//============================================================================
BUITicker::~BUITicker( void )
{
   sInstance = 0;
   //gLSPManager.removeTicker();
}

//============================================================================
//============================================================================
void BUITicker::renderTicker( void )
{
   SCOPEDSAMPLEID( FlashUIGame, 0XFFFF0000 );
   if(sInstance && sInstance->mEnabled)
   {
      if( sInstance->mbVisible )
      {
         sInstance->mpMovie->render();
      }
   }
}

//============================================================================
//============================================================================
void BUITicker::addString( const BUString& text, int bucket, int duration, int64 id )
{
   if(sInstance)
   {
      GFxValue args[5];
      args[0].SetStringW( text );
      args[1].SetNumber( bucket );
      if( duration >= 0 )
         args[2].SetNumber( duration );
      
      if( id != -1 )
      {
         int high = static_cast<int>(id >> 32);
         int low = id & 0xFFFFFFFF;
         args[3].SetNumber( high );
         args[4].SetNumber( low );
      }

      sInstance->invokeActionScript( "addString", args, 5 );
   }
}

//============================================================================
//============================================================================
void BUITicker::clear( void )
{
   if(sInstance)
   {
      sInstance->invokeActionScript( "clear" );
   }
}

//============================================================================
//============================================================================
void BUITicker::setSpeed( float speed )
{
   if(sInstance)
   {
      GFxValue args[1];
      args[0].SetNumber( speed );
      sInstance->invokeActionScript( "setSpeed", args, 1 );
   }
}

//============================================================================
//============================================================================
void BUITicker::setSpacing( float spacing )
{
   if(sInstance)
   {
      GFxValue args[1];
      args[0].SetNumber( spacing );
      sInstance->invokeActionScript( "setSpacing", args, 1 );
   }
}

//============================================================================
//============================================================================
void BUITicker::showBackground( bool bShow )
{
   if( sInstance )
   {
      GFxValue args[1];
      args[0].SetBoolean( bShow );
      sInstance->invokeActionScript( "showBackground", args, 1 );
   }
}

//============================================================================
//============================================================================
void BUITicker::setSeparatorString( const BUString& text )
{
   if( sInstance )
   {
      GFxValue args[1];
      args[0].SetStringW( text );
      sInstance->invokeActionScript( "setSeparatorString", args, 1 );
   }
}

//============================================================================
//============================================================================
void BUITicker::install()
{
#ifndef BUILD_FINAL
   if( gConfig.isDefined( "noTicker" ) )
      return;
#endif

   delete sInstance;
   sInstance = new BUITicker();
   if( sInstance->initScreen("art\\ui\\flash\\pregame\\Ticker\\UITicker.gfx", cFlashAssetCategoryInGame, "art\\ui\\flash\\pregame\\Ticker\\UITicker.xml") )
   {  
      sInstance->setVisible( true );
      gLSPManager.requestTickerData(sInstance);

#ifdef TICKERTEST
      setSpeed((float)cDefaultScrollSpeed * 5);
#else
      setSpeed((float)cDefaultScrollSpeed);
#endif
      setSpacing((float)cDefaultSpacing);
      setSeparatorString( L"$$SoF$$" );

      enable(true);

#ifndef BUILD_FINAL
      if( gConfig.isDefined( "debugTicker" ) )
         sInstance->getMovie()->setVariable( "_root.mDebugTicker", GFxValue(true) );
#endif
   }
   else
   {
      delete sInstance;
      sInstance = 0;
   }
}

void BUITicker::remove()
{
   delete sInstance;
   sInstance = 0;
}

bool BUITicker::isEnabled()
{
   if( sInstance )
   {
      return sInstance->isTickerEnabled();
   }

   return false;
}

//============================================================================
//============================================================================
void BUITicker::enable( bool enable )
{
   if( sInstance )
   {
      sInstance->enableInternal(enable);
   }
}

//============================================================================
//============================================================================
void BUITicker::enableInternal( bool enable )
{
   if( enable && (!mEnabled))
   {
      mNextUpdate = 0;
      mEnabled = true;
   }
   else if ( !enable && (mEnabled) )
   {
      clear();
      mTipsSetup = false;
      mProfileSetup = false;
      mNextUpdate = 0;
      mEnabled = false;
   }
}

//============================================================================
//============================================================================
void BUITicker::profileUpdate()
{
   if( sInstance )
      sInstance->profileUpdateInternal();
}

//============================================================================
//============================================================================
void BUITicker::profileUpdateInternal()
{
   mProfileSetup = false;
}


//============================================================================
//============================================================================
void BUITicker::updateTickerInfo()
{
   if( sInstance )
   {
      sInstance->updateTickerInfoInternal();
   }
}

//============================================================================
//============================================================================
void BUITicker::updateTickerInfoInternal()
{
   if(!mEnabled)
      return;

   BUser* pUser = gUserManager.getPrimaryUser();
   if(!pUser)
      return;


   if( !mProfileSetup )
   {
      //Update the achievement info
      gAchievementManager.setupTickerInfo();

      //Update the collectibles info
      gCollectiblesManager.setupTickerInfo();

      if( pUser->getProfile() )
      {
         //Update service record info
         pUser->getProfile()->setupTickerInfo();

         //Update campaign info
         pUser->getProfile()->mCampaignProgress.setupTickerInfo();
      }
      mProfileSetup = true;
   }


   if( !mTipsSetup )
   {
      //Add in the Tips
      BDynamicArray<int> tipList;
      int count = gDatabase.getTips().getTipCount();
      int i;
      for (i = 0; i < count; i++)
      {
         //BUITicker::addString(*gDatabase.getTips().getTip(i), 1, -1, BUITicker::eTipString + i);
         tipList.add(i);
      }

      // [10/23/2008 xemu] init the random seed, since this is done too early for it to normally happen in game
      setRandSeed(cUIRand, GetTickCount());
      for (i = 0; i < count; i++)
      {
         int j = getRandRange(cUIRand, 0, count-1);
         if (i != j)
         {
            int v = tipList[j];
            tipList[j] = tipList[i];
            tipList[i] = v;
         }
      }

      for (i = 0; i < count; i++)
      {
         int tipIndex = tipList[i];
         BUITicker::addString(*gDatabase.getTips().getTip(tipIndex), 1);
      }

      mTipsSetup = true;
   }

   DWORD currtime = timeGetTime();
   if( currtime >= mNextUpdate )
   {
      //Update Live friend info
      pUser->updateFriends();

      mNextUpdate = currtime + TICKER_TEXT_UPDATE_INTERVAL;

   }

}

