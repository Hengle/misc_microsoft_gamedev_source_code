//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Proxy Framework Factory.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_PROXYFRAMEWORKFACTORY_H
#define _AK_COMM_PROXYFRAMEWORKFACTORY_H

#include <AK/SoundEngine/Common/AkTypes.h>

namespace AK
{
	namespace Comm
	{
		class IProxyFrameworkConnected;
	}
}
/// Create an instance of the proxy framework.
/// \return Pointer to proxy framework interface.
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm
/// - AK::Comm::DEFAULT_MEMORY_POOL_ATTRIBUTES
AK::Comm::IProxyFrameworkConnected* AkCreateProxyFramework( AkMemPoolId in_pool );

#endif
