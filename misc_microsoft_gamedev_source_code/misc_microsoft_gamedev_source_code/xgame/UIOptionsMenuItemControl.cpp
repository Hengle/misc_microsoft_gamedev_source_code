//============================================================================
// UIOptionsMenuItemControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UIOptionsMenuItemControl.h"
#include "UICheckboxControl.h"
#include "UISliderControl.h"
#include "UIFlyoutListControl.h"
#include "UIMenuItemControl.h"
#include "UILabelControl.h"
#include "UIButtonControl.h"
#include "user.h"
#include "database.h"

//============================================================================
//============================================================================
BUIOptionsMenuItemControl::BUIOptionsMenuItemControl( void ) :
   mType( eUnknown ),
   mControl( NULL ),
   mpUser( NULL )
{
   INIT_EVENT_MAP();
}

//============================================================================
//============================================================================
BUIOptionsMenuItemControl::~BUIOptionsMenuItemControl( void )
{
   for( int i = 0; i < (int)mControls.size(); ++i )
   {
      delete mControls[i];
   }
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::init( IUIControlEventHandler* parent, const char* controlPath, int controlID, BXMLNode* initData )
{
   if( !__super::init( parent, controlPath, controlID, initData ) )
      return false;
  
   bool result = true;

   if( initData )
   {
      initData->getAttribValueAsString( "name", mOptionName );

      BUString labelText;
      initData->getAttribValueAsString( "label", labelText );
      setLabelText( gDatabase.decodeLocString( labelText ) );

      initData->getAttribValueAsString( "description", mDescription );
      gDatabase.decodeLocString( mDescription );

      initData->getAttribValueAsString( "image", mImageURL );

      result = false;
      if( initData->getNumberChildren() )
      {
         BXMLNode controlNode = initData->getChild( (long)0 );

         BString controlType = controlNode.getName();

         if( controlType == "checkbox" )
         {
            attachMovie( "UICheckboxMC", "checkbox" );
            result = initCheckbox( &controlNode );
         }
         else if( controlType == "slider" )
         {
            attachMovie( "UISliderMC", "slider" );
            result = initSlider( &controlNode );
         }
         else if( controlType == "list" )
         {
            result = initFlyout( &controlNode );
         }
         else if( controlType == "button" )
         {
            result = initButton( &controlNode );
         }
         
         positionControl();
      }
   }

   if( !result )
   {
      delete mControl;
   }

   return result;
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::show( bool force /*= false*/ )
{
//    if( mControl )
//    {
//       mControl->show( force );
//    }

   __super::show( force );
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::hide( bool force /*= false*/ )
{
//    if( mControl )
//    {
//       mControl->hide( force );
//    }

   __super::hide( force );
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::handleUIControlEvent( BUIControlEvent& event )
{
   if( !mpUser )
      return false;

   switch( mType )
   {
      case eCheckbox:
         return handleCheckboxControlEvent( event );

      case eSlider:
         return handleSliderControlEvent( event );

      case eFlyout:
         return handleFlyoutControlEvent( event );

      case eButton:
         return handleButtonControlEvent( event );

      default:
         return __super::handleUIControlEvent( event );
   }
}

//============================================================================
// IInputControlEventHandler 
//============================================================================
bool BUIOptionsMenuItemControl::handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail )
{
   bool handled = mControl->handleInput( port, event, controlType, detail );

   if( isFlyoutOpen() && !handled )
   {
      handled = __super::handleInput( port, event, controlType, detail );
   }

   return handled;
}

bool BUIOptionsMenuItemControl::executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl )
{
   if( isFlyoutOpen() && command == "cancel" )
   {
      closeFlyout( true );
      return true;
   }
   return __super::executeInputEvent( port, event, controlType, detail, command, pInputControl );
}
//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::setLabelText( const BUString& text )
{
   GFxValue args[1];
   args[0].SetStringW( text.getPtr() );
   invokeActionScript( "setLabelText", args, 1 );
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::setUser( BUser* pUser )
{
   BASSERT( pUser );
   mpUser = pUser;

   GFxValue gfxValue;
   if( mType == eCheckbox )
   {
      bool boolValue = false;
      mpUser->getOptionByName( mOptionName, boolValue );
      gfxValue.SetBoolean( boolValue );
   }
   else if( mType != eButton )
   {
      uint8 uint8Value = 0;
      mpUser->getOptionByName( mOptionName, uint8Value );
      gfxValue.SetNumber( uint8Value );
   }
   setValue( gfxValue );
}

//============================================================================
//============================================================================
GFxValue BUIOptionsMenuItemControl::getValue( void ) const
{
   GFxValue value;
   
   switch( mType )
   {
      case eCheckbox:
         value.SetBoolean( ((BUICheckboxControl*)mControl)->isChecked() );
      break;

      case eSlider:
         value.SetNumber( ((BUISliderControl*)mControl)->getValue() );
      break;

      case eFlyout:
         value.SetNumber( getFlyoutList()->getSelectedControl()->getControlID() );
      break;

      default:
         value.SetNull();
      break;
   }

   return value;
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::setValue( GFxValue& value )
{
   disableSound();
   switch( mType )
   {
      case eCheckbox:
         ((BUICheckboxControl*)mControl)->check( value.GetBool() );
      break;

      case eSlider:
         ((BUISliderControl*)mControl)->setValue( (int)value.GetNumber() );
      break;

      case eFlyout:
      {
         int intValue = (int)value.GetNumber();
         BUIListControl* list = getFlyoutList();
         int numControls = list->getControlCount();
         for( int itemIdx = 0; itemIdx < numControls; ++itemIdx )
         {
            if( list->getControl( itemIdx )->getControlID() == intValue )
            {
               list->setIndex( itemIdx );
               break;
            }
         }

         // Update the selected item text
         BUIMenuItemControl* label = getFlyoutLabel();
//-- FIXING PREFIX BUG ID 2073
         const BUIMenuItemControl* selectedItem = (BUIMenuItemControl*)list->getSelectedControl();
//--
         label->setText( selectedItem->getText() );
      }         
      break;

      default:
      break;
   }
   enableSound();
}

//============================================================================
//============================================================================
const BString& BUIOptionsMenuItemControl::getOptionName( void ) const
{
   return mOptionName;
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::setOptionName( const char* optionName )
{
   mOptionName = optionName;
}

//============================================================================
//============================================================================
const BUString& BUIOptionsMenuItemControl::getDescription( void ) const
{
   return mDescription;
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::setDescription( const BUString& description )
{
   mDescription = description;
}

//============================================================================
//============================================================================
const BString& BUIOptionsMenuItemControl::getImageURL( void ) const
{
   return mImageURL;
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::setImageURL( const char* imageURL )
{
   mImageURL = imageURL;
}

//============================================================================
//============================================================================
BUIOptionsMenuItemControl::EControlType BUIOptionsMenuItemControl::getOptionType( void ) const
{
   return mType;
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::positionControl( void )
{
   switch( mType )
   {
      case eCheckbox:
      case eSlider:
      {
         GFxValue arg;
         arg.SetString( mControl->getControlPath() );
         invokeActionScript( "positionControl", &arg, 1 );
         break;
      }

      case eFlyout:
      {
         GFxValue arg;
         arg.SetString( mControls[eListIndex]->getControlPath() );
         invokeActionScript( "positionControl", &arg, 1 );

         arg.SetString( mControls[eLabelIndex]->getControlPath() );
         invokeActionScript( "positionControl", &arg, 1 );
         break;
      }
   }
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::handleCheckboxControlEvent( BUIControlEvent& event )
{
   switch( event.getID() )
   {
      case BUICheckboxControl::eCheck:
      case BUICheckboxControl::eUncheck:
      {
//-- FIXING PREFIX BUG ID 2074
         const BUICheckboxControl* checkbox = (BUICheckboxControl*)mControl;
//--
         mpUser->setOptionByName( mOptionName, checkbox->isChecked() );
         return true;
      }

      default:
         return __super::handleUIControlEvent( event );
   }
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::handleSliderControlEvent( BUIControlEvent& event )
{
   switch( event.getID() )
   {
      case BUISliderControl::eValueChanged:
      {
//-- FIXING PREFIX BUG ID 2075
         const BUISliderControl* slider = (BUISliderControl*)mControl;
//--
         mpUser->setOptionByName( mOptionName, (uint8)slider->getValue() );
         return true;
      }

      default:
         return __super::handleUIControlEvent( event );
   }
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::handleFlyoutControlEvent( BUIControlEvent& event )
{
   if( (event.getID() == BUIListControl::eChildControlEvent && event.getChildEvent()->getString() == "accept") ||
        event.getString() == "accept" )
   {
      toggleFlyout();
      return true;
   }
   else if( !isFlyoutOpen() && event.getID() == BUIListControl::eSelectionChanged )
   {
      // Update the option
      mpUser->setOptionByName( mOptionName, (uint8)getFlyoutList()->getSelectedControl()->getControlID() );

      // Update the selected item text
      BUIMenuItemControl* label = getFlyoutLabel();
//-- FIXING PREFIX BUG ID 2076
      const BUIMenuItemControl* selectedItem = (BUIMenuItemControl*)getFlyoutList()->getSelectedControl();
//--
      label->setText( selectedItem->getText() );
   }
   return __super::handleUIControlEvent( event );
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::handleButtonControlEvent( BUIControlEvent& event )
{
   if( event.getID() == BUIButtonControl::ePress )
   {
      mpUser->setOptionByName( mOptionName, true );
      return true;
   }
   return __super::handleUIControlEvent( event );
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::initCheckbox( BXMLNode* initData )
{
   mType = eCheckbox;
   mControl = new BUICheckboxControl();
   mControls.add( mControl );
   return mControl->init( this, mScriptPrefix + "checkbox", mControlID, initData );
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::initSlider( BXMLNode* initData )
{
   mType = eSlider;
   mControl = new BUISliderControl();
   mControls.add( mControl );
   
   if( mControl->init( this, mScriptPrefix + "slider", mControlID, initData ) )
   {
      BUISliderControl* slider = (BUISliderControl*)mControl;

      BUString minLabel, maxLabel;
      initData->getAttribValueAsString( "minLabel", minLabel );
      initData->getAttribValueAsString( "maxLabel", maxLabel );
      slider->setMinText( gDatabase.decodeLocString( minLabel ) );
      slider->setMaxText( gDatabase.decodeLocString( maxLabel ) );

      uint8 minValue = 0;
      uint8 maxValue = 1;
      if( !mpUser->getOptionRangeByName( mOptionName, minValue, maxValue ) )
      {
         initData->getAttribValueAsUInt8( "minValue", minValue );
         initData->getAttribValueAsUInt8( "maxValue", maxValue );
      }

      uint8 step = 1;
      initData->getAttribValueAsUInt8( "step", step );

      slider->setMin( minValue );
      slider->setMax( maxValue );
      slider->setStep( step );

      return true;
   }
   return false;
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::initFlyout( BXMLNode* initData )
{
   BASSERT( initData );

   mType = eFlyout;

   // List
   attachMovie( "UIOptionPaneMC", "flyoutList" );
   BUIListControl* list = new BUIListControl();
   if( !list->init( this, mScriptPrefix + "flyoutList", mControlID, initData ) )
   {
      delete list;
      return false;
   }
   
   mControls.insertAtIndex( list, eListIndex );
   
   int numControls = initData->getNumberChildren();

   for( int itemIdx = 0; itemIdx < numControls; ++itemIdx )
   {
      BXMLNode itemNode = initData->getChild( itemIdx );
      if( itemNode.getValid() )
      {
         BUString itemName;
         itemNode.getAttribValueAsString( "name", itemName );

         uint8 itemValue = 0;
         itemNode.getAttribValueAsUInt8( "value", itemValue );

         BString itemControlName;
         itemControlName.format( "item%d", itemIdx + 1 );
         list->attachMovie( "UIOptionFlyoutItemMC", itemControlName );

         BString itemPath;
         itemPath.format( "%s.%s", list->getControlPath().getPtr(), itemControlName.getPtr() );

         BUIMenuItemControl* itemControl = new BUIMenuItemControl();
         if( itemControl->init( list, itemPath, itemValue ) )
         {
            itemControl->setText( gDatabase.decodeLocString( itemName ) );
            mControls.add( itemControl );
            list->addControl( itemControl );
            itemControl->unfocus(true);
         }
         else
         {
            delete itemControl;
         }
      }
   }

   attachMovie( "UIOptionFlyoutItemMC", "flyoutLabel" );
   BUIMenuItemControl* label = new BUIMenuItemControl();
   if( label->init( this, mScriptPrefix + "flyoutLabel" ) )
   {
      mControl = label;
      mControls.insertAtIndex( label, eLabelIndex );
      list->setIndex( 0 );
      
//-- FIXING PREFIX BUG ID 2077
      const BUIMenuItemControl* selectedItem = (BUIMenuItemControl*)list->getControl( 0 );
//--
      label->setText( selectedItem->getText() );
   }
   else
   {
      delete label;
      return false;
   }

   list->hide();

   return true;
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::initButton( BXMLNode* initData )
{
   mType = eButton;

   BUIButtonControl* button = new BUIButtonControl();
   mControls.add( button );
   if( button->init( this, mControlPath, mControlID, initData ) )
   {
      mControl = button;
      return true;
   }
   else
   {
      delete button;
      return false;
   }
}

//============================================================================
//============================================================================
BUIListControl* BUIOptionsMenuItemControl::getFlyoutList( void ) const
{
   if( mType == eFlyout )
   {
      return (BUIListControl*)mControls[eListIndex];
   }
   return NULL;
}

//============================================================================
//============================================================================
BUIMenuItemControl* BUIOptionsMenuItemControl::getFlyoutLabel( void )
{
   if( mType == eFlyout )
   {
      return (BUIMenuItemControl*)mControls[eLabelIndex];
   }
   return NULL;
}

//============================================================================
//============================================================================
bool BUIOptionsMenuItemControl::isFlyoutOpen( void ) const
{
   return mType == eFlyout && mControl == mControls[eListIndex];
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::openFlyout( void )
{
   if( mType == eFlyout )
   {
      mControl = mControls[eListIndex];
      mControls[eListIndex]->show();
      mControls[eLabelIndex]->hide();
      unfocus();
      invokeActionScript( "onOpenFlyout" );
      fireUIControlEvent( eFlyoutOpen );

      uint8 value = 0;
      mpUser->getOptionByName( mOptionName, value );
      BUIListControl* list = getFlyoutList();
      int numItems = list->getControlCount();
      for( int idx = 0; idx < numItems; ++idx )
      {
         if( list->getControl( idx )->getControlID() == value )
         {
            list->setIndex( idx );
            break;
         }
      }
   }
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::closeFlyout( bool cancel /*= false*/ )
{
   if( mType == eFlyout )
   {
      invokeActionScript( "onCloseFlyout" );
      fireUIControlEvent( eFlyoutClose );

      mControl = mControls[eLabelIndex];
      mControls[eLabelIndex]->show();
      mControls[eListIndex]->hide();
      focus();

      if( !cancel )
      {
         // Update the option
         mpUser->setOptionByName( mOptionName, (uint8)getFlyoutList()->getSelectedControl()->getControlID() );

         // Update the selected item text
         BUIMenuItemControl* label = getFlyoutLabel();
//-- FIXING PREFIX BUG ID 2078
         const BUIMenuItemControl* selectedItem = (BUIMenuItemControl*)getFlyoutList()->getSelectedControl();
//--
         label->setText( selectedItem->getText() );
      }
   }
}

//============================================================================
//============================================================================
void BUIOptionsMenuItemControl::toggleFlyout( void )
{
   if( isFlyoutOpen() )
   {
      closeFlyout();
   }
   else
   {
      openFlyout();
   }
}