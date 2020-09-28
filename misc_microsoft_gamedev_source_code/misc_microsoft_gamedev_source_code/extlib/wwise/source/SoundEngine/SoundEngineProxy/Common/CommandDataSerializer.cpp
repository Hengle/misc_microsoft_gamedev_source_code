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


#include "StdAfx.h"

#include "CommandDataSerializer.h"
#include "AkEndianByteSwap.h"


CommandDataSerializer::CommandDataSerializer( bool in_bSwapEndian )
	: m_pReadBytes( NULL )
	, m_readPos( 0 )
	, m_readPosBeforePeeking( 0 )
	, m_bSwapEndian( in_bSwapEndian )
{
}

CommandDataSerializer::~CommandDataSerializer()
{
	m_writer.Clear();
}

AkUInt8* CommandDataSerializer::GetWrittenBytes() const
{
	return m_writer.Bytes();
}

int CommandDataSerializer::GetWrittenSize() const
{
	return m_writer.Count();
}

void CommandDataSerializer::Deserializing( const AkUInt8* in_pData )
{
	m_pReadBytes = in_pData;
	m_readPos = 0;
	m_readPosBeforePeeking = 0;
}

const AkUInt8* CommandDataSerializer::GetReadBytes() const
{
	return m_pReadBytes + m_readPos;
}

void CommandDataSerializer::Reset()
{
	m_writer.Clear();
	Deserializing( NULL );
}

bool CommandDataSerializer::Put( AkInt8 in_value )
{
	return m_writer.Write( in_value );
}

bool CommandDataSerializer::Get( AkInt8& out_rValue )
{
	out_rValue = *(AkUInt8*)(m_pReadBytes + m_readPos);
	m_readPos += 1;

	return true;
}

bool CommandDataSerializer::Put( AkUInt8 in_value )
{
	return m_writer.Write( in_value );
}

bool CommandDataSerializer::Get( AkUInt8& out_rValue )
{
	out_rValue = *(AkUInt8*)(m_pReadBytes + m_readPos);
	m_readPos += 1;

	return true;
}

bool CommandDataSerializer::Put( AkInt16 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkInt16& out_rValue )
{
	out_rValue = Swap( *(AkInt16*)(m_pReadBytes + m_readPos) );
	m_readPos += 2;

	return true;
}

bool CommandDataSerializer::Put( AkInt32 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkInt32& out_rValue )
{
	out_rValue = Swap( *(AkInt32*)(m_pReadBytes + m_readPos) );
	m_readPos += 4;

	return true;
}

bool CommandDataSerializer::Put( AkUInt16 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkUInt16& out_rValue )
{
	out_rValue = Swap( *(AkUInt16*)(m_pReadBytes + m_readPos) );
	m_readPos += 2;

	return true;
}

bool CommandDataSerializer::Put( AkUInt32 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkUInt32& out_rValue )
{
	out_rValue = Swap( *(AkUInt32*)(m_pReadBytes + m_readPos) );
	m_readPos += 4;

	return true;
}

bool CommandDataSerializer::Put( AkInt64 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkInt64& out_rValue )
{
	out_rValue = Swap( *(AkInt64*)(m_pReadBytes + m_readPos) );
	m_readPos += 8;

	return true;
}

bool CommandDataSerializer::Put( AkUInt64 in_value )
{
    return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkUInt64& out_rValue )
{
    out_rValue = Swap( *(AkUInt64*)(m_pReadBytes + m_readPos) );
	m_readPos += 8;

	return true;
}
/*
bool CommandDataSerializer::Put( AkUInt64 in_value )
{
	return m_writer.Write( Swap( in_value ) );
}

bool CommandDataSerializer::Get( AkUInt64& out_rValue )
{
	out_rValue = Swap( *(AkUInt64*)(m_pReadBytes + m_readPos) );
	m_readPos += 8;

	return true;
}
*/
bool CommandDataSerializer::Put( AkReal32 in_value )
{
	long lDummy = 0;

	AkReal32 valToWrite = Swap( in_value );
	
	return m_writer.WriteBytes( &valToWrite, 4, lDummy );
}

bool CommandDataSerializer::Get( AkReal32& out_rValue )
{
	// Align the float for consoles like Xbox 360.
	AkInt32 temp = *(AkInt32*)(m_pReadBytes + m_readPos);
	out_rValue = Swap( *reinterpret_cast<AkReal32*>( &temp ) );
	m_readPos += 4;

/*	out_rValue = 0;
	::memcpy( &out_rValue, m_pReadBytes + m_readPos, 4 );
	m_readPos += 4;
*/
	return true;
}

bool CommandDataSerializer::Put( AkReal64 in_value )
{
	long lDummy = 0;

	AkReal64 valToWrite = Swap( in_value );
	
	return m_writer.WriteBytes( &valToWrite, 8, lDummy );
}

bool CommandDataSerializer::Get( AkReal64& out_rValue )
{
	return Get( *reinterpret_cast<AkInt64*>(&out_rValue) );
}

bool CommandDataSerializer::Put( bool in_value )
{
	return Put( (AkUInt8)in_value );
}

bool CommandDataSerializer::Get( bool& out_rValue )
{
	return Get( (AkUInt8&)out_rValue );
}

bool CommandDataSerializer::Put( const AKRESULT& in_rValue )
{
	return Put( (AkUInt32)in_rValue );
}

bool CommandDataSerializer::Get( AKRESULT& out_rValue )
{
	return Get( (AkUInt32&)out_rValue );
}

bool CommandDataSerializer::Put( const AkSoundPosition& in_rValue )
{
	return Put( in_rValue.Position )
		&& Put( in_rValue.Orientation );
}

bool CommandDataSerializer::Get( AkSoundPosition& out_rValue )
{
	return Get( out_rValue.Position )
		&& Get( out_rValue.Orientation );
}

bool CommandDataSerializer::Put( const AkListenerPosition& in_rValue )
{
	return Put( in_rValue.Position )
		&& Put( in_rValue.OrientationFront )
		&& Put( in_rValue.OrientationTop );
}

bool CommandDataSerializer::Get( AkListenerPosition& out_rValue )
{
	return Get( out_rValue.Position )
		&& Get( out_rValue.OrientationFront )
		&& Get( out_rValue.OrientationTop );
}

bool CommandDataSerializer::Put( const AkSpeakerVolumes& in_rValue )
{
	return Put( in_rValue.fFrontLeft )
		&& Put( in_rValue.fFrontRight)
#ifdef AK_LFECENTER
		&& Put( in_rValue.fCenter)
		&& Put( in_rValue.fLfe)
#else
		&& Put( 0.f )
		&& Put( 0.f )
#endif
		&& Put( in_rValue.fRearLeft)
		&& Put( in_rValue.fRearRight);
}

bool CommandDataSerializer::Get( AkSpeakerVolumes& out_rValue )
{
#ifndef AK_LFECENTER
		AkReal32 fDummy;
#endif
	return Get( out_rValue.fFrontLeft )
		&& Get( out_rValue.fFrontRight)
#ifdef AK_LFECENTER
		&& Get( out_rValue.fCenter)
		&& Get( out_rValue.fLfe)
#else
		&& Get( fDummy )
		&& Get( fDummy )
#endif
		&& Get( out_rValue.fRearLeft)
		&& Get( out_rValue.fRearRight);
}

bool CommandDataSerializer::Put( const AkVector& in_rValue )
{
	return Put( in_rValue.X )
		&& Put( in_rValue.Y )
		&& Put( in_rValue.Z );
}

bool CommandDataSerializer::Get( AkVector& out_rValue )
{
	return Get( out_rValue.X )
		&& Get( out_rValue.Y )
		&& Get( out_rValue.Z );
}

bool CommandDataSerializer::Put( const AkMeterInfo& in_rValue )
{
	return Put( in_rValue.fGridOffset )
		&& Put( in_rValue.fGridPeriod )
		&& Put( in_rValue.fTempo )
		&& Put( in_rValue.uTimeSigBeatValue )
		&& Put( in_rValue.uTimeSigNumBeatsBar );
}
bool CommandDataSerializer::Get( AkMeterInfo& out_rValue )
{
	return Get( out_rValue.fGridOffset )
		&& Get( out_rValue.fGridPeriod )
		&& Get( out_rValue.fTempo )
		&& Get( out_rValue.uTimeSigBeatValue )
		&& Get( out_rValue.uTimeSigNumBeatsBar );
}

bool CommandDataSerializer::Put( const AkPathVertex& in_rValue )
{
	return Put( in_rValue.Vertex )
		&& Put( (AkUInt32)in_rValue.Duration );
}

bool CommandDataSerializer::Get( AkPathVertex& out_rValue )
{
	return Get( out_rValue.Vertex )
		&& Get( (AkUInt32&)out_rValue.Duration );
}

bool CommandDataSerializer::Put( const AkPathListItemOffset& in_rValue )
{
	return Put( in_rValue.ulVerticesOffset )
		&& Put( in_rValue.iNumVertices );
}

bool CommandDataSerializer::Get( AkPathListItemOffset& out_rValue )
{
	return Get( (AkUInt32&)out_rValue.ulVerticesOffset )
		&& Get( (AkInt32&)out_rValue.iNumVertices );
}

bool CommandDataSerializer::Put( const AkEnvironmentValue& in_rValue )
{
	return Put( in_rValue.EnvID )
		&& Put( in_rValue.fControlValue )
		&& Put( in_rValue.fUserData);
}

bool CommandDataSerializer::Get( AkEnvironmentValue& out_rValue )
{
	return Get( out_rValue.EnvID )
		&& Get( out_rValue.fControlValue )
		&& Get( out_rValue.fUserData);
}

