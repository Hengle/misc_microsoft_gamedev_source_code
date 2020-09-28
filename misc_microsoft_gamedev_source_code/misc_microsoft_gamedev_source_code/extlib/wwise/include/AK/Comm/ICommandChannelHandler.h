//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Declaration of ICommandChannelHandler interface
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_ICOMMANDCHANNELHANDLER_H
#define _AK_COMM_ICOMMANDCHANNELHANDLER_H

#include <AK/SoundEngine/Common/AkTypes.h>

namespace AK
{
	class IWriteBytes;

	namespace Comm
	{
		/// Command channel handler
		class ICommandChannelHandler
		{
		public:
			virtual void HandleExecute( const AkUInt8* in_pData, AK::IWriteBytes* in_pReturnData ) = 0;
		};
	}
}

#endif
