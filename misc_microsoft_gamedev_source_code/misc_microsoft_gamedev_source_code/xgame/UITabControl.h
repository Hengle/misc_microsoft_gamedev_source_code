//============================================================================
// UITabControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIControl.h"
#include "UIListControl.h"
#include "xcore.h"

class BUITabControl : public BUIControl
{
public:
   enum Events
   {
      eTabChanged = UITabControlID,
   };

   BEGIN_EVENT_MAP( UITabControl )
      MAP_CONTROL_EVENT( TabChanged )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UITabControl );

public:
   BUITabControl( void );
   virtual ~BUITabControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   // IUIControlEventHandler
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   //----- IInputControlEventHandler 
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void addTab( BUIControl* tab, BUIControl* pane );

   virtual void setActiveTab(int index);
   virtual int getActiveTab();

   virtual void setWrap( bool wrap );
   virtual bool getWrap( void ) const;

   virtual bool next( void );
   virtual bool prev( void );

   virtual BUIControl* getPane( int index );
   virtual BUIControl* getActivePane( void );

   virtual int getNumberTabs() { return mPanes.getNumber(); }

protected:

   virtual void updateContentVisiblity( void );

   BUIListControl mTabList;
   BDynamicArray<BUIControl*> mPanes;
};