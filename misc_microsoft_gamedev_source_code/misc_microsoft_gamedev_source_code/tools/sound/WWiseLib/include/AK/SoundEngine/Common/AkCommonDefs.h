//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkCommonDefs.h

/// \file 
/// AudioLib common defines, enums, and structs.


#ifndef _AK_COMMON_DEFS_H_
#define _AK_COMMON_DEFS_H_

#include <AK/SoundEngine/Common/AkTypes.h>

//-----------------------------------------------------------------------------
// AUDIO DATA FORMAT
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants for AkDataCompID that define the type of data 
// PCM, ADPCM, and so on
//-----------------------------------------------------------------------------
const AkDataCompID      AK_PCM              = 0;        ///< PCM
const AkDataCompID      AK_ADPCM            = 1;        ///< ADPCM compression

const AkDataTypeID		AK_INT				= 0;		///< Integer data type (uchar, short, and so on)
const AkDataTypeID		AK_FLOAT			= 1;		///< Float data type

const AkDataInterleaveID AK_INTERLEAVED		= 0;		///< Interleaved data
const AkDataInterleaveID AK_NONINTERLEAVED	= 1;		///< Non-interleaved data

// Native format currently the same on all supported platforms, may become platform specific in the future
const AkUInt32 AK_LE_NATIVE_BITSPERSAMPLE  = 32;					///< Native number of bits per sample.
const AkUInt32 AK_LE_NATIVE_SAMPLETYPE = AK_FLOAT;					///< Native data type.
const AkUInt32 AK_LE_NATIVE_INTERLEAVE = AK_NONINTERLEAVED;			///< Native interleaved setting.
const AkUInt32 AK_LE_NATIVE_FORMAT = AK_PCM;						///< Native audio format.

/// AkAudioFormat bitfields
// |1 0 9 8|7 6 5 4|3 2 1 0|9 8 7 6|5 4 3 2|1 0    |       |       |
// |3 3 2 2|2 2 2 2|2 2 2 2|1 1 1 1|1 1 1 1|1 1 9 8|7 6 5 4|3 2 1 0|
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                     | I | T | C | BlckAlg |  BtsPSmp  | NumCh |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#define AKAUDIOFORMAT_NUMCHANNELS_MASK		0x0000000F ///< Mask for NumChannels
#define AKAUDIOFORMAT_BITSPERSAMPLE_MASK	0x0000003F ///< Mask for BitsPerSample
#define AKAUDIOFORMAT_BLOCKALIGN_MASK		0x0000001F ///< Mask for BlockAlign
#define AKAUDIOFORMAT_COMPID_MASK			0x00000003 ///< Mask for CompID
#define AKAUDIOFORMAT_TYPEID_MASK			0x00000003 ///< Mask for TypeID
#define AKAUDIOFORMAT_INTERLEAVEID_MASK		0x00000003 ///< Mask for InterleaveID

#define AKAUDIOFORMAT_NUMCHANNELS_SHIFT		0  ///< Shift for NumChannels
#define AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT	4  ///< Shift for BitsPerSample
#define AKAUDIOFORMAT_BLOCKALIGN_SHIFT		10 ///< Shift for BlockAlign
#define AKAUDIOFORMAT_COMPID_SHIFT			15 ///< Shift for CompID
#define AKAUDIOFORMAT_TYPEID_SHIFT			17 ///< Shift for TypeID
#define AKAUDIOFORMAT_INTERLEAVEID_SHIFT	19 ///< Shift for InterleaveID


/// Defines the parameters of an audio buffer format.
struct AkAudioFormat
{
    AkUInt32	uSampleRate;	///< Number of samples per second

	AkUInt32	uFormatBits;	///< AkAudioFormat bitfields

	/// Get the number of channels.
	/// \return The number of channels
	AkUInt32 GetNumChannels()							
	{ return ((uFormatBits >> AKAUDIOFORMAT_NUMCHANNELS_SHIFT) & AKAUDIOFORMAT_NUMCHANNELS_MASK); }

	/// Get the number of bits per sample.
	/// \return The number of bits per sample
	AkUInt32 GetBitsPerSample()							
	{ return ((uFormatBits >> AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT)	& AKAUDIOFORMAT_BITSPERSAMPLE_MASK); }

	/// Get the block alignment.
	/// \return The block alignment
	AkUInt32 GetBlockAlign()							
	{ return ((uFormatBits >> AKAUDIOFORMAT_BLOCKALIGN_SHIFT)	& AKAUDIOFORMAT_BLOCKALIGN_MASK); }

	/// Get the data compression type.
	/// \return The data compression type
	AkUInt32 GetCompID()								
	{ return ((uFormatBits >> AKAUDIOFORMAT_COMPID_SHIFT)	& AKAUDIOFORMAT_COMPID_MASK); }

	/// Get the data sample format (Float or Integer).
	/// \return The data sample format
	AkUInt32 GetTypeID()								
	{ return ((uFormatBits >> AKAUDIOFORMAT_TYPEID_SHIFT)	& AKAUDIOFORMAT_TYPEID_MASK); }

	/// Get the interleaved type.
	/// \return The interleaved type
	AkUInt32 GetInterleaveID()							
	{ return ((uFormatBits >> AKAUDIOFORMAT_INTERLEAVEID_SHIFT)	& AKAUDIOFORMAT_INTERLEAVEID_MASK); }

