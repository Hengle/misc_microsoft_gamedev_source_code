//============================================================================
//
//  xsystem.h
//  
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================
#pragma once

#include "xcore.h"

typedef void* BHandle;
typedef long BSoundHandle;
const BSoundHandle SOUND_INVALID_HANDLE = -1;
extern bool gFinalBuild;

//#include "xfs.h"

#include "xsystemtimegettime.h"

//-- Containers
#include "Array.h"
#include "Stack.h"
#include "Set.h"
#include "containers\PointerList.h"
#include "CopyList.h"
#include "ObjectPool.h"

#include "trace.h"
#include "math\vector.h"
#include "SphereCoord.h"
#include "math\matrix.h"
#include "random.h"

//-- Other stuff
#include "logmanager.h"
#include "FileSystem.h"
#include "netutil.h"
#include "string\bsnprintf.h"
#include "timelineprofilersample.h"

// some typedefs
typedef BDynamicArray<BVector> BDynamicVectorArray;
typedef BDynamicArray<BVector4> BDynamicVector4Array;
typedef BSmallDynamicSimArray<BVector, 16> BLocationArray;
typedef BSmallDynamicSimArray<BVector, 16> BVectorArray;
typedef BDynamicArray<BVector, 16, BDynamicArraySimHeapAllocator> BDynamicSimVectorArray;
typedef BDynamicArray<BVector4, 16, BDynamicArraySimHeapAllocator> BDynamicSimVector4Array;
