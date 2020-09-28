//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Communication Central interface.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_ICOMMUNICATIONCENTRAL_H
#define _AK_COMM_ICOMMUNICATIONCENTRAL_H

namespace AK
{
	namespace Comm
	{
		class ICommunicationCentralNotifyHandler;
		class ICommandChannelHandler;
		class INotificationChannel;

		/// Communication Central interface.
		/// \sa
		/// - \ref initialization_comm
		/// - \ref framerendering_comm
		/// - \ref termination_comm
		class ICommunicationCentral
		{
		public:

			/// Initialize central.
			/// This method has to be called once, before any other method call. 
			/// \return Initialization successful (\b true or \b false).
			/// \sa
			/// - \ref initialization_comm
			virtual bool Init( 
				ICommunicationCentralNotifyHandler* in_pNotifyHandler,	///< Pointer to notify interface (part of AK::Comm::IProxyFrameworkConnected).
				ICommandChannelHandler* in_pCmdChannelHandler			///< Pointer to channel interface (part of AK::Comm::IProxyFrameworkConnected).
				) = 0;

			/// Terminate central.
			/// This method has to be called once, before Destroy().
			/// \sa
			/// - \ref termination_comm
			virtual void Term() = 0;

			/// Destroy communication central.
			/// \sa
			/// - \ref termination_comm
			virtual void Destroy() = 0;

			/// Tell central to process communication at this time.
			/// This method has to be called periodically (usually once per game frame).
			/// \sa
			/// - \ref framerendering_comm
			virtual void Process() = 0;

			/// Get notification channel.
			/// \return Pointer to notification channel interface.
			virtual INotificationChannel* GetNotificationChannel() = 0;
		};
	}
}

#endif
