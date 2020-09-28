//	EventDumper.cpp : Recorded event file translator.
//
//	Created 2004/10/20 Rich Bonny    <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#ifdef _XCR_

#include "xecr_EventDumper.h"
#include "xutil_InputStream.h"
#include "xecr_Interfaces.h"
#include "xecr_FileFormat.h"
#include "xecr_EventInputSequence.h"

#define CHECK_RESULT(a,b) if (!a) OutputDebugStringA(b)


namespace XCR
{
	namespace Dumper
	{
		// Constructor.
		EventDumper::EventDumper(const char* inputFile, const char* outputFile)
		{
			// Open input and output streams
			// m_input = input;
			m_input = &m_disk_input_stream;
			m_output = &m_disk_output_stream;
			m_input->SetFilename(inputFile);
			m_output->SetFilename(outputFile);
			m_dwTickTotal = 0;
		}


		// Translate from binary to readable format
		XCR::Result EventDumper::Translate()
		{
			//long lineCount = 0; // counter to test timing
			XCR::Result result = Start();
			while (result == XCR::Result::SUCCESS)
			{
				result = DumpNextEvent();
				//if (0 == lineCount++ % 1000)
				//{
				//	OutputDebugString("Another 1000 lines\n");
				//}
			}
			result = Stop();
			return result;
		}


		// Start.
		XCR::Result EventDumper::Start()
		{
			XCR::Result result = m_output->Start();
			CHECK_RESULT(result, "!!Could not start Event dump.\n");

			Sequence::IFileFormat::FileHeader header;
			result = m_input->Start();
			if (!result)
			{
				return result;
			}
			result = m_input->Read(&header, sizeof (header));
			if (!result)
			{
				m_input->Stop();
			}
			if (!header.IsValid())
			{
				m_input->Stop();
				return XCR::Result::ERROR_INVALID_FILE;
			}
			return result;
		}


		// Stop.
		XCR::Result EventDumper::Stop()
		{
			m_input->Stop();
			m_output->Stop();
			return XCR::Result::SUCCESS_EOF;
		}

		// Translate next event in file
		XCR::Result EventDumper::DumpNextEvent()
		{
			using namespace Sequence;

			IFileFormat::InputEvent event;
			IFileFormat::InputEvent::Type type;
			XCR::Result result;

			result = m_input->Read(&event, sizeof (event));
			if (result == Result::SUCCESS_EOF)
			{
				event.type = IFileFormat::InputEvent::TYPE_EOF;
			}
			else if (!result)
			{
				return result;
			}
			DumpTime(event.tick_count_delta);
			type = (IFileFormat::InputEvent::Type) event.type;
			switch (type)
			{
			case IFileFormat::InputEvent::TYPE_EOF:
				{
					DumpEventType("End Of File");
					DumpEOL();
					result = XCR::Result::ERROR_EOF;
				}
				break;

			case IFileFormat::InputEvent::TYPE_INPUT_CONNECT_GAMEPAD:
				{
					IFileFormat::GamepadConnectEvent entry;
					result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
					DumpEventType("Gamepad Connect");
					Dump("Port=");
					Dump(entry.port);
					DumpEOL();
					m_gamepad_data_flags = 0x3FFF;
					memcpy(m_gamepad_data, &entry.state, sizeof (entry.state));
				}
				break;

			case IFileFormat::InputEvent::TYPE_INPUT_DISCONNECT_ANY_DEVICE:
				{
					IFileFormat::AnyDeviceDisconnectEvent entry;
					result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
					DumpEventType("Disconnect Any Device");
					Dump("Port=");
					Dump(entry.port);
					DumpEOL();
				}
				break;


			case IFileFormat::InputEvent::TYPE_INPUT_GAMEPAD_STATE:
				{
					IFileFormat::GamepadStateEvent entry;
					// Read the mask (everything except the diff_data).
					result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event) - sizeof (entry.diff_data));
					m_gamepad_data_flags = entry.mask;
					// Count how many bytes changed and read them.
					DWORD bytes_changed = 0;
					DWORD i;
					for (i = 0; i < 10; i++)
					{
						if (entry.mask & (1 << i))
						{
							bytes_changed++;
						}
					}
					for (i = 10; i < 14; i++)
					{
						if (entry.mask & (1 << i))
						{
							bytes_changed += 2;
						}
					}
					result = m_input->Read(m_gamepad_diff_data, bytes_changed);
					DumpEventType("Input Gamepad State");
					Dump("Port=");
					Dump(entry.port);
					Dump("   ");
					DumpGamepadState();
				}
				break;

