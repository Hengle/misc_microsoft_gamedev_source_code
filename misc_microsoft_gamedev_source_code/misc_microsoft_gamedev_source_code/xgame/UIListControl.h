//============================================================================
// UIListControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "xcore.h"

class BUIListControl : public BUIControl
{
public:
   enum Events
   {
      eNext = UIListControlID,
      ePrev,
      eStopBegin,
      eStopEnd,
      eItemSelected,
      eSelectionChanged
   };
   
   BEGIN_EVENT_MAP( UIListControl )
      MAP_CONTROL_EVENT( Next )
      MAP_CONTROL_EVENT( Prev )
      MAP_CONTROL_EVENT( StopBegin )
      MAP_CONTROL_EVENT( StopEnd )
      MAP_CONTROL_EVENT( ItemSelected )
      MAP_CONTROL_EVENT( SelectionChanged )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIListControl );

public:
   BUIListControl( void );
   virtual ~BUIListControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1 , BXMLNode* initData = NULL );

   virtual void setWrap( bool wrap );
   virtual bool getWrap( void ) const;

   enum EListAlignment { eHorizontal, eVertical };
   virtual void setAlignment( EListAlignment alignment );
   virtual EListAlignment getAlignment( void ) const;

   virtual void addControl( BUIControl* control );
   virtual void clearControls();
   virtual BUIControl* getControl( int index );
   virtual BUIControl* getSelectedControl( void );
   virtual int getControlCount( void ) const;

   virtual bool next( void );
   virtual bool prev( void );
   virtual bool setIndex( int index );
   virtual int getSelectedIndex( void );

   //----- IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

   virtual void focus( bool force = false );
   virtual void unfocus( bool force = false );

   void setIgnoreNextNext(bool v) { mIgnoreNextNext=v; }
   void setIgnoreNextPrev(bool v) { mIgnoreNextPrev=v; }

protected:

   int getNextFocusIndex(int currentIndex);
   int getPrevFocusIndex(int currentIndex);

   
   bool mWrap;
   bool mIgnoreNextNext;
   bool mIgnoreNextPrev;

   EListAlignment mAlignment;
   int mSelectedIndex;
   BDynamicArray<BUIControl*> mControls;
};