bool CommandDataSerializer::Put( const AkMusicMarkerWwise& in_rValue )
{
	return Put( in_rValue.id )
		&& Put( in_rValue.fPosition);
}

bool CommandDataSerializer::Get( AkMusicMarkerWwise& out_rValue )
{
	return Get( out_rValue.id )
		&& Get( out_rValue.fPosition);
}

bool CommandDataSerializer::Put( const AkTrackSrcInfo& in_rValue )
{
	return Put( in_rValue.fBeginTrimOffset )
		&& Put( in_rValue.fEndTrimOffset)
		&& Put( in_rValue.fPlayAt)
		&& Put( in_rValue.fSrcDuration)
		&& Put( in_rValue.sourceID)
		&& Put( in_rValue.trackID);
}

bool CommandDataSerializer::Get( AkTrackSrcInfo& out_rValue )
{
	return Get( out_rValue.fBeginTrimOffset )
		&& Get( out_rValue.fEndTrimOffset)
		&& Get( out_rValue.fPlayAt)
		&& Get( out_rValue.fSrcDuration)
		&& Get( out_rValue.sourceID)
		&& Get( out_rValue.trackID);
}

bool CommandDataSerializer::Put( const AkWwiseMusicTransitionRule& in_rValue )
{
	return Put( in_rValue.srcID )
		&& Put( in_rValue.destID )
		&& Put( in_rValue.srcFade )
		&& Put( in_rValue.eSrcSyncType )
		&& Put( in_rValue.bSrcPlayPostExit )
		&& Put( in_rValue.destFade )
		&& Put( in_rValue.uDestmarkerID )
		&& Put( in_rValue.uDestJumpToID )
		&& Put( in_rValue.eDestEntryType )
		&& Put( in_rValue.bDestPlayPreEntry )
		&& Put( in_rValue.bIsTransObjectEnabled )
		&& Put( in_rValue.segmentID )
		&& Put( in_rValue.transFadeIn )
		&& Put( in_rValue.transFadeOut )
		&& Put( in_rValue.bPlayPreEntry )
		&& Put( in_rValue.bPlayPostExit );
}

bool CommandDataSerializer::Get( AkWwiseMusicTransitionRule& out_rValue )
{
	return Get( out_rValue.srcID )
		&& Get( out_rValue.destID )
		&& Get( out_rValue.srcFade )
		&& Get( out_rValue.eSrcSyncType )
		&& Get( out_rValue.bSrcPlayPostExit )
		&& Get( out_rValue.destFade )
		&& Get( out_rValue.uDestmarkerID )
		&& Get( out_rValue.uDestJumpToID )
		&& Get( out_rValue.eDestEntryType )
		&& Get( out_rValue.bDestPlayPreEntry )
		&& Get( out_rValue.bIsTransObjectEnabled )
		&& Get( out_rValue.segmentID )
		&& Get( out_rValue.transFadeIn )
		&& Get( out_rValue.transFadeOut )
		&& Get( out_rValue.bPlayPreEntry )
		&& Get( out_rValue.bPlayPostExit );
}

bool CommandDataSerializer::Put( const AkMusicFade& in_rValue )
{
	return Put( in_rValue.iFadeOffset )
		&& Put( in_rValue.transitionTime )
		&& PutEnum( in_rValue.eFadeCurve );
}

bool CommandDataSerializer::Get( AkMusicFade& out_rValue )
{
	return Get( out_rValue.iFadeOffset )
		&& Get( out_rValue.transitionTime )
		&& GetEnum( out_rValue.eFadeCurve );
}

bool CommandDataSerializer::Put( const AkMusicRanSeqPlaylistItem& in_rValue )
{
	return Put( in_rValue.m_SegmentID )
		&& Put( in_rValue.m_playlistItemID )
		&& Put( in_rValue.m_NumChildren )
		&& Put( in_rValue.m_wAvoidRepeatCount )
		&& Put( in_rValue.m_Weight )
		&& Put( in_rValue.m_bIsShuffle )
		&& Put( in_rValue.m_bIsUsingWeight )
		&& PutEnum( in_rValue.m_eRSType )
		&& Put( in_rValue.m_Loop );
}

bool CommandDataSerializer::Get( AkMusicRanSeqPlaylistItem& out_rValue )
{
	return Get( out_rValue.m_SegmentID )
		&& Get( out_rValue.m_playlistItemID )
		&& Get( out_rValue.m_NumChildren )
		&& Get( out_rValue.m_wAvoidRepeatCount )
		&& Get( out_rValue.m_Weight )
		&& Get( out_rValue.m_bIsShuffle )
		&& Get( out_rValue.m_bIsUsingWeight )
		&& GetEnum( out_rValue.m_eRSType )
		&& Get( out_rValue.m_Loop );
}

bool CommandDataSerializer::Put( const CAkStinger& in_rValue )
{
	return Put( in_rValue.m_TriggerID )
		&& Put( in_rValue.m_SegmentID )
		&& PutEnum( in_rValue.m_SyncPlayAt )
		&& Put( in_rValue.m_DontRepeatTime )
		&& Put( in_rValue.m_numSegmentLookAhead );
}

bool CommandDataSerializer::Get( CAkStinger& out_rValue )
{
	return Get( out_rValue.m_TriggerID )
		&& Get( out_rValue.m_SegmentID )
		&& GetEnum( out_rValue.m_SyncPlayAt )
		&& Get( out_rValue.m_DontRepeatTime )
		&& Get( out_rValue.m_numSegmentLookAhead );
}

bool CommandDataSerializer::Put( const AkMusicTrackRanSeqType& in_rValue )
{
	return PutEnum( in_rValue );
}

bool CommandDataSerializer::Get( AkMusicTrackRanSeqType& out_rValue )
{
	return GetEnum( out_rValue );
}

bool CommandDataSerializer::Put( const AkMusicSwitchAssoc& in_rValue )
{
	return Put( in_rValue.nodeID )
		&& Put( in_rValue.switchID );
}

bool CommandDataSerializer::Get( AkMusicSwitchAssoc& out_rValue )
{
	return Get( out_rValue.nodeID )
		&& Get( out_rValue.switchID );
}

