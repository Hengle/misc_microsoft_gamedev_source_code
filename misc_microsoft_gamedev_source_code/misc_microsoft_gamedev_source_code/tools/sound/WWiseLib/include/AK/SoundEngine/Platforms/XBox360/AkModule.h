//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Audiokinetic's implementation-specific definitions and factories of 
/// overridable modules.
/// Xbox 360 implementation.
#ifndef _AK_MODULE_H_
#define _AK_MODULE_H_

#include <AK/SoundEngine/Common/AkTypes.h>

/// \name Audiokinetic Memory Manager's implementation-specific definitions.
//@{
/// Memory Manager's initialization settings.
/// \sa AK::MemoryMgr
struct AkMemSettings
{
    AkUInt32 uMaxNumPools;              ///< Maximum number of memory pools.
};
//@}

namespace AK
{
    /// \name Audiokinetic implementation-specific modules factories.
    //@{
	namespace MemoryMgr
	{
	    /// Memory Manager initialization.
	    /// \sa AK::MemoryMgr
		extern AKRESULT Init(
        AkMemSettings * in_pSettings        ///< Memory manager initialization settings.
        );
	}
    //@}
}

#endif //_AK_MODULE_H_
