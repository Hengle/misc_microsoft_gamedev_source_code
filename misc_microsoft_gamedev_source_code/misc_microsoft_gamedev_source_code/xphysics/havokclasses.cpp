//==============================================================================
// havokclasses.cpp
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

#include "common.h"
//#include "havokclasses.h"

#define HK_CLASSES_FILE "havokclasses.h"
//#define HK_CLASSES_FILE <Common/Serialize/ClassList/hkPhysicsClasses.h>
#include <Common/Serialize/Util/hkBuiltinTypeRegistry.cxx>

// Generate a custom list to trim memory requirements
#define HK_COMPAT_FILE <Common/Compat/hkCompatVersions.h>
#include <Common/Compat/hkCompat_None.cxx>
