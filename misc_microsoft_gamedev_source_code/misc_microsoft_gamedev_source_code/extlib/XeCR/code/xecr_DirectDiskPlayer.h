//	DirectDiskPlayer.h : Direct-from-disk input playback.
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
#include "xecr_FileFormat.h"
#include <string>

#include "xecr_EventInputSequence.h"
#include "xutil_InputStream.h"

#include "xecr_SequencePlayer.h"




namespace XCR
{
	namespace Sequence
	{

		// ================================================================
		//	DirectDiskPlayer
		//
		//	This plays directly from the disk, without buffering.
		// ================================================================
		class DirectDiskPlayer :
			public Player
		{
		public:
			DirectDiskPlayer();

			virtual ~DirectDiskPlayer(void);

			// ----------------------------------------------------------------
			// IComponent interface
			// ----------------------------------------------------------------

			// Set a property for this component.
			virtual XCR::Result SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2 = NULL);

            virtual XCR::Result SetDefault();

			// Retrieve helpstring.
			virtual const char *GetHelpString()
			{
				return "Play recorded controller input directly from disk.\vProperties:\v  file <full path> - file to play\v  synch time|frame - playback synchronization mode.";
			}

		protected:

			// ----------------------------------------------------------------
			// Implementation
			// ----------------------------------------------------------------

			// File handling.
			DirectDiskInputStream m_disk_input_stream;

			// Event sequence.
			BinaryEventInput m_binary_event_sequence;
		};

	}
}