//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file 
/// Software source plug-in and effect plug-in interfaces.

#ifndef _IAK_PLUGIN_H_
#define _IAK_PLUGIN_H_

#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <AK/SoundEngine/Common/IAkRTPCSubscriber.h>
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <AK/Tools/Common/AkLock.h>

#ifdef WIN32
#include <xmmintrin.h>
#endif

#ifdef XBOX360
#include "ppcintrinsics.h"
#endif

#ifdef __PPU__
#include <stdlib.h>
#include <string.h>
#endif

#ifdef AK_PS3
#include <AK/Plugin/PluginServices/PS3/MultiCoreServices.h>
#else
#include <AK/Plugin/PluginServices/MultiCoreServices.h>
#endif

#ifdef RVL_OS
#include <revolution/ax.h>
#endif

/// Plug-in type.
/// \sa 
/// - AkPluginInfo
enum AkPluginType
{
	AkPluginTypeNone            = 0,	///< Unknown/invalid plug-in type
	AkPluginTypeCodec           = 1,	///< Compressor/decompressor plug-in (allows support for custom audio file types)
	AkPluginTypeSource          = 2,	///< Source plug-in: creates sound by synthesis method (no input, just output)
	AkPluginTypeEffect          = 3,	///< Effect plug-in: applies processing to audio data
	/// @cond NOT_YET_AVAILABLE
	AkPluginTypeFeedbackDevice	= 4,	///< Feedback Device plug-in: feeds movement data to devices.
	AkPluginTypeFeedbackSource	= 5,	///< Feedback Device source plug-in: feeds movement data to device busses.
	/// @endcond
	AkPluginTypeMask            = 0xf 	///< Plug-in type mask is 4 bits
};

/// Plug-in information structure.
/// \remarks The bIsInPlace field is only relevant for effect plug-ins.
/// \remarks Currently asynchronous effects are only supported on PS3 effect plug-ins (not source plug-ins), otherwise effects should be synchronous.
/// \sa
/// - \ref iakeffect_geteffectinfo
struct AkPluginInfo
{
	AkPluginType eType;            ///< Plug-in type
	bool         bIsInPlace; 	   ///< Buffer usage (in-place or not)
	bool         bIsAsynchronous;  ///< Asynchronous plug-in flag
};

/// Audio buffer structure including the address of an audio buffer, the number of valid frames inside, 
/// and the maximum number of frames the audio buffer can hold.
/// \sa
/// - \ref fx_audiobuffer_struct
struct AkPluginAudioBuffer
{
	union
	{
		AkReal32 **		pData;		///< Array of audio buffers for each channel( default union type, used on most platforms )
		AkReal32 **		pR32Data;	///< Array of audio buffers for each channel( real 32 union type )
		AkInt16 **		pI16Data;	///< Array of audio buffers for each channel( signed 16 bits integer union type )
		AkInt8 **		pI8Data;	///< Array of audio buffers for each channel( signed 8 bits integer union type )
		AkUInt8 **		pU8Data;	///< Array of audio buffers for each channel( unsigned 8 bits integer union type )
	};
	AkUInt32		uMaxFrames;		///< Number of sample frames the buffer can hold
	AkUInt32		uValidFrames;	///< Number of valid sample frames in the audio buffer
	AKRESULT		eState;			///< Plug-in execution status	
} AK_ALIGNED_16;

namespace AK
{
	/// Interface to retrieve contextual information for an effect plug-in.
	/// \sa
	/// - \ref iakmonadiceffect_init
	class IAkEffectPluginContext
	{
	public:
		/// Determine whether the effect is to be used in Send Mode or not.
		/// Effects used as environmentals are always used in Send Mode.
		/// \return True if the effect is in Send Mode, False otherwise
		virtual bool IsSendModeEffect( ) = 0;
	};

	/// Interface to retrieve contextual information for a source plug-in.
	/// \sa
	/// - \ref iaksourceeffect_init
	class IAkSourcePluginContext
	{
	public:

		/// Retrieve the number of loops the source should produce.
		/// \return The number of loop iterations the source should produce (0 if infinite looping)
		virtual AkUInt16 GetNumLoops( ) = 0;

		/// Retrieve the streaming manager access interface.
		virtual IAkStreamMgr * GetStreamMgr( ) = 0;
	};

