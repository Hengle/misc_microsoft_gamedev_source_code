// Audio System -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "Common.h"
#include "AudioSystem.h"

// Methods.

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static AudioSystem *audio = NULL;
	static float rate = 1.0f;
	static bool playing = false;

	switch (msg)
	{
		case WM_INITDIALOG:
		{
			audio = AudioSystem::CreateAudioSystem(hDlg);
			if (audio == NULL)
			{
				MessageBox(
					hDlg,
					"Error creating the audio system.",
					"Error",
					MB_OK | MB_ICONSTOP
				);
				EndDialog(hDlg, FALSE);
				return FALSE;
			}
			
			const char *tracks[] =
			{
				"track1.wav",
				"track2.wav",
				"track3.wav",
				"track4.wav",
				"track5.wav",
			};
			const int numTracks = sizeof(tracks) / sizeof(tracks[0]);
			
			for (int index = 0; index < numTracks; ++index)
				if (!audio->LoadTrack(tracks[index]))
				{
					MessageBox(
						hDlg,
						"Error loading audio track.",
						"Error",
						MB_OK | MB_ICONSTOP
					);
					SAFE_DELETE(audio);
					EndDialog(hDlg, FALSE);
					return FALSE;
				}

			audio->Seek(0.0f);
			
			SetTimer(hDlg, 1, 20, NULL);
		}
		return FALSE;
		
		case WM_TIMER:
		{
			float time;
			int frames, seconds, minutes, hours;
			char buffer[256];
			
			time = audio->GetTime();
			frames	= static_cast<int>(fmod(30.0f * time,			30.0f));
			seconds	= static_cast<int>(fmod(	    time,			60.0f));
			minutes	= static_cast<int>(fmod(        time / 60.0f,	60.0f));
			hours	= static_cast<int>(fmod(        time / 3600.0f,	24.0f));
			_snprintf(buffer, 255, "%02i:%02i:%02i:%02i", hours, minutes, seconds, frames);
			SetDlgItemText(hDlg, IDC_TIME, buffer);
		}
		return 0;
		
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_PLAY:
					audio->Play(rate);
					playing = true;
				return TRUE;

				case IDC_STOP:
					audio->Stop();
					playing = false;
				return TRUE;
				
				case IDC_REWIND:
					audio->Seek(0.0f);
				return TRUE;

				case IDC_SLOWER:
					rate /= pow(2.0f, 1.0f / 12.0f);
					if (playing) audio->Play(rate);
				return TRUE;
				
				case IDC_NORMAL:
					rate = 1.0f;
					if (playing) audio->Play(rate);
				return TRUE;

				case IDC_FASTER:
					rate *= pow(2.0f, 1.0f / 12.0f);
					if (playing) audio->Play(rate);
				return TRUE;
			
				case IDCANCEL:
					KillTimer(hDlg, 1);
					SAFE_DELETE(audio);
					EndDialog(hDlg, FALSE);
				return TRUE;
			}
		}
		return FALSE;
	}
	
	return FALSE;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    HRESULT hr;

	// COM.
	
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	_ASSERTE(SUCCEEDED(hr));

	// Dialog.

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_AUDIO), NULL, DialogProc);
	
	// Cleanup.

	CoUninitialize();
	
	return 0;
}
