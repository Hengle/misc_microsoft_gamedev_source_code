//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Proxy Framework interface.
/// \sa
/// - \ref initialization_comm
/// - \ref framerendering_comm
/// - \ref termination_comm

#ifndef _AK_COMM_IPROXYFRAMEWORK_H
#define _AK_COMM_IPROXYFRAMEWORK_H

namespace AK
{
	namespace Comm
	{
		class INotificationChannel;
		class IRendererProxy;

		/// Proxy Framework interface.
		/// \sa
		/// - \ref initialization_comm
		/// - \ref framerendering_comm
		/// - \ref termination_comm
		class IProxyFramework
		{
		public:
				
			/// Initialize framework.
			/// This method has to be called once, before any other method call. 
			/// \sa
			/// - \ref initialization_comm
			virtual void Init() = 0;

			/// Terminate framework.
			/// This method has to be called once, before Destroy().
			/// \sa
			/// - \ref termination_comm
			virtual void Term() = 0;

			/// Destroy framework.
			/// \sa
			/// - \ref termination_comm
			virtual void Destroy() = 0;

			/// Set notification channel.
			virtual void SetNotificationChannel( 
				INotificationChannel* in_pNotificationChannel	///< Pointer to channel interface (see ICommunicationCentral::GetNotificationChannel()).
				) = 0;
			
			/// Get renderer proxy.
			/// \return Pointer to renderer proxy interface.
			virtual IRendererProxy* GetRendererProxy() = 0;
		};
	}
}

#endif
