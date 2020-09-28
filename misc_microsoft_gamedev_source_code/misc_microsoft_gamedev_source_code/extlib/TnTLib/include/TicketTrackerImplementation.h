#pragma once

#ifdef _TICKET_TRACKER_

	#include "TicketTracker.h"
	#include "LspUploader.h"
	#include "StandardUploader.h"
	#include "ZlibCompression.h"
	#include "XBoxEncryption.h"
	#include "TTStatus.h"

	using namespace TicketTracker;

	LONG WINAPI TicketTrackerUnhandledExceptionHandler(struct _EXCEPTION_POINTERS *ExceptionInfo);

	LONG WINAPI TicketTrackerUnhandledExceptionHandler(struct _EXCEPTION_POINTERS *ExceptionInfo)
	{
		//DebugBreak();
		if (ExceptionInfo)
		{
			ITicketTracker *pTT = TicketTrackerFactory::GetTicketTracker();
			return pTT->ExceptionHandler(ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo);

		}
		else
			return ERROR_INVALID_DATA;
	}

	void LoadTicketTrackerSettings();

	void LoadTicketTrackerSettings()
	{
		// Get the TicketTracker object
		ITicketTracker *pTT = TicketTrackerFactory::GetTicketTracker();
		static TTStatus ttStatusScreen;

		TTResult res = pTT->LoadSettings("GAME:\\TicketTracker\\TTConfig.xml");
		if(res != Success) 
		{
    		OutputDebugString("TICKET TRACKER NOT ENABLED ");
		}
		else
		{
			// Set the build version string so that the AutoCreateBuildVersionFile option
			// can be used
			HANDLE buildFile = CreateFile("Game:\\tools\\build\\build_number.txt",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
			if (buildFile != INVALID_HANDLE_VALUE)
			{
				char buf[128] = "";
				DWORD numBytesRead = 0;
				BOOL bSuccess = ReadFile(buildFile, buf, 128, &numBytesRead, NULL);
				if (bSuccess && numBytesRead > 0)
				{
					// file is of the form 
					// buildName ######
					// so skip past the name and get the number
					char * pBuildNumber = strchr(buf, ' ') + 1;
					if (pBuildNumber)
					{
						// trim off any extra junk after the number (spaces, newlines, etc)
						_itoa_s(atoi(pBuildNumber),buf,128,10);
						pTT->SetBuildVersion( buf );
					}
					else
					{
						pTT->SetBuildVersion( "Found Game:\\tools\\build\\build_number.txt but it was not a valid format.  Expected \"BuildName #####\"." );
					}
				}
				else
				{
					pTT->SetBuildVersion( "Reading Game:\\tools\\build\\build_number.txt failed." );
				}
				CloseHandle(buildFile);
			}
			else
			{
				pTT->SetBuildVersion( "Could Not Find Game:\\tools\\build\\build_number.txt" );
			}

			// Set encryption to use
			TnT::XboxEncryption* pEnc = new TnT::XboxEncryption();
			pTT->SetEncryptor(pEnc);
			
			// Set compression to use
			//TnT::ZlibCompression* pComp = new TnT::ZlibCompression();
			//pTT->SetCompressor(pComp);

			// Provide uploaders to the upload info specified in the config file
			UploadInfo *ui = pTT->GetUploadInfo("Corpnet");
			ui->SetUploader(new TnT::StandardUploader());
			ui = pTT->GetUploadInfo("LSP");
			ui->SetUploader(new TnT::LspUploader(0x4D5307E1));

			// NOTE: If you want to use the TicketTracker screenshot capture feature
			// and/or the TicketTracker status display, you will need to call the 
			// SetDevice() function once you’ve initialized your graphics system:
			//pTT->SetDevice(device);
			D3DDevice* theDevice = gRenderThread.releaseThreadOwnership();
			ttStatusScreen.Initialize(theDevice, "Game:\\TicketTracker\\Arial_16.xpr");
			gRenderThread.acquireThreadOwnership();
			pTT->SetStatusListener(&ttStatusScreen);
			SetUnhandledExceptionFilter(TicketTrackerUnhandledExceptionHandler);
		}
	}	
#endif //_TICKET_TRACKER_