	/// Set the number of channels.
	void SetNumChannels(
		AkUInt32 uiNum	///< Number of channels
		)				
	{ 
		uFormatBits &= ~(AKAUDIOFORMAT_NUMCHANNELS_MASK << AKAUDIOFORMAT_NUMCHANNELS_SHIFT);
		uFormatBits |= (uiNum & AKAUDIOFORMAT_NUMCHANNELS_MASK) << AKAUDIOFORMAT_NUMCHANNELS_SHIFT; 
	}

	/// Set the number of bits per sample.
	void SetBitsPerSample(
		AkUInt32 uiBps	///< Number of bits per sample
		)			
	{ 
		uFormatBits &= ~(AKAUDIOFORMAT_BITSPERSAMPLE_MASK << AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT);
		uFormatBits |= ((uiBps & AKAUDIOFORMAT_BITSPERSAMPLE_MASK) << AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT); 
	}

	/// Set the block alignment.
	void SetBlockAlign(
		AkUInt32 uiBlA	///< Block alignment
		)				
	{ 
		uFormatBits &= ~(AKAUDIOFORMAT_BLOCKALIGN_MASK << AKAUDIOFORMAT_BLOCKALIGN_SHIFT);
		uFormatBits |= ((uiBlA & AKAUDIOFORMAT_BLOCKALIGN_MASK) << AKAUDIOFORMAT_BLOCKALIGN_SHIFT); 
	}

	/// Set the data compression type.
	void SetCompID(
		AkUInt32 uiCID	///< Data compression type
		)					
	{ 
		uFormatBits &= ~(AKAUDIOFORMAT_COMPID_MASK << AKAUDIOFORMAT_COMPID_SHIFT);
		uFormatBits |= ((uiCID & AKAUDIOFORMAT_COMPID_MASK) << AKAUDIOFORMAT_COMPID_SHIFT); 
	}

	/// Set the data sample format (Float or Integer).
	void SetTypeID(
		AkUInt32 uiTID	///< Data sample format
		)					
	{ 
		uFormatBits &= ~(AKAUDIOFORMAT_TYPEID_MASK << AKAUDIOFORMAT_TYPEID_SHIFT);
		uFormatBits |= ((uiTID & AKAUDIOFORMAT_TYPEID_MASK) << AKAUDIOFORMAT_TYPEID_SHIFT); 
	}

	/// Set the interleaved type.
	void SetInterleaveID(
		AkUInt32 uiIID	///< Interleaved type
		)			
	{ 
		uFormatBits &= ~(AKAUDIOFORMAT_INTERLEAVEID_MASK << AKAUDIOFORMAT_INTERLEAVEID_SHIFT);
		uFormatBits |= ((uiIID & AKAUDIOFORMAT_INTERLEAVEID_MASK) << AKAUDIOFORMAT_INTERLEAVEID_SHIFT); 
	}

	/// Set all parameters of the audio format structure.
	void SetAll(
		AkUInt32    in_uSampleRate,		///< Number of samples per second
		AkUInt32	in_uNumChannels,	///< Number of channels
		AkUInt32    in_uBitsPerSample,	///< Number of bits per sample
		AkUInt32    in_uBlockAlign,		///< Block alignment
		AkUInt32    in_uCompID,			///< Data compression type
		AkUInt32    in_uTypeID,			///< Data sample format (Float or Integer)
		AkUInt32    in_uInterleaveID	///< Interleaved type
		)
	{
		uSampleRate = in_uSampleRate;
		uFormatBits =  ((in_uNumChannels	& AKAUDIOFORMAT_NUMCHANNELS_MASK)	<< AKAUDIOFORMAT_NUMCHANNELS_SHIFT)
					|  ((in_uBitsPerSample	& AKAUDIOFORMAT_BITSPERSAMPLE_MASK) << AKAUDIOFORMAT_BITSPERSAMPLE_SHIFT)
					|  ((in_uBlockAlign		& AKAUDIOFORMAT_BLOCKALIGN_MASK)	<< AKAUDIOFORMAT_BLOCKALIGN_SHIFT)
					|  ((in_uCompID			& AKAUDIOFORMAT_COMPID_MASK)		<< AKAUDIOFORMAT_COMPID_SHIFT)
					|  ((in_uTypeID			& AKAUDIOFORMAT_TYPEID_MASK)		<< AKAUDIOFORMAT_TYPEID_SHIFT)
					|  ((in_uInterleaveID	& AKAUDIOFORMAT_INTERLEAVEID_MASK)	<< AKAUDIOFORMAT_INTERLEAVEID_SHIFT);
	}

	// Uncompressed definition, for reference only.
	// AkUInt32				uNbChannels;                // [1, 8] Number of channels
	// AkUInt32				uBitsPerSample;             // [8, 32] Number of bits per sample
    // AkUInt32				uBlockAlign;                // [1, 24] Atomic Size (in bytes) of a sample * Nb Channel
    // AkDataCompID			DataCompID;                 // [0, 2] Data compression type ID
	// AkDataTypeID			DataTypeID;					// [0, 1] what is the data format
	// AkDataInterleaveID	DataInterleaveID;			// [0, 1] Channel interleaving ID
	// AkUInt16				wSmplFramesPerADPCMBlock;   // [0, 2047] Number of sample frames per block for ADPCM data (0 if PCM)
};

#endif // _AK_COMMON_DEFS_H_

