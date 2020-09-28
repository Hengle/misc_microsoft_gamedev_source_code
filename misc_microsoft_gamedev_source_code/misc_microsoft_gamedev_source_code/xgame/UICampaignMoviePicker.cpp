//============================================================================
// UICampaignMoviePicker.cpp
// Ensemble Studios (c) 2008
//============================================================================

#include "common.h"
#include "UICampaignMoviePicker.h"

#include "UIMenuItemControl.h"
#include "campaignmanager.h"
#include "configsgame.h"
#include "gamedirectories.h"
#include "modemanager.h"
#include "ModeCampaign2.h"
#include "campaignprogress.h"
#include "database.h"
#include "usermanager.h"
#include "user.h"

//============================================================================
//============================================================================
BUICampaignMoviePicker::BUICampaignMoviePicker( void ) :
   mPreviewVideoHandle(cInvalidVideoHandle),
   mMoviePreviewLocX(0),
   mMoviePreviewLocY(0),
   mMoviePreviewWidth(0),
   mMoviePreviewHeight(0),
   mPlayAllMenuIndex(0)
{
   mMovieListItems.clear();
}

//============================================================================
//============================================================================
BUICampaignMoviePicker::~BUICampaignMoviePicker( void )
{
   BUIMenuItemControl* c = NULL;
   for (int i=0; i<mMovieListItems.getNumber(); i++)
   {
      c = mMovieListItems[i];
      delete c;
      mMovieListItems[i]=NULL;
   }
   mMovieListItems.clear();

   gBinkInterface.stopVideo(mPreviewVideoHandle,false);
   mPreviewVideoHandle = cInvalidVideoHandle;
}

//============================================================================
//============================================================================
bool BUICampaignMoviePicker::init( const char* filename, const char* datafile )
{
   // read our data file
   BUIScreen::init(filename, datafile);

   // initialize all the components for our screen
   mMovieList.init( this, "mMovieList", 0, NULL );
   mMovieList.setWrap(true);

   // Add our menu items into the menu
   initMenuItems();

   mHelpText.init(this, "mHelpText", 0, NULL);

   // Initialize the button bar
   mButtonBar.init(this, "mButtonBar", 0, NULL);

   mDescriptionLabel.init(this, "mDescriptionLabel");
   mDescriptionLabel.setText(gDatabase.getLocStringFromID(25662));
   mTitleLabel.init(this, "mTitleLabel");
   mTitleLabel.setText(gDatabase.getLocStringFromID(24824));

   populateMovieList();
 
   displayButtons();


   // fixme - this may go elsewhere
   mMovieController.init(this, "mMovieController", 0, NULL);
   mMovieController.hide();


   return true;
}

//============================================================================
// BUICampaignMoviePicker::init
//============================================================================
bool BUICampaignMoviePicker::init(BXMLNode dataNode)
{

   int numChildren = dataNode.getNumberChildren();
   for (int i = 0; i < numChildren; ++i)
   {
      BXMLNode node = dataNode.getChild(i);
      if (node.getName().compare("MoviePreviewLocation") == 0)
      {
         node.getAttribValueAsFloat("x", mMoviePreviewLocX);
         node.getAttribValueAsFloat("y", mMoviePreviewLocY);
         node.getAttribValueAsLong("width", mMoviePreviewWidth);
         node.getAttribValueAsLong("height", mMoviePreviewHeight);
      }
   }
   
   return(BUIScreen::init(dataNode));
}