bool CommandDataSerializer::Put( const AkMonitorData::MonitorDataItem& in_rValue )
{
	AkUInt32 sizeItem = AkMonitorData::RealSizeof( in_rValue );

	bool bRet = Put( sizeItem )
		&& Put( in_rValue.eDataType )
		&& Put( in_rValue.timeStamp );

	switch( in_rValue.eDataType )
	{
	case AkMonitorData::MonitorDataObject:
		bRet = bRet && Put( in_rValue.objectData );
		break;

	case AkMonitorData::MonitorDataState:
		bRet = bRet && Put( in_rValue.stateData );
		break;

	case AkMonitorData::MonitorDataParamChanged:
		bRet = bRet && Put( in_rValue.paramChangedData );
		break;

	case AkMonitorData::MonitorDataBank:
		bRet = bRet && Put( in_rValue.bankData );
		break;

	case AkMonitorData::MonitorDataPrepare:
		bRet = bRet && Put( in_rValue.prepareData );
		break;

	case AkMonitorData::MonitorDataEventTriggered:
		bRet = bRet && Put( in_rValue.eventTriggeredData );
		break;

	case AkMonitorData::MonitorDataActionDelayed:
		bRet = bRet && Put( in_rValue.actionDelayedData );
		break;

	case AkMonitorData::MonitorDataActionTriggered:
		bRet = bRet && Put( in_rValue.actionTriggeredData );
		break;

	case AkMonitorData::MonitorDataBusNotif:
		bRet = bRet && Put( in_rValue.busNotifData );
		break;

	case AkMonitorData::MonitorDataSetParam:
		bRet = bRet && Put( in_rValue.setParamData );
		break;

	case AkMonitorData::MonitorDataAudioPerf:
		bRet = bRet && Put( in_rValue.audioPerfData );
		break;

	case AkMonitorData::MonitorDataGameObjPosition:
		bRet = bRet && Put( in_rValue.gameObjPositionData );
		break;

	case AkMonitorData::MonitorDataObjRegistration:
		bRet = bRet && Put( in_rValue.objRegistrationData );
		break;

	case AkMonitorData::MonitorDataPath:
		bRet = bRet && Put( in_rValue.pathData );
		break;

	case AkMonitorData::MonitorDataErrorString:
	case AkMonitorData::MonitorDataMessageString:
		bRet = bRet && Put( in_rValue.debugData );
		break;

	case AkMonitorData::MonitorDataErrorCode:
	case AkMonitorData::MonitorDataMessageCode:
		bRet = bRet && Put( in_rValue.errorData1 );
		break;

	case AkMonitorData::MonitorDataSwitch:
		bRet = bRet && Put( in_rValue.switchData );
		break;

	case AkMonitorData::MonitorDataPluginTimer:
		bRet = bRet && Put( in_rValue.pluginTimerData );
		break;

	case AkMonitorData::MonitorDataMemoryPool:
		bRet = bRet && Put( in_rValue.memoryData );
		break;

	case AkMonitorData::MonitorDataMemoryPoolName:
		bRet = bRet && Put( in_rValue.memoryPoolNameData );
		break;

	case AkMonitorData::MonitorDataEnvironment:
		bRet = bRet && Put( in_rValue.environmentData );
		break;

	case AkMonitorData::MonitorDataObsOcc:
		bRet = bRet && Put( in_rValue.obsOccData );
		break;

	case AkMonitorData::MonitorDataListeners:
		bRet = bRet && Put( in_rValue.listenerData );
		break;

	case AkMonitorData::MonitorDataControllers:
		bRet = bRet && Put( in_rValue.controllerData );
		break;

    case AkMonitorData::MonitorDevicesRecord:
		bRet = bRet && Put( in_rValue.deviceRecordData );
		break;

    case AkMonitorData::MonitorDataStreamsRecord:
		bRet = bRet && Put( in_rValue.streamRecordData );
		break;

    case AkMonitorData::MonitorDataStreaming:
		bRet = bRet && Put( in_rValue.streamingData );
		break;

	case AkMonitorData::MonitorDataPipeline:
		bRet = bRet && Put( in_rValue.pipelineData );
		break;

	case AkMonitorData::MonitorDataMarkers:
		bRet = bRet && Put( in_rValue.markersData );
		break;

	case AkMonitorData::MonitorDataOutput:
		bRet = bRet && Put( in_rValue.outputData );
		break;

	case AkMonitorData::MonitorDataSegmentPosition:
		bRet = bRet && Put( in_rValue.segmentPositionData );
		break;

	case AkMonitorData::MonitorDataRTPCValues:
		bRet = bRet && Put( in_rValue.rtpcValuesData );
		break;

	case AkMonitorData::MonitorDataSoundBank:
		bRet = bRet && Put( in_rValue.loadedSoundBankData );
		break;

	case AkMonitorData::MonitorDataFeedback:
		bRet = bRet && Put( in_rValue.feedbackData );
		break;

	case AkMonitorData::MonitorDataMedia:
		bRet = bRet && Put( in_rValue.mediaPreparedData );
		break;

	case AkMonitorData::MonitorDataEvent:
		bRet = bRet && Put( in_rValue.eventPreparedData );
		break;

	case AkMonitorData::MonitorDataGameSync:
		bRet = bRet && Put( in_rValue.gameSyncData );
		break;

	case AkMonitorData::MonitorDataResolveDialogue:
		bRet = bRet && Put( in_rValue.resolveDialogueData );
		break;

	case AkMonitorData::MonitorDataFeedbackDevices:
		bRet = bRet && Put( in_rValue.feedbackDevicesData );
		break;

	case AkMonitorData::MonitorDataFeedbackGameObjs:
		bRet = bRet && Put( in_rValue.feedbackGameObjData );
		break;

    default:
		AKASSERT( !"Unknown MonitorDataItem to serialize." );
		bRet = false;
	}

	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::MonitorDataItem*& out_rpValue )
{
	AkUInt32 ulSize;
	if ( !Get( ulSize ) )
		return false;

	out_rpValue = (AkMonitorData::MonitorDataItem*) malloc( ulSize );

	bool bRet = Get( out_rpValue->eDataType )
		&& Get( out_rpValue->timeStamp );

	switch( out_rpValue->eDataType )
	{
	case AkMonitorData::MonitorDataObject:
		bRet = bRet && Get( out_rpValue->objectData );
		break;

	case AkMonitorData::MonitorDataState:
		bRet = bRet && Get( out_rpValue->stateData );
		break;

	case AkMonitorData::MonitorDataParamChanged:
		bRet = bRet && Get( out_rpValue->paramChangedData );
		break;

	case AkMonitorData::MonitorDataBank:
		bRet = bRet && Get( out_rpValue->bankData );
		break;

	case AkMonitorData::MonitorDataPrepare:
		bRet = bRet && Get( out_rpValue->prepareData );
		break;

	case AkMonitorData::MonitorDataEventTriggered:
		bRet = bRet && Get( out_rpValue->eventTriggeredData );
		break;

	case AkMonitorData::MonitorDataActionDelayed:
		bRet = bRet && Get( out_rpValue->actionDelayedData );
		break;

	case AkMonitorData::MonitorDataActionTriggered:
		bRet = bRet && Get( out_rpValue->actionTriggeredData );
		break;

	case AkMonitorData::MonitorDataBusNotif:
		bRet = bRet && Get( out_rpValue->busNotifData );
		break;

	case AkMonitorData::MonitorDataSetParam:
		bRet = bRet && Get( out_rpValue->setParamData );
		break;

	case AkMonitorData::MonitorDataAudioPerf:
		bRet = bRet && Get( out_rpValue->audioPerfData );
		break;

	case AkMonitorData::MonitorDataGameObjPosition:
		bRet = bRet && Get( out_rpValue->gameObjPositionData );
		break;
		
	case AkMonitorData::MonitorDataObjRegistration:
		bRet = bRet && Get( out_rpValue->objRegistrationData );
		break;

	case AkMonitorData::MonitorDataPath:
		bRet = bRet && Get( out_rpValue->pathData );
		break;

	case AkMonitorData::MonitorDataErrorString:
	case AkMonitorData::MonitorDataMessageString:
		bRet = bRet && Get( out_rpValue->debugData );
		break;

	case AkMonitorData::MonitorDataErrorCode:
	case AkMonitorData::MonitorDataMessageCode:
		bRet = bRet && Get( out_rpValue->errorData1 );
		break;

	case AkMonitorData::MonitorDataSwitch:
		bRet = bRet && Get( out_rpValue->switchData );
		break;

	case AkMonitorData::MonitorDataPluginTimer:
		bRet = bRet && Get( out_rpValue->pluginTimerData );
		break;

	case AkMonitorData::MonitorDataMemoryPool:
		bRet = bRet && Get( out_rpValue->memoryData );
		break;

	case AkMonitorData::MonitorDataMemoryPoolName:
		bRet = bRet && Get( out_rpValue->memoryPoolNameData );
		break;

	case AkMonitorData::MonitorDataEnvironment:
		bRet = bRet && Get( out_rpValue->environmentData );
		break;

	case AkMonitorData::MonitorDataObsOcc:
		bRet = bRet && Get( out_rpValue->obsOccData );
		break;

	case AkMonitorData::MonitorDataListeners:
		bRet = bRet && Get( out_rpValue->listenerData );
		break;

	case AkMonitorData::MonitorDataControllers:
		bRet = bRet && Get( out_rpValue->controllerData );
		break;

    case AkMonitorData::MonitorDevicesRecord:
		bRet = bRet && Get( out_rpValue->deviceRecordData );
		break;

     case AkMonitorData::MonitorDataStreamsRecord:
		bRet = bRet && Get( out_rpValue->streamRecordData );
		break;

    case AkMonitorData::MonitorDataStreaming:
		bRet = bRet && Get( out_rpValue->streamingData );
		break;

	case AkMonitorData::MonitorDataPipeline:
		bRet = bRet && Get( out_rpValue->pipelineData );
		break;

	case AkMonitorData::MonitorDataMarkers:
		bRet = bRet && Get( out_rpValue->markersData );
		break;

	case AkMonitorData::MonitorDataOutput:
		bRet = bRet && Get( out_rpValue->outputData );
		break;

	case AkMonitorData::MonitorDataSegmentPosition:
		bRet = bRet && Get( out_rpValue->segmentPositionData );
		break;

	case AkMonitorData::MonitorDataRTPCValues:
		bRet = bRet && Get( out_rpValue->rtpcValuesData );
		break;

	case AkMonitorData::MonitorDataSoundBank:
		bRet = bRet && Get( out_rpValue->loadedSoundBankData );
		break;

	case AkMonitorData::MonitorDataFeedback:
		bRet = bRet && Get( out_rpValue->feedbackData );
		break;

	case AkMonitorData::MonitorDataMedia:
		bRet = bRet && Get( out_rpValue->mediaPreparedData );
		break;

	case AkMonitorData::MonitorDataEvent:
		bRet = bRet && Get( out_rpValue->eventPreparedData );
		break;

	case AkMonitorData::MonitorDataGameSync:
		bRet = bRet && Get( out_rpValue->gameSyncData );
		break;

	case AkMonitorData::MonitorDataResolveDialogue:
		bRet = bRet && Get( out_rpValue->resolveDialogueData );
		break;

	case AkMonitorData::MonitorDataFeedbackDevices:
		bRet = bRet && Get( out_rpValue->feedbackDevicesData );
		break;

	case AkMonitorData::MonitorDataFeedbackGameObjs:
		bRet = bRet && Get( out_rpValue->feedbackGameObjData );
		break;

	default:
		AKASSERT( !"Unknown MonitorDataItem to deserialize." );
		bRet = false;
	}

	return bRet;
}

bool CommandDataSerializer::Put( const	AkMonitorData::Watch& in_rValue )
{
	bool bRet = Put( (AkUInt32)in_rValue.eType );
	switch( in_rValue.eType )
	{
	case AkMonitorData::WatchType_GameObjectID:
		bRet = bRet && Put( in_rValue.ID.gameObjectID );
		break;
	case AkMonitorData::WatchType_ListenerID:
		bRet = bRet && Put( in_rValue.ID.uiListenerID );
		break;
	case AkMonitorData::WatchType_GameObjectName:
		bRet = bRet && Put( in_rValue.wNameSize )
			&& Put( in_rValue.wNameSize ? in_rValue.szName : NULL );
		break;
	default:
		AKASSERT( !"Unknown watch type" );
		break;
	}
	
	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::Watch& out_rValue )
{
	AkInt32 wNameSize = 0;
	char *pszName = NULL;

	bool bRet = Get( (AkUInt32&)out_rValue.eType );

	switch( out_rValue.eType )
	{
	case AkMonitorData::WatchType_GameObjectID:
		bRet = bRet && Get( out_rValue.ID.gameObjectID );
		break;
	case AkMonitorData::WatchType_ListenerID:
		bRet = bRet && Get( out_rValue.ID.uiListenerID );
		break;
	case AkMonitorData::WatchType_GameObjectName:
		bRet = bRet && Get( out_rValue.wNameSize )
			&& Get( pszName, wNameSize );
		if ( bRet && out_rValue.wNameSize )
		{
			AkUInt16 wLen = AkMin( out_rValue.wNameSize, sizeof(out_rValue.szName)-1);
			memcpy( &out_rValue.szName, pszName, wLen );	
			out_rValue.szName[wLen] = NULL;
		}
		break;
	default:
		AKASSERT( !"Unknown watch type" );
		break;
	}

	return bRet;
}

