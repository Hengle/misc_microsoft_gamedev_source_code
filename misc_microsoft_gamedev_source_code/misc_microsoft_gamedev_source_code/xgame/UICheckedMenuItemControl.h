//============================================================================
// BUICheckedMenuItemControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "UICheckboxControl.h"

//============================================================================
//============================================================================
class BUICheckedMenuItemControl: public BUIControl
{
   enum Events
   {
      eMenuItemControlEvent = UICheckedMenuItemControlID,
      eCheck,
      eUncheck
   };
   
   BEGIN_EVENT_MAP( UICheckboxControl )
      MAP_CONTROL_EVENT( Check )
      MAP_CONTROL_EVENT( Uncheck )
   END_EVENT_MAP()   
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UICheckedMenuItemControl );

   public:

      BUICheckedMenuItemControl( void );
      virtual ~BUICheckedMenuItemControl( void );

      virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );
      virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

      virtual void setText(const BUString& text);
      virtual const BUString& getText() const { return mText; }

      virtual void check( bool bCheck = true );
      virtual void uncheck( void );
      virtual bool isChecked( void ) const;
      virtual void toggleChecked( void );

      virtual int getData() const { return mData; }
      virtual void setData(int data) { mData = data; }

   protected:
      BUICheckboxControl mCheckbox;
      BUString mText;
      int      mData;            
};