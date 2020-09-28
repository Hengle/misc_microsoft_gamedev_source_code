//	DirectDiskRecorder.h : Direct-to-disk input recording.
//
//	Created 2004/10/06 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
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
		using namespace ControlUnit;
		
		// ================================================================
		// DirectDiskRecorder class
		//
		//	This records directly to the disk, without buffering.
		// ================================================================
		class DirectDiskRecorder :
			public Recorder
		{
		public:
			DirectDiskRecorder(void);
			~DirectDiskRecorder(void);

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

			virtual const char *GetHelpString()
			{
				return "Record controller input directly to disk.\vProperties:\v  file <full path> - file to write to";
			}

            XCR::Result SetDefault();

		protected:

			DirectDiskOutputStream disk_output_stream;
			BinaryEventOutput binary_event_sequence;
		};
	}
}