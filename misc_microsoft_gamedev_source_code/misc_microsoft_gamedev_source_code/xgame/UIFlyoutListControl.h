//============================================================================
// UIFlyoutListControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "UIListControl.h"
#include "UIMenuItemControl.h"

class BUIFlyoutListControl : public BUIControl
{
public:
   enum Events
   {
      eOpen = UIFlyoutListControlID,
      eClose,
      eSelectionChanged
   };
   
   BEGIN_EVENT_MAP( UIFlyoutListControl )
      MAP_CONTROL_EVENT( Open )
      MAP_CONTROL_EVENT( Close )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIFlyoutListControl );

   
public:
   BUIFlyoutListControl( void );
   virtual ~BUIFlyoutListControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   // IUIControlEventHandler
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   // IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);

   // UIControl
   virtual void focus( bool force = false );
   virtual void unfocus( bool force = false );

   virtual void show( bool force = false );
   virtual void hide( bool force = false );

   virtual bool isOpen( void ) const;

   virtual void addItem( const BUString& itemText, int itemValue );
   virtual int getValue( void );
   virtual void setValue( int value );
   virtual int getValueByIndex( int index );

   virtual int getSelectedIndex( void );
   virtual void setIndex( int index );

   virtual int getNumValues( void );

   virtual void clear( void );

protected:
   virtual void open( void );
   virtual void close( void );

   bool mbOpen;
   
   BUIMenuItemControl mClosed;
   
   BUIListControl mOpen;
   static const long MAX_ITEMS = 15;
   BUIMenuItemControl mItems[MAX_ITEMS];
};