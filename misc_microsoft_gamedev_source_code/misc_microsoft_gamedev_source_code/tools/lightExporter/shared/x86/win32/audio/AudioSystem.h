// Audio System -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _AUDIOSYSTEM_H_
#define _AUDIOSYSTEM_H_

#include "common.h"

// Classes.

class AudioSystem
{
	private:
		struct Track
		{
			IDirectSoundBuffer	*buffer;
			float				length;
			float				frequency;
			float				sampleSize;
			float				offset;
			float				scale;
		};
		
		typedef std::vector<Track> Tracks;
	
		IDirectSound8		*dsound;
		
		Tracks				tracks;
	
		// Private constructor.
		AudioSystem();
		
		// Do NOT copy construct or assign.
		AudioSystem(const AudioSystem &) { };
		AudioSystem & operator =(const AudioSystem &) { return *this; };
	
	public:
		~AudioSystem();
		
		float GetTime();
		
		bool LoadTrack(const std::string &filename, float offset = 0.0f, float scale = 1.0f);

		void Seek(float time);
		void Play(float rate = 1.0f);
		void Stop();
		
		void Clear();
	
		static AudioSystem * CreateAudioSystem(HWND hWnd);
};

class TrackLoader
{
	private:
//		IGraphBuilder	*graph;
//		IMediaFilter	*mediaFilter;
//		IBaseFilter		*source;
//		IBaseFilter		*grabberBase;
//		ISampleGrabber	*grabber;
//		IBaseFilter		*null;
//		AM_MEDIA_TYPE	mediaType;
//		IMediaControl	*mediaControl;
//		IMediaEvent		*mediaEvent;

		HMMIO			hmmio;
		
		WAVEFORMATEX	*format;
		void			*data;
		unsigned int	length;
	
		// Private constructor.
		TrackLoader();
		
		// Do NOT copy construct or assign.
		TrackLoader(const TrackLoader &) { };
		TrackLoader & operator =(const TrackLoader &) { return *this; };

//		static WCHAR * WideFromAscii(const char *string);
//		static IPin * GetAvailablePin(IBaseFilter *filter, PIN_DIRECTION direction);

	public:
		~TrackLoader();
		
		const WAVEFORMATEX * GetFormat() const;
		const void * GetData() const;
		unsigned int GetLength() const;
		
		static TrackLoader * CreateTrackLoader(const std::string &filename);
};

#endif /* _AUDIOSYSTEM_H_ */