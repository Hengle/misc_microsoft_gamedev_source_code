// ========================================================================
// $File: //jeffr/granny/rt/granny_file_info.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #6 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_FILE_INFO_H)
#include "granny_file_info.h"
#endif

#if !defined(GRANNY_DATA_TYPE_IO_H)
#include "granny_data_type_io.h"
#endif

#if !defined(GRANNY_DATA_TYPE_CONVERSION_H)
#include "granny_data_type_conversion.h"
#endif

#if !defined(GRANNY_FILE_FORMAT_H)
#include "granny_file_format.h"
#endif

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_FILE_H)
#include "granny_file.h"
#endif

/* ----
   TODO: It would be nice to do this, but then compiles can break because
         these will be undefined types.
extern data_type_definition StringTableType[];
extern data_type_definition TextureType[];
extern data_type_definition MaterialType[];
extern data_type_definition SkeletonType[];
extern data_type_definition VertexDataType[];
extern data_type_definition TriTopologyType[];
extern data_type_definition MeshType[];
extern data_type_definition ModelType[];
extern data_type_definition TrackGroupType[];
extern data_type_definition AnimationType[];
extern data_type_definition ArtToolInfoType[];
extern data_type_definition ExporterInfoType[];
   ---- */
#if !defined(GRANNY_STRING_TABLE_H)
#include "granny_string_table.h"
#endif

#if !defined(GRANNY_TEXTURE_H)
#include "granny_texture.h"
#endif

#if !defined(GRANNY_MATERIAL_H)
#include "granny_material.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_ART_TOOL_INFO_H)
#include "granny_art_tool_info.h"
#endif

#if !defined(GRANNY_EXPORTER_INFO_H)
#include "granny_exporter_info.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif


USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition FileInfoType[] =
{
    {ReferenceMember, "ArtToolInfo", ArtToolInfoType},
    {ReferenceMember, "ExporterInfo", ExporterInfoType},

    {StringMember, "FromFileName"},

    {ArrayOfReferencesMember, "Textures", TextureType},
    {ArrayOfReferencesMember, "Materials", MaterialType},

    {ArrayOfReferencesMember, "Skeletons", SkeletonType},
    {ArrayOfReferencesMember, "VertexDatas", VertexDataType},
    {ArrayOfReferencesMember, "TriTopologies", TriTopologyType},
    {ArrayOfReferencesMember, "Meshes", MeshType},
    {ArrayOfReferencesMember, "Models", ModelType},

    {ArrayOfReferencesMember, "TrackGroups", TrackGroupType},
    {ArrayOfReferencesMember, "Animations", AnimationType},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

END_GRANNY_NAMESPACE;

file_info *GRANNY
GetFileInfo(file &File)
{
    variant Root;
    GetDataTreeFromFile(File, &Root);

    uint32 TypeTag = File.Header->TypeTag;
    if(TypeTag == CurrentGRNStandardTag)
    {
        return((file_info *)Root.Object);
    }
    else
    {
        if(!File.ConversionBuffer)
        {
            Log2(WarningLogMessage, FileReadingLogMessage,
                 "File has run-time type tag of 0x%x, which doesn't match this "
                 "version of Granny (0x%x).  Automatic conversion will "
                 "be attempted.", TypeTag, CurrentGRNStandardTag);

            File.ConversionBuffer =
                ConvertTree(Root.Type, Root.Object, FileInfoType);
        }

        return((file_info *)File.ConversionBuffer);
    }
}