	/// Parameter node interface that must synchronize access to an enclosed parameter structure 
	/// that may be shared by various instances running in different threads.
	/// \aknote It is the responsibility of the parameter node implementer to make the parameter structure access
	/// thread-safe. \endaknote
	/// \aknote Plug-ins should declare the AK_USE_PLUGIN_ALLOCATOR() macro in their public interface. \endaknote
	/// \aknote The implementer of this interface should also expose a static creation function
	/// that will return a new parameter node instance when required (see \ref se_plugins_overview). \endaknote
	/// \sa
	/// - \ref shared_parameter_interface
	class IAkPluginParam : public IAkRTPCSubscriber
	{
	public:

		/// Create a duplicate of the parameter node instance in its current state.
		/// \aknote The allocation of the new parameter node should be done through the AK_PLUGIN_NEW() macro. \endaknote
		/// \return Pointer to a duplicated plug-in parameter node interface
		/// \sa
		/// - \ref iakeffectparam_clone
		virtual IAkPluginParam * Clone( 
			IAkPluginMemAlloc * in_pAllocator 	///< Interface to memory allocator to be used
			) = 0;

		/// Initialize the plug-in parameter node interface.
		/// Initializes the internal parameter structure to default values or with the provided parameter 
		/// block if it is valid. \endaknote
		/// \aknote If the provided parameter block is valid, use SetParamsBlock() to set all parameters at once. \endaknote
		/// \return Possible return values are: AK_Success, AK_Fail, AK_InvalidParameter
		/// \sa
		/// - \ref iakeffectparam_init
		virtual AKRESULT Init( 
			IAkPluginMemAlloc *	in_pAllocator,		///< Interface to the memory allocator to be used					   
			void *				in_pParamsBlock,	///< Pointer to a parameter structure block
			AkUInt32        	in_uBlockSize		///< Size of the parameter structure block
			) = 0;

		/// Called by the sound engine when a parameter node is terminated.
		/// \aknote The self-destruction of the parameter node must be done using the AK_PLUGIN_DELETE() macro. \endaknote
		/// \return AK_Success if successful, AK_Fail otherwise
		/// \sa
		/// - \ref iakeffectparam_term
		virtual AKRESULT Term( 
			IAkPluginMemAlloc * in_pAllocator		///< Interface to memory allocator to be used
			) = 0;	

		/// Set all plug-in parameters at once using a parameter block.
		/// \return AK_Success if successful, AK_InvalidParameter otherwise
		/// \sa
		/// - \ref iakeffectparam_setparamsblock
		virtual AKRESULT SetParamsBlock( 
			void *		in_pParamsBlock, 	///< Pointer to a parameter structure block
			AkUInt32	in_uBlockSize		///< Size of the parameter structure block
			) = 0;

		/// Update a single parameter at a time and perform the necessary actions on the parameter changes.
		/// \aknote The parameter ID corresponds to the AudioEnginePropertyID in the plug-in XML description file. \endaknote
		/// \return AK_Success if successful, AK_InvalidParameter otherwise
		/// \sa
		/// - \ref iakeffectparam_setparam
		virtual AKRESULT SetParam( 
			AkPluginParamID	in_paramID,		///< ID number of the parameter to set
			void *			in_pValue, 		///< Pointer to the value of the parameter to set
			AkUInt32    	in_uParamSize	///< Size of the value of the parameter to set
			) = 0;

	protected:

		/// Acquire the lock to provide thread-safe access to the effect parameter structure. This is only required when the effect is being
		/// used by Wwise authoring application. Define AK_OPTIMIZED in the preprocessor defines of the release version of your effect to 
		/// avoid the performance cost of the critical section. 
		AkForceInline void LockParams()
		{
#ifndef AK_OPTIMIZED
			m_ParamLock.Lock();
#endif
		}

		/// Release the lock to provide thread-safe access to the effect parameter structure. This is only required when the effect is being
		/// used by Wwise authoring application. Define AK_OPTIMIZED in the preprocessor defines of the release version of your effect to 
		/// avoid the performance cost of the critical section. 
		AkForceInline void UnlockParams()
		{
#ifndef AK_OPTIMIZED
			m_ParamLock.Unlock();
#endif
		}

		/// Parameter lock declaration to provide thread-safe access to the effect parameter structure. This is only required when the effect is being
		/// used by the Wwise authoring application. Define AK_OPTIMIZED in the preprocessor defines of the release version of your effect to 
		/// avoid the performance cost of the critical section. 
#ifndef AK_OPTIMIZED
		CAkLock m_ParamLock;	///< Parameter structure lock declaration
#endif
	};

