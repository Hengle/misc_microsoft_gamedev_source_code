//============================================================================
// UICampaignMoviePicker.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIListControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UIMoviePlayerControl.h"
#include "UITextFieldControl.h"
#include "binkInterface.h"

class BUIMenuItemControl;
class BCampaignNode;


class BUICampaignMoviePicker : public BUIScreen, public BBinkVideoStatus
{
   enum
   {
      cMoviePlayerMenuItemControl,
   };

public:
   BUICampaignMoviePicker( void );
   virtual ~BUICampaignMoviePicker( void );

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );
   virtual void setVisible( bool visible );

   // BBinkVideoStatus methods
   void              onVideoEnded(BBinkVideoHandle handle, BByteArray *preloadedData, eBinkStatusCode statusCode);

   void enterScreen();
   int getNextPlayAllIndex();
   void refresh();

   void updateRScrollButton();

protected:

   void populateMovieList();
   //void playMovie(int nodeIndex);

   void updateSelection(BCampaignNode *pNode);

   bool initMenuItems();
   bool initMenuItem(int index, bool addToList);
   bool displayButtons();



   BUIButtonBarControl           mButtonBar;
   BUIListControl                mMovieList;
   BUIScrollableTextFieldControl mHelpText;
   BUIMoviePlayerControl         mMovieController;
   BUITextFieldControl           mDescriptionLabel;
   BUITextFieldControl           mTitleLabel;


   BBinkVideoHandle              mPreviewVideoHandle;

   float                         mMoviePreviewLocX;
   float                         mMoviePreviewLocY;
   long                          mMoviePreviewWidth;
   long                          mMoviePreviewHeight;
   
   int                           mPlayAllMenuIndex;

   // Menu Items
   BDynamicArray<BUIMenuItemControl*> mMovieListItems;
};