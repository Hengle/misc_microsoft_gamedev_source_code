// ========================================================================
// $File: //jeffr/granny/rt/granny_model.cpp $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MODEL_H)
#include "granny_model.h"
#endif

#if !defined(GRANNY_MESH_H)
#include "granny_mesh.h"
#endif

#if !defined(GRANNY_SKELETON_H)
#include "granny_skeleton.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition ModelMeshBindingType[] =
{
    {ReferenceMember, "Mesh", MeshType},
    {EndMember},
};

data_type_definition ModelType[] =
{
    {StringMember, "Name"},
    {ReferenceMember, "Skeleton", SkeletonType},

    {TransformMember, "InitialPlacement"},

    {ReferenceToArrayMember, "MeshBindings", ModelMeshBindingType},

    {EndMember},
};

END_GRANNY_NAMESPACE;

void GRANNY
GetModelInitialPlacement4x4(model const &Model,
                            real32 *Placement4x4)
{
    BuildCompositeTransform4x4(Model.InitialPlacement, Placement4x4);
}