//============================================================================
//============================================================================
void BUICampaignMoviePicker::populateMovieList()
{
   // iterate over all the nodes
   //    mSecondaryMenu
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

   // [7/17/2008 xemu] grab the progress info out of the profile 
   bool showAll = false;
   BCampaignProgress *pCampaignProgress = BCampaignProgress::getCampaignProgress(&showAll);

   int selectedIndex=0;
   int menuIndex = 0;
   bool firstEntry = true;
   for (int i=0; i<pCampaign->getNumberNodes(); i++)
   {
      BCampaignNode* pNode = pCampaign->getNode(i);

      if (!pNode->getFlag(BCampaignNode::cCinematic))
         continue;

      // we are not showing the legendary clip in the theater
      if (pNode->getFlag(BCampaignNode::cFlagLegendary))
         continue;

      // [7/16/2008 xemu] skip if we haven't unlocked the movie
      if (!showAll && (pCampaignProgress != NULL) && !pCampaignProgress->isCinematicUnlocked(pNode->getID()))
         continue;

      BUIMenuItemControl* pMenuItem = NULL;

      // add the node to the menu
      if (menuIndex<mMovieListItems.getNumber())
      {
         pMenuItem = mMovieListItems[menuIndex];
         if (!pMenuItem)
            continue;

         pMenuItem->setText(pNode->getDisplayName());
         pMenuItem->setControlID(cMoviePlayerMenuItemControl);

         // [8/12/2008 xemu] first entry is the special "play all" one 
         if (firstEntry)
         {
            firstEntry = false;
            pMenuItem->setData(-1);
         }
         else
         {
            pMenuItem->setData(i);
         }

         // [7/28/2008 xemu] set up the movie preview, etc for first in the list
         //if (menuIndex == 0)
            //updateSelection(pNode);

         menuIndex++;

      }
   }

   // fill out the rest as not visible
   // fixme

   // set the index (0 right now)
   mMovieList.setIndex(selectedIndex);
}

//============================================================================
//============================================================================
void BUICampaignMoviePicker::updateSelection(BCampaignNode *pNode)
{
   BASSERT(pNode);
   if (pNode)
   {
      // Scrolling text box
      mHelpText.setText(pNode->getIntroText());

      gBinkInterface.stopVideo(mPreviewVideoHandle,false);

      // [7/28/2008 xemu] play a movie preview
      BSimString movie;
      if (gConfig.get(cConfigUIBackgroundMovieCampaign, movie))
      {
         BBinkInterface::BLoadParams lp;
         lp.mFilename.set(pNode->getFilename());
         lp.mCaptionDirID = cDirData;
         //lp.mpStatusCallback = this;
         lp.mLoopVideo = true;
         lp.mLoopAtFrame = 600;
         lp.mFullScreen = false;
         lp.mXOffset = mMoviePreviewLocX;
         lp.mYOffset = mMoviePreviewLocY;
         lp.mWidth = mMoviePreviewWidth;
         lp.mHeight = mMoviePreviewHeight;
         lp.mNoSound = true;
         // [8/12/2008 xemu] temp mask for testing  
         lp.mMaskTextureFilename.set("ui\\flash\\shared\\textures\\pregame\\masks\\letterboxmask");
         mPreviewVideoHandle = gBinkInterface.loadActiveVideo(lp);
         BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
         if (pMode)
            pMode->setForegroundVideoHandle(mPreviewVideoHandle);
      }
   }
}

//============================================================================
//============================================================================
bool BUICampaignMoviePicker::initMenuItem(int index, bool addToList)
{
   BSimString controlPath;
   controlPath.format("mMenuItem%d", index);
   //BUString text;
   //text.set(menuText);

   BUIMenuItemControl *pMenuItem = new BUIMenuItemControl();
   pMenuItem->init( this, controlPath.getPtr(), 0);
   pMenuItem->setText(L"");

   if (addToList)
   {
      mMovieList.addControl(pMenuItem);
      mMovieListItems.add(pMenuItem);
   }

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignMoviePicker::initMenuItems()
{
   mMovieList.clearControls();
   mMovieListItems.clear();

   BCampaignProgress *pProgress = BCampaignProgress::getCampaignProgress(NULL);
   long movieCount = 0;
   long cMaxCinematics = 17; 
   if (pProgress != NULL)
      movieCount = pProgress->getUnlockedMovieCount();
   else
      movieCount = cMaxCinematics;

   for (int i=0; i < movieCount; i++)
      initMenuItem(i, true);
   for (i=movieCount; i < cMaxCinematics; i++)
      initMenuItem(i, false);

   return true;
}

//============================================================================
//============================================================================
bool BUICampaignMoviePicker::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if (command=="cancel")
   {
      gBinkInterface.stopVideo(mPreviewVideoHandle,false);
      mPreviewVideoHandle = cInvalidVideoHandle;

      gUI.playCancelSound();

      this->setVisible(false);
      if (mpParent)
         mpParent->setVisible(true);
      return true;
   }
   return false;
}

