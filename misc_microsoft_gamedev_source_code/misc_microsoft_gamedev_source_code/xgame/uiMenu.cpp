//==============================================================================
// uilist.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "uimenu.h"
#include "database.h"
#include "UIInputHandler.h"
#include "soundmanager.h"
#include "inputcontrol.h"

// Constants


//==============================================================================
// BUIMenuItem::BUIMenuItem
//==============================================================================
BUIMenuItem::BUIMenuItem() : 
mIsVisible(true),
mIsActive(true)
{
}

//==============================================================================
// BUIMenuItem::~BUIMenuItem
//==============================================================================
BUIMenuItem::~BUIMenuItem()
{
}

//==============================================================================
// BUIMenuItem::load
//==============================================================================
bool BUIMenuItem::load(BXMLNode root)
{
   //   <Item name="Campaign"   _locID="$$$$" command="gotoCampaign"/>
   // BString  mName;
   // long     mLocStringIndex; // set up on lookup
   // BString  mCommand;

   if (!root)
      return (false);

   //BSimString szName;
   if (!root.getAttribValueAsString("name", mName))
      return (false);

   // Get the loc string table index
   long locID = -1;
   if (!root.getAttribValueAsLong("_locID", locID))
      return false;

   // By caching the value, we can then change the text inside the menu dynamically
//   mLocStringIndex = gDatabase.getLocStringIndex(locID);       // lookup and cache the index
   mText.set(gDatabase.getLocStringFromID(locID));

   // get the command for this menu item
   root.getAttribValueAsString("command", mCommand);

   // Get the nextMenu string for this menu item.
   root.getAttribValueAsString("nextMenu", mNextMenu);

   return true;
}

//==============================================================================
// BUIMenuItem::getDisplayString
//==============================================================================
const BUString& BUIMenuItem::getDisplayString() const
{
   return mText;
   // return gDatabase.getLocStringFromIndex(mLocStringIndex);
}

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------


//==============================================================================
// BUIMenuStrip::BUIMenuStrip
//==============================================================================
BUIMenuStrip::BUIMenuStrip() :
   mCurrentItem(-1),
   mItems()
{
}

//==============================================================================
// BUIMenuStrip::~BUIMenuStrip
//==============================================================================
BUIMenuStrip::~BUIMenuStrip()
{
}

//==============================================================================
// BUIMenuStrip::load
//==============================================================================
bool BUIMenuStrip::load(BXMLNode root)
{
   if (!root)
      return (false);

   //<MenuStrip name="MainMenu" previousMenu="" previousItem=""  _locID="$$$$">
   //   <Item name="Campaign"   _locID="$$$$" command="gotoCampaign"/>

   //BSimString szName;
   if (!root.getAttribValueAsString("name", mName))
      return (false);

   // Iterate over the children -> <MenuItems>
   mItems.setNumber(root.getNumberChildren());
   int count = 0;
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      BXMLNode child(root.getChild(i));

      const BPackedString name2(child.getName());

      if(name2==B("Item"))
      {
         count++;
         // add a child node
         if (!mItems[i].load(child))
            return false;
      }
   }
   if (mItems.getNumber()>count)
      mItems.setNumber(count);      // trim if necessary

   return true;
}

//==============================================================================
// BUIMenuStrip::getItem
//==============================================================================
BUIMenuItem* BUIMenuStrip::getItem(long index)
{ 
   if ( (index<0) || (index>=mItems.getNumber()))
      return NULL;

   return &(mItems[index]);
}

//==============================================================================
// BUIMenuStrip::setNumberItems
//==============================================================================
void BUIMenuStrip::setNumberItems(int count)
{
   mItems.setNumber(count);
}

//==============================================================================
// BUIMenuStrip::getPreviousItemIndex
//==============================================================================
long BUIMenuStrip::getPreviousItemIndex()
{
   if ( (mCurrentItem<0) || (mCurrentItem>=mItems.getNumber()))
      return 0;

   long previous = mCurrentItem-1;
   if (previous<0)
      previous = mItems.getNumber()-1;

   return previous;
}

//==============================================================================
// BUIMenuStrip::getNextItemIndex
//==============================================================================
long BUIMenuStrip::getNextItemIndex()
{
   if ( (mCurrentItem<0) || (mCurrentItem>=mItems.getNumber()))
      return 0;

   long next = mCurrentItem+1;
   if (next>=mItems.getNumber())
      next = 0;

   return next;
}


//==============================================================================
// BUIMenuStrip::setCurrentItem
//==============================================================================
void BUIMenuStrip::setCurrentItem(long index)
{
   if(mItems.getNumber()==0)
   {
      mCurrentItem=-1;
      return;
   }
   if(index<0)
   {
      mCurrentItem=-1;
      return;
   }
   if(index>=mItems.getNumber())
      mCurrentItem=mItems.getNumber()-1;
   else
      mCurrentItem=index;
}

//==============================================================================
// BUIMenuStrip::getCurrentMenuItem
//==============================================================================
BUIMenuItem* BUIMenuStrip::getCurrentMenuItem()
{
   if ( (mCurrentItem<0) || (mCurrentItem>=mItems.getNumber()))
      return NULL;

   return &(mItems[mCurrentItem]);
}