			case IFileFormat::InputEvent::TYPE_INPUT_CONNECT_KEYBOARD:
				{
					IFileFormat::KeyboardConnectEvent entry;
					result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event));
					// Always 7 bytes
					result = m_input->Read(m_gamepad_diff_data, 7);
					DumpEventType("Connect Keyboard");
					Dump("Port=");
					Dump(entry.port);
					DumpEOL();
				}
				break;

			case IFileFormat::InputEvent::TYPE_INPUT_KEYBOARD_STATE:
				{
					IFileFormat::KeyboardStateEvent entry;
					// Read the mask (everything except the diff_data).
					result = m_input->Read((char *) &entry + sizeof (event), sizeof (entry) - sizeof (event) - sizeof (entry.diff_data));
					// Always 7 bytes of data
					DWORD bytes_changed = 7;
					char keyboard_data[sizeof (XINPUT_KEYSTROKE)];
					result = m_input->Read(keyboard_data, bytes_changed);
					DumpEventType("Input Keyboard State");
					Dump("Port=");
					Dump(entry.port);
					Dump("   ");
					Dump("Keystroke=");
					for (int i = 0; i < 7; i++)
						Dump( (BYTE)keyboard_data[i]);
					DumpEOL();
				}
				break;

			case IFileFormat::InputEvent::TYPE_WAIT:
				{
					IFileFormat::WaitEvent wait;
					_ASSERTE(sizeof (wait) > sizeof (event));
					memcpy(&wait, &event, sizeof (event));
					result = m_input->Read((char *) &wait + sizeof (event), sizeof (wait) - sizeof (event));
					DumpEventType("Wait");
					DumpEOL();
				}
				break;

			default:
				result = XCR::Result::ERROR_INVALID_FILE;
			}
			return result ? XCR::Result::SUCCESS : result;
		}

		// Applies data changes to gamepad.
		void EventDumper::DumpGamepadState()
		{
			Dump("Pad=");
			BYTE *new_data = (BYTE *) &m_gamepad_data;
			BYTE *diff_data = (BYTE *) m_gamepad_diff_data;
			DWORD i;
            //MMILLS: MARCH, 2005 XDK UPDATE
			// Digital buttons and analog buttons (1 word, 2 bytes = 4 bytes).
			for (i = 0; i < 4; i++, new_data++)
			{
				if (m_gamepad_data_flags & (1 << i))
				{
					*new_data = *diff_data;
					diff_data++;
				}
				Dump(*new_data);
			}
			Dump("  ");
			// Thumbsticks (4 shorts).
			for (i = 10; i < 14; i++, new_data += 2)
			{
				if (m_gamepad_data_flags & (1 << i))
				{
					*(WORD *) new_data = *(WORD *) diff_data;
					diff_data += 2;
				}
				Dump(*(WORD *) new_data);
			}
			DumpEOL();
		}

		// Dump helper functions

		void EventDumper::DumpTime(WORD wTickDelta)
		{
			m_dwTickTotal += wTickDelta;
			char temp[12];
			_snprintf(temp, 11, "%09d  ", m_dwTickTotal);
			m_output->Write(temp, 11);
		}

		void EventDumper::DumpEventType(const char* strType)
		{
			char temp[25];
			_snprintf(temp, 24, "%-24s", strType);
			m_output->Write(temp, 24);
		}

		void EventDumper::Dump(const char* strDump)
		{
			m_output->Write(strDump, strlen(strDump));
		}

		void EventDumper::Dump(BYTE bDump)
		{
			char temp[4];
			_snprintf(temp, 3, " %02X", bDump);
			m_output->Write(temp, 3);
		}

		void EventDumper::Dump(WORD wDump)
		{
			char temp[6];
			_snprintf(temp, 5, " %04X", wDump);
			m_output->Write(temp, 5);
		}

		void EventDumper::Dump(DWORD dwDump)
		{
			char temp[10];
			_snprintf(temp, 9, " %08X", dwDump);
			m_output->Write(temp, 9);
		}

		void EventDumper::DumpEOL()
		{
			static char eol[3] = "\r\n";
			m_output->Write(eol, 2);
		}

	}
}


#endif