bool CommandDataSerializer::Put( const void* in_pvData, AkInt32 in_size )
{
	long lDummy = 0;
	
	return Put( in_size )
		&& m_writer.WriteBytes( in_pvData, in_size, lDummy );
}

bool CommandDataSerializer::Get( void*& out_rpData, AkInt32& out_rSize )
{
	out_rpData = NULL;
	out_rSize = 0;
	
	if( Get( out_rSize ) && out_rSize != 0 )
	{
		out_rpData = (void*)(m_pReadBytes + m_readPos);

		m_readPos += out_rSize;
	}

	return true;
}

bool CommandDataSerializer::Put( const void* in_pvData, AkUInt32 in_size )
{
	long lDummy = 0;
	
	return Put( in_size )
		&& m_writer.WriteBytes( in_pvData, in_size, lDummy );
}

bool CommandDataSerializer::Get( void*& out_rpData, AkUInt32& out_rSize )
{
	out_rpData = NULL;
	out_rSize = 0;
	
	if( Get( out_rSize ) && out_rSize != 0 )
	{
		out_rpData = (void*)(m_pReadBytes + m_readPos);

		m_readPos += out_rSize;
	}

	return true;
}

bool CommandDataSerializer::Put( const char* in_pszData )
{
	return Put( (void*)in_pszData, in_pszData != NULL ? (AkInt32)(::strlen( in_pszData ) + 1)  : 0 );
}

bool CommandDataSerializer::Put( const AkTChar* in_pszData )
{
    // NOTE. Should be swapped (useless for now, since it is always only swapped on receive).
    AKASSERT( !m_bSwapEndian || !"Swapped Put() not implemented" );
    return Put( (void*)in_pszData, in_pszData != NULL ? (AkInt32)(sizeof(AkTChar)*(::wcslen( in_pszData ) + 1)) : 0 );
}

bool CommandDataSerializer::Get( char*& out_rpszData, AkInt32& out_rSize )
{
	return Get( (void*&)out_rpszData, out_rSize );
}

bool CommandDataSerializer::Get( AkTChar*& out_rpszData, AkInt32& out_rSize )
{
	bool bReturnedValue = Get( (void*&)out_rpszData, out_rSize );
	out_rSize /= sizeof(AkTChar);

    if ( bReturnedValue && 
         out_rpszData &&
         out_rSize ) // freak
    {
        for ( AkInt32 i=0; i<out_rSize; i++ )
            out_rpszData[i] = Swap( (AkUInt16&)out_rpszData[i] );
    }

	return bReturnedValue;
}

bool CommandDataSerializer::Put( const AkMonitorData::ObjectMonitorData& in_rValue )
{
	bool bRet = Put( in_rValue.playingID )
		&& Put( in_rValue.gameObjPtr )
		&& Put( (AkInt32)in_rValue.eNotificationReason );
	
	// Container history
	{
		bRet = bRet
			&& Put( in_rValue.cntrHistArray.uiArraySize );

		for( AkUInt32 i = 0; i < in_rValue.cntrHistArray.uiArraySize && bRet; ++i )
			Put( in_rValue.cntrHistArray.aCntrHist[i] );
	}

	bRet = bRet && Put( in_rValue.customParam );

	bRet = bRet
		&& Put( in_rValue.targetObjectID )
		&& Put( in_rValue.fadeTime )
		&& Put( in_rValue.playlistItemID );

	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::ObjectMonitorData& out_rValue )
{
	bool bRet = Get( out_rValue.playingID )
		&& Get( out_rValue.gameObjPtr )
		&& Get( (AkInt32&)out_rValue.eNotificationReason );
	
	// Container history
	{
		bRet = bRet
			&& Get( out_rValue.cntrHistArray.uiArraySize );

		for( AkUInt32 i = 0; i < out_rValue.cntrHistArray.uiArraySize && bRet; ++i )
			Get( out_rValue.cntrHistArray.aCntrHist[i] );
	}

	bRet = bRet && Get( out_rValue.customParam );

	bRet = bRet
		&& Get( out_rValue.targetObjectID )
		&& Get( out_rValue.fadeTime )
		&& Get( out_rValue.playlistItemID );

	return bRet;
}

bool CommandDataSerializer::Put( const AkMonitorData::StateMonitorData& in_rValue )
{
	return Put( in_rValue.stateGroupID )
		&& Put( in_rValue.stateFrom )
		&& Put( in_rValue.stateTo );
}

bool CommandDataSerializer::Get( AkMonitorData::StateMonitorData& out_rValue )
{
	return Get( out_rValue.stateGroupID )
		&& Get( out_rValue.stateFrom )
		&& Get( out_rValue.stateTo );
}

bool CommandDataSerializer::Put( const AkMonitorData::ParamChangedMonitorData& in_rValue )
{
	return Put( (AkUInt32)in_rValue.eNotificationReason )
		&& Put( in_rValue.gameObjPtr )
		&& Put( in_rValue.elementID );
}

bool CommandDataSerializer::Get( AkMonitorData::ParamChangedMonitorData& out_rValue )
{
	return Get( (AkUInt32&)out_rValue.eNotificationReason )
		&& Get( out_rValue.gameObjPtr )
		&& Get( out_rValue.elementID );
}

bool CommandDataSerializer::Put( const AkMonitorData::SetParamMonitorData& in_rValue )
{
	bool bRet = Put( (AkUInt32)in_rValue.eNotificationReason )
		&& Put( in_rValue.gameObjPtr )
		&& Put( in_rValue.elementID );

	if( in_rValue.eNotificationReason == AkMonitorData::NotificationReason_VolumeChanged )
		bRet = bRet && Put( in_rValue.volumeTarget );
	else if( in_rValue.eNotificationReason == AkMonitorData::NotificationReason_PitchChanged )
		bRet = bRet && Put( in_rValue.pitchTarget );

	bRet = bRet && PutEnum( in_rValue.valueMeaning )
		&& Put( in_rValue.transitionTime );

	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::SetParamMonitorData& out_rValue )
{
	bool bRet = Get( (AkUInt32&)out_rValue.eNotificationReason )
		&& Get( out_rValue.gameObjPtr )
		&& Get( out_rValue.elementID );

	if( out_rValue.eNotificationReason == AkMonitorData::NotificationReason_VolumeChanged )
		bRet = bRet && Get( out_rValue.volumeTarget );
	else if( out_rValue.eNotificationReason == AkMonitorData::NotificationReason_PitchChanged )
		bRet = bRet && Get( out_rValue.pitchTarget );

	bRet = bRet 
		&& GetEnum( out_rValue.valueMeaning )
		&& Get( out_rValue.transitionTime );

	return bRet;
}

bool CommandDataSerializer::Put( const AkMonitorData::ActionTriggeredMonitorData& in_rValue )
{
	return Put( in_rValue.playingID )
		&& Put( in_rValue.actionID )
		&& Put( in_rValue.gameObjPtr )
		&& Put( in_rValue.customParam );
}

bool CommandDataSerializer::Get( AkMonitorData::ActionTriggeredMonitorData& out_rValue )
{
	return Get( out_rValue.playingID )
		&& Get( out_rValue.actionID )
		&& Get( out_rValue.gameObjPtr )
		&& Get( out_rValue.customParam );
}

bool CommandDataSerializer::Put( const AkMonitorData::ActionDelayedMonitorData& in_rValue )
{
	return Put( static_cast<AkMonitorData::ActionTriggeredMonitorData>( in_rValue ) )
		&& Put( in_rValue.delayTime );
}

bool CommandDataSerializer::Get( AkMonitorData::ActionDelayedMonitorData& out_rValue )
{
	return Get( static_cast<AkMonitorData::ActionTriggeredMonitorData&>( out_rValue ) )
		&& Get( out_rValue.delayTime );
}

bool CommandDataSerializer::Put( const AkMonitorData::EventTriggeredMonitorData& in_rValue )
{
	return Put( in_rValue.playingID )
		&& Put( in_rValue.eventID )
		&& Put( in_rValue.gameObjPtr )
		&& Put( in_rValue.customParam );
}

bool CommandDataSerializer::Get( AkMonitorData::EventTriggeredMonitorData& out_rValue )
{
	return Get( out_rValue.playingID )
		&& Get( out_rValue.eventID )
		&& Get( out_rValue.gameObjPtr )
		&& Get( out_rValue.customParam );
}

bool CommandDataSerializer::Put( const AkMonitorData::BankMonitorData& in_rValue )
{
	return Put( (AkUInt32)in_rValue.eNotificationReason )
		&& Put( in_rValue.bankID )
		&& Put( in_rValue.languageID )
		&& Put( in_rValue.wStringSize )
		&& Put( in_rValue.wStringSize ? in_rValue.szBankName : NULL );
}

