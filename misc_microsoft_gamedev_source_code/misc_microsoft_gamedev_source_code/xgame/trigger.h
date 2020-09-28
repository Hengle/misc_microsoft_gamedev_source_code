//==============================================================================
// trigger.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xsystem
#include "xmlreader.h"
#include "gamefilemacros.h"

// forward declarations
class BTrigger;
class BTriggerCondition;
class BTriggerEffect;
class BTriggerScript;

typedef uint BTriggerID;
__declspec(selectany) extern const BTriggerID cInvalidTriggerID = 0xFFFFFFFF;


//==============================================================================
// class BTrigger
//==============================================================================
class BTrigger : public IPoolable
{
   public:
      #ifndef BUILD_FINAL
         enum
         {
            cPerformanceWarningThreshold = 1000,
            cInfiniteLoopWarningThreshold = 50000,
         };
         const BSimString& getName() const { return(mName); }
      #endif

      BTrigger(){}
      ~BTrigger(){}
      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BTrigger, 6);

      bool loadFromXML(BXMLNode  node);
      bool timeToEvaluate(DWORD currentUpdateTime);
      void updateAsyncConditions();
      bool evaluateConditions(DWORD currentUpdateTime);
      void fireEffects(bool onTrue);
      void setAsyncConditionState(long conditionID, bool state);
      void resetEvaluateCount() { mEvaluateCount = 0; }
      bool hasEvaluationsRemaining();


      // Accessors
      bool isStartActive() const { return (mbStartActive); }
      bool isConditional() const { return (mbConditional); }
      BTriggerID getID() const { return (mID); }
      void setID(BTriggerID v) { mID = v; }
      BTriggerID getEditorID() const { return (mEditorID); }
      DWORD getActivatedTime() const { return (mActivatedTime); }
      DWORD getEvaluateTime() const { return (mNextEvaluateTime); }
      void onActivated();
      
      void setParentTriggerScript(BTriggerScript* pParentScript) { mpParentTriggerScript = pParentScript; }
      BTriggerScript* getParentTriggerScript() { return (mpParentTriggerScript); }

      uint getNumberConditions() const { return mConditions.size(); }
      BTriggerCondition* getConditionByIndex(uint index) { return mConditions[index]; }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
                                                               // 52 bytes total
      #ifndef BUILD_FINAL
         BSimString mName;
         long mGroupID;
      #endif
      BSmallDynamicSimArray<BTriggerCondition*> mConditions;   // 8 bytes
      BSmallDynamicSimArray<BTriggerEffect*> mEffectsOnTrue;   // 8 bytes
      BSmallDynamicSimArray<BTriggerEffect*> mEffectsOnFalse;  // 8 bytes
      
      BTriggerID mID;                                          // 4 bytes - The unique ID of the trigger (so we can compare ID's not names.)
      BTriggerID mEditorID;                                    // 4 bytes - Editor side id
      BTriggerScript* mpParentTriggerScript;                   // 4 bytes - The parent trigger script.
      DWORD mActivatedTime;                                    // 4 bytes - The game time this trigger became active
      DWORD mNextEvaluateTime;                                 // 4 bytes - How long has it been since the last condition evaluation.
      DWORD mEvaluateFrequency;                                // 4 bytes - How long do we wait between evaluation times.
      DWORD mEvaluateCount;                                    // 4 bytes - How many times has the trigger been updated THIS UPDATE frame.  (Reset to 0 each update.)
      DWORD mEvaluateLimit;                                    // 4 bytes - How many times can the trigger be evaluated per update. (0 means unlimited)

      bool mbStartActive   : 1;                                // 1 byte - Does this trigger start active?
      bool mbConditional   : 1;                                //          Is this trigger a conditional?
      bool mbOrConditions  : 1;                                //          Do we OR the conditions on this trigger?
};