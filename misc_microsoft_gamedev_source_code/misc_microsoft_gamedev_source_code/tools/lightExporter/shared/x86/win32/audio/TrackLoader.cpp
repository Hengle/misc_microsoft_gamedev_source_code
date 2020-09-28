// Audio System -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "AudioSystem.h"

// Debug methods.

#ifdef _DEBUG
/*
// NOTE: This method was borrowed from the DirectShow SDK.
HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath) 
{
    const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
    HRESULT hr;
    
    IStorage *pStorage = NULL;
    hr = StgCreateDocfile(
        wszPath,
        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        0, &pStorage);
    if(FAILED(hr)) 
    {
        return hr;
    }

    IStream *pStream;
    hr = pStorage->CreateStream(
        wszStreamName,
        STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
        0, 0, &pStream);
    if (FAILED(hr)) 
    {
        pStorage->Release();    
        return hr;
    }

    IPersistStream *pPersist = NULL;
    pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
    hr = pPersist->Save(pStream, TRUE);
    pStream->Release();
    pPersist->Release();
    if (SUCCEEDED(hr)) 
    {
        hr = pStorage->Commit(STGC_DEFAULT);
    }
    pStorage->Release();
    return hr;
}
*/
#endif /* _DEBUG */

// Methods.

TrackLoader::TrackLoader()
{
//	graph = NULL;
//	mediaFilter = NULL;
//	source = NULL;
//	grabberBase = NULL;
//	grabber = NULL;
//	null = NULL;
//	ZeroMemory(&mediaType, sizeof(mediaType));
//	mediaControl = NULL;
//	mediaEvent = NULL;

	hmmio = NULL;

	format = NULL;
	data = NULL;
	length = 0;
}

TrackLoader::~TrackLoader()
{
	SAFE_DELETE_ARRAY(data);
	SAFE_DELETE_ARRAY(format);
	
	if (hmmio != NULL)
		mmioClose(hmmio, 0);

//	SAFE_RELEASE(mediaEvent);
//	SAFE_RELEASE(mediaControl);
//	if (mediaType.pbFormat != NULL)
//		CoTaskMemFree(mediaType.pbFormat);
//	SAFE_RELEASE(mediaType.pUnk);
//	SAFE_RELEASE(null);
//	SAFE_RELEASE(grabber);
//	SAFE_RELEASE(grabberBase);
//	SAFE_RELEASE(source);
//	SAFE_RELEASE(mediaFilter);
//	SAFE_RELEASE(graph);
}

const WAVEFORMATEX * TrackLoader::GetFormat() const
{
//	return reinterpret_cast<const WAVEFORMATEX *>(mediaType.pbFormat);
	return format;
}

const void * TrackLoader::GetData() const
{
	return data;
}

unsigned int TrackLoader::GetLength() const
{
	return length;
}

