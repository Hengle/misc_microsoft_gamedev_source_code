//	DirectDiskPlayer.cpp : Direct-from-disk input playback.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//     and modified by Jon Burns     <jburns@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_DirectDiskPlayer.h"




namespace XCR
{
	namespace Sequence
	{

		DirectDiskPlayer::DirectDiskPlayer(void) :
			Player(&m_binary_event_sequence),
			m_binary_event_sequence(&m_disk_input_stream)
		{
		}


		DirectDiskPlayer::~DirectDiskPlayer(void)
		{
		}


		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------


		XCR::Result DirectDiskPlayer::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
		{
			if (lstrcmpiA(name, "file") == 0)
			{
				if (value2 != NULL)
				{
					return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				}
				m_disk_input_stream.SetFilename(value1);
				return XenonUtility::Result::SUCCESS;
			}
			return Player::SetStringProperty(name, value1, value2);
		}

        XCR::Result DirectDiskPlayer::SetDefault()
        {
            m_disk_input_stream.SetFilename(0);
            return Player::SetDefault();
        }
	}
}


#endif