	/// Wwise sound engine plug-in interface. Shared functionality across different plug-in types.
	/// \aknote Plug-ins should declare the AK_USE_PLUGIN_ALLOCATOR() macro in their public interface. \endaknote
	/// \aknote The implementer of this interface should also expose a static creation function
	/// that will return a new plug-in instance when required (see \ref soundengine_plugins). \endaknote
	class IAkPlugin
	{
	public:

		/// Release the resources upon termination of the plug-in.
		/// \return AK_Success if successful, AK_Fail otherwise
		/// \aknote The self-destruction of the plug-in must be done using AK_PLUGIN_DELETE() macro. \endaknote
		/// \sa
		/// - \ref iakeffect_term
		virtual AKRESULT Term( 
			IAkPluginMemAlloc * in_pAllocator 	///< Interface to memory allocator to be used by the plug-in
			) = 0;

		/// The reset action should perform any actions required to reinitialize the state of the plug-in 
		/// to its original state (e.g. after Init() or on effect bypass).
		/// \return AK_Success if successful, AK_Fail otherwise.
		/// \sa
		/// - \ref iakeffect_reset
		virtual AKRESULT Reset( ) = 0;

		/// Plug-in information query mechanism used when the sound engine requires information 
		/// about the plug-in to determine its behavior
		/// \return AK_Success if successful.
		/// \sa
		/// - \ref iakeffect_geteffectinfo
		virtual AKRESULT GetPluginInfo( 
			AkPluginInfo & out_rPluginInfo	///< Reference to the plug-in information structure to be retrieved
			) = 0;

		/// Software effect plug-in DSP execution.
		/// \aknote The effect can output as much as wanted up to uMaxFrames. All sample frames passed uValidFrames at input time are 
		/// not initialized and it is the responsibility of the effect to do so. When modifying the number of valid frames within execution
		/// (e.g. to flush delay lines) the effect should notify the pipeline by updating uValidFrames accordingly.
		/// \aknote The effect will stop being called by the pipeline when AK_NoMoreData is returned in the the eState field of the AkPluginAudioBuffer structure.
		virtual void Execute( 
				AkPluginAudioBuffer *				io_pBuffer		///< In/Out audio buffer data structure (in-place processing)
#ifdef AK_PS3
				, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif
				) = 0;

	};

	/// Software effect plug-in interface (see \ref soundengine_plugins_effects).
	class IAkEffectPlugin : public IAkPlugin
	{
	public:

		/// Software effect plug-in initialization. Prepares the effect for data processing, allocates memory and sets up the initial conditions. 
		/// \aknote Memory allocation should be done through appropriate macros (see \ref fx_memory_alloc). \endaknote
		/// \sa
		/// - \ref iakmonadiceffect_init
		virtual AKRESULT Init( 
			IAkPluginMemAlloc *			in_pAllocator,				///< Interface to memory allocator to be used by the effect
			IAkEffectPluginContext *	in_pEffectPluginContext,	///< Interface to effect plug-in's context		    
			IAkPluginParam *			in_pParams,					///< Interface to plug-in parameters
			AkAudioFormat &				in_rFormat					///< Audio data format of the input/output signal
			) = 0;

#ifdef RVL_OS
		/// Wii effects must provide the callback function that will be called on effect execution.
		virtual AXAuxCallback GetFXCallback() = 0;

		/// Wii effects must provide a pointer to the params that will be used by the effect callback.
		virtual void* GetFXParams() = 0;

		/// Wii effects must provide an estimated time after what the effect tail will be considered finished.
		/// This information will be used by the sound engine to stop processing the environmentals effects
		/// that are not in use so that the Aux_A, Aux_B and Aux_C can be attributed to other effects.
		/// This function will be called only after the FX initialization was completed.
		/// \return The estimated time in milliseconds.
		virtual AkUInt32 GetTailTime() = 0;
#endif
	};

	/// Wwise sound engine source plug-in interface (see \ref soundengine_plugins_source).
	class IAkSourcePlugin : public IAkPlugin
	{
	public:

		/// Source plug-in initialization. Gets the plug-in ready for data processing, allocates memory and sets up the initial conditions. 
		/// \aknote Memory allocation should be done through the appropriate macros (see \ref fx_memory_alloc). \endaknote
		/// \sa
		/// - \ref iaksourceeffect_init
		virtual AKRESULT Init( 
			IAkPluginMemAlloc *			in_pAllocator,					///< Interface to the memory allocator to be used by the plug-in
			IAkSourcePluginContext *	in_pSourcePluginContext,		///< Interface to the source plug-in's context
			IAkPluginParam *			in_pParams,						///< Interface to the plug-in parameters
			AkAudioFormat &				io_rFormat						///< Audio format of the output data to be produced by the plug-in (mono native by default)
			) = 0;

