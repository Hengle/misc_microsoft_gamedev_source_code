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


#include <AK\SoundEngine\Common\AkTypes.h>

#define DBOX_FRONT_RIGHT 	0x1
#define DBOX_FRONT_LEFT 	0x2
#define DBOX_BACK_RIGHT 	0x4
#define DBOX_BACK_LEFT 		0x8

#define DBOX_CHANNELS 4
#define DBOX_CHANNEL_SETUP	(DBOX_FRONT_RIGHT | DBOX_FRONT_LEFT | DBOX_BACK_RIGHT | DBOX_BACK_LEFT )
#define DBOX_SAMPLERATE 375
#define DBOX_DRIVER_NAME L"KineAudio"
#define DBOX_PRODUCT_ID 0x0102
#define DBOX_VENDOR_ID 0x15C9
#define DBOX_OUTPUT_RATE 8000
#define DBOX_BITS_PER_SAMPLE 16
#define DBOX_SAMPLE_TYPE AkInt16
