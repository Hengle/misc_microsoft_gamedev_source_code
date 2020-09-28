// Audio System -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _AUDIO_COMMON_INL_
#define _AUDIO_COMMON_INL_

// Macros.

#ifndef SAFE_DELETE
	#define SAFE_DELETE(p) { delete (p); (p) = NULL; }
#endif
#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p) { delete [] (p); (p) = NULL; }
#endif
#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p) { if ((p) != NULL) { (p)->Release(); (p) = NULL; } }
#endif

#ifndef MMFAILED
	#define MMFAILED(mmr) (mmr != MMSYSERR_NOERROR)
#endif	

#ifndef NOTIMPL
	#define NOTIMPL _RPTF0(_CRT_ASSERT, "Not Implemented.")
#endif	

// Inlines.

#endif /* _AUDIO_COMMON_INL_ */