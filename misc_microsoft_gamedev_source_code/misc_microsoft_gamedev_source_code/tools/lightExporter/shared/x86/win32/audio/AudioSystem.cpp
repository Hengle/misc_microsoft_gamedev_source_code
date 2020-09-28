// Audio System -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "AudioSystem.h"

// Methods.

AudioSystem::AudioSystem()
{
	dsound = NULL;
}

AudioSystem::~AudioSystem()
{
	SAFE_RELEASE(dsound);
}

float AudioSystem::GetTime()
{
	if (tracks.empty())
		return 0.0f;

	HRESULT hr;
	Tracks::iterator iter = tracks.begin();
	
	DWORD play, write;
	hr = iter->buffer->GetCurrentPosition(&play, &write);
	
	return static_cast<float>(play) / (iter->frequency * iter->sampleSize);
}

bool AudioSystem::LoadTrack(const std::string &filename, float offset, float scale)
{
	HRESULT hr;

	TrackLoader *loader = TrackLoader::CreateTrackLoader(filename);
	if (loader == NULL)
		return false;

	int formatSize = sizeof(WAVEFORMATEX) + loader->GetFormat()->cbSize;
	WAVEFORMATEX *formatCopy = reinterpret_cast<WAVEFORMATEX *>(new BYTE[formatSize]);
	memcpy(formatCopy, loader->GetFormat(), formatSize);

	DSBUFFERDESC bufferDesc =
	{
		sizeof(bufferDesc),				// dwSize
		DSBCAPS_CTRLFREQUENCY |
		DSBCAPS_CTRLVOLUME |
		DSBCAPS_GETCURRENTPOSITION2 |
		DSBCAPS_GLOBALFOCUS,			// dwFlags
		loader->GetLength(),			// dwBufferBytes
		0,								// dwReserved
		formatCopy,						// lpwfxFormat
		GUID_NULL,						// guid3DAlgorithm
	};

	IDirectSoundBuffer *buffer;
	
	hr = dsound->CreateSoundBuffer(&bufferDesc, &buffer, NULL);
	SAFE_DELETE_ARRAY(formatCopy);
	if (FAILED(hr))
	{
		delete loader;
		return false;
	}
	
	void *lockData[2];
	DWORD lockSize[2];
	
	hr = buffer->Lock(
		0,
		0,
		&lockData[0],
		&lockSize[0],
		&lockData[1],
		&lockSize[1],
		DSBLOCK_ENTIREBUFFER
	);
	if (FAILED(hr))
	{
		delete loader;
		return false;
	}
	
	DWORD temp = 0;
	for (int section = 0; section < 2; ++section)
	{
		memcpy(
			lockData[section],
			&reinterpret_cast<const BYTE *>(loader->GetData())[temp],
			lockSize[section]
		);
		temp += lockSize[section];
	}
	
	hr = buffer->Unlock(lockData[0], lockSize[0], lockData[1], lockSize[1]);
	if (FAILED(hr))
	{
		delete loader;
		return false;
	}
	
	Track track;
	
	track.buffer = buffer;
	track.frequency = static_cast<float>(loader->GetFormat()->nSamplesPerSec);
	track.length = static_cast<float>(loader->GetLength()) / track.frequency;
	track.sampleSize = static_cast<float>(loader->GetFormat()->nBlockAlign);
	track.offset = offset;
	track.scale = scale;
	
//	hr = track.buffer->SetFrequency(static_cast<DWORD>(track.frequency * track.scale));
//	hr = track.buffer->SetCurrentPosition(
//		static_cast<DWORD>(track.offset * track.sampleSize * track.frequency)
//	);
	
	tracks.push_back(track);
	
	delete loader;
	return true;
}

void AudioSystem::Seek(float time)
{
	HRESULT hr;

	Tracks::iterator iter = tracks.begin();
	for (; iter != tracks.end(); ++iter)
	{
		float position = iter->offset + time;
		if (position < 0) position += iter->length * iter->scale;
		position = fmod(position, iter->length);
		position *= iter->sampleSize * iter->frequency;
	
		hr = iter->buffer->SetCurrentPosition(static_cast<DWORD>(position));
	}
}

void AudioSystem::Play(float rate)
{
	HRESULT hr;

	Tracks::iterator iter = tracks.begin();
	for (; iter != tracks.end(); ++iter)
	{
		hr = iter->buffer->SetFrequency(
			static_cast<DWORD>(iter->frequency * iter->scale * rate)
		);
	
		hr = iter->buffer->Play(0, 0, DSBPLAY_LOOPING);
	}
}

void AudioSystem::Stop()
{
	HRESULT hr;

	Tracks::iterator iter = tracks.begin();
	for (; iter != tracks.end(); ++iter)
	{
		hr = iter->buffer->Stop();
	}
}

void AudioSystem::Clear()
{
	Tracks::iterator iter = tracks.begin();
	for (; iter != tracks.end(); ++iter)
	{
		iter->buffer->Stop();
		iter->buffer->Release();
	}
	
	tracks.clear();
}

// Static methods.

AudioSystem * AudioSystem::CreateAudioSystem(HWND hWnd)
{
	AudioSystem *audio = new AudioSystem();
	HRESULT hr;
	
	hr = DirectSoundCreate8(NULL, &audio->dsound, NULL);
	if (FAILED(hr))
	{
		delete audio;
		return NULL;
	}
	
	hr = audio->dsound->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	if (FAILED(hr))
	{
		delete audio;
		return NULL;
	}
	
	return audio;
}