//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------



//==============================================================================
// BUIMenu::BUIMenu
//==============================================================================
BUIMenu::BUIMenu() : 
   mpInputHandler(NULL),
   mpEventHandler(NULL),
   mVisible(true),
   mWrap(false),
   mUseSelectButton(false),
   mpMovie(NULL)
{
   mMovieClipName.set("");
}

//==============================================================================
// BUIMenu::~BUIMenu
//==============================================================================
BUIMenu::~BUIMenu()
{
   if (mpInputHandler)
   {
      delete mpInputHandler;
      mpInputHandler=NULL;
   }
}

//==============================================================================
// BUIMenu::load
//==============================================================================
bool BUIMenu::init(BFlashMovieInstance* pMovie, const char* inputDefinitionFile)
{
   mpMovie=pMovie;

   mpInputHandler = new BUIInputHandler();
   mpInputHandler->loadControls(inputDefinitionFile, this);
   mpInputHandler->enterContext("Main");

   return true;
}

//==============================================================================
// BUIMenu::load
//==============================================================================
bool BUIMenu::load(const char * filename)
{
   BSimString path;
   path.set(filename);

   BXMLReader reader;
   if(!reader.load(cDirProduction, path))
   {
      BASSERT(0);
      return false;
   }

   BXMLNode root(reader.getRootNode());
   mMenuStrips.setNumber(root.getNumberChildren());
   int count = 0;
   for(long i=0; i<root.getNumberChildren(); i++)
   {
      BXMLNode child(root.getChild(i));
      const BPackedString name2(child.getName());

      if(name2==B("MenuStrip"))
      {
         count++;
         if (!mMenuStrips[i].load(child))
            return false;
      }
   }

   if (mMenuStrips.getNumber()> count)
      mMenuStrips.setNumber(count);       // trim if necessary

   return true;
}

//==============================================================================
// BUIMenu::resetInput
//==============================================================================
void BUIMenu::resetInput()
{
   mpInputHandler->reset();
}

//==============================================================================
// BUIMenu::handleInput
//==============================================================================
bool BUIMenu::handleInput(long port, long event, long controlType, BInputEventDetail& detail)
{
   return mpInputHandler->handleInput(port, event, controlType, detail);
}

// IInputControlEventHandler 
//==============================================================================
// BUIMenu::executeInputEvent
//==============================================================================
bool BUIMenu::executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl)
{
   bool handled = false;
   if (command == "navUp")
   {
      if (controlType==cStickLeftUp)
      {
         if (detail.mY == 0.0f)
            return false;
      }
      handled=navigateUp();
   }
   else if (command == "navDown")
   {
      if (controlType==cStickLeftDown)
      {
         if (detail.mY == 0.0f)
            return false;
      }
      handled=navigateDown();
   }   
   else
   {
      // accept, cancel
      handled = sendEvent(command);
   }

   if (handled)
   {
      // execute the sound that goes with this command.
      gSoundManager.playCue(pInputControl->getSoundEvent().getPtr());
   }
   return handled;
}

//==============================================================================
// BUIMenu::sendEvent
//==============================================================================
bool BUIMenu::sendEvent(const BSimString& command)
{
   if (mpEventHandler)
      return mpEventHandler->menuEvent(command, this);

   return false;
}

//==============================================================================
// BUIMenu::enterContext
//==============================================================================
void BUIMenu::enterContext(const char* contextName)
{
   if (mpInputHandler)
      mpInputHandler->enterContext(contextName);
}

//==============================================================================
// BUIMenu::getMenuStrip
//==============================================================================
BUIMenuStrip* BUIMenu::getMenuStrip(int index)
{
   if ( (index<0) || (index>mMenuStrips.getNumber()) )
      return NULL;

   return &(mMenuStrips[index]);
}

//==============================================================================
// BUIMenu::setActiveStrip
//==============================================================================
bool BUIMenu::setActiveStrip(int index)
{
   if ( (index<0) || (index>=mMenuStrips.getNumber()) )
      return false;

   mCurrentStrip = index;

   // fixme - repopulate the menu
   populateMenu();

   //setFocus(0);

   return true;
}

//==============================================================================
// BUIMenu::populateMenu
//==============================================================================
void BUIMenu::populateMenu()
{
   BUIMenuStrip * pStrip = getCurrentMenuStrip();
   if (!pStrip)
      return;

   // call flash
   setNumberMenuItems((int8)pStrip->getNumberItems());
   setMenuItems();
}



//==============================================================================
// BUIMenu::navigateDown
//==============================================================================
bool BUIMenu::navigateDown()
{
   return changeSelection(false);
}


//==============================================================================
// BUIMenu::navigateUp
//==============================================================================
bool BUIMenu::navigateUp()
{
   return changeSelection(true);
}

//==============================================================================
// BUIMenu::getCurrentMenuStrip
//==============================================================================
BUIMenuStrip*  BUIMenu::getCurrentMenuStrip()
{
   if ( (mCurrentStrip<0) || (mCurrentStrip>=mMenuStrips.getNumber()) )
      return NULL;

   return &(mMenuStrips[mCurrentStrip]);
}

