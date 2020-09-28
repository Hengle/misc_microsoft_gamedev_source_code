// ========================================================================
// $File: //jeffr/granny/rt/granny_thread_safety.cpp $
// $DateTime: 2007/07/23 15:42:28 $
// $Change: 15572 $
// $Revision: #5 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_THREAD_SAFETY_H)
#include "granny_thread_safety.h"
#endif

#if !defined(GRANNY_PARAMETER_CHECKING_H)
#include "granny_parameter_checking.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

#undef SubsystemCode
#define SubsystemCode ThreadSafetyLogMessage

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

thread_id_callback *ThreadIDCallback = 0;

END_GRANNY_NAMESPACE;

static bool AllowGlobalStateChanges = true;
static uint32 ThreadMakingChanges   = 0xFFFFBEEF;  // randomish, don't use 0 or -1


void GRANNY
SetAllowGlobalStateChanges(bool Allowed)
{
    if (Allowed)
    {
        CheckCondition(AllowGlobalStateChanges == false, return);
        AllowGlobalStateChanges = true;
        ThreadMakingChanges = GetThreadID();
    }
    else
    {
        AllowGlobalStateChanges    = false;
        ThreadMakingChanges = 0xFFFFBEEF;
    }
}

bool GRANNY
GetAllowGlobalStateChanges()
{
    return (AllowGlobalStateChanges);
}


bool GRANNY
ThreadAllowedToCallGranny()
{
    if (ThreadIDCallback && AllowGlobalStateChanges)
    {
        return GetThreadID() == ThreadMakingChanges;
    }

    return true;
}


thread_id_callback *GRANNY
SetThreadIDCallback(thread_id_callback *Callback)
{
    thread_id_callback *OldCallback = ThreadIDCallback;

    ThreadIDCallback = Callback;

    // If the allowglobalstatechanges is set, then we assume that the
    // correct thread id is the current thread
    if (ThreadIDCallback != 0 && AllowGlobalStateChanges)
    {
        ThreadMakingChanges = (*Callback)();
    }

    return OldCallback;
}
