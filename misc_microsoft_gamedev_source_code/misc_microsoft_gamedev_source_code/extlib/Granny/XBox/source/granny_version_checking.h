#if !defined(GRANNY_VERSION_CHECKING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_version_checking.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

#if !defined(GRANNY_VERSION_H)
#include "granny_version.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(VersionGroup);

EXPAPI GS_SAFE char const *GetVersionString(void);
EXPAPI GS_SAFE void GetVersion(int32x &MajorVersion, int32x &MinorVersion,
                               int32x &Customization, int32x &BuildNumber);
EXPAPI GS_SAFE bool VersionsMatch_(int32x MajorVersion, int32x MinorVersion,
                                   int32x Customization, int32x BuildNumber);

#define VersionsMatch GrannyVersionsMatch_(GrannyProductMajorVersion, GrannyProductMinorVersion, GrannyProductCustomization, GrannyProductBuildNumber) EXPMACRO

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_VERSION_CHECKING_H
#endif
