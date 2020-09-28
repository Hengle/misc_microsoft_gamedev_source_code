// ========================================================================
// $File: //jeffr/granny/rt/granny_type_listing.cpp $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #9 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_TYPE_LISTING_H)
#include "granny_type_listing.h"
#endif

#if !defined(GRANNY_ANIMATION_H)
#include "granny_animation.h"
#endif

#if !defined(GRANNY_ART_TOOL_INFO_H)
#include "granny_art_tool_info.h"
#endif

#if !defined(GRANNY_CURVE_H)
#include "granny_curve.h"
#endif

#if !defined(GRANNY_EXPORTER_INFO_H)
#include "granny_exporter_info.h"
#endif

#if !defined(GRANNY_MATERIAL_H)
#include "granny_material.h"
#endif

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_PERIODIC_LOOP_H)
#include "granny_periodic_loop.h"
#endif

#if !defined(GRANNY_PIXEL_LAYOUT_H)
#include "granny_pixel_layout.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_STRING_TABLE_H)
#include "granny_string_table.h"
#endif

#if !defined(GRANNY_TEXTURE_H)
#include "granny_texture.h"
#endif

#if !defined(GRANNY_TRACK_GROUP_H)
#include "granny_track_group.h"
#endif

#if !defined(GRANNY_TRI_TOPOLOGY_H)
#include "granny_tri_topology.h"
#endif

#if !defined(GRANNY_VERTEX_DATA_H)
#include "granny_vertex_data.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

// Must agree with entry in export/granny_export_ui.h
int32x const DefinedTypeIDStart = (1 << 16);

defined_type DefinedTypes[] =
{
    // NOTE: THESE CANNOT BE RENUMBERED!
    //  These are also used in the granny_export_ui code.
    //  Touch not, lest ye mess stuff up.

    {DefinedTypeIDStart +   1, "animation", AnimationType},
    {DefinedTypeIDStart +   2, "art_tool_info", ArtToolInfoType},
    {DefinedTypeIDStart +   3, "curve2", Curve2Type},
    {DefinedTypeIDStart +   4, "exporter_info", ExporterInfoType},
    {DefinedTypeIDStart +   5, "material_map", MaterialMapType},
    {DefinedTypeIDStart +   6, "material", MaterialType},
    {DefinedTypeIDStart +   7, "bone_binding", BoneBindingType},
    {DefinedTypeIDStart +   8, "material_binding", MaterialBindingType},
    {DefinedTypeIDStart +   9, "morph_target", MorphTargetType},
    {DefinedTypeIDStart +  10, "mesh", MeshType},
    {DefinedTypeIDStart +  11, "model_mesh_binding", ModelMeshBindingType},
    {DefinedTypeIDStart +  12, "model", ModelType},
    {DefinedTypeIDStart +  13, "periodic_loop", PeriodicLoopType},
    {DefinedTypeIDStart +  14, "pixel_layout", PixelLayoutType},

//     {DefinedTypeIDStart +  15, "light_info", LightInfoType},       GONE
//     {DefinedTypeIDStart +  16, "camera_info", CameraInfoType},     GONE

    {DefinedTypeIDStart +  17, "bone", BoneType},
    {DefinedTypeIDStart +  18, "skeleton", SkeletonType},
    {DefinedTypeIDStart +  19, "strings", StringType},
    {DefinedTypeIDStart +  20, "texture_mip_level", TextureMIPLevelType},
    {DefinedTypeIDStart +  21, "texture_image", TextureImageType},
    {DefinedTypeIDStart +  22, "texture", TextureType},
    {DefinedTypeIDStart +  23, "vector_track", VectorTrackType},
    {DefinedTypeIDStart +  24, "transform_track", TransformTrackType},
    {DefinedTypeIDStart +  25, "text_track_entry", TextTrackEntryType},
    {DefinedTypeIDStart +  26, "text_track", TextTrackType},
    {DefinedTypeIDStart +  27, "track_group", TrackGroupType},
    {DefinedTypeIDStart +  28, "tri_material_group", TriMaterialGroupType},
    {DefinedTypeIDStart +  29, "tri_annotation_set", TriAnnotationSetType},
    {DefinedTypeIDStart +  30, "tri_topology", TriTopologyType},
    {DefinedTypeIDStart +  31, "vertex_annotation_set", VertexAnnotationSetType},
    {DefinedTypeIDStart +  32, "vertex_data", VertexDataType},
};

END_GRANNY_NAMESPACE;

int32x GRANNY
GetDefinedTypeCount(void)
{
    return(ArrayLength(DefinedTypes));
}
