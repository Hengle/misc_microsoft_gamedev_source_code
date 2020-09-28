//	XCR.cpp : Defines global XCR objects and default component initialization list.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include <xtl.h>

#include "xecr_Core.h"
#include "xecr_XInputSource.h"
#include "xecr_DebugChannelControl.h"
#include "xecr_AsynchronousDiskRecorder.h"
#include "xecr_AsynchronousDiskPlayer.h"
#include "xecr_TextSequencePlayer.h"
#include "xecr_RandomStateControl.h"
#include "xecr_DirectDiskRecorder.h"
#include "xecr_DirectDiskPlayer.h"
#include "xecr_RandomInputSource.h"
#include "xecr_Logger.h"




namespace XCR
{
	using namespace ControlUnit;

	// The single global XCRCore object.
	Core xcr;
	// The input source for the real Xbox ports.
	XInputSource xcr_ports;
	// The single XCR debug channel control.
	DebugChannelControl xcr_dcc;
	// A buffered disk recorder.
	XCR::Sequence::BufferedAsynchronousDiskRecorder xcr_bufrec;
	// A buffered disk player.
	XCR::Sequence::BufferedAsynchronousDiskPlayer xcr_bufplay;
	// A text sequence player.
	XCR::Sequence::TextSequencePlayer xcr_seqplay;
	// A random state control.
	RandomStateControl xcr_random_state;
	// A direct disk recorder.
	XCR::Sequence::DirectDiskRecorder xcr_ddrec;
	// A direct disk player.
	XCR::Sequence::DirectDiskPlayer xcr_ddplay;
	// A random input source.
	RandomInputSource xcr_rand_input;

	
	OutputDebugStringLogger xcr_dbglog;
	DirectDiskLogger xcr_ddlog;
	BufferedAsynchronousDiskLogger xcr_buflog;


	// List of components to initialize automatically.
	// You must terminate this list with a NULL entry.
	// The first recorder and first player listed become the default recorder and player.
	Core::ComponentInitialize Core::s_initial_components[] =
	{
		{ "ports", (IComponent *) &xcr_ports },
		{ "dbgchnl", (IComponent *) &xcr_dcc  },
		{ "bufrec", (IComponent *) &xcr_bufrec },
		{ "bufplay", (IComponent *) &xcr_bufplay },
		{ "seqplay", (IComponent *) &xcr_seqplay },
		{ "ddrec", (IComponent *) &xcr_ddrec },
		{ "ddplay", (IComponent *) &xcr_ddplay },
		{ "monkey", (IComponent *) &xcr_rand_input },
		{ "rs", (IComponent *) &xcr_random_state },
		{ "dbglog", (IComponent *) &xcr_dbglog },
		{ "ddlog", (IComponent *) &xcr_ddlog },
		{ "buflog", (IComponent *) &xcr_buflog },
		NULL
	};

}


#endif