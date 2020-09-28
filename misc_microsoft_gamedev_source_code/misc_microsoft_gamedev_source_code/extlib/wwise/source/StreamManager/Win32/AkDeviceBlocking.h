//////////////////////////////////////////////////////////////////////
//
// AkDeviceBlocking.h
//
// Win32 specific Blocking Scheduler Device implementation.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_DEVICE_BLOCKING_H_
#define _AK_DEVICE_BLOCKING_H_

#include "AkDeviceBase.h"

namespace AK
{

    //-----------------------------------------------------------------------------
    // Name: CAkDeviceBlocking
    // Desc: Implementation of the Blocking Scheduler device.
    //-----------------------------------------------------------------------------
    class CAkDeviceBlocking : public CAkDeviceBase
    {
    public:

        CAkDeviceBlocking();
        virtual ~CAkDeviceBlocking();

    protected:

        // This device's implementation of PerformIO().
        void PerformIO();

        // Execute task chosen by scheduler.
        void ExecuteTask( 
            CAkStmTask * in_pTask,
            void * in_pBuffer
            );
    };
}

#endif //_AK_DEVICE_BLOCKING_H_
