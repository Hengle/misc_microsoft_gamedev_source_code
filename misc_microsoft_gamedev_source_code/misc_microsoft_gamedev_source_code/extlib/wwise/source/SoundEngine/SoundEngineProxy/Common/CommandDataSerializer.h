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


#pragma	once

#ifdef AK_WWISE_APP
#include "BytesMem.h"
#else
#include "ALBytesMem.h"
#endif

#include "AkMonitorData.h"
#include "AkRTPC.h"
#include "AkPath.h"
#include "AkMusicStructs.h"

struct AkMeterInfo;


class CommandDataSerializer
{
public:
	CommandDataSerializer( bool	in_bSwapEndian = false );
	~CommandDataSerializer();

	AkUInt8* GetWrittenBytes()	const;
	int	GetWrittenSize() const;

	void Deserializing(	const AkUInt8*	in_pData );
	const AkUInt8*	GetReadBytes() const;

	void Reset();

	class AutoSetDataPeeking
	{
	public:
		AutoSetDataPeeking(	CommandDataSerializer& in_rSerializer )
			: m_rSerializer( in_rSerializer	)
		{
			m_rSerializer.SetDataPeeking( true );
		}

		~AutoSetDataPeeking()
		{
			m_rSerializer.SetDataPeeking( false	);
		}

	private:
		CommandDataSerializer& m_rSerializer;
	};

	// Template, catch all
	template <class	T>
	bool Put( const	T& in_rValue );

	template <class	T>
	bool Get( T& out_rValue	);

	// Basic, known	size types.
	bool Put( AkInt8 in_value );
	bool Get( AkInt8& out_rValue );
	bool Put( AkUInt8 in_value );
	bool Get( AkUInt8& out_rValue );
	bool Put( AkInt16 in_value );
	bool Get( AkInt16& out_rValue );
	bool Put( AkInt32 in_value );
	bool Get( AkInt32& out_rValue );
	bool Put( AkUInt16 in_value	);
	bool Get( AkUInt16&	out_rValue );
	bool Put( AkUInt32 in_value	);
	bool Get( AkUInt32&	out_rValue );
	bool Put( AkInt64 in_value );
	bool Get( AkInt64& out_rValue );

	bool Put( AkUInt64 in_value	);
	bool Get( AkUInt64&	out_rValue );

//	bool Put( AkUInt64	in_value );
//	bool Get( AkUInt64& out_rValue	);
	bool Put( AkReal32 in_value );
	bool Get( AkReal32& out_rValue );
	bool Put( AkReal64 in_value );
	bool Get( AkReal64& out_rValue );

	// Basic, possibly unknown size	types.
	bool Put( bool in_value	);
	bool Get( bool&	out_rValue );
	
	// Typedefs, enums,	etc. types

	template <class	T>
	bool PutEnum( const	T& in_rValue );

	template <class	T>
	bool GetEnum( T& out_rValue	);

	bool Put( const	AKRESULT& in_rValue	);
	bool Get( AKRESULT&	out_rValue );

	// Composite types
	bool Put( const	AkSoundPosition& in_rValue );
	bool Get( AkSoundPosition& out_rValue );
	bool Put( const	AkListenerPosition&	in_rValue );
	bool Get( AkListenerPosition& out_rValue );
	bool Put( const	AkSpeakerVolumes&	in_rValue );
	bool Get( AkSpeakerVolumes& out_rValue );
	bool Put( const AkVector& in_rValue );
	bool Get( AkVector& out_rValue );
	bool Put( const AkMeterInfo& in_rValue );
	bool Get( AkMeterInfo& out_rValue );

	template< class VALUE_TYPE >
	bool Put( const AkRTPCGraphPointBase<VALUE_TYPE>& in_rValue );
	template< class VALUE_TYPE >
	bool Get( AkRTPCGraphPointBase<VALUE_TYPE>& out_rValue );

	bool Put( const AkPathVertex& in_rValue );
	bool Get( AkPathVertex& out_rValue );
	bool Put( const AkPathListItemOffset& in_rValue );
	bool Get( AkPathListItemOffset& out_rValue );
	bool Put( const AkEnvironmentValue& in_rValue );
	bool Get( AkEnvironmentValue& out_rValue );

	bool Put( const AkMusicMarkerWwise& in_rValue );
	bool Get( AkMusicMarkerWwise& out_rValue );
	bool Put( const AkTrackSrcInfo& in_rValue );
	bool Get( AkTrackSrcInfo& out_rValue );
	bool Put( const AkWwiseMusicTransitionRule& in_rValue );
	bool Get( AkWwiseMusicTransitionRule& out_rValue );
	bool Put( const AkMusicFade& in_rValue );
	bool Get( AkMusicFade& out_rValue );
	bool Put( const AkMusicRanSeqPlaylistItem& in_rValue );
	bool Get( AkMusicRanSeqPlaylistItem& out_rValue );
	bool Put( const CAkStinger& in_rValue );
	bool Get( CAkStinger& out_rValue );
	bool Put( const AkMusicTrackRanSeqType& in_rValue );
	bool Get( AkMusicTrackRanSeqType& out_rValue );

