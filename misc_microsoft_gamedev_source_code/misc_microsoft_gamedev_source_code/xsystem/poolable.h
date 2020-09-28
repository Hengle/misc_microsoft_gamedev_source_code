//==============================================================================
// poolable.h
//
// Interface that is used by the pool managers to initialize, reset and manage
// objects in the pool.
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

class IPoolable
{
public:
   virtual void onAcquire() = 0;
   virtual void onRelease() = 0;
};