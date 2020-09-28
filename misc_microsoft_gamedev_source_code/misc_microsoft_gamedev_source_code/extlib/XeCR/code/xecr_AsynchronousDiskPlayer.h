//	AsynchronousDiskPlayer.h : Asynchronous buffered controller input playback.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once
#include "xecr_Interfaces.h"
#include "xecr_SequencePlayer.h"
#include "xutil_InputStream.h"




namespace XCR
{
	namespace Sequence
	{

		// ================================================================
		//	BufferedAsynchronousDiskPlayer
		//
		//	This plays data from the disk by first reading it into a buffer in sector-granular chunks,
		//		then extracting it.  The buffer writes can be done synchronously or asynchronously.
		//	Default read buffer size is 32kb, the recommended amount (it's twice the size of a disk cluster).
		//	The synchronous mode may block for 100s of ms if the disk is fragmented enough.
		//	The asynchronous mode uses a separate thread to perform the read, so execution can
		//		continue while reading ahead occurs, but it requires more memory to use it:
		//			13k kernel + 12k stack + another read buffer
		//		It may still block if the read hasn't completed by the time the buffer is needed again,
		//		which can happen, for example, if the thread starves.
		// ================================================================
		class BufferedAsynchronousDiskPlayer :
			public Player
		{
		public:
			BufferedAsynchronousDiskPlayer(void);
			~BufferedAsynchronousDiskPlayer(void);

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			// Set a property for this component.
			XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2);

            XCR::Result SetDefault();

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "Play recorded controller input asynchronously from buffer.\vProperties:\v  file <full path> - file to play\v  synch time|frame - playback synchronization mode\v  asynch on|off|true|false - use thread to perform asynchronous writes?\v  bufsize <bytes> - size of buffer to use";
			}

		protected:

			// File handling.
			BufferedAsynchronousDiskInputStream m_disk_input_stream;

			// Event sequence.
			BinaryEventInput m_binary_event_sequence;

		};
	}
}