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

#include "stdafx.h"
#include <mmdeviceapi.h>			// IMMDeviceEnumerator
#include <endpointvolume.h>			// IAudioEndpointVolume
#include <atlbase.h>				// CComPtr
#include <math.h>					// exp, log
#include <AK/Tools/Common/AkAssert.h>

#define err_outf(sFormat, ...) 
//	printf("Vista: " __FUNCTION__ " " sFormat, __VA_ARGS__);	

#define info_outf(sFormat, ...)
//	printf("Vista: " __FUNCTION__ " " sFormat, __VA_ARGS__);	

#define SAFE_RELEASE(ptr)  \
	if ((ptr) != NULL) {	\
	(ptr)->Release();	\
	(ptr) = NULL; \
	}

#define KINE_AUDIO_BASS_BOOST_NODE 6

struct MotionDeviceContext {
	/// Desired device name.
	const WCHAR * sDeviceName;	
};

/// Sample code from Microsoft.
HRESULT GetKsFilterFromEndpoint(IMMDeviceEnumerator* pEnumerator, IMMDevice* pEndpoint, IMMDevice** ppController)
{
	HRESULT hr;
	CComPtr<IDeviceTopology> spTopology;
	CComPtr<IConnector>      spPlug;
	LPWSTR                   pwstrControllerId = NULL;

	*ppController = NULL;

	hr = pEndpoint->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&spTopology);
	AKASSERTANDRETURN(SUCCEEDED(hr), hr);

	// Get the zero-th connector.  Note that by definition, Endpoint devices only have 1 
	// connector, representing the plug.
	hr = spTopology->GetConnector(0, &spPlug);
	AKASSERTANDRETURN(SUCCEEDED(hr), hr);

	// This is the quickest way to get from an Endpoint plug to the audio controller.
	hr = spPlug->GetDeviceIdConnectedTo(&pwstrControllerId);
	AKASSERTANDRETURN(SUCCEEDED(hr), hr);

	hr = pEnumerator->GetDevice(pwstrControllerId, ppController);
	AKASSERTANDRETURN(SUCCEEDED(hr), hr);

	// Remember to free device IDs using CoTaskMemFree
	if (pwstrControllerId != NULL)
		CoTaskMemFree(pwstrControllerId);
	return hr;
}

// call this on the controller
HRESULT GetIKsControlFromEndpoint(IMMDeviceEnumerator* pEnumerator, IMMDevice* pEndpoint, IKsControl** ppKsControl)
{
	HRESULT hr;
	CComPtr<IMMDevice>  spController;

	hr = GetKsFilterFromEndpoint(pEnumerator, pEndpoint, &spController);
	AKASSERTANDRETURN(SUCCEEDED(hr), hr);

	hr = spController->Activate(__uuidof(IKsControl), CLSCTX_INPROC_SERVER, NULL, (void**)ppKsControl);
	AKASSERTANDRETURN(SUCCEEDED(hr), hr);
	return hr;
}

void ProcessBassBoost(IMMDeviceEnumerator *pEnum, IMMDevice *pDevice, IPart *pPart) {
	// We can not rely on GetName (cultural) to detect BassBoost node, so try all nodes.
	UINT nLocalId;
	HRESULT hr = pPart->GetLocalId(&nLocalId);
	if (FAILED(hr)) {
		err_outf("Bass Boost GetLocalId failed: hr = 0x%08x\n", hr);
	} else {
		ULONG nNodeId = nLocalId & PARTID_MASK;	// KineAudio BASS BOOST node is 6.
		// The only known BASS BOOST node is in KineAudio:
		switch (nNodeId) {
			case KINE_AUDIO_BASS_BOOST_NODE:
				IKsControl * pKsControl;
				hr = GetIKsControlFromEndpoint(pEnum, pDevice, &pKsControl);
				if (FAILED(hr) || pKsControl == NULL) {
					err_outf("GetIKsControlFromEndpoint failed: hr = 0x%08x\n", hr);
				} else {
					KSNODEPROPERTY_AUDIO_CHANNEL oNode;
					::ZeroMemory(&oNode, sizeof(oNode));
					oNode.NodeProperty.Property.Set = KSPROPSETID_Audio;
					oNode.NodeProperty.Property.Id = KSPROPERTY_AUDIO_BASS_BOOST;
					oNode.NodeProperty.Property.Flags = KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_TOPOLOGY;
					oNode.NodeProperty.NodeId = nNodeId;	
					oNode.Channel = 0;
					DWORD nBytesReturned;
					BOOL bValue = FALSE;
					info_outf("Clearing Bass Boost (node %i)\n", nNodeId);
					hr = pKsControl->KsProperty(reinterpret_cast<PKSPROPERTY>(&oNode), sizeof(oNode), &bValue, sizeof(bValue), &nBytesReturned);
					if (FAILED(hr)) {
						DWORD nError = GetLastError();
						err_outf("SetKsProperty Error: %i, hr: 0x%08x\n", nError, hr);
					} else {
						info_outf("Bass Boost successfully disabled\n");
					}
				}
				break;
		}
	}
}

