//============================================================================
// UIOptionsMenuItemControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIControl.h"

class BUser;
class BUIListControl;
class BUIMenuItemControl;

class BUIOptionsMenuItemControl : public BUIControl
{
public:
   enum Events
   {
      eFlyoutOpen = UIOptionsMenuItemControlID,
      eFlyoutClose
   };

   BEGIN_EVENT_MAP( UIOptionsMenuItemControl )
      MAP_CONTROL_EVENT( FlyoutOpen )
      MAP_CONTROL_EVENT( FlyoutClose )
   END_EVENT_MAP()

   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIOptionsMenuItemControl );

public:
   BUIOptionsMenuItemControl( void );
   ~BUIOptionsMenuItemControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData );

   virtual void show( bool force = false );
   virtual void hide( bool force = false );

   // IUIControlEventHandler
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   // IInputControlEventHandler 
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
      
   virtual void setLabelText( const BUString& text );
   virtual void setUser( BUser* pUser );

   virtual GFxValue getValue( void ) const;
   virtual void setValue( GFxValue& value );

   virtual const BString& getOptionName( void ) const;
   virtual void setOptionName( const char* optionName );

   virtual const BUString& getDescription( void ) const;
   virtual void setDescription( const BUString& description );

   virtual const BString& getImageURL( void ) const;
   virtual void setImageURL( const char* imageURL );

   enum EControlType { eCheckbox, eSlider, eFlyout, eButton, eUnknown };
   virtual EControlType getOptionType( void ) const;

protected:
   virtual void positionControl( void );

   virtual bool handleCheckboxControlEvent( BUIControlEvent& event );
   virtual bool handleSliderControlEvent( BUIControlEvent& event );
   virtual bool handleFlyoutControlEvent( BUIControlEvent& event );
   virtual bool handleButtonControlEvent( BUIControlEvent& event );

   virtual bool initCheckbox( BXMLNode* initData );
   virtual bool initSlider( BXMLNode* initData );
   virtual bool initFlyout( BXMLNode* initData );
   virtual bool initButton( BXMLNode* initData );

   BUIListControl* getFlyoutList( void ) const;
   BUIMenuItemControl* getFlyoutLabel( void );
   virtual bool isFlyoutOpen( void ) const;
   virtual void openFlyout( void );
   virtual void closeFlyout( bool cancel = false );
   virtual void toggleFlyout( void );

   
   enum EControlIndex { eListIndex, eLabelIndex };

   EControlType mType;
   BUIControl* mControl;
   BDynamicArray<BUIControl*> mControls;
   BString mOptionName;
   BUser* mpUser;
   BUString mDescription;
   BString mImageURL;
};