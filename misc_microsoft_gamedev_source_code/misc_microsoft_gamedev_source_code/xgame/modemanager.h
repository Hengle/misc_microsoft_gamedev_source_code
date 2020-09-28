//==============================================================================
// modemanager.h
//
// Copyright (c) 2002-2007, Ensemble Studios
//==============================================================================
#pragma once

// Forward declarations
class BMode;
class BModeGame;
class BModeMenu;
class BModeManager;
class BInputEventDetail;

// Global pointer to the one BModeManager object
extern BModeManager gModeManager;

//==============================================================================
// BModeManager
//==============================================================================
class BModeManager
{
   public:

      // Each cMode* enum needs to have a corresponding mode allocated in setup()
      // when modes are deprecated, both the enum and allocation need to be removed.
      enum
      {
         cModeIntro,
         cModeMenu,
         cModeGame,
         cModeViewer,
         cModeCalibrate,
         cModeFlash,
         cModeCinematic,
         cModeModelView,
         cModeCampaign2,
         cModePartyRoom2,
         //cModeServiceRecord,
         cModeCount
      };

                              BModeManager();
                              ~BModeManager();

      bool                    setup();
      void                    shutdown();

      bool                    update();
      void                    frameStart();
      void                    frameEnd();
      void                    renderBegin();
      void                    render();
      void                    renderEnd();
      bool                    handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      void                    setMode(long modeType);

      BMode*                  getMode();
      long                    getModeType() const;

      BMode*                  getMode(long modeType);

      BModeMenu*              getModeMenu() { return (mModes.getNumber()>0 ? (BModeMenu*)mModes[cModeMenu] : NULL); }
      BModeGame*              getModeGame() { return (mModes.getNumber()>0 ? (BModeGame*)mModes[cModeGame] : NULL); }

      bool                    inModeMenu() { return (mModes.getNumber()>0 && mMode==mModes[cModeMenu]); }
      bool                    inModeGame() { return (mModes.getNumber()>0 && mMode==mModes[cModeGame]); }

      static void             shutdownFPSLogFile(bool reopen);

   protected:
      bool                    addMode(BMode* mode);

      BDynamicSimArray<BMode*>    mModes;

      BMode*                  mMode;

};