		/// This method is called to determine the approximate duration of the source.
		/// \return The duration of the source, in milliseconds.
		/// \sa
		/// - \ref iaksourceeffect_getduration
		virtual AkTimeMs GetDuration( ) const = 0;
	};

#ifdef XBOX360
	/// Utility function to read 32-bit floating point data from unaligned memory boundaries. Required by some platforms.
	/// \sa
	/// - \ref iakeffectparam_setparamsblock
	inline AkReal32 ReadBankReal32( AkReal32* __unaligned ptr )
	{
		return *ptr;
	}
#elif defined (__PPU__)
	/// Utility function to read 32-bit floating point data from memory unaligned to 64-bit boundaries. Required by some platforms.
	/// \sa
	/// - \ref iakeffectparam_setparamsblock
	inline AkReal32 ReadBankReal32( AkReal32* ptr )
	{
		struct _Real { AkReal32 Real; } __attribute__((packed)) *p = (_Real *)ptr;
		return p->Real;
	}
#elif defined (RVL_OS)
	
	template < typename TO, typename FROM >
	inline TO unionSys_cast( FROM value )
	{
		union { FROM from; TO to; } convert;
		convert.from = value;
		return convert.to;
	}
	/// Utility function to read 32-bit floating point data from memory unaligned to 32-bit boundaries. Required by some platforms.
	/// \sa
	/// - \ref iakeffectparam_setparamsblock
	inline AkReal32 ReadBankReal32( AkReal32* ptr )
	{
		AkUInt32 *puint = reinterpret_cast<AkUInt32 *>( ptr );
		volatile AkUInt32 uint = *puint;
		return unionSys_cast<AkReal32>( uint );
	}
	
#else
	/// Utility function to read 32-bit floating point data from memory unaligned to 64-bit boundaries. Required by some platforms.
	/// \sa
	/// - \ref iakeffectparam_setparamsblock
	inline AkReal32 ReadBankReal32( AkReal32* ptr )
	{
		return *ptr;
	}
#endif

#define PluginMin(_a_,_b_) ((_a_) < (_b_) ? (_a_) : (_b_))		///< Minimum between 2 values
#define PluginMax(_a_,_b_) ((_a_) > (_b_) ? (_a_) : (_b_))		///< Maximum between 2 values

#ifdef XBOX360
#define PluginFPMin(_a_,_b_) ((AkReal32)fpmin(_a_,_b_))			///< Minimum between 2 floating point values (optimized on some platforms)
#define PluginFPMax(_a_,_b_) ((AkReal32)fpmax(_a_,_b_))			///< Maximum between 2 floating point values (optimized on some platforms)
#define PluginFPSel(_a_,_b_,_c_) ((AkReal32)__fsel(_a_,_b_,_c_)) ///< Faster floating point branches on some platforms
#elif defined (__PPU__)
#define PluginFPMin(_a_,_b_) (__fsels((_a_) - (_b_),_b_,_a_ ))	///< Minimum between 2 floating point values (optimized on some platforms)
#define PluginFPMax(_a_,_b_) (__fsels((_a_) - (_b_),_a_,_b_))	///< Maximum between 2 floating point values (optimized on some platforms)
#define PluginFPSel(_a_,_b_,_c_) (__fsels(_a_,_b_,_c_))			///< Faster floating point branches on some platforms
#else
#define PluginFPMin(_a_,_b_) ((_a_) < (_b_) ? (_a_) : (_b_))	///< Minimum between 2 floating point values (optimized on some platforms)
#define PluginFPMax(_a_,_b_) ((_a_) > (_b_) ? (_a_) : (_b_))	///< Maximum between 2 floating point values (optimized on some platforms)
#define PluginFPSel(_a_,_b_,_c_) (((_a_) >= 0) ? (_b_) : (_c_)) ///< Faster floating point branches on some platforms
#endif

}

/// Registered plugin creation function prototype.
typedef AK::IAkPlugin* (*AkCreatePluginCallback)( AK::IAkPluginMemAlloc * in_pAllocator );
/// Registered plugin parameter node creation function prototype.
typedef AK::IAkPluginParam * (*AkCreateParamCallback)( AK::IAkPluginMemAlloc * in_pAllocator );

#endif // _IAK_PLUGIN_H_
