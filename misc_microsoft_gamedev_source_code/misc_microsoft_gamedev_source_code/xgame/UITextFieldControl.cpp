//============================================================================
// UILabelControl.cpp
// Ensemble Studios (c) 2008
//============================================================================
#include "common.h"
#include "UITextFieldControl.h"

//============================================================================
//============================================================================
BUITextFieldControl::BUITextFieldControl( void ) :
   mColor(cDWORDWhite),
   mUseColor(false)

{
   mControlType.set("BUITextFieldControl");
   mText.set("");
}

//============================================================================
//============================================================================
BUITextFieldControl::~BUITextFieldControl( void )
{
   mText.set("");
}


//============================================================================
//============================================================================
void BUITextFieldControl::setText(const BUString& text)
{
   mText = text;

   BFlashMovieInstance* pFlashMovie = getMovie();
   if (!pFlashMovie)
      return;

   if (mControlPath.length() == 0)
      return;

   GFxValue value;
   value.SetStringW(mText.getPtr());

   pFlashMovie->setVariable( mControlPath+".htmlText", value, GFxMovie::SV_Normal);

   if (mUseColor)
   {
      GFxValue colorValue;
      colorValue.SetNumber(mColor);
      pFlashMovie->setVariable( mControlPath+".textColor", colorValue, GFxMovie::SV_Normal);
   }
}

//==============================================================================
//==============================================================================
void BUITextFieldControl::show( bool force )
{ 
   if( !mbShown || force)
   {
      mbShown = true;

      BFlashMovieInstance* pFlashMovie = getMovie();
      if (pFlashMovie && (mControlPath.length() > 0) )
      {
         GFxValue value;
         value.SetBoolean(mbShown);

         pFlashMovie->setVariable( mControlPath+"._visible", value, GFxMovie::SV_Normal);
      }

      fireUIControlEvent( eShow );
   }
}

//==============================================================================
//==============================================================================
void BUITextFieldControl::hide( bool force )
{ 
   if( mbShown || force)
   {
      mbShown = false;
      BFlashMovieInstance* pFlashMovie = getMovie();
      if (pFlashMovie && (mControlPath.length() > 0) )
      {
         GFxValue value;
         value.SetBoolean(mbShown);

         pFlashMovie->setVariable( mControlPath+"._visible", value, GFxMovie::SV_Normal);
      }
      fireUIControlEvent( eHide );
   }
}


