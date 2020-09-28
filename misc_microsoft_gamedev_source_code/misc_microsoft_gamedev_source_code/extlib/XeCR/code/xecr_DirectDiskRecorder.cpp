//	DirectDiskRecorder.cpp : Direct-to-disk input recording.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_DirectDiskRecorder.h"




namespace XCR
{
	namespace Sequence
	{

		// ----------------------------------------------------------------
		// Constructors / destructor
		// ----------------------------------------------------------------


		DirectDiskRecorder::DirectDiskRecorder(void) :
			Recorder(&binary_event_sequence),
			binary_event_sequence(&disk_output_stream)
		{
		}


		DirectDiskRecorder::~DirectDiskRecorder(void)
		{
		}


		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------


		XCR::Result DirectDiskRecorder::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
		{
			if (lstrcmpiA(name, "file") == 0)
			{
				if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				if (is_recording) return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
				disk_output_stream.SetFilename(value1);
				return XenonUtility::Result::SUCCESS;
			}
			return Recorder::SetStringProperty(name, value1, value2);
		}

        XCR::Result DirectDiskRecorder::SetDefault()
        {
            disk_output_stream.SetFilename(0);
            return XCR::Result::SUCCESS;
        }
	}
}


#endif