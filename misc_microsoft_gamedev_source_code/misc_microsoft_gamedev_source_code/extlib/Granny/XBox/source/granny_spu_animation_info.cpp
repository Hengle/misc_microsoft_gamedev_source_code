// ========================================================================
// $File: //jeffr/granny/rt/granny_spu_animation_info.cpp $
// $DateTime: 2007/08/15 15:47:14 $
// $Change: 15741 $
// $Revision: #3 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================
#if !defined(GRANNY_SPU_ANIMATION_INFO_H)
#include "granny_spu_animation_info.h"
#endif

#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "granny_data_type_conversion.h"
#endif

#if !defined(GRANNY_FILE_H)
#include "granny_file.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(PS3_GRANNY_ANIMATION_H)
#include "granny_spu_animation.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition SPUAnimationInfoType[] =
{
    {ArrayOfReferencesMember, "SPUAnimations", SPUAnimationType},
    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

spu_animation_info* GRANNY
GetSPUAnimationInfo(file &File)
{
    variant Root;
    GetDataTreeFromFile(File, &Root);

    uint32 TypeTag = File.Header->TypeTag;
    if(TypeTag == CurrentGRNStandardTag)
    {
        return((spu_animation_info *)Root.Object);
    }
    else
    {
        if (!File.ConversionBuffer)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "File has run-time type tag of 0x%x, which doesn't match this "
                 "version of Granny (0x%x).  Automatic conversion will "
                 "be attempted.", TypeTag, CurrentGRNStandardTag);

            File.ConversionBuffer =
                ConvertTree(Root.Type, Root.Object, SPUAnimationInfoType);
        }

        return((spu_animation_info *)File.ConversionBuffer);
    }
}

