//==============================================================================
// uimenu.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "xmlreader.h"
#include "flashgateway.h"

// Forwards
class BUIInputHandler;
class BUIMenu;


//==============================================================================
// BUIMenu
//==============================================================================
class BUIMenuItem
{
public:
   BUIMenuItem();
   ~BUIMenuItem();

   bool                          load(BXMLNode node);
   const BUString&               getDisplayString() const;

//   <Item name="Campaign"   _locID="$$$$" command="gotoCampaign"/>
   BString  mName;
   // long     mLocStringIndex; // set up on lookup
   BUString mText;
   BUString mHelpText;
   BString  mCommand;
   int      mCommandID;
   int8     mID;

   bool     mIsVisible:1;
   bool     mIsActive:1;

protected:
   BString  mNextMenu;
};

//==============================================================================
// BUIMenu
//==============================================================================
class BUIMenuStrip
{
   public:
                                    BUIMenuStrip();
                                    ~BUIMenuStrip();

      //<Menu name="MainMenu" previousMenu="" previousItem=""  _locID="$$$$">
      //   <Item name="Campaign"   _locID="$$$$" command="gotoCampaign"/>

      bool                          load(BXMLNode node);

      void                          clearItems() { mItems.clear(); }
      long                          getCurrentItemIndex() const { return mCurrentItem; }

      BUIMenuItem*                  getCurrentMenuItem();

      long                          getPreviousItemIndex();
      long                          getNextItemIndex();

      long                          getNumberItems() const { return mItems.getNumber(); }
      void                          setNumberItems(int count);
      BUIMenuItem*                  getItem(long index);
      void                          setName(const char * name) { mName.set(name); }
      const BString&                getName() const { return mName; }

      void                          setCurrentItem(long index);

   protected:
      BString                       mName;
      long                          mCurrentItem;
      BDynamicSimArray<BUIMenuItem> mItems;
};

class IMenuEventHandler
{
public:
   // this may change
   virtual bool menuEvent(const BSimString& command, BUIMenu* pMenu)=0;
};


//==============================================================================
// BUIMenu
//==============================================================================
class BUIMenu : public IInputControlEventHandler
{
public:
   BUIMenu();
   ~BUIMenu();

   bool     init(BFlashMovieInstance* pMovie, const char* inputDefinitionFile);
   bool     load(const char* filename);

   bool     handleInput(long port, long event, long controlType, BInputEventDetail& detail);
   void     resetInput();

   void     setEventHandler(IMenuEventHandler* eventHandler) { mpEventHandler = eventHandler; }


   // IInputControlEventHandler 
   bool     executeInputEvent(long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl);
   void     enterContext(const char* contextName);

   void     setMovieClip(const char * movieClipName) { mMovieClipName.set(movieClipName); }
   const BSimString& getMovieClip() const { return mMovieClipName; }

   int      getMenuStripCount() { return mMenuStrips.getNumber(); }
   void     setMenuStripCount(int numStrips) { mMenuStrips.setNumber(numStrips); }
   BUIMenuStrip* getMenuStrip(int index);
   bool     setActiveStrip(int index);
   BUIMenuStrip*  getCurrentMenuStrip();

   BUIMenuItem* getCurrentMenuItem();

   int      getFocus();
   void     setFocus(int i);
   void     clearItemFocus();

   void     positionMovieClip(int slot, const char * movieClipName);

   void     setIsVisible(bool bVisible);
   bool     isVisible() const { return mVisible; };

   void     setUseSelectButton(bool bUseSelectButton) { mUseSelectButton = bUseSelectButton; }
   bool     getUseSelectButton() { return mUseSelectButton; }

   void     show();
   void     hide();


protected:
   bool     sendEvent(const BSimString& command);
   void     populateMenu();

   bool     navigateDown();
   bool     navigateUp();


   // helper methods
   void getASName(BSimString& fullName, const char* methodName);
   void invokeActionScript(const char* method, const GFxValue* pArgs, int numArgs);

   // flash calls
   // Init Menu
   void setNumberMenuItems(int8 i);
   void setMenuItems();
   bool changeSelection(bool goUp);
   // easeIn()

   BDynamicSimArray<BUIMenuStrip>   mMenuStrips;
   int                              mCurrentStrip;

   // Input handler for the menu
   BUIInputHandler*     mpInputHandler;
   IMenuEventHandler*   mpEventHandler;

   BSimString           mMovieClipName;

   // The flash movie where the menu resides
   BFlashMovieInstance*       mpMovie;

   bool                 mVisible:1;
   bool                 mWrap:1;
   bool                 mUseSelectButton:1;
};

