//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Declaration of ICommunicationCentralNotifyHandler interface
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_ICOMMUNICATIONCENTRALNOTIFYHANDLER_H
#define _AK_COMM_ICOMMUNICATIONCENTRALNOTIFYHANDLER_H

namespace AK
{
	namespace Comm
	{
		/// Communication central notification handler
		class ICommunicationCentralNotifyHandler
		{
		public:
			virtual void ControllerDisconnected() = 0;
		};
	}
}

#endif
