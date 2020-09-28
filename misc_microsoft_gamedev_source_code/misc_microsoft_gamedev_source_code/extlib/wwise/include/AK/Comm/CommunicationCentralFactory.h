//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Communication Central Factory.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_COMMUNICATIONCENTRALFACTORY_H
#define _AK_COMM_COMMUNICATIONCENTRALFACTORY_H

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>

namespace AK
{
	/// Communication namespace (for communication between Wwise and the game)
	namespace Comm
	{
		/// Flags to be passed to AkMemoryMgr::CreatePool() when creating the Communication memory pool.
		/// \sa
		/// - \ref initialization_comm
		/// - AkMemoryMgr::CreatePool()
		/// - AkMemPoolAttributes
		/// - AkCreateCommunicationCentral()
		/// - AkCreateProxyFramework()
#ifdef RVL_OS
		static const AkUInt32 DEFAULT_MEMORY_POOL_ATTRIBUTES = AkMallocMEM2;
#else
		static const AkUInt32 DEFAULT_MEMORY_POOL_ATTRIBUTES = AkMalloc;
#endif

		class ICommunicationCentral;
	}
}

/// Create an instance of the communication central.
/// \return Pointer to communication central interface.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm
/// - AK::Comm::DEFAULT_MEMORY_POOL_ATTRIBUTES
AK::Comm::ICommunicationCentral* AkCreateCommunicationCentral(
	AkMemPoolId in_pool		///< What memory pool to allocate from.
	);

#endif // _AK_COMM_COMMUNICATIONCENTRALFACTORY_H