//============================================================================
//============================================================================
void BUICampaignMoviePicker::setVisible( bool visible )
{
   __super::setVisible(visible);

   BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);
   if (pMode)
   {
      pMode->renderBackgroundVideos(!visible);
   }
}

//============================================================================
//============================================================================
bool BUICampaignMoviePicker::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
   bool handled = false;

   handled = BUIScreen::handleInput(port, event, controlType, detail);
   if (handled)
      return handled;

/*
   // test
   handled = mMovieController.handleInput(port, event, controlType, detail);
   if (handled)
      return handled;
*/

   handled = mMovieList.handleInput(port, event, controlType, detail);
   if (handled)
      return true;

   handled = mHelpText.handleInput(port, event, controlType, detail);

   return handled;
}

// IUIControlEventHandler
//==============================================================================
//==============================================================================
bool BUICampaignMoviePicker::handleUIControlEvent( BUIControlEvent& event )
{
   BUIControl * control = event.getControl();
   if (!control)
      return false;        // can't do anything with this.
   //bool handled = false;

   if (control->getControlID() == cMoviePlayerMenuItemControl)
   {
      if (event.getID() == BUIListControl::eItemSelected)
      {
         BUIListControl *listControl = (BUIListControl*)control;
         BUIMenuItemControl *mi = (BUIMenuItemControl *)listControl->getSelectedControl();

         BCampaign * pCampaign = gCampaignManager.getCampaign(0);
         BASSERT(pCampaign);

         // [8/12/2008 xemu] handle the special "play all" case 
         if (mi->getData() == -1)
         {
            mDescriptionLabel.setText(gDatabase.getLocStringFromID(25662));
            BCampaignNode* pNode = pCampaign->getNode((long)0);
            updateSelection(pNode);
            //mDescriptionLabel.setText("");
            //mHelpText.setText("");
            //gBinkInterface.stopVideo(mPreviewVideoHandle);
            //mPreviewVideoHandle = cInvalidVideoHandle;
         }
         else
         {
            mDescriptionLabel.setText(gDatabase.getLocStringFromID(25662));
            BCampaignNode* pNode = pCampaign->getNode(mi->getData());
            updateSelection(pNode);
         }
      }
      else if (control->getControlTypeID() == UIMenuItemControlID)
      {
         if ((event.getID() == BUIControl::eStringEvent) && (event.getString() == "Accept"))
         {
//-- FIXING PREFIX BUG ID 1355
            const BUIMenuItemControl* mi = (BUIMenuItemControl*)control;
//--
            BModeCampaign2* pMode = (BModeCampaign2*)gModeManager.getMode(BModeManager::cModeCampaign2);

            // [8/12/2008 xemu] in "play all" mode reset our count to the beginning 
            if (mi->getData() == -1)
               mPlayAllMenuIndex = 1;

            if (pMode)
               pMode->playMovie(mi->getData());
         }
      }
   }
   return true;
}

//============================================================================
//============================================================================
/*
void BUICampaignMoviePicker::playMovie(int nodeIndex)
{
   // fixme - remove this.
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);

//-- FIXING PREFIX BUG ID 1356
   const BCampaignNode* pNode = pCampaign->getNode(nodeIndex);
//--
   if (pNode)
   {
      // gSoundManager.overrideBackgroundMusic(true);

      bool showSubtitles=true;
      // check to see if subtitles are turned on.
      if (gUserManager.getPrimaryUser())
         showSubtitles=gUserManager.getPrimaryUser()->getOption_SubtitlesEnabled();

      BSimString captionFile;
      if (showSubtitles)
         captionFile = pNode->getCaptionsFile();

      BBinkInterface::BLoadParams lp;
      lp.mFilename.set(pNode->getFilename());
      lp.mCaptionDirID = cDirData;
      lp.mCaptionFilename.set(captionFile.getPtr());
      lp.mpStatusCallback = this;
      lp.mLoopVideo = false;
      lp.mFullScreen = true;
      
      gBinkInterface.loadActiveVideo(lp);// gBinkInterface.loadActiveVideoFullScreen(pNode->getFilename(), cDirData, captionFile.getPtr(), "", this, false);
   }

}
*/

