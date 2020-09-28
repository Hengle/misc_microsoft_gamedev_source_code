/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

///////////////////////////////////////////////////////////////////////
//
// AkCommonBase.h
//
// Declaration of the Basic Common base template
//
///////////////////////////////////////////////////////////////////////
#ifndef _COMMON_BASE_H_
#define _COMMON_BASE_H_

#include <AK/Tools/Common/AkObject.h>

class CAkCommonBase : public CAkObject
{
public:
    CAkCommonBase()
		: m_lRef( 1 )
	{
	}

    // Objects must be fully released before we're here.
    // This assert usually means that someone called
    // "delete" on an object instead of Release().
	~CAkCommonBase() 
	{
		AKASSERT(!m_lRef);
	}

	// STRICTLY FOR USE BY CAkIndexItem::GetPtrAndAddRef
	// (avoid inner lock for performance)
	AkForceInline AkUInt32 AddRefUnsafe()
	{
		return ++m_lRef; 
	}

	virtual AkUInt32 AddRef() = 0; 
    virtual AkUInt32 Release() = 0; 

protected:
	AkInt32 m_lRef;
};
#endif
