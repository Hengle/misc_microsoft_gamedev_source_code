//	AsynchronousDiskPlayer.cpp : Asynchronous buffered controller input playback.
//
//	Created 2004/10/12 Rich Bonny    <rbonny@microsoft.com>
//
//  - based on original XCR code:
//	Created 2003/03/XX David Eichorn <deichorn@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_AsynchronousDiskPlayer.h"




namespace XCR
{
	namespace Sequence
	{

		BufferedAsynchronousDiskPlayer::BufferedAsynchronousDiskPlayer(void) :
			Player(&m_binary_event_sequence),
			m_binary_event_sequence(&m_disk_input_stream)
		{
		}


		BufferedAsynchronousDiskPlayer::~BufferedAsynchronousDiskPlayer(void)
		{
		}


		// ----------------------------------------------------------------
		// IComponent interface
		// ----------------------------------------------------------------


		XCR::Result BufferedAsynchronousDiskPlayer::SetStringProperty(LPCSTR name, LPCSTR value1, LPCSTR value2)
		{
			if (lstrcmpiA(name, "file") == 0)
			{
				if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				if (is_playing) return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
				m_disk_input_stream.SetFilename(value1);
				return XenonUtility::Result::SUCCESS;
			}

			else if (lstrcmpiA(name, "asynch") == 0)
			{
				if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				if (is_playing) return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
				if (lstrcmpiA(value1, "on") == 0 || lstrcmpiA(value1, "true") == 0)
				{
					m_disk_input_stream.SetAsynchronous(true);
					return XCR::Result::SUCCESS;
				}
				if (lstrcmpiA(value1, "off") == 0 || lstrcmpiA(value1, "false") == 0)
				{
					m_disk_input_stream.SetAsynchronous(false);
					return XCR::Result::SUCCESS;
				}
				return XCR::Result::ERROR_INVALID_VALUE;
			}

			else if (lstrcmpiA(name, "bufsize") == 0)
			{
				if (value2 != NULL) return XCR::Result::ERROR_TOO_MANY_ARGUMENTS;
				if (is_playing) return XCR::Result::ERROR_CANT_SET_PROPERTY_WHILE_USING;
				long bufsize = atol(value1);
				if (bufsize > 0)
				{
					size_t buffer_size = (bufsize + XBOX_HD_SECTOR_SIZE - 1) / XBOX_HD_SECTOR_SIZE * XBOX_HD_SECTOR_SIZE;
					m_disk_input_stream.SetBufferSize(buffer_size);
					if (buffer_size > 16 * 1024 * 1024)
					{
						return XCR::Result::SUCCESS_OUTSIDE_RECOMMENDED_RANGE;
					}

					//JKBTP1
					if (bufsize % XBOX_HD_SECTOR_SIZE != 0)
					{
						return XCR::Result::SUCCESS_BUFFER_NOT_MULTIPLE_OF_SECTOR_SIZE;
					}

					return XCR::Result::SUCCESS;
				}
				return XCR::Result::ERROR_INVALID_VALUE;
			}

			return Player::SetStringProperty(name, value1, value2);
		}

        XCR::Result BufferedAsynchronousDiskPlayer::SetDefault()
        {
            m_disk_input_stream.SetFilename(0);
            m_disk_input_stream.SetAsynchronous(false);
            m_disk_input_stream.SetBufferSize(16384);
            return Player::SetDefault();
        }
	}
}

#endif