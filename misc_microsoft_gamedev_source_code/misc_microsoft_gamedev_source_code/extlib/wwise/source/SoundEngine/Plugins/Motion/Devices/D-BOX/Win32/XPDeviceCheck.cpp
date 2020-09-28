/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

/************************************************************************/
/* This code comes from DBOX.  And they got it from Microsoft.			*/
/* It will remove the Bass Boost option and put the volume to maximum	*/
/************************************************************************/
// Handles device-specific behaviors

#include "stdafx.h"
#include <assert.h>
#include <Windows.h>
#include <mmsystem.h>

#pragma comment(lib,"winmm.lib") 

#define outf(sFormat, ...) 	

#define ReturnIfFailed(sMethod, nResult) \
	if (nResult != MMSYSERR_NOERROR) { \
	outf("%s FAILED! (%d)\n", sMethod, nResult); \
	return; \
	}

#define ReturnFalseIfFailed(sMethod, nResult) \
	if (nResult != MMSYSERR_NOERROR) { \
	outf("%s FAILED! (%d)\n", sMethod, nResult); \
	return false; \
	}

struct MotionDeviceContext {
	/// Desired device name.
	const WCHAR * sDeviceName;	
};


// hMixer is device mixer handle.
// nDeviceLineId is destination or source line unique identifier for device.
// nControlType is the control to be set.
// dValue is its new value in range 0-100% (unsigned) or 0/1 (boolean).
// Returns true if success.
bool SetLineControl( HMIXER hMixer, DWORD nDeviceLineId, DWORD nControlType, float dValue ) {
	bool bSuccess = false;
	MMRESULT nResult;
	MIXERCONTROL oControl;
	MIXERLINECONTROLS oLineControls;
	ZeroMemory(&oLineControls, sizeof(oLineControls));
	oLineControls.cbStruct = sizeof(oLineControls);
	oLineControls.dwLineID = nDeviceLineId;
	oLineControls.dwControlType = nControlType;
	oLineControls.cbmxctrl = sizeof(oControl);
	oLineControls.pamxctrl = &oControl;
	nResult = mixerGetLineControls( (HMIXEROBJ)hMixer, &oLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
	ReturnFalseIfFailed("mixerGetLineControls", nResult);

	if ((oControl.fdwControl & MIXERCONTROL_CONTROLF_MULTIPLE)) {
		outf("Controls with multiple values not supported.");
	} else {
		MIXERCONTROLDETAILS oDetails;
		ZeroMemory(&oDetails, sizeof(oDetails));
		oDetails.cbStruct = sizeof(oDetails);
		oDetails.dwControlID = oControl.dwControlID;
		oDetails.cChannels = 1;		// 1 for uniform, set all channels as if the were uniform.
		oDetails.cMultipleItems = 0;

		int nUnits = oControl.dwControlType & MIXERCONTROL_CT_UNITS_MASK;
		switch (nUnits) {
			case MIXERCONTROL_CT_UNITS_BOOLEAN:
				MIXERCONTROLDETAILS_BOOLEAN oBool;
				oBool.fValue = (dValue != 0.0f);

				oDetails.cbDetails = sizeof(oBool);
				oDetails.paDetails = &oBool;
				nResult = mixerSetControlDetails((HMIXEROBJ)hMixer, &oDetails, 0L);
				ReturnFalseIfFailed("mixerSetControlDetails bool", nResult);
				bSuccess = true;
				break;

			case MIXERCONTROL_CT_UNITS_UNSIGNED:
				MIXERCONTROLDETAILS_UNSIGNED oUnsigned;
				// Map from 0-100 to 0-Maximum.
				oUnsigned.dwValue = static_cast<DWORD>(dValue * oControl.Bounds.dwMaximum * 0.01f);
				if (oUnsigned.dwValue > oControl.Bounds.dwMaximum) oUnsigned.dwValue = oControl.Bounds.dwMaximum;
				if (oUnsigned.dwValue < oControl.Bounds.dwMinimum) oUnsigned.dwValue = oControl.Bounds.dwMinimum;

				oDetails.cbDetails = sizeof(oUnsigned);
				oDetails.paDetails = &oUnsigned;
				nResult = mixerSetControlDetails((HMIXEROBJ)hMixer, &oDetails, 0L);
				ReturnFalseIfFailed("mixerSetControlDetails unsigned", nResult);
				bSuccess = true;
				break;
			default:
				outf("Unsupported control type.");
				break;
		}
	}
	return bSuccess;
}

void SetDestControls(MotionDeviceContext * pContext, HMIXER hMixer, int nDestIndex ) {
	MMRESULT nResult;
	MIXERLINEA oMixLine;
	ZeroMemory(&oMixLine, sizeof(oMixLine));
	oMixLine.cbStruct = sizeof(oMixLine);
	oMixLine.dwDestination = nDestIndex;
	nResult = mixerGetLineInfoA(reinterpret_cast<HMIXEROBJ>(hMixer), &oMixLine, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_DESTINATION);
	ReturnIfFailed("mixerGetLineInfo dest", nResult);
	outf("\tDESTINATION %d\n", nDestIndex);
	outf("\tDst\t%d\n", oMixLine.dwDestination);
	outf("\tSrc\t0x%x\n", oMixLine.dwSource);
	outf("\tLnId\t0x%x\n", oMixLine.dwLineID);
	outf("\tConn\t%d\n", oMixLine.cConnections);
	outf("\tCtrls\t%d\n", oMixLine.cControls);
	outf("\tLName\t%s\n", oMixLine.szName);

	// Getting all line's controls
	int nControlCount = oMixLine.cControls;
	if (nControlCount > 0) {
		DWORD nDestLineId = oMixLine.dwLineID;
		bool bContinue = false;
		// Prevent any mixing, set all volumes to max.
		bContinue = SetLineControl(hMixer, nDestLineId, MIXERCONTROL_CONTROLTYPE_VOLUME, 100);
		SetLineControl(hMixer, nDestLineId, MIXERCONTROL_CONTROLTYPE_BASS, 100);
		SetLineControl(hMixer, nDestLineId, MIXERCONTROL_CONTROLTYPE_TREBLE, 100);
		// Unmute destination.
		SetLineControl(hMixer, nDestLineId, MIXERCONTROL_CONTROLTYPE_MUTE, 0);
		// No bass-boost.
		SetLineControl(hMixer, nDestLineId, MIXERCONTROL_CONTROLTYPE_BASS_BOOST, 0);

		if (bContinue) {
			// All motion controls where set successfully (found good dest line).
			// Set WAV source line volume and mute.
			ZeroMemory(&oMixLine, sizeof(oMixLine));	// Reuse same structure.
			oMixLine.cbStruct = sizeof(oMixLine);
			oMixLine.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT;
			oMixLine.dwDestination = nDestIndex;
			nResult = mixerGetLineInfoA(reinterpret_cast<HMIXEROBJ>(hMixer), &oMixLine, MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
			ReturnIfFailed("mixerGetLineInfo src", nResult);

			int nControlCount = oMixLine.cControls;
			if (nControlCount > 0) {
				DWORD nSrcLineId = oMixLine.dwLineID;
				// Unmute source wav.
				SetLineControl(hMixer, nSrcLineId, MIXERCONTROL_CONTROLTYPE_MUTE, 0);
				// Max volume.
				SetLineControl(hMixer, nSrcLineId, MIXERCONTROL_CONTROLTYPE_VOLUME, 100);
			}
		}
	} // if nControlCount
}

void ProcessDeviceByHandle(MotionDeviceContext * pContext, HMIXER hMixer) 
{
	MMRESULT nResult;
	MIXERCAPSW oMixCaps;
	nResult = mixerGetDevCapsW(reinterpret_cast<UINT_PTR>(hMixer), &oMixCaps, sizeof(oMixCaps));
	ReturnIfFailed("mixerGetDevCaps", nResult);
	outf("Pname\t%s\n", oMixCaps.szPname);
	outf("M id\t%d\n", oMixCaps.wMid);
	outf("P id\t%d\n", oMixCaps.wPid);
	outf("DrvVer\t%d\n", oMixCaps.vDriverVersion);
	outf("Supprt\t%x\n", oMixCaps.fdwSupport);
	outf("Dests\t%d\n", oMixCaps.cDestinations);

	if (wcsstr( pContext->sDeviceName, oMixCaps.szPname) != 0) {
		outf("FOUND DEVICE!\n");

		for (DWORD nDestIndex = 0; nDestIndex < oMixCaps.cDestinations; nDestIndex++) {
			SetDestControls(pContext, hMixer, nDestIndex);
		} // for dest
	}
}

void ProcessDeviceByIndex(MotionDeviceContext * pContext, int nMixDevice ) {
	MMRESULT nResult;
	HMIXER hMixer = NULL;
	nResult = mixerOpen(&hMixer, nMixDevice, 0, 0, MIXER_OBJECTF_MIXER);
	ReturnIfFailed("mixerOpen", nResult);

	ProcessDeviceByHandle(pContext, hMixer);

	nResult = mixerClose(hMixer);
	ReturnIfFailed("mixerClose", nResult);
}

void XPMotionDeviceInit(const WCHAR * sDeviceName) 
{
	MotionDeviceContext oContext;
	oContext.sDeviceName = sDeviceName;
	int nMixDevices = mixerGetNumDevs();
	outf("Mixer Devices: %d\n", nMixDevices);

	for (int nMixDevice = 0; nMixDevice < nMixDevices; nMixDevice++) {
		ProcessDeviceByIndex(&oContext, nMixDevice);

		outf("\n");
	}
}