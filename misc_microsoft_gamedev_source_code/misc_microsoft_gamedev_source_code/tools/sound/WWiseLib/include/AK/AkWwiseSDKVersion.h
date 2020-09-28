//////////////////////////////////////////////////////////////////////
//
// Copyright (c) Audiokinetic Inc. 2006-2008. All rights reserved.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKWWISESDKVERSION_H_
#define _AKWWISESDKVERSION_H_

/// \file 
/// Audiokinetic Wwise SDK version, build number and copyright defines. These
/// are used by sample projects to display the version and to include it in DLL or
/// EXE resources. They can also be used by games or tools to display the current
/// version and build number of the Wwise Sound Engine.

/// @name Wwise SDK Version - Numeric values
//@{

/// Wwise SDK major version
#define AK_WWISESDK_VERSION_MAJOR				2008

/// Wwise SDK minor version
#define AK_WWISESDK_VERSION_MINOR				1

/// Wwise SDK sub-minor version
#define AK_WWISESDK_VERSION_SUBMINOR			1

/// Wwise SDK build number
#define AK_WWISESDK_VERSION_BUILD				2713

//@}

/// @name Wwise SDK Version - String values
//@{

/// Macro that "converts" a numeric define to a string
/// \sa
/// - \ref AK_WWISESDK_NUM2STRING
#define _AK_WWISESDK_NUM2STRING( n )			#n

/// Macro that "converts" a numeric define to a string
#define AK_WWISESDK_NUM2STRING( n )				_AK_WWISESDK_NUM2STRING( n )

/// String representing the Wwise SDK version
#if AK_WWISESDK_VERSION_SUBMINOR > 0
	#define AK_WWISESDK_VERSIONNAME				"v" AK_WWISESDK_NUM2STRING( AK_WWISESDK_VERSION_MAJOR ) \
												"."	AK_WWISESDK_NUM2STRING( AK_WWISESDK_VERSION_MINOR ) \
												"."	AK_WWISESDK_NUM2STRING( AK_WWISESDK_VERSION_SUBMINOR )
#else
	#define AK_WWISESDK_VERSIONNAME				"v" AK_WWISESDK_NUM2STRING( AK_WWISESDK_VERSION_MAJOR ) \
												"."	AK_WWISESDK_NUM2STRING( AK_WWISESDK_VERSION_MINOR )
#endif

/// @name Wwise SDK Copyright Notice

//@{

#ifndef __PPU__
	/// Wwise SDK copyright notice
	#define AK_WWISESDK_COPYRIGHT 				"© Audiokinetic Inc. 2006-2008. All rights reserved."
#else
	/// Wwise SDK copyright notice
	#define AK_WWISESDK_COPYRIGHT 				"(C) Audiokinetic Inc. 2006-2008. All rights reserved."
#endif

//@}

#endif // _AKWWISESDKVERSION_H_