//==============================================================================
// BUIMenu::getFocus
//==============================================================================
int BUIMenu::getFocus()
{
//-- FIXING PREFIX BUG ID 1286
   const BUIMenuStrip* pStrip = getCurrentMenuStrip();
//--
   if (!pStrip)
      return -1;

   return pStrip->getCurrentItemIndex();
}

//==============================================================================
// BUIMenu::getCurrentMenuItem
//==============================================================================
BUIMenuItem* BUIMenu::getCurrentMenuItem()
{
   BUIMenuStrip* pStrip = getCurrentMenuStrip();
   if (!pStrip)
      return NULL;

   return pStrip->getCurrentMenuItem();
}


//==============================================================================
// BUIMenu::changeSelection
//==============================================================================
bool BUIMenu::changeSelection(bool goUp)
{
   BUIMenuStrip* pStrip = getCurrentMenuStrip();
   if (!pStrip)
      return false;

   int newItem = 0;
   if (goUp)
   {
      if (!mWrap && pStrip->getCurrentItemIndex() == 0)
      {
         BSimString event("topVertSlotReached");
         return sendEvent(event);
      }
      newItem = pStrip->getPreviousItemIndex();
   }
   else
   {
      if (!mWrap && (pStrip->getCurrentItemIndex() == (pStrip->getNumberItems()-1)))
      {
         BSimString event("bottomVertSlotReached");
         return sendEvent(event);
      }
      newItem = pStrip->getNextItemIndex();
   }

   setFocus(newItem);

   return true;
}

//==============================================================================
// BUIMenu::setNumberMenuItems
//==============================================================================
void BUIMenu::setNumberMenuItems(int8 i)
{
   GFxValue values[1];
   values[0].SetNumber(i);

   invokeActionScript("setNumberMenuItems", values, 1);
}

//==============================================================================
// BUIMenu::setMenuItems
//==============================================================================
void BUIMenu::setMenuItems()
{

   BUIMenuStrip* pMenuStrip = getCurrentMenuStrip();
   if (!pMenuStrip)
      return;


   for (int i=0; i<pMenuStrip->getNumberItems(); i++)
   {
//-- FIXING PREFIX BUG ID 1288
      const BUIMenuItem* pItem = pMenuStrip->getItem(i);
//--

      if (!pItem)
         continue;

      // treat both the same for right now.
      if ( (!pItem->mIsVisible) || (!pItem->mIsActive) )
      {
         GFxValue values[2];
         values[0].SetNumber(i);
         values[1].SetBoolean(false);
         invokeActionScript("setMenuItemVisible", values, 2);
      }
      else
      {
         GFxValue values[2];
         values[0].SetNumber(i);
         values[1].SetStringW( pItem->getDisplayString().getPtr() );
         invokeActionScript("setMenuItemData", values, 2);
      }
   }
}

//==============================================================================
// BUIMenu::clearItemFocus
//==============================================================================
void BUIMenu::clearItemFocus()
{
   BUIMenuStrip* pStrip = getCurrentMenuStrip();
   if (!pStrip)
      return;

   pStrip->setCurrentItem(-1);

   GFxValue values[1];
   values[0].SetNumber(0);
   invokeActionScript("clearMenuItemFocus", values, 1);
}

//==============================================================================
// BUIMenu::setFocus
//==============================================================================
void BUIMenu::setFocus(int i)
{
   BUIMenuStrip* pStrip = getCurrentMenuStrip();
   if (!pStrip)
      return;

   pStrip->setCurrentItem(i);

   GFxValue values[2];
   values[0].SetNumber(i);
   values[1].SetBoolean(mUseSelectButton);

   invokeActionScript("setMenuItemFocus", values, 2);

   BSimString event("focusChanged");
   sendEvent(event);
}


//==============================================================================
// BUIMenu::invokeActionScript
//==============================================================================
void BUIMenu::invokeActionScript(const char* method, const GFxValue* pArgs, int numArgs)
{
   BSimString function;
   getASName(function, method);
   mpMovie->invokeActionScript(function.getPtr(), pArgs, numArgs);
}

//==============================================================================
// BUIMenu::getASName
//==============================================================================
void BUIMenu::getASName(BSimString& fullName, const char* methodName)
{
   if (mMovieClipName.isEmpty())
      fullName.set(methodName);
   else
      fullName.format("%s.%s", mMovieClipName.getPtr(), methodName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIMenu::show()
{
   // based on the civ and leader, figure out who the selected leader is and set him.
   setIsVisible(true);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIMenu::hide()
{ 
   setIsVisible(false); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIMenu::setIsVisible(bool bVisible) 
{ 
   if (mVisible == bVisible)
      return;

   mVisible = bVisible; 

   GFxValue values[1];
   values[0].SetBoolean(mVisible);

   invokeActionScript("setIsVisible", values, 1);

};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BUIMenu::positionMovieClip(int slot, const char * movieClipName)
{
   GFxValue values[2];
   values[0].SetNumber(slot);
   values[1].SetString(movieClipName);

   invokeActionScript("positionMovieClip", values, 2);
}

