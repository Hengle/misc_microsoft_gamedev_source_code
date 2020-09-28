//============================================================================
// flashbuttonpanel.cpp
// Ensemble Studios (c) 2006
//============================================================================

#include "common.h"
#include "flashbuttonpanel.h"

//-- render
#include "render.h"
#include "renderDraw.h"
#include "renderThread.h"
#include "rendertargetTexture.h"
//-- game classes
#include "user.h"
#include "usermanager.h"
#include "player.h"
#include "uigame.h"
#include "visualmanager.h"

class BLabelLookup
{
public:
   int      mEnum;
   char     mName[64];
};

#define LABELENUM(type, str) {BFlashButtonPanel::type, str},
BLabelLookup gLabelLookup[BFlashButtonPanel::eLabelTotal]=
{
   LABELENUM(eLabelUp,    "_root.y_text" )
   LABELENUM(eLabelDown,  "_root.a_text" )
   LABELENUM(eLabelLeft,  "_root.x_text" )
   LABELENUM(eLabelRight, "_root.b_text" )
};
#undef LABELENUM

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashButtonPanel::BFlashButtonPanel():
mpMovie(NULL)
{
   mEnableStates.setAll(1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BFlashButtonPanel::~BFlashButtonPanel()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::init(const char* filename)
{
   if (!filename)
      return;

   gFlashGateway.getOrCreateData(filename, mDataHandle);
   mpMovie = gFlashGateway.createInstance(mDataHandle, true);
   gFlashGateway.registerEventHandler(mpMovie, mSimEventHandle);

   mText.resize(eLabelTotal);

   setFlag(cFlagInitialized, true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::deinit()
{
   if (mpMovie)
   {
      gFlashGateway.unregisterEventHandler(mpMovie, mSimEventHandle);
      gFlashGateway.releaseInstance(mpMovie);
   }
   mpMovie = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::enter()
{
   if (!getFlag(cFlagInitialized))
      return;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::leave()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::update()
{
   if (!getFlag(cFlagInitialized))
      return;   

   updatePanel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::renderBegin()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::render()
{   
   BScopedPIXNamedEvent pixEvent("FlashUIGame", 0xFFFF0000);

   mpMovie->render();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::renderEnd()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool BFlashButtonPanel::receiveEvent(const BEvent& event, BThreadIndex threadIndex)
{
   return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::initPanel()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::updatePanel()
{     
   for (int i = 0; i < eLabelTotal; i++)
   {
      if (mEnableStates.isSet(i) != 0)
         setText(i, BStrConv::toA(mText[i]));
      else
         setText(i, "");
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::refresh()
{   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::setText(int labelID, const char* text)
{
   BSimString str(gLabelLookup[labelID].mName);
   str.append(".value");
   
   mpMovie->setVariable(str, text, GFxMovie::SV_Normal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BTextureHandle BFlashButtonPanel::getRenderTargetTexture()
{
   if (mpMovie)
      return mpMovie->mRenderTargetHandle;

   return cInvalidTextureHandle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::setLabelText(int labelID, const char* text)
{
   debugRangeCheck(labelID, eLabelTotal);
   mText[labelID] = text;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::setLabelVisible(int labelID, bool bVisible)
{
   debugRangeCheck(labelID, eLabelTotal);
   if (bVisible) 
      mEnableStates.set(labelID);
   else
      mEnableStates.unset(labelID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BFlashButtonPanel::setAllLabelsVisible(bool bVisible)
{
   for (int i = 0; i < eLabelTotal; i++)
      setLabelVisible(i, bVisible);
}