//============================================================================
//============================================================================
void BUICampaignMoviePicker::onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode)
{
   // fixme - do I need this?

   // If for some reason we got preloaded data back (not expected here at the moment), delete it since otherwise
   // it would be orphaned.
   if(preloadedData)
   {
      delete preloadedData;
      preloadedData = NULL;
   }
}




//============================================================================
//============================================================================
bool BUICampaignMoviePicker::displayButtons()
{
   uint b0=BUIButtonBarControl::cFlashButtonOff;
   uint b1=BUIButtonBarControl::cFlashButtonOff;
   uint b2=BUIButtonBarControl::cFlashButtonOff;
   uint b3=BUIButtonBarControl::cFlashButtonOff;
   uint b4=BUIButtonBarControl::cFlashButtonOff;

   // change the string IDs and button faces as needed

   // A - accept
   // B - back to main menu
   BUString s0;
   BUString s1;
   BUString s2;
   BUString s3;
   BUString s4;

   b0 = BUIButtonBarControl::cFlashButtonA;
   s0.set(gDatabase.getLocStringFromID(24801)); // PLAY

   b1=BUIButtonBarControl::cFlashButtonB;
   s1.set(gDatabase.getLocStringFromID(24802)); // BACK

   b4=BUIButtonBarControl::cFlashButtonudRStick;
   s4.set(gDatabase.getLocStringFromID(24803)); // SCROLL

   mButtonBar.setButtonStates(b0, b1, b2, b3, b4);
   mButtonBar.setButtonTexts(s0, s1, s2, s3, s4);

   return true;
}


//============================================================================
void BUICampaignMoviePicker::enterScreen()
{
   // [8/12/2008 xemu] reset screen
   mDescriptionLabel.setText("");
   mHelpText.setText("");
   mMovieList.setIndex(0);

   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   BCampaignNode* pNode = pCampaign->getNode((long)0);
   updateSelection(pNode);
}

//============================================================================
int BUICampaignMoviePicker::getNextPlayAllIndex()
{
   int retval = -1;
   // [8/12/2008 xemu] convert from our menu index, to the appropriate movie index 
   int count = mMovieListItems.getNumber();
   if (mPlayAllMenuIndex < count)
   {
      BUIMenuItemControl *pControl = mMovieListItems[mPlayAllMenuIndex];
      retval = pControl->getData();
   }
   mPlayAllMenuIndex++;

   // [8/12/2008 xemu] signal a stop if we get to the end 
   // [9/24/2008 xemu] fixed an off-by-one here, where it was ending improperly on the next-to-last movie 
   if (mPlayAllMenuIndex > count)
      return(-1);

   return retval;
}

//============================================================================
// BUICampaignMoviePicker::updateRScrollButton
//============================================================================
void BUICampaignMoviePicker::updateRScrollButton()
{
   GFxValue value;
   value.SetStringW(gDatabase.getLocStringFromID(24803).getPtr());
   invokeActionScript( "updateRScrollButton", &value, 1);
}

//============================================================================
void BUICampaignMoviePicker::refresh()
{
   BCampaign * pCampaign = gCampaignManager.getCampaign(0);
   BASSERT(pCampaign);
   BUIMenuItemControl *mi = (BUIMenuItemControl *)mMovieList.getSelectedControl();
   BCampaignNode* pNode = pCampaign->getNode(mi->getData());

   // [9/24/2008 xemu] default to "play all" if nothing else works 
   if (pNode == NULL)
      pNode = pCampaign->getNode((long)0);

   updateSelection(pNode);
}