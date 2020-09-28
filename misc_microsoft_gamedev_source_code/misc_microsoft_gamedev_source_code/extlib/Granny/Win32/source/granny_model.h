#if !defined(GRANNY_MODEL_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_model.h $
// $DateTime: 2006/10/19 16:35:09 $
// $Change: 13632 $
// $Revision: #7 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_TRANSFORM_H)
#include "granny_transform.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(ModelGroup);

struct mesh;
struct skeleton;

extern data_type_definition SkeletonType[];

EXPTYPE struct model_mesh_binding
{
    mesh *Mesh;
};
EXPCONST EXPGROUP(model_mesh_binding) extern data_type_definition ModelMeshBindingType[];

EXPTYPE struct model
{
    char const *Name;
    skeleton *Skeleton;

    transform InitialPlacement;

    int32 MeshBindingCount;
    model_mesh_binding *MeshBindings;
};
EXPCONST EXPGROUP(model) extern data_type_definition ModelType[];

EXPAPI GS_PARAM void GetModelInitialPlacement4x4(model const &Model, real32 *Placement4x4);

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_MODEL_H
#endif