bool CommandDataSerializer::Get( AkMonitorData::BankMonitorData& out_rValue )
{
	AkInt32 nNameSize = 0;
	char *pszBankName = NULL;

	bool bResult = Get( (AkUInt32&)out_rValue.eNotificationReason )
		&& Get( out_rValue.bankID )
		&& Get( out_rValue.languageID )
		&& Get( out_rValue.wStringSize )
		&& Get( pszBankName, nNameSize );

	if ( bResult && out_rValue.wStringSize )
		memcpy( &out_rValue.szBankName, pszBankName, out_rValue.wStringSize );	

	return bResult;
}

bool CommandDataSerializer::Put( const AkMonitorData::PrepareMonitorData& in_rValue )
{
	return Put( (AkUInt32)in_rValue.eNotificationReason )
		&& Put( in_rValue.gamesyncIDorEventID )
		&& Put( in_rValue.groupID )
		&& PutEnum( in_rValue.groupType )
		&& Put( in_rValue.uNumEvents );
}

bool CommandDataSerializer::Get( AkMonitorData::PrepareMonitorData& out_rValue )
{
	return Get( (AkUInt32&)out_rValue.eNotificationReason )
		&& Get( out_rValue.gamesyncIDorEventID )
		&& Get( out_rValue.groupID )
		&& GetEnum( out_rValue.groupType )
		&& Get( out_rValue.uNumEvents );
}

bool CommandDataSerializer::Put( const AkMonitorData::BusNotifMonitorData& in_rValue )
{
	return Put( in_rValue.busID )
		&& Put( (AkUInt32)in_rValue.notifReason )
		&& Put( in_rValue.bitsFXBypass )
		&& Put( in_rValue.bitsMask );
}

bool CommandDataSerializer::Get( AkMonitorData::BusNotifMonitorData& out_rValue )
{
	return Get( out_rValue.busID )
		&& Get( (AkUInt32&)out_rValue.notifReason )
		&& Get( out_rValue.bitsFXBypass )
		&& Get( out_rValue.bitsMask );
}

bool CommandDataSerializer::Put( const AkMonitorData::AudioPerfMonitorData& in_rValue )
{
	bool bRet = Put( in_rValue.uiNotifFilter )
		&& Put( in_rValue.numFadeTransitionsUsed )
		&& Put( in_rValue.maxFadeNumTransitions )
		&& Put( in_rValue.numStateTransitionsUsed )
		&& Put( in_rValue.maxStateNumTransitions )
		&& Put( in_rValue.numRegisteredObjects )
		&& Put( in_rValue.timers.fInterval )
		&& Put( in_rValue.timers.fAudioThread )
		&& Put( in_rValue.uCommandQueueActualSize )
	    && Put( in_rValue.fCommandQueuePercentageUsed )
		&& Put( in_rValue.fDSPUsage )
		&& Put( in_rValue.uNumPreparedEvents )
		&& Put( in_rValue.uTotalMemBanks )
		&& Put( in_rValue.uTotalPreparedMemory )
		&& Put( in_rValue.uTotalMediaMemmory );

	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::AudioPerfMonitorData& out_rValue )
{
	bool bRet = Get( out_rValue.uiNotifFilter )
		&& Get( out_rValue.numFadeTransitionsUsed )
		&& Get( out_rValue.maxFadeNumTransitions )
		&& Get( out_rValue.numStateTransitionsUsed )
		&& Get( out_rValue.maxStateNumTransitions )
		&& Get( out_rValue.numRegisteredObjects )
		&& Get( out_rValue.timers.fInterval )
		&& Get( out_rValue.timers.fAudioThread )
		&& Get( out_rValue.uCommandQueueActualSize )
		&& Get( out_rValue.fCommandQueuePercentageUsed )
		&& Get( out_rValue.fDSPUsage )
		&& Get( out_rValue.uNumPreparedEvents )
		&& Get( out_rValue.uTotalMemBanks )
		&& Get( out_rValue.uTotalPreparedMemory )
		&& Get( out_rValue.uTotalMediaMemmory );

	return bRet;
}