/// Sets part level to maximum dB. 
void ProcessLevelMax(REFIID uuid, IPart *pPart) {
	// see if this is a volume node part
	IPerChannelDbLevel *pLevel = NULL;
	HRESULT hr = pPart->Activate(CLSCTX_ALL, uuid, (void**)&pLevel);
	if (E_NOINTERFACE == hr) {
		// not a volume node
	} else if (FAILED(hr)) {
		err_outf("Unexpected failure trying to activate per channel db level: hr = 0x%08x\n", hr);
	} else {
		// it's a volume node...
		UINT nChannels = 0;
		hr = pLevel->GetChannelCount(&nChannels);
		if (FAILED(hr)) {
			err_outf("GetChannelCount failed: hr = %08x\n", hr); 
		} else {
			for (UINT nChannel = 0; nChannel < nChannels; nChannel++) {
				float dMinLevelDB, dMaxLevelDB, dStepping;
				hr = pLevel->GetLevelRange(nChannel, &dMinLevelDB, &dMaxLevelDB, &dStepping);
				if (FAILED(hr)) {
					err_outf("GetLevelRange failed [%i]: hr = 0x%08x\n", nChannel, hr);
				} else {
					info_outf("Setting level of channel %i to %.0f dB\n", nChannel, dMaxLevelDB);
					hr = pLevel->SetLevel(nChannel, dMaxLevelDB, NULL);
					if (FAILED(hr)) {
						err_outf("GetLevel failed: hr = 0x%08x\n", hr);
					}
				}
			}
		}
		SAFE_RELEASE(pLevel);
	}
}

/// dVolume is in 0.0 to 1.0 range.
void ProcessVolume(float dVolume, IMMDevice *pDevice) {
	IAudioEndpointVolume *pEndpointVolume = NULL;
	HRESULT hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&pEndpointVolume);
	if (FAILED(hr)) {
		err_outf("Could not activate endpoint volume interface: hr = 0x%08x\n", hr);
	} else {
		info_outf("Setting volume to %f\n", dVolume);
		hr = pEndpointVolume->SetMasterVolumeLevelScalar(dVolume, NULL);
		if (FAILED(hr)) {
			err_outf("Failed to set volume to %f: hr = 0x%08x\n", dVolume, hr);
		}
	}
	SAFE_RELEASE(pEndpointVolume);
}

void ProcessMute(IPart *pPart) {
	// see if this is a mute node part
	IAudioMute *pMute = NULL;
	HRESULT hr = pPart->Activate(CLSCTX_ALL, __uuidof(IAudioMute), (void**)&pMute);
	if (E_NOINTERFACE == hr) {
		// not a mute node
	} else if (FAILED(hr)) {
		err_outf("Unexpected failure trying to activate IAudioMute: hr = 0x%08x\n", hr);
	} else {
		// it's a mute node...
		BOOL bMuted = FALSE;
		info_outf("Unmuting\n");
		hr = pMute->SetMute(bMuted, NULL);
		if (FAILED(hr)) {
			err_outf("Unmuting failed: hr = 0x%08x\n", hr);
		}
		SAFE_RELEASE(pMute);
	}
}

void ProcessPartTree(MotionDeviceContext * pContext, IMMDeviceEnumerator *pEnum, IMMDevice *pDevice, IPart *pPart) {
	// Process this part, check for relevant changes.
	ProcessBassBoost(pEnum, pDevice, pPart);
	ProcessMute(pPart);										//< Unmute if applicable
	ProcessLevelMax(__uuidof(IAudioBass), pPart);			//< No Bass reduction if applicable
	ProcessLevelMax(__uuidof(IAudioTreble), pPart);			//< No Treble reduction if applicable
	// get the list of incoming parts
	IPartsList *pIncomingParts = NULL;
	HRESULT hr = pPart->EnumPartsIncoming(&pIncomingParts);
	if (E_NOTFOUND == hr) {
		// not an error... we have just reached the end of the path
	} else if (FAILED(hr)) {
		err_outf("Couldn't enum incoming parts: hr = 0x%08x\n", hr);
	} else {
		UINT nParts = 0;
		hr = pIncomingParts->GetCount(&nParts);
		if (FAILED(hr)) {
			err_outf("Couldn't get count of incoming parts: hr = 0x%08x\n", hr);
		} else {
			// walk the tree on each incoming part recursively
			for (UINT n = 0; n < nParts; n++) {
				IPart *pIncomingPart = NULL;
				hr = pIncomingParts->GetPart(n, &pIncomingPart);
				if (FAILED(hr)) {
					err_outf("Couldn't get part #%u (0-based) of %u (1-based) hr = 0x%08x\n", n, nParts, hr);
				} else {
					ProcessPartTree(pContext, pEnum, pDevice, pIncomingPart);
				}
				SAFE_RELEASE(pIncomingPart);
			}
		}
		SAFE_RELEASE(pIncomingParts);
	}
}