	bool Put( const AkMusicSwitchAssoc& in_rValue );
	bool Get( AkMusicSwitchAssoc& out_rValue );

	bool Put( const	AkMonitorData::MonitorDataItem&	in_rValue );
	bool Get( AkMonitorData::MonitorDataItem*& out_rpValue ); // client	of method must deallocate returned pointer with	free()

	bool Put( const	AkMonitorData::Watch& in_rValue );
	bool Get( AkMonitorData::Watch& out_rValue );

	// Variable	length types
	bool Put( const	void* in_pvData, AkInt32 in_size );
	bool Get( void*& out_rpData, AkInt32& out_rSize	);
	bool Put( const	void* in_pvData, AkUInt32 in_size );
	bool Get( void*& out_rpData, AkUInt32& out_rSize );
	bool Put( const	char* in_pszData );
	bool Get( char*& out_rpszData, AkInt32&	out_rSize );
	bool Put( const	AkTChar* in_pszData	);
	bool Get( AkTChar*&	out_rpszData, AkInt32& out_rSize );

private:
	// Composite types not serializable	from outside this class.
	bool Put( const	AkMonitorData::ObjectMonitorData& in_rValue	);
	bool Get( AkMonitorData::ObjectMonitorData&	out_rValue );
	bool Put( const	AkMonitorData::StateMonitorData& in_rValue );
	bool Get( AkMonitorData::StateMonitorData& out_rValue );
	bool Put( const	AkMonitorData::ParamChangedMonitorData&	in_rValue );
	bool Get( AkMonitorData::ParamChangedMonitorData& out_rValue );
	bool Put( const	AkMonitorData::SetParamMonitorData&	in_rValue );
	bool Get( AkMonitorData::SetParamMonitorData& out_rValue );
	bool Put( const	AkMonitorData::ActionTriggeredMonitorData& in_rValue );
	bool Get( AkMonitorData::ActionTriggeredMonitorData& out_rValue	);
	bool Put( const	AkMonitorData::ActionDelayedMonitorData& in_rValue );
	bool Get( AkMonitorData::ActionDelayedMonitorData& out_rValue );
	bool Put( const	AkMonitorData::EventTriggeredMonitorData& in_rValue	);
	bool Get( AkMonitorData::EventTriggeredMonitorData&	out_rValue );
	bool Put( const	AkMonitorData::BankMonitorData&	in_rValue );
	bool Get( AkMonitorData::BankMonitorData& out_rValue );
	bool Put( const	AkMonitorData::PrepareMonitorData&	in_rValue );
	bool Get( AkMonitorData::PrepareMonitorData& out_rValue );
	bool Put( const	AkMonitorData::BusNotifMonitorData&	in_rValue );
	bool Get( AkMonitorData::BusNotifMonitorData& out_rValue );
	bool Put( const	AkMonitorData::AudioPerfMonitorData& in_rValue );
	bool Get( AkMonitorData::AudioPerfMonitorData& out_rValue );
	bool Put( const	AkMonitorData::GameObjPositionMonitorData& in_rValue );
	bool Get( AkMonitorData::GameObjPositionMonitorData& out_rValue );
	bool Put( const	AkMonitorData::ObjRegistrationMonitorData& in_rValue );
	bool Get( AkMonitorData::ObjRegistrationMonitorData& out_rValue	);
	bool Put( const	AkMonitorData::ErrorMonitorData1& in_rValue );
	bool Get( AkMonitorData::ErrorMonitorData1& out_rValue );
	bool Put( const	AkMonitorData::DebugMonitorData& in_rValue );
	bool Get( AkMonitorData::DebugMonitorData& out_rValue );
	bool Put( const	AkMonitorData::PathMonitorData&	in_rValue );
	bool Get( AkMonitorData::PathMonitorData& out_rValue );
	bool Put( const	AkCustomParamType& in_rValue );
	bool Get( AkCustomParamType& out_rValue	);
	bool Put( const	AkMonitorData::SwitchMonitorData& in_rValue	);
	bool Get( AkMonitorData::SwitchMonitorData&	out_rValue );
	bool Put( const	AkMonitorData::PluginTimerMonitorData& in_rValue );
	bool Get( AkMonitorData::PluginTimerMonitorData& out_rValue	);
	bool Put( const	AkMonitorData::MemoryMonitorData& in_rValue	);
	bool Get( AkMonitorData::MemoryMonitorData&	out_rValue );
	bool Put( const	AkMonitorData::MemoryPoolNameMonitorData& in_rValue	);
	bool Get( AkMonitorData::MemoryPoolNameMonitorData&	out_rValue );
	bool Put( const	AkMonitorData::EnvironmentMonitorData& in_rValue );
	bool Get( AkMonitorData::EnvironmentMonitorData& out_rValue	);
	bool Put( const AkMonitorData::ObsOccMonitorData& in_rValue );
	bool Get( AkMonitorData::ObsOccMonitorData& out_rValue );
	bool Put( const	AkMonitorData::ListenerMonitorData&	in_rValue );
	bool Get( AkMonitorData::ListenerMonitorData& out_rValue );
	bool Put( const	AkMonitorData::ControllerMonitorData&	in_rValue );
	bool Get( AkMonitorData::ControllerMonitorData& out_rValue );
	bool Put( const	AkMonitorData::DeviceRecordMonitorData&	in_rValue );
	bool Get( AkMonitorData::DeviceRecordMonitorData& out_rValue );
	bool Put( const	AkMonitorData::StreamRecordMonitorData&	in_rValue );
	bool Get( AkMonitorData::StreamRecordMonitorData& out_rValue );
	bool Put( const	AkMonitorData::StreamingMonitorData& in_rValue );
	bool Get( AkMonitorData::StreamingMonitorData& out_rValue );
	bool Put( const	AkMonitorData::PipelineMonitorData&	in_rValue );
	bool Get( AkMonitorData::PipelineMonitorData& out_rValue );
	bool Put( const	AkMonitorData::MarkersMonitorData& in_rValue );
	bool Get( AkMonitorData::MarkersMonitorData& out_rValue );
	bool Put( const	AkMonitorData::OutputMonitorData& in_rValue );
	bool Get( AkMonitorData::OutputMonitorData& out_rValue );
	bool Put( const	AkMonitorData::SegmentPositionMonitorData& in_rValue );
	bool Get( AkMonitorData::SegmentPositionMonitorData& out_rValue );
	bool Put( const	AkMonitorData::RTPCValuesMonitorData& in_rValue );
	bool Get( AkMonitorData::RTPCValuesMonitorData& out_rValue );
	bool Put( const	AkMonitorData::FeedbackMonitorData& in_rValue );
	bool Get( AkMonitorData::FeedbackMonitorData& out_rValue );

