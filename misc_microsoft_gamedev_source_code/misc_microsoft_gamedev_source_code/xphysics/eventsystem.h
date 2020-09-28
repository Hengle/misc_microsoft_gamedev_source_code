//============================================================================
// eventsystem.h
//
// Copyright (c) 2003, Ensemble Studios
//============================================================================

#ifndef _EVENT_SYSTEM
#define _EVENT_SYSTEM

// Forward declarations
class BEventSystem;

// Extern to global
extern BEventSystem gEventSystem;

// Event type helper macros
#define DECLARE_EVENT_TYPE(name)  long cEvent##name=-1;
#define REGISTER_EVENT_TYPE(name) { long index=gEventSystem.registerEventType("" #name ""); if(index==-1) return false; cEvent##name=index; }

//============================================================================
// BEventSystem
//============================================================================
class BEventSystem
{
   public:
                           BEventSystem();
                           ~BEventSystem();

      long                 registerEventType(const char* name);
      long                 lookupEventType(const char* name);
      long                 getEventTypeCount() { return mEventTypes.getNumber(); }

   protected:
      BDynamicSimArray<const char*>  mEventTypes;

};

#endif