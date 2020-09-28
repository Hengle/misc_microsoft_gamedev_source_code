//	EventDumper.h : Recorded event file translator.
//
//	Created 2004/10/20 Rich Bonny    <rbonny@microsoft.com>
//
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) 2004 Microsoft Corp.  All rights reserved.




#pragma once

#include "XenonUtility.h"
#include "xecr_Interfaces.h"


namespace XCR
{
	namespace Dumper
	{

		class EventDumper
		{
		public:
			EventDumper(const char* inputFile, const char* outputFile);
			XCR::Result Translate();

		protected:
			XCR::Result Start();
			XCR::Result Stop();
			XCR::Result DumpNextEvent();

			// State information for gamepad.
			char m_gamepad_data[sizeof (XINPUT_GAMEPAD)];
			// State change information for gamepad.
			char m_gamepad_diff_data[sizeof (XINPUT_GAMEPAD)];
			// Flags indicating which information to use.
			DWORD m_gamepad_data_flags;

			// State info for keyboard:
			char m_keyboard_data[sizeof (XINPUT_KEYSTROKE)];
			// Flags indicating which information to use
			DWORD m_keyboard_data_flags;

			// Our input stream.
			DirectDiskInputStream m_disk_input_stream;
			InputStream* m_input;

			// Our output stream.
			DirectDiskOutputStream m_disk_output_stream;
			OutputStream* m_output;

			// Cumulative tick counter
			DWORD m_dwTickTotal;

			// Dump helper functions
			void DumpGamepadState();
			void EventDumper::Dump(SHORT shDump);
			void DumpTime(WORD wEvent);
			void DumpEventType(const char* strType);
			void Dump(const char* strDump);
			void Dump(BYTE bDump);
			void Dump(WORD wDump);
			void Dump(DWORD dwDump);
			void DumpEOL();
		};
	}
}