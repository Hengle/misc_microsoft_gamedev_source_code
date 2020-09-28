//==============================================================================
// xsound.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _XSOUND_H_
#define _XSOUND_H_

// System files
//#include <xact.h>
//#include <X3DAudio.h>

// Dependant files
#include "xsystem.h"
#include "configssound.h"

class ISoundInfoProvider;

// Functions

bool XSoundCreate();
void XSoundRelease();
void XSoundSetInterface(ISoundInfoProvider* pProvider);

// Typedefs
typedef uint           BCueIndex;
typedef uint           BCueHandle;
typedef uint           BWwiseObjectID;

// Constants
static const uint cInvalidCueIndex = 0;
static const uint cInvalidCueHandle = 0;
static const uint cInvalidWwiseObjectID  = (uint)(-1);

// Reference counted sound IDs
const DWORD cWSIDIndexMask   = 0x0003FFFF;
const DWORD cWSIDCountMask   = 0x7FFC0000;
const uint  cWSIDIndexMax    = 262143;
const uint  cWSIDCountMax    = 8191;
#define WSIDINDEX(id)        ((id) & cWSIDIndexMask) 
#define WSIDCOUNT(id)        (((id) & cWSIDCountMask) >> 18)
#define WSID(count, index)   (((count << 18) & cWSIDCountMask) | (index))

//-- 0x0 and 0x1 are reserved by WWise
const long cWWisePlayer1ID = 2;
const long cWWisePlayer2ID = 3;

#endif
