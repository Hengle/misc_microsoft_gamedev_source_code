//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Declaration of IProxyFrameworkConnected interface
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_IPROXYFRAMEWORKCONNECTED_H
#define _AK_COMM_IPROXYFRAMEWORKCONNECTED_H

#include <AK/Comm/IProxyFramework.h>
#include <AK/Comm/ICommandChannelHandler.h>
#include <AK/Comm/ICommunicationCentralNotifyHandler.h>

namespace AK
{
	namespace Comm
	{
		/// Connected proxy framework
		/// \sa
		/// - \ref initialization_comm
		/// - \ref framerendering_comm
		/// - \ref termination_comm
		class IProxyFrameworkConnected : public IProxyFramework
										, public ICommandChannelHandler
										, public ICommunicationCentralNotifyHandler
		{
		};
	}
}

#endif