	bool Put( const	AkMonitorData::LoadedSoundBankMonitorData& in_rValue );
	bool Get( AkMonitorData::LoadedSoundBankMonitorData& out_rValue );
	bool Put( const	AkMonitorData::MediaPreparedMonitorData& in_rValue );
	bool Get( AkMonitorData::MediaPreparedMonitorData& out_rValue );
	bool Put( const	AkMonitorData::EventPreparedMonitorData& in_rValue );
	bool Get( AkMonitorData::EventPreparedMonitorData& out_rValue );
	bool Put( const	AkMonitorData::GameSyncMonitorData& in_rValue );
	bool Get( AkMonitorData::GameSyncMonitorData& out_rValue );
	bool Put( const	AkMonitorData::ResolveDialogueMonitorData& in_rValue );
	bool Get( AkMonitorData::ResolveDialogueMonitorData& out_rValue );
	bool Put( const	AkMonitorData::FeedbackDevicesMonitorData& in_rValue );
	bool Get( AkMonitorData::FeedbackDevicesMonitorData& out_rValue );
	bool Put( const	AkMonitorData::FeedbackGameObjMonitorData& in_rValue );
	bool Get( AkMonitorData::FeedbackGameObjMonitorData& out_rValue );

	friend class AutoSetDataPeeking;
	void SetDataPeeking( bool in_bPeekData );

	AkInt16	Swap( const	AkInt16& in_rValue ) const;
	AkUInt16 Swap( const AkUInt16& in_rValue ) const;
	AkInt32	Swap( const	AkInt32& in_rValue ) const;
	AkUInt32 Swap( const AkUInt32& in_rValue ) const;
	AkInt64	Swap( const	AkInt64& in_rValue ) const;
	AkUInt64 Swap( const AkUInt64& in_rValue ) const;
//	AkUInt64 Swap( const AkUInt64& in_rValue ) const;
	AkReal32 Swap( const AkReal32& in_rValue ) const;
	AkReal64 Swap( const AkReal64& in_rValue ) const;

	WriteBytesMem m_writer;

	const AkUInt8*	m_pReadBytes;
	AkUInt32 m_readPos;
	AkUInt32 m_readPosBeforePeeking;

	const bool m_bSwapEndian;
};

template <class	T>
bool CommandDataSerializer::Put( const T& in_rValue	)
{
	return in_rValue.Serialize(	*this );
}

template <class	T>
bool CommandDataSerializer::Get( T&	out_rValue )
{
	return out_rValue.Deserialize( *this );
}

template <class	T>
bool CommandDataSerializer::PutEnum( const T& in_rValue	)
{
	return Put(	(AkUInt32) in_rValue );
}

template <class	T>
bool CommandDataSerializer::GetEnum( T&	out_rValue )
{
	return Get(	(AkUInt32&)	out_rValue );
}

template< class VALUE_TYPE >
bool CommandDataSerializer::Put( const AkRTPCGraphPointBase<VALUE_TYPE>& in_rValue )
{
	return Put( in_rValue.From )
		&& Put( in_rValue.To )
		&& PutEnum( in_rValue.Interp );
}

template< class VALUE_TYPE >
bool CommandDataSerializer::Get( AkRTPCGraphPointBase<VALUE_TYPE>& out_rValue )
{
	return Get( out_rValue.From )
		&& Get( out_rValue.To )
		&& GetEnum( out_rValue.Interp );
}
