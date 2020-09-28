// ========================================================================
// $File: //jeffr/granny/rt/granny_version_checking.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_VERSION_CHECKING_H)
#include "granny_version_checking.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

char const *GRANNY
GetVersionString(void)
{
    return(ProductVersion);
}

void GRANNY
GetVersion(int32x &MajorVersion, int32x &MinorVersion,
           int32x &Customization, int32x &BuildNumber)
{
    MajorVersion = ProductMajorVersion;
    MinorVersion = ProductMinorVersion;
    Customization = ProductCustomization;
    BuildNumber = ProductBuildNumber;
}

bool GRANNY
VersionsMatch_(int32x MajorVersion, int32x MinorVersion,
               int32x Customization, int32x BuildNumber)
{
    if((MajorVersion == ProductMajorVersion) &&
       (MinorVersion == ProductMinorVersion) &&
       (Customization == ProductCustomization) &&
       (BuildNumber == ProductBuildNumber))
    {
        return(true);
    }
    else
    {
        Log8(WarningLogMessage, VersionLogMessage,
             "Version of DLL (%d.%d.%d.%d) does not match "
             "version of .h (%d.%d.%d.%d)",
             ProductMajorVersion, ProductMinorVersion,
             ProductCustomization, ProductBuildNumber,
             MajorVersion, MinorVersion,
             Customization, BuildNumber);
        return(false);
    }
}
