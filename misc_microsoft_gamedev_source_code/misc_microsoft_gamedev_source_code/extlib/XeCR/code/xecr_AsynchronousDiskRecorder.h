//	AsynchronousDiskRecorder.h : Asynchronous buffered controller input recording.
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
#include "xecr_SequenceRecorder.h"
#include "xutil_OutputStream.h"



namespace XCR
{
	namespace Sequence
	{
		// ================================================================
		//	BufferedAsynchronousDiskRecorder
		//
		//	This records data to the disk by first accumulating it to a buffer, then writing it in
		//		sector-granular chunks.  The buffer writes can be done synchronously or asynchronously.
		//	Default write buffer size is 16kb, the size of a disk cluster.
		//	The synchronous mode may block for 100s of ms if the disk is fragmented enough.
		//	The asynchronous mode uses a separate thread to perform the write, so execution can
		//		continue while writing occurs, but it requires more memory to use it:
		//			13k kernel + 12k stack + another write buffer
		//		It may still block if the write hasn't completed by the time the buffer is needed again,
		//		which can happen, for example, if the thread starves.
		// ================================================================
		class BufferedAsynchronousDiskRecorder :
			public Recorder
		{
		public:
			BufferedAsynchronousDiskRecorder(void);
			~BufferedAsynchronousDiskRecorder(void);

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			// Set a property for this component.
			XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2);

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				// Help message corrected: RWB Fix Bug #346
				return "Record controller input asynchronously to buffer.\vProperties:\v  file <full path> - file to write to\v  asynch on|off|true|false - use thread to perform asynchronous writes?\v  bufsize <bytes> - size of buffer to use";
			}

            XCR::Result SetDefault();

		protected:

			BufferedAsynchronousDiskOutputStream disk_output_stream;
			BinaryEventOutput binary_event_sequence;
		};
	}
}