// Static methods.
/*
WCHAR * TrackLoader::WideFromAscii(const char *string)
{
	const char *empty = "";
	if (string == NULL) string = empty;

	int length = MultiByteToWideChar(CP_ACP, 0, string, -1, NULL, 0);
	WCHAR *wide = new WCHAR[length + 1];
	MultiByteToWideChar(CP_ACP, 0, string, -1, wide, length);
	
	return wide;
}

IPin * TrackLoader::GetAvailablePin(IBaseFilter *filter, PIN_DIRECTION direction)
{
	if (filter == NULL)
		return NULL;

	HRESULT hr;
	IEnumPins *pins = NULL;
	IPin *pin = NULL;
	
	hr = filter->EnumPins(&pins);
	if (FAILED(hr))
		return NULL;

	for (;;)
	{	
		hr = pins->Next(1, &pin, NULL);
		_ASSERTE(SUCCEEDED(hr));
		
		if (hr == S_FALSE)
		{
			SAFE_RELEASE(pins);
			return NULL;
		}
		
		PIN_INFO info;
		
		hr = pin->QueryPinInfo(&info);
		_ASSERTE(SUCCEEDED(hr));
		
		if (info.dir == direction)
		{
			SAFE_RELEASE(pins);
			return pin;
		}
		
		SAFE_RELEASE(pin);
	}
}
*/
TrackLoader * TrackLoader::CreateTrackLoader(const std::string &filename)
{
	TrackLoader *loader = new TrackLoader();
/*	HRESULT hr;
	
	hr = CoCreateInstance(
		CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC,
		IID_IGraphBuilder,
		reinterpret_cast<void **>(&loader->graph)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = loader->graph->QueryInterface(
		IID_IMediaFilter,
		reinterpret_cast<void **>(&loader->mediaFilter)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
	
	hr = loader->mediaFilter->SetSyncSource(NULL);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
	
	WCHAR *wideFilename = WideFromAscii(filename.data());
	hr = loader->graph->AddSourceFilter(wideFilename, L"Source", &loader->source);
	SAFE_DELETE_ARRAY(wideFilename);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = CoCreateInstance(
		CLSID_SampleGrabber,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IBaseFilter,
		reinterpret_cast<void **>(&loader->grabberBase)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
	
	hr = loader->grabberBase->QueryInterface(
		IID_ISampleGrabber,
		reinterpret_cast<void **>(&loader->grabber)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = loader->graph->AddFilter(loader->grabberBase, L"Grabber");
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	loader->mediaType.majortype = MEDIATYPE_Audio;
	loader->mediaType.subtype = MEDIASUBTYPE_PCM;

	hr = loader->grabber->SetMediaType(&loader->mediaType);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = loader->grabber->SetBufferSamples(TRUE);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
	
	IPin *sourceOut = GetAvailablePin(loader->source, PINDIR_OUTPUT);
	IPin *grabberIn = GetAvailablePin(loader->grabberBase, PINDIR_INPUT);
	
	hr = loader->graph->Connect(sourceOut, grabberIn);
	SAFE_RELEASE(grabberIn);
	SAFE_RELEASE(sourceOut);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = CoCreateInstance(
		CLSID_NullRenderer,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IBaseFilter,
		reinterpret_cast<void **>(&loader->null)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = loader->graph->AddFilter(loader->null, L"Null");
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	IPin *grabberOut = GetAvailablePin(loader->grabberBase, PINDIR_OUTPUT);
	IPin *nullIn = GetAvailablePin(loader->null, PINDIR_INPUT);
	
	hr = loader->graph->Connect(grabberOut, nullIn);
	SAFE_RELEASE(grabberOut);
	SAFE_RELEASE(nullIn);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
*/
#ifdef _DEBUG
//	SaveGraphFile(loader->graph, L"trackloader.grf");
#endif /* _DEBUG */
/*
	hr = loader->grabber->GetConnectedMediaType(&loader->mediaType);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
	
	if (loader->mediaType.formattype != FORMAT_WaveFormatEx)
	{
		delete loader;
		return NULL;
	}

	hr = loader->graph->QueryInterface(
		IID_IMediaControl,
		reinterpret_cast<void **>(&loader->mediaControl)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	hr = loader->graph->QueryInterface(
		IID_IMediaEvent,
		reinterpret_cast<void **>(&loader->mediaEvent)
	);
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
	
	hr = loader->mediaControl->Run();
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}

	for (;;)
	{
		FILTER_STATE state;
		hr = loader->mediaControl->GetState(
			1,
			reinterpret_cast<OAFilterState *>(&state)
		);
		if (FAILED(hr))
		{
			delete loader;
			return NULL;
		}
			
		if (state == State_Running && hr == S_OK)
			break;
		
		Sleep(1);
	}
	
//	long eventCode;
//	hr = loader->mediaEvent->WaitForCompletion(INFINITE, &eventCode);
//	if (FAILED(hr))
//	{
//		delete loader;
//		return NULL;
//	}
//	
//	if (eventCode != EC_COMPLETE)
//	{
//		delete loader;
//		return NULL;
//	}
//	
//	hr = loader->grabber->GetCurrentBuffer(
//		reinterpret_cast<long *>(&loader->length),
//		NULL
//	);
//	if (FAILED(hr))
//	{
//		delete loader;
//		return NULL;
//	}
//	
//	loader->data = new BYTE[loader->length];
//
//	hr = loader->grabber->GetCurrentBuffer(
//		reinterpret_cast<long *>(&loader->length),
//		reinterpret_cast<long *>(loader->data)
//	);
//	if (FAILED(hr))
//	{
//		delete loader;
//		return NULL;
//	}

	for (;;)
	{
		long eventCode, params[2];
	
		hr = loader->mediaEvent->GetEvent(&eventCode, &params[0], &params[1], 1);
		if (SUCCEEDED(hr))
		{
			hr = loader->mediaEvent->FreeEventParams(eventCode, params[0], params[1]);
			if (FAILED(hr))
			{
				delete loader;
				return NULL;
			}
			
			if (eventCode == EC_COMPLETE)
				break;
		}
		
		long length;
		
		hr = loader->grabber->GetCurrentBuffer(&length, NULL);
		if (FAILED(hr))
		{
			delete loader;
			return NULL;
		}
		
		BYTE *data = new BYTE[length];
		
		hr = loader->grabber->GetCurrentBuffer(
			&length,
			reinterpret_cast<long *>(data)
		);
		if (FAILED(hr))
		{
			delete loader;
			return NULL;
		}
		
		if (loader->data != NULL)
		{
			BYTE *concat = new BYTE[loader->length + length];
			memcpy(concat, loader->data, loader->length);
			memcpy(&concat[loader->length], data, length);
			
			SAFE_DELETE_ARRAY(loader->data);
			SAFE_DELETE_ARRAY(data);
			
			loader->data = concat;
			loader->length += length;
		}
		else
		{
			loader->data = data;
			loader->length = length;
		}
	}

	hr = loader->mediaControl->Stop();
	if (FAILED(hr))
	{
		delete loader;
		return NULL;
	}
*/
	MMRESULT mmr;

	char filenameCopy[MAX_PATH];
	ZeroMemory(filenameCopy, sizeof(filenameCopy));
	strncpy(filenameCopy, filename.data(), sizeof(filenameCopy) - 1);
	
	MMIOINFO info;
	ZeroMemory(&info, sizeof(info));
	
	loader->hmmio = mmioOpen(filenameCopy, &info, MMIO_READ);
	if (loader->hmmio == NULL)
	{
		delete loader;
		return NULL;
	}
	
	MMCKINFO riff;
	mmr = mmioDescend(loader->hmmio, &riff, NULL, 0);
	if (MMFAILED(mmr))
	{
		delete loader;
		return NULL;
	}
	
	if ((riff.ckid != FOURCC_RIFF) || (riff.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
	{
		delete loader;
		return NULL;
	}
	
	MMCKINFO fmt;
	ZeroMemory(&fmt, sizeof(fmt));
	fmt.ckid = mmioFOURCC('f', 'm', 't', ' ');

	mmr = mmioDescend(loader->hmmio, &fmt, &riff, MMIO_FINDCHUNK);
	if (MMFAILED(mmr))
	{
		delete loader;
		return NULL;
	}

	int formatSize = max(fmt.cksize, sizeof(WAVEFORMATEX));
	loader->format = reinterpret_cast<WAVEFORMATEX *>(
		new BYTE[formatSize]
	);
	ZeroMemory(loader->format, formatSize);
	
	LONG bytesRead;
	
	bytesRead = mmioRead(loader->hmmio, reinterpret_cast<HPSTR>(loader->format), fmt.cksize);
	if (bytesRead != fmt.cksize)
	{
		delete loader;
		return NULL;
	}
	
	mmr = mmioAscend(loader->hmmio, &fmt, 0);
	if (MMFAILED(mmr))
	{
		delete loader;
		return NULL;
	}

	LONG newPos;
	
	newPos = mmioSeek(loader->hmmio, riff.dwDataOffset + sizeof(FOURCC), SEEK_SET);
	if (newPos == -1)
	{
		delete loader;
		return NULL;
	}
	
	MMCKINFO data;
	ZeroMemory(&data, sizeof(data));
	data.ckid = mmioFOURCC('d', 'a', 't', 'a');
	
	mmr = mmioDescend(loader->hmmio, &data, &riff, MMIO_FINDCHUNK);
	if (MMFAILED(mmr))
	{
		delete loader;
		return NULL;
	}
	
	loader->data = new BYTE[data.cksize];
	loader->length = data.cksize;

	bytesRead = mmioRead(loader->hmmio, reinterpret_cast<HPSTR>(loader->data), data.cksize);
	if (bytesRead != data.cksize)
	{
		delete loader;
		return NULL;
	}

	return loader;
}