bool CommandDataSerializer::Put( const AkMonitorData::GameObjPositionMonitorData& in_rValue )
{
	if ( !Put( in_rValue.ulNumGameObjPositions ) ||
		 !Put( in_rValue.ulNumListenerPositions ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumGameObjPositions; ++i )
	{
		const AkMonitorData::GameObjPosition & data = in_rValue.positions[ i ].gameObjPosition;
		if ( 	!Put( data.gameObjID ) 
			|| 	!Put( data.position ) )
			return false;
	}

	for ( AkUInt32 i = 0; i < in_rValue.ulNumListenerPositions; ++i )
	{
		const AkMonitorData::ListenerPosition & data = in_rValue.positions[ in_rValue.ulNumGameObjPositions + i ].listenerPosition;
		if ( 	!Put( data.uIndex ) 
			|| 	!Put( data.position ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::GameObjPositionMonitorData& out_rValue )
{
	if ( !Get( out_rValue.ulNumGameObjPositions ) ||
		 !Get( out_rValue.ulNumListenerPositions ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumGameObjPositions; ++i )
	{
		AkMonitorData::GameObjPosition & data = out_rValue.positions[ i ].gameObjPosition;
		if ( 	!Get( data.gameObjID ) 
			|| 	!Get( data.position ) )
			return false;
	}

	for ( AkUInt32 i = 0; i < out_rValue.ulNumListenerPositions; ++i )
	{
		AkMonitorData::ListenerPosition & data = out_rValue.positions[ out_rValue.ulNumGameObjPositions + i ].listenerPosition;
		if ( 	!Get( data.uIndex ) 
			|| 	!Get( data.position ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::ObjRegistrationMonitorData& in_rValue )
{
	return Put( in_rValue.isRegistered )
		&& Put( in_rValue.gameObjPtr )
		&& Put( in_rValue.wStringSize )
		&& Put( in_rValue.wStringSize ? in_rValue.szName : NULL );
}

bool CommandDataSerializer::Get( AkMonitorData::ObjRegistrationMonitorData& out_rValue )
{
	AkInt32 iDummy = 0;
	char* pszMessage = NULL;

	bool bResult = Get( out_rValue.isRegistered )
				&& Get( out_rValue.gameObjPtr )
				&& Get( out_rValue.wStringSize )
				&& Get( pszMessage, iDummy );

	if ( bResult && out_rValue.wStringSize )
		memcpy( &out_rValue.szName, pszMessage, out_rValue.wStringSize );	

	return bResult;
}

bool CommandDataSerializer::Put( const AkMonitorData::ErrorMonitorData1& in_rValue )
{
	return PutEnum( in_rValue.eErrorCode )
		&& Put( in_rValue.uParam1 );
}

bool CommandDataSerializer::Get( AkMonitorData::ErrorMonitorData1& out_rValue )
{
	return GetEnum( out_rValue.eErrorCode )
		&& Get( out_rValue.uParam1 );
}

bool CommandDataSerializer::Put( const AkMonitorData::DebugMonitorData& in_rValue )
{
	return Put( in_rValue.wStringSize )
		&& Put( in_rValue.wStringSize ? in_rValue.szMessage : NULL );
}

bool CommandDataSerializer::Get( AkMonitorData::DebugMonitorData& out_rValue )
{
	AkInt32 iDummy = 0;
	AkTChar* pszMessage = NULL;

	bool bResult = Get( out_rValue.wStringSize )
		&& Get( pszMessage, iDummy );

	if ( bResult && out_rValue.wStringSize )
		memcpy( &out_rValue.szMessage, pszMessage, out_rValue.wStringSize * sizeof(AkTChar) );	

	return bResult;
}

bool CommandDataSerializer::Put( const AkMonitorData::PathMonitorData& in_rValue )
{
	return Put( in_rValue.playingID )
		&& Put( in_rValue.ulUniqueID )
		&& Put( (AkInt32)in_rValue.eEvent )
		&& Put( in_rValue.ulIndex );
}

bool CommandDataSerializer::Get( AkMonitorData::PathMonitorData& out_rValue )
{
	return Get( out_rValue.playingID )
		&& Get( out_rValue.ulUniqueID )
		&& Get( (AkInt32&)out_rValue.eEvent )
		&& Get( out_rValue.ulIndex );
}

bool CommandDataSerializer::Put( const AkCustomParamType& in_rValue )
{
	return Put( in_rValue.customParam )
		&& Put( in_rValue.ui32Reserved );
}

bool CommandDataSerializer::Get( AkCustomParamType& out_rValue )
{
	return Get( out_rValue.customParam )
		&& Get( out_rValue.ui32Reserved );
}

bool CommandDataSerializer::Put( const AkMonitorData::SwitchMonitorData& in_rValue )
{
	return Put( in_rValue.switchGroupID )
		&& Put( in_rValue.switchState )
		&& Put( in_rValue.gameObj );
}

bool CommandDataSerializer::Get( AkMonitorData::SwitchMonitorData& out_rValue )
{
	return Get( out_rValue.switchGroupID )
		&& Get( out_rValue.switchState )
		&& Get( out_rValue.gameObj );
}

bool CommandDataSerializer::Put( const AkMonitorData::PluginTimerMonitorData& in_rValue )
{
    if ( !Put( in_rValue.fInterval ) )
		return false;

	if ( !Put( in_rValue.ulNumTimers ) )
		return false;

    for ( AkUInt32 i = 0; i < in_rValue.ulNumTimers; ++i )
	{
		const AkMonitorData::PluginTimerData & data = in_rValue.pluginData[ i ];
		if ( !Put( data.uiPluginID )
			|| !Put( data.fMillisecs )
			|| !Put( data.uiNumInstances ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::PluginTimerMonitorData& out_rValue )
{
    if ( !Get( out_rValue.fInterval ) )
		return false;

	if ( !Get( out_rValue.ulNumTimers ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumTimers; ++i )
	{
		AkMonitorData::PluginTimerData & data = out_rValue.pluginData[ i ];
		if ( !Get( data.uiPluginID )
			|| !Get( data.fMillisecs ) 
			|| !Get( data.uiNumInstances ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::MemoryMonitorData& in_rValue )
{
	if ( !Put( in_rValue.ulNumPools ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumPools; ++i )
	{
		const AkMonitorData::MemoryPoolData & data = in_rValue.poolData[ i ];
		if ( !Put( data.uReserved )
		  || !Put( data.uUsed )
		  || !Put( data.uMaxFreeBlock )
		  || !Put( data.uAllocs )
		  || !Put( data.uFrees )
		  || !Put( data.uPeakUsed ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::MemoryMonitorData& out_rValue )
{
	if ( !Get( out_rValue.ulNumPools ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumPools; ++i )
	{
		AkMonitorData::MemoryPoolData & data = out_rValue.poolData[ i ];
		if ( !Get( data.uReserved )
		  || !Get( data.uUsed )
		  || !Get( data.uMaxFreeBlock )
		  || !Get( data.uAllocs )
		  || !Get( data.uFrees )
		  || !Get( data.uPeakUsed ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::MemoryPoolNameMonitorData& in_rValue )
{
	return Put( in_rValue.ulPoolId )
		&& Put( in_rValue.wStringSize )
		&& Put( in_rValue.wStringSize ? in_rValue.szName : NULL );

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::MemoryPoolNameMonitorData& out_rValue )
{
	AkInt32 iDummy = 0;
	AkTChar* pszName = NULL;

	bool bResult = 
		   Get( out_rValue.ulPoolId )
		&& Get( out_rValue.wStringSize )
		&& Get( pszName, iDummy );

	if ( bResult && out_rValue.wStringSize )
		memcpy( &out_rValue.szName, pszName, out_rValue.wStringSize * sizeof(AkTChar) );

	return bResult;
}

bool CommandDataSerializer::Put( const AkMonitorData::EnvironmentMonitorData& in_rValue )
{
	if ( !Put( in_rValue.ulNumEnvPacket) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumEnvPacket; ++i )
	{
		const AkMonitorData::EnvPacket & data = in_rValue.envPacket[ i ];
		if ( 	!Put( data.gameObjID ) 
			|| 	!Put( data.fDryValue ) )
			return false;

		for(int i = 0; i < AK_MAX_ENVIRONMENTS_PER_OBJ; ++i )
		{
			if ( !Put( data.environments[i] ) )
				return false;
		}
		for(int i = 0; i < AK_MAX_ENVIRONMENTS_PER_OBJ; ++i )
		{
			if ( !Put( data.bypass[i] ) )
				return false;
		}
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::EnvironmentMonitorData& out_rValue )
{
	if ( !Get( out_rValue.ulNumEnvPacket ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumEnvPacket; ++i )
	{
		AkMonitorData::EnvPacket & data = out_rValue.envPacket[ i ];
		if ( 	!Get( data.gameObjID ) 
			|| 	!Get( data.fDryValue ) )
			return false;

		for(int i = 0; i < AK_MAX_ENVIRONMENTS_PER_OBJ; ++i )
		{
			if ( !Get( data.environments[i] ) )
				return false;
		}

		for(int i = 0; i < AK_MAX_ENVIRONMENTS_PER_OBJ; ++i )
		{
			if ( !Get( data.bypass[i] ) )
				return false;
		}
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::ObsOccMonitorData& in_rValue )
{
	if ( !Put( in_rValue.ulNumPacket) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumPacket; ++i )
	{
		const AkMonitorData::ObsOccPacket & data = in_rValue.obsOccPacket[ i ];
		if ( !Put( data.gameObjID ) )
			return false;

		for(int i = 0; i < AK_NUM_LISTENERS; ++i )
		{
			if ( !Put( data.fObsValue[i] ) )
				return false;

			if ( !Put( data.fOccValue[i] ) )
				return false;
		}
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::ObsOccMonitorData& out_rValue )
{
	if ( !Get( out_rValue.ulNumPacket ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumPacket; ++i )
	{
		AkMonitorData::ObsOccPacket & data = out_rValue.obsOccPacket[ i ];
		if ( !Get( data.gameObjID ) )
			return false;

		for(int i = 0; i < AK_NUM_LISTENERS; ++i )
		{
			if ( !Get( data.fObsValue[i] ) )
				return false;

			if ( !Get( data.fOccValue[i] ) )
				return false;
		}
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::ListenerMonitorData& in_rValue )
{
	for( AkUInt32 i = 0; i < AK_NUM_LISTENERS; ++i )
	{
		const AkMonitorData::ListenerPacket & data = in_rValue.listeners[ i ];
		if ( 	!Put( data.bSpatialized ) 
			||	!Put( data.VolumeOffset.fFrontLeft )
			||	!Put( data.VolumeOffset.fFrontRight )
			||	!Put( data.VolumeOffset.fCenter )
			||	!Put( data.VolumeOffset.fLfe )
			||	!Put( data.VolumeOffset.fRearLeft )
			|| 	!Put( data.VolumeOffset.fRearRight ) 
			||  !Put( data.bMotion )
			||  !Put( data.iMotionPlayer ) )
			return false;
	}

	if ( !Put( in_rValue.ulNumGameObjMask ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumGameObjMask; ++i )
	{
		const AkMonitorData::GameObjectListenerMaskPacket & data = in_rValue.gameObjMask[ i ];
		if ( 	!Put( data.gameObject ) 
			|| 	!Put( data.uListenerMask ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::ListenerMonitorData& out_rValue )
{
	for( AkUInt32 i = 0; i < AK_NUM_LISTENERS; ++i )
	{
		AkMonitorData::ListenerPacket & data = out_rValue.listeners[ i ];
		if ( 	!Get( data.bSpatialized ) 
			||	!Get( data.VolumeOffset.fFrontLeft )
			||	!Get( data.VolumeOffset.fFrontRight )
			||	!Get( data.VolumeOffset.fCenter )
			||	!Get( data.VolumeOffset.fLfe )
			||	!Get( data.VolumeOffset.fRearLeft )
			|| 	!Get( data.VolumeOffset.fRearRight ) 
			||  !Get( data.bMotion )
			||  !Get( data.iMotionPlayer ) )
			return false;
	}

	if ( !Get( out_rValue.ulNumGameObjMask ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumGameObjMask; ++i )
	{
		AkMonitorData::GameObjectListenerMaskPacket & data = out_rValue.gameObjMask[ i ];
		if ( 	!Get( data.gameObject ) 
			|| 	!Get( data.uListenerMask ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::ControllerMonitorData& in_rValue )
{
	for( AkUInt32 i = 0; i < AK_MAX_NUM_CONTROLLER_MONITORING; ++i )
	{
		const AkMonitorData::ControllerPacket & data = in_rValue.controllers[ i ];
		if ( 	!Put( data.bIsActive ) 
			|| 	!Put( data.Volume ) )
			return false;
	}

	if ( !Put( in_rValue.ulNumGameObjMask ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumGameObjMask; ++i )
	{
		const AkMonitorData::GameObjectControllerMaskPacket & data = in_rValue.gameObjMask[ i ];
		if ( 	!Put( data.gameObject ) 
			|| 	!Put( data.uControllerMask ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::ControllerMonitorData& out_rValue )
{
	for( AkUInt32 i = 0; i < AK_MAX_NUM_CONTROLLER_MONITORING; ++i )
	{
		AkMonitorData::ControllerPacket & data = out_rValue.controllers[ i ];
		if ( 	!Get( data.bIsActive ) 
			|| 	!Get( data.Volume ) )
			return false;
	}

	if ( !Get( out_rValue.ulNumGameObjMask ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumGameObjMask; ++i )
	{
		AkMonitorData::GameObjectControllerMaskPacket & data = out_rValue.gameObjMask[ i ];
		if ( 	!Get( data.gameObject ) 
			|| 	!Get( data.uControllerMask ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::DeviceRecordMonitorData& in_rValue )
{
	if ( !Put( in_rValue.deviceID )
		|| !Put( in_rValue.bCanWrite )
        || !Put( in_rValue.bCanRead )
		|| !Put( in_rValue.szDeviceName )
		|| !Put( in_rValue.uStringSize ) )
		return false;

    return true;
}

bool CommandDataSerializer::Get( AkMonitorData::DeviceRecordMonitorData& out_rValue )
{
    AkInt32 iDummy = 0;
	AkTChar* pszName = NULL;
    bool bResult = false;

	bResult = 
        ( Get( out_rValue.deviceID )
        && Get( out_rValue.bCanWrite )
		&& Get( out_rValue.bCanRead )
		&& Get( pszName, iDummy ) 
        && Get( out_rValue.uStringSize ) );

    if ( bResult )
    {
     	if ( out_rValue.uStringSize )
		{
			AKASSERT( iDummy == out_rValue.uStringSize );
    
			out_rValue.uStringSize = AkMin( out_rValue.uStringSize, AK_MONITOR_DEVICENAME_MAXLENGTH );
    
			memcpy( &out_rValue.szDeviceName, pszName, out_rValue.uStringSize*sizeof(AkTChar) );	
    
			// In case the string was bigger than MONITOR_MSG_MAXLENGTH
			out_rValue.szDeviceName[ out_rValue.uStringSize - 1 ] = 0;
		}
		else
		{
			out_rValue.szDeviceName[0] = 0;
		}
    }
	
	return bResult;
}

bool CommandDataSerializer::Put( const AkMonitorData::StreamRecordMonitorData& in_rValue )
{
    if ( !Put( in_rValue.ulNumNewRecords ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumNewRecords; ++i )
	{
		const AkMonitorData::StreamRecord & data = in_rValue.streamRecords[ i ];
		if ( !Put( data.uStreamID )
		  || !Put( data.deviceID )
		  || !Put( data.szStreamName )
          || !Put( data.uStringSize )
		  || !Put( data.uFileSize )
		  || !Put( data.bIsAutoStream ) )
			return false;
	}
    return true;
}

bool CommandDataSerializer::Get( AkMonitorData::StreamRecordMonitorData& out_rValue )
{
    AkInt32 iDummy = 0;
	AkTChar* pszName = NULL;
    bool bResult = false;

    if ( !Get( out_rValue.ulNumNewRecords ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumNewRecords; ++i )
	{
		AkMonitorData::StreamRecord & data = out_rValue.streamRecords[ i ];
		bResult = 
          ( Get( data.uStreamID )
		  && Get( data.deviceID )
		  && Get( pszName, iDummy )
          && Get( data.uStringSize )
		  && Get( data.uFileSize )
		  && Get( data.bIsAutoStream ) );

        if ( bResult )
        {
     	    if ( out_rValue.streamRecords[i].uStringSize )
		    {
			    AKASSERT( iDummy == out_rValue.streamRecords[i].uStringSize );
    	
				out_rValue.streamRecords[i].uStringSize = AkMin( out_rValue.streamRecords[i].uStringSize, AK_MONITOR_STREAMNAME_MAXLENGTH );
    	
			    memcpy( &out_rValue.streamRecords[i].szStreamName, pszName, out_rValue.streamRecords[i].uStringSize*sizeof(AkTChar) );	
    	
			    // In case the string was bigger than MONITOR_MSG_MAXLENGTH
			    out_rValue.streamRecords[i].szStreamName[ out_rValue.streamRecords[i].uStringSize - 1 ] = 0;
		    }
		    else
		    {
			    out_rValue.streamRecords[i].szStreamName[0] = 0;
		    }
        }
	}

	return bResult;
}

bool CommandDataSerializer::Put( const AkMonitorData::StreamingMonitorData& in_rValue )
{
    if ( !Put( in_rValue.fInterval ) )
		return false;

    if ( !Put( in_rValue.ulNumStreams ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.ulNumStreams; ++i )
	{
		const AkMonitorData::StreamData & data = in_rValue.streamData[ i ];
		if ( !Put( data.uStreamID )
		  || !Put( data.uPriority )
		  || !Put( data.uFilePosition )
		  || !Put( data.uBufferSize )
		  || !Put( data.uAvailableData )
		  || !Put( data.uNumBytesTransfered ) )
			return false;
	}
    return true;
}

bool CommandDataSerializer::Get( AkMonitorData::StreamingMonitorData& out_rValue )
{
    if ( !Get( out_rValue.fInterval ) )
		return false;

    if ( !Get( out_rValue.ulNumStreams ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.ulNumStreams; ++i )
	{
		AkMonitorData::StreamData & data = out_rValue.streamData[ i ];
		if ( !Get( data.uStreamID )
		  || !Get( data.uPriority )
		  || !Get( data.uFilePosition )
		  || !Get( data.uBufferSize )
		  || !Get( data.uAvailableData ) 
		  || !Get( data.uNumBytesTransfered ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::PipelineMonitorData& in_rValue )
{
    if ( !Put( in_rValue.fInterval ) 
	  || !Put( in_rValue.numPipelines ) )
	  return false;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
	{
		if ( !Put( in_rValue.masterBusFxId[ uFXIndex ] ) )
			return false;
	}

	if ( !Put( in_rValue.numPipelinesFeedback ) )
		return false;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
	{
		if ( !Put( in_rValue.feedbackMasterBusFxId[ uFXIndex ] ) )
			return false;
	}

	for ( AkUInt16 i = 0; i < in_rValue.numPipelines; ++i )
	{
		const AkMonitorData::PipelineData & data = in_rValue.pipelines[ i ];

		if ( !Put( data.pipelineID )
		  || !Put( data.gameObjID )
		  || !Put( data.soundID )
		  || !Put( data.mixBusID ) )
		  return false;

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
		{
			if ( !Put( data.fxID[ uFXIndex ] ) )
				return false;
		}

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
		{
			if ( !Put( data.busFxID[ uFXIndex ] ) )
				return false;
		}

		if ( !Put( data.feedbackMixBusID ) )
			return false;

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
		{
			if ( !Put( data.feedbackBusFxID[ uFXIndex ] ) )
				return false;
		}

		if ( !PutEnum( data.srcType )
		  || !Put( data.priority )
		  || !Put( data.fVolume )
		  || !Put( data.bIsStarving )
		  || !Put( data.bIsVirtual ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::PipelineMonitorData& out_rValue )
{
    if ( !Get( out_rValue.fInterval ) 
	  || !Get( out_rValue.numPipelines ) )
	  return false;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
	{
		if ( !Get( out_rValue.masterBusFxId[ uFXIndex ] ) )
			return false;
	}

	if ( !Get( out_rValue.numPipelinesFeedback ) )
		return false;

	for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
	{
		if ( !Get( out_rValue.feedbackMasterBusFxId[ uFXIndex ] ) )
			return false;
	}

	for ( AkUInt16 i = 0; i < out_rValue.numPipelines; ++i )
	{
		AkMonitorData::PipelineData & data = out_rValue.pipelines[ i ];

		if ( !Get( data.pipelineID )
		  || !Get( data.gameObjID )
		  || !Get( data.soundID )
		  || !Get( data.mixBusID ) )
		  return false;

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
		{
			if ( !Get( data.fxID[ uFXIndex ] ) )
				return false;
		}

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
		{
			if ( !Get( data.busFxID[ uFXIndex ] ) )
				return false;
		}

		if ( !Get( data.feedbackMixBusID ) )
			return false;

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PROFILER; ++uFXIndex )
		{
			if ( !Get( data.feedbackBusFxID[ uFXIndex ] ) )
				return false;
		}

		if ( !GetEnum( data.srcType )
		  || !Get( data.priority )
		  || !Get( data.fVolume )
		  || !Get( data.bIsStarving )
		  || !Get( data.bIsVirtual ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::MarkersMonitorData& in_rValue )
{
	bool bRet = Put( in_rValue.playingID )
		&& Put( in_rValue.gameObjPtr )
		&& Put( (AkInt32)in_rValue.eNotificationReason );
	
	// Container history
	{
		bRet = bRet
			&& Put( in_rValue.cntrHistArray.uiArraySize );

		for( AkUInt32 i = 0; i < in_rValue.cntrHistArray.uiArraySize && bRet; ++i )
			Put( in_rValue.cntrHistArray.aCntrHist[i] );
	}

	bRet = bRet && Put( in_rValue.customParam )
		&& Put( in_rValue.targetObjectID )
		&& Put( in_rValue.wStringSize )
		&& Put( in_rValue.wStringSize ? in_rValue.szLabel : NULL );

	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::MarkersMonitorData& out_rValue )
{
	bool bRet = Get( out_rValue.playingID )
		&& Get( out_rValue.gameObjPtr )
		&& Get( (AkInt32&)out_rValue.eNotificationReason );
	
	// Container history
	{
		bRet = bRet
			&& Get( out_rValue.cntrHistArray.uiArraySize );

		for( AkUInt32 i = 0; i < out_rValue.cntrHistArray.uiArraySize && bRet; ++i )
			Get( out_rValue.cntrHistArray.aCntrHist[i] );
	}

	AkInt32 nLabelSize = 0;
	char *pszLabel = NULL;

	bRet = bRet && Get( out_rValue.customParam )
		&& Get( out_rValue.targetObjectID )
		&& Get( out_rValue.wStringSize )
		&& Get( pszLabel, nLabelSize );

	if ( bRet && out_rValue.wStringSize )
		memcpy( &out_rValue.szLabel, pszLabel, out_rValue.wStringSize );	

	return bRet;
}

bool CommandDataSerializer::Put( const AkMonitorData::OutputMonitorData& in_rValue )
{
	bool bRet = Put( in_rValue.fOffset )
		&& Put( in_rValue.fPeak )
		&& Put( in_rValue.fRMS );

	return bRet;
}

bool CommandDataSerializer::Get( AkMonitorData::OutputMonitorData& out_rValue )
{
	bool bRet = Get( out_rValue.fOffset )
		&& Get( out_rValue.fPeak )
		&& Get( out_rValue.fRMS );

	return bRet;
}

bool CommandDataSerializer::Put( const AkMonitorData::SegmentPositionMonitorData& in_rValue )
{
    if ( !Put( in_rValue.numPositions )  )
		return false;

	for ( AkUInt16 i = 0; i < in_rValue.numPositions; ++i )
	{
		const AkMonitorData::SegmentPositionData & data = in_rValue.positions[ i ];

		if ( !Put( data.f64Position )
		  || !Put( data.playingID )
		  || !Put( data.segmentID )
		  || !Put( data.customParam ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::SegmentPositionMonitorData& out_rValue )
{
    if ( !Get( out_rValue.numPositions ) )
		return false;

	for ( AkUInt16 i = 0; i < out_rValue.numPositions; ++i )
	{
		AkMonitorData::SegmentPositionData & data = out_rValue.positions[ i ];

		if ( !Get( data.f64Position )
		  || !Get( data.playingID )
		  || !Get( data.segmentID )
		  || !Get( data.customParam ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::RTPCValuesMonitorData& in_rValue )
{
    if ( !Put( in_rValue.ulNumRTPCValues )  )
		return false;

	for ( AkUInt16 i = 0; i < in_rValue.ulNumRTPCValues; ++i )
	{
		const AkMonitorData::RTPCValuesPacket & data = in_rValue.rtpcValues[ i ];

		if ( !Put( data.rtpcID )
		  || !Put( data.gameObjectID )
		  || !Put( data.value ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::RTPCValuesMonitorData& out_rValue )
{
    if ( !Get( out_rValue.ulNumRTPCValues ) )
		return false;

	for ( AkUInt16 i = 0; i < out_rValue.ulNumRTPCValues; ++i )
	{
		AkMonitorData::RTPCValuesPacket & data = out_rValue.rtpcValues[ i ];

		if ( !Get( data.rtpcID )
		  || !Get( data.gameObjectID )
		  || !Get( data.value ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Put( const	AkMonitorData::LoadedSoundBankMonitorData& in_rValue )
{
	return Put( in_rValue.bankID )
		&& Put( in_rValue.memPoolID )
		&& Put( in_rValue.uBankSize )
		&& Put( in_rValue.uNumIndexableItems )
		&& Put( in_rValue.uNumMediaItems )
		&& Put( in_rValue.bIsExplicitelyLoaded )
		&& Put( in_rValue.bIsDestroyed );
}

bool CommandDataSerializer::Get( AkMonitorData::LoadedSoundBankMonitorData& out_rValue )
{
	return Get( out_rValue.bankID )
		&& Get( out_rValue.memPoolID )
		&& Get( out_rValue.uBankSize )
		&& Get( out_rValue.uNumIndexableItems )
		&& Get( out_rValue.uNumMediaItems )
		&& Get( out_rValue.bIsExplicitelyLoaded )
		&& Get( out_rValue.bIsDestroyed );
}

bool CommandDataSerializer::Put( const	AkMonitorData::MediaPreparedMonitorData& in_rValue )
{
	if ( !Put( in_rValue.uMediaID ) 
	  || !Put( in_rValue.uMediaSize ) 
	  || !Put( in_rValue.uArraySize ) )
		return false;

	for ( AkUInt16 i = 0; i < in_rValue.uArraySize; ++i )
	{
		if ( !Put( in_rValue.bankIDs[i] ) )
			return false;
	}
	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::MediaPreparedMonitorData& out_rValue )
{
	if ( !Get( out_rValue.uMediaID ) 
	  || !Get( out_rValue.uMediaSize ) 
	  || !Get( out_rValue.uArraySize ) )
		return false;

	for ( AkUInt16 i = 0; i < out_rValue.uArraySize; ++i )
	{
		if ( !Get( out_rValue.bankIDs[i] ) )
			return false;
	}
	return true;
}

bool CommandDataSerializer::Put( const	AkMonitorData::EventPreparedMonitorData& in_rValue )
{
	return Put( in_rValue.eventID )
		&& Put( in_rValue.uRefCount );
}

bool CommandDataSerializer::Get( AkMonitorData::EventPreparedMonitorData& out_rValue )
{
	return Get( out_rValue.eventID )
		&& Get( out_rValue.uRefCount );
}

bool CommandDataSerializer::Put( const	AkMonitorData::GameSyncMonitorData& in_rValue )
{
	return Put( in_rValue.groupID )
		&& Put( in_rValue.syncID )
		&& PutEnum( in_rValue.eSyncType )
		&& Put( in_rValue.bIsEnabled );
}

bool CommandDataSerializer::Get( AkMonitorData::GameSyncMonitorData& out_rValue )
{
	return Get( out_rValue.groupID )
		&& Get( out_rValue.syncID )
		&& GetEnum( out_rValue.eSyncType )
		&& Get( out_rValue.bIsEnabled );
}

bool CommandDataSerializer::Put( const AkMonitorData::ResolveDialogueMonitorData& in_rValue )
{
	if ( !Put( in_rValue.idDialogueEvent )
		|| !Put( in_rValue.idResolved )
		|| !Put( in_rValue.uPathSize ) )
		return false;

	for ( AkUInt32 i = 0; i < in_rValue.uPathSize; ++i )
	{
		if ( !PutEnum( in_rValue.aPath[i] ) )
			return false;
	}

	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::ResolveDialogueMonitorData& out_rValue )
{
	if ( !Get( out_rValue.idDialogueEvent )
		|| !Get( out_rValue.idResolved )
		|| !Get( out_rValue.uPathSize ) )
		return false;

	for ( AkUInt32 i = 0; i < out_rValue.uPathSize; ++i )
	{
		if ( !GetEnum( out_rValue.aPath[i] ) )
			return false;
	}

	return true;
}
	
bool CommandDataSerializer::Put( const	AkMonitorData::FeedbackMonitorData& in_rValue )
{
	if (!Put(in_rValue.timer.fInterval) ||
		!Put(in_rValue.timer.fAudioThread) ||
		!Put(in_rValue.fPeak) )
		return false;
	return true;
}

bool CommandDataSerializer::Get( AkMonitorData::FeedbackMonitorData& out_rValue )
{
	if (!Get(out_rValue.timer.fInterval) ||
		!Get(out_rValue.timer.fAudioThread) || 
		!Get(out_rValue.fPeak))
		return false;
	return true;
}

bool CommandDataSerializer::Put( const AkMonitorData::FeedbackDevicesMonitorData& in_rValue )
{
	if (Put(in_rValue.usDeviceCount))
	{
		for(AkUInt32 i = 0; i < in_rValue.usDeviceCount; i++)
		{
			if (!Put(in_rValue.deviceIDs[i].usCompanyID) ||
				!Put(in_rValue.deviceIDs[i].usDeviceID) ||
				!Put(in_rValue.deviceIDs[i].ucPlayerActive))
				return false;
		}
		return true;
	}
	return false;
}

bool CommandDataSerializer::Get( AkMonitorData::FeedbackDevicesMonitorData& out_rValue )
{
	if (Get(out_rValue.usDeviceCount))
	{
		for(AkUInt32 i = 0; i < out_rValue.usDeviceCount; i++)
		{
			if (!Get(out_rValue.deviceIDs[i].usCompanyID) ||
				!Get(out_rValue.deviceIDs[i].usDeviceID) ||
				!Get(out_rValue.deviceIDs[i].ucPlayerActive))
				return false;
		}
		return true;
	}
	return false;
}

bool CommandDataSerializer::Put( const AkMonitorData::FeedbackGameObjMonitorData& in_rValue )
{
	if (Put(in_rValue.ulNumGameObjMask))
	{
		for(AkUInt32 i = 0; i < in_rValue.ulNumGameObjMask; i++)
		{
			if (!Put(in_rValue.gameObjInfo[i].gameObject) ||
				!Put(in_rValue.gameObjInfo[i].uPlayerMask))
				return false;
		}
		return true;
	}
	return false;
}

bool CommandDataSerializer::Get( AkMonitorData::FeedbackGameObjMonitorData& out_rValue )
{
	if (Get(out_rValue.ulNumGameObjMask))
	{
		for(AkUInt32 i = 0; i < out_rValue.ulNumGameObjMask; i++)
		{
			if (!Get(out_rValue.gameObjInfo[i].gameObject) ||
				!Get(out_rValue.gameObjInfo[i].uPlayerMask))
				return false;
		}
		return true;
	}
	return false;
}

void CommandDataSerializer::SetDataPeeking( bool in_bPeekData )
{
	if( in_bPeekData )
		m_readPosBeforePeeking = m_readPos;
	else
		m_readPos = m_readPosBeforePeeking;
}

AkInt16 CommandDataSerializer::Swap( const AkInt16& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::WordSwap( in_rValue ) : in_rValue;
}

AkUInt16 CommandDataSerializer::Swap( const AkUInt16& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::WordSwap( in_rValue ) : in_rValue;
}

AkInt32 CommandDataSerializer::Swap( const AkInt32& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::DWordSwap( in_rValue ) : in_rValue;
}

AkUInt32 CommandDataSerializer::Swap( const AkUInt32& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::DWordSwap( in_rValue ) : in_rValue;
}

AkInt64 CommandDataSerializer::Swap( const AkInt64& in_rValue ) const
{
    AkInt64 liSwapped;
    liSwapped = m_bSwapEndian ? AK::EndianByteSwap::Int64Swap( in_rValue ) : in_rValue;
	return liSwapped;
}

AkUInt64 CommandDataSerializer::Swap( const AkUInt64& in_rValue ) const
{
    AkUInt64 uiSwapped;
    uiSwapped = m_bSwapEndian ? AK::EndianByteSwap::Int64Swap( in_rValue ) : in_rValue;
	return uiSwapped;
}
/*
AkUInt64 CommandDataSerializer::Swap( const AkUInt64& in_rValue ) const
{
	return m_bSwapEndian ? AK::EndianByteSwap::Int64Swap( in_rValue ) : in_rValue;
}
*/
AkReal32 CommandDataSerializer::Swap( const AkReal32& in_rValue ) const
{
	AkReal32 retVal = in_rValue;
	
	if( m_bSwapEndian )
		AK::EndianByteSwap::Swap( (AkUInt8*)&in_rValue, 4, (AkUInt8*)&retVal );
	
	return retVal;
}

AkReal64 CommandDataSerializer::Swap( const AkReal64& in_rValue ) const
{
	AkReal64 retVal = in_rValue;
	
	if( m_bSwapEndian )
		AK::EndianByteSwap::Swap( (AkUInt8*)&in_rValue, 8, (AkUInt8*)&retVal );
	
	return retVal;
}