void ProcessEndpoint(MotionDeviceContext * pContext, IMMDeviceEnumerator *pEnum, IMMDevice *pDevice) {
	// Handle global output volume
	ProcessVolume(1.0f, pDevice);							//< Max hardware volume level (software controlled volume)
	// get device topology object for that endpoint
	IDeviceTopology *pDT = NULL;
	HRESULT hr = pDevice->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL, NULL, (void**)&pDT);
	if (FAILED(hr)) {
		err_outf("Couldn't get device topology object: hr = 0x%08x\n", hr);
	} else {
		// get the single connector for that endpoint
		IConnector *pConnEndpoint = NULL;
		hr = pDT->GetConnector(0, &pConnEndpoint);
		if (FAILED(hr)) {
			err_outf("Couldn't get the connector on the endpoint: hr = 0x%08x\n", hr);
		} else {
			// get the connector on the device that is connected to the connector on the endpoint
			IConnector *pConnDevice = NULL;
			hr = pConnEndpoint->GetConnectedTo(&pConnDevice);
			if (FAILED(hr)) {
				err_outf("Couldn't get the connector on the device: hr = 0x%08x\n", hr);
			} else {
				// QI on the device's connector for IPart
				IPart *pPart = NULL;
				hr = pConnDevice->QueryInterface(__uuidof(IPart), (void**)&pPart);
				if (FAILED(hr)) {
					err_outf("Couldn't get the part: hr = 0x%08x\n", hr);
				} else {
					// all the real work is done in this function
					ProcessPartTree(pContext, pEnum, pDevice, pPart);
					SAFE_RELEASE(pPart);
				}
				SAFE_RELEASE(pConnDevice);
			}
			SAFE_RELEASE(pConnEndpoint);
		}
		SAFE_RELEASE(pDT);
	}
}

BOOL IsContextDevice(LPCWSTR sDeviceName, IMMDevice * pDevice) {
	BOOL bMatch = FALSE;
	IPropertyStore* pProperties;
	HRESULT hr = pDevice->OpenPropertyStore(STGM_READ, &pProperties);
	if (FAILED(hr)) {
		err_outf("Error opening property store: hr = 0x%08x\n", hr);
	} else {
		PROPVARIANT value;
		PropVariantInit(&value);
		hr = pProperties->GetValue(PKEY_Device_FriendlyName, &value);
		if (FAILED(hr)) {
			err_outf("Error getting friendly name: hr = 0x%08x\n", hr);
		} else {
			info_outf("Device: %ws\n", value.pwszVal);
			if (!value.pwszVal) {
				err_outf("No friendly name\n");
			} else {
				if (wcsstr(value.pwszVal, sDeviceName) != 0) {
					// Found device
					bMatch = true;
				}
			}
		}
		PropVariantClear(&value);
	}
	return bMatch;
}

void ProcessEnumerator(MotionDeviceContext * pContext, IMMDeviceEnumerator *pEnum) {
	IMMDeviceCollection *pEndPoints = NULL; 
	HRESULT hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pEndPoints);
	if (FAILED(hr) || pEndPoints == NULL) {
		err_outf("Couldn't get render device enumerator: hr = 0x%08x\n", hr);
	} else {
		UINT nEndPoints;
		hr = pEndPoints->GetCount(&nEndPoints);
		if (FAILED(hr)) {
			err_outf("Error getting enumerator count: hr = 0x%08x\n", hr);
		} else {
			info_outf("Searching for: %ws\n", pContext->sDeviceName);
			IMMDevice *pDevice = NULL;
			for (UINT nEndPoint = 0; nEndPoint < nEndPoints; nEndPoint++) {
				hr = pEndPoints->Item(nEndPoint, &pDevice);
				if (FAILED(hr)) {
					err_outf("Error getting endpoint #%i: hr = 0x%08x\n", nEndPoint, hr);
				} else {
					if (IsContextDevice(pContext->sDeviceName, pDevice)) {
						ProcessEndpoint(pContext, pEnum, pDevice);
					}
					SAFE_RELEASE(pDevice);
				}
			}
		}
		SAFE_RELEASE(pEndPoints);
	}
}

void VistaMotionDeviceInit(const WCHAR * sDeviceName) 
{
	MotionDeviceContext oContext;
	oContext.sDeviceName = sDeviceName;
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr) && hr != S_FALSE) {
		err_outf("CoInitialize Warning: hr = 0x%08x\n", hr);
	} 
	IMMDeviceEnumerator *pEnum = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
	if (FAILED(hr)) {
		err_outf("Couldn't get device enumerator: hr = 0x%08x\n", hr);
	} else {
		ProcessEnumerator(&oContext, pEnum);
		SAFE_RELEASE(pEnum);
	}
	CoUninitialize();
}