#if !defined(GRANNY_VERTEX_DATA_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_vertex_data.h $
// $DateTime: 2007/05/17 21:25:15 $
// $Change: 14972 $
// $Revision: #14 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_DATA_TYPE_DEFINITION_H)
#include "granny_data_type_definition.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif


BEGIN_GRANNY_NAMESPACE EXPGROUP(VertexDataGroup);

extern data_type_definition VertexLayoutType[];

EXPTYPE struct vertex_annotation_set
{
    // The name of this annotation set
    char const *Name;

    // Annotation per vertex
    data_type_definition *VertexAnnotationType;
    int32 VertexAnnotationCount;
    uint8 *VertexAnnotations;

    // Indexing (if this is 0, then it's direct-mapped)
    // Some indices may be -1, which means they do not have an entry in VertexAnnotations
    // If IndicesMapFromVertexToAnnotation is false, then
    //   VertexAnnotationIndexCount==VertexAnnotationCount,
    //   and for each member in VertexAnnotations, there is an entry
    //   in VertexAnnotationIndices that says which mesh vertex it is for.
    // If IndicesMapFromVertexToAnnotation is true, then
    //   VertexAnnotationIndexCount==VertexCount,
    //   and for each vertex there is an entry in VertexAnnotationIndices
    //   that says which entry in VertexAnnotations is applied to it,
    //   or if the index is -1, there is no annotation of this type.
    int32 IndicesMapFromVertexToAnnotation;
    int32 VertexAnnotationIndexCount;
    int32 *VertexAnnotationIndices;
};
EXPCONST EXPGROUP(vertex_annotation_set) extern data_type_definition
    VertexAnnotationSetType[];

EXPTYPE struct vertex_data
{
    // The vertex data
    data_type_definition *VertexType;
    int32 VertexCount;
    uint8 *Vertices;

    int32 VertexComponentNameCount;
    char const **VertexComponentNames;

    // Annotation (i.e. not primary rendering values)
    int32 VertexAnnotationSetCount;
    vertex_annotation_set *VertexAnnotationSets;
};

EXPCONST EXPGROUP(vertex_data) extern data_type_definition VertexDataType[];

EXPAPI GS_PARAM void ConvertVertexLayouts(int32x VertexCount,
                                          data_type_definition const *SourceLayout,
                                          void const *SourceVertices,
                                          data_type_definition const *DestLayout,
                                          void *DestVertices);
EXPAPI GS_PARAM void EnsureExactOneNorm(data_type_definition const &WeightsType,
                                        void *WeightsInit);
EXPAPI GS_PARAM void OneNormalizeWeights(int32x VertexCount,
                                         data_type_definition const *Layout,
                                         void *Vertices);
EXPAPI GS_PARAM void TransformVertices(int32x VertexCount,
                                       data_type_definition const *Layout,
                                       void *Vertices,
                                       real32 const *Affine3,
                                       real32 const *Linear3x3,
                                       real32 const *InverseLinear3x3,
                                       bool Renormalize,
                                       bool TreatAsDeltas);
EXPAPI GS_PARAM void NormalizeVertices(int32x VertexCount,
                                       data_type_definition const *LayoutType,
                                       void *Vertices);

#define VertexPositionName "Position" EXPMACRO
#define VertexNormalName "Normal" EXPMACRO
#define VertexTangentName "Tangent" EXPMACRO
#define VertexBinormalName "Binormal" EXPMACRO
#define VertexTangentBinormalCrossName "TangentBinormalCross" EXPMACRO
#define VertexBoneWeightsName "BoneWeights" EXPMACRO
#define VertexBoneIndicesName "BoneIndices" EXPMACRO
#define VertexDiffuseColorName "DiffuseColor" EXPMACRO
#define VertexSpecularColorName "SpecularColor" EXPMACRO
#define VertexTextureCoordinatesName "TextureCoordinates" EXPMACRO

#define VertexMorphCurvePrefix "VertexMorphCurve" EXPMACRO


EXPAPI GS_PARAM int32x GetVertexTextureCoordinatesName(int32x Index, char *Buffer);
EXPAPI GS_PARAM int32x GetVertexDiffuseColorName(int32x Index, char *Buffer);
EXPAPI GS_PARAM int32x GetVertexSpecularColorName(int32x Index, char *Buffer);

EXPAPI GS_SAFE bool IsSpatialVertexMember(char const *Name);
EXPAPI GS_READ int32x GetVertexBoneCount(data_type_definition const *VertexType);
EXPAPI GS_READ int32x GetVertexChannelCount(data_type_definition const *VertexType);

void GetSingleVertex(data_type_definition const *SourceType,
                     void const *SourceVertices,
                     int32x VertexIndex, data_type_definition const *As,
                     void *Dest);
EXPAPI GS_PARAM void GetSingleVertex(vertex_data const &VertexData,
                                     int32x VertexIndex,
                                     data_type_definition const *As,
                                     void *Dest);


EXPAPI GS_PARAM void SetVertexPosition(data_type_definition const *VertexLayout,
                                       void *VertexPointer, real32 const *Position);
EXPAPI GS_PARAM void SetVertexNormal(data_type_definition const *VertexLayout,
                                     void *VertexPointer, real32 const *Normal);
EXPAPI GS_PARAM void SetVertexColor(data_type_definition const  *VertexLayout,
                                    void *VertexPointer, int32x ColorIndex,
                                    real32 const *Color);
EXPAPI GS_PARAM void SetVertexUVW(data_type_definition const *VertexLayout,
                                  void *VertexPointer, int32x UVWIndex,
                                  real32 const *UVW);

EXPAPI GS_PARAM int32x GetVertexComponentCount(data_type_definition const *VertexLayout);
EXPAPI GS_PARAM int32x GetVertexComponentIndex(char const *ComponentName, data_type_definition const *VertexLayout);
EXPAPI GS_PARAM char const * GetVertexComponentToolName(char const *ComponentName, vertex_data const *VertexData);

//
// Pre-defined vertex formats
//

EXPTYPE struct p3_vertex
{
    real32 Position[3];
};
EXPCONST EXPGROUP(p3_vertex) extern data_type_definition P3VertexType[];

EXPTYPE struct pt32_vertex
{
    real32 Position[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pt32_vertex) extern data_type_definition PT32VertexType[];

EXPTYPE struct pn33_vertex
{
    real32 Position[3];
    real32 Normal[3];
};
EXPCONST EXPGROUP(pn33_vertex) extern data_type_definition PN33VertexType[];

EXPTYPE struct png333_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 Tangent[3];
};
EXPCONST EXPGROUP(png333_vertex) extern data_type_definition PNG333VertexType[];

EXPTYPE struct pngt3332_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 Tangent[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pngt3332_vertex) extern data_type_definition PNGT3332VertexType[];

EXPTYPE struct pntg3323_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 UV[2];
    real32 Tangent[3];
};
EXPCONST EXPGROUP(pntg3323_vertex) extern data_type_definition PNTG3323VertexType[];

EXPTYPE struct pngb3333_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
};
EXPCONST EXPGROUP(pngb3333_vertex) extern data_type_definition PNGB3333VertexType[];

// TODO: Determine what the proper ordering is for this type before
// exporting it
struct pngbx33333_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 TangentBinormalCross[3];
};
extern data_type_definition PNGBX33333VertexType[];

EXPTYPE struct pnt332_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pnt332_vertex) extern data_type_definition PNT332VertexType[];

EXPTYPE struct pngbt33332_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pngbt33332_vertex) extern data_type_definition PNGBT33332VertexType[];

EXPTYPE struct pnt333_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 UVW[3];
};
EXPCONST EXPGROUP(pnt333_vertex) extern data_type_definition PNT333VertexType[];

EXPTYPE struct pngbt33333_vertex
{
    real32 Position[3];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 UVW[3];
};
EXPCONST EXPGROUP(pngbt33333_vertex) extern data_type_definition PNGBT33333VertexType[];


EXPTYPE struct pwn313_vertex
{
    real32 Position[3];
    uint32 BoneIndex;
    real32 Normal[3];
};
EXPCONST EXPGROUP(pwn313_vertex) extern data_type_definition PWN313VertexType[];

EXPTYPE struct pwng3133_vertex
{
    real32 Position[3];
    uint32 BoneIndex;
    real32 Normal[3];
    real32 Tangent[3];
};
EXPCONST EXPGROUP(pwng3133_vertex) extern data_type_definition PWNG3133VertexType[];

EXPTYPE struct pwngt31332_vertex
{
    real32 Position[3];
    uint32 BoneIndex;
    real32 Normal[3];
    real32 Tangent[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwngt31332_vertex) extern data_type_definition PWNGT31332VertexType[];

EXPTYPE struct pwngb31333_vertex
{
    real32 Position[3];
    uint32 BoneIndex;
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
};
EXPCONST EXPGROUP(pwngb31333_vertex) extern data_type_definition PWNGB31333VertexType[];

EXPTYPE struct pwnt3132_vertex
{
    real32 Position[3];
    uint32 BoneIndex;
    real32 Normal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwnt3132_vertex) extern data_type_definition PWNT3132VertexType[];

EXPTYPE struct pwngbt313332_vertex
{
    real32 Position[3];
    uint32 BoneIndex;
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwngbt313332_vertex) extern data_type_definition PWNGBT313332VertexType[];


EXPTYPE struct pwn323_vertex
{
    real32 Position[3];
    uint8 BoneWeights[2];
    uint8 BoneIndices[2];
    real32 Normal[3];
};
EXPCONST EXPGROUP(pwn323_vertex) extern data_type_definition PWN323VertexType[];

EXPTYPE struct pwng3233_vertex
{
    real32 Position[3];
    uint8 BoneWeights[2];
    uint8 BoneIndices[2];
    real32 Normal[3];
    real32 Tangent[3];
};
EXPCONST EXPGROUP(pwng3233_vertex) extern data_type_definition PWNG3233VertexType[];

EXPTYPE struct pwngt32332_vertex
{
    real32 Position[3];
    uint8 BoneWeights[2];
    uint8 BoneIndices[2];
    real32 Normal[3];
    real32 Tangent[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwngt32332_vertex) extern data_type_definition PWNGT32332VertexType[];

EXPTYPE struct pwngb32333_vertex
{
    real32 Position[3];
    uint8 BoneWeights[2];
    uint8 BoneIndices[2];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
};
EXPCONST EXPGROUP(pwngb32333_vertex) extern data_type_definition PWNGB32333VertexType[];

EXPTYPE struct pwnt3232_vertex
{
    real32 Position[3];
    uint8 BoneWeights[2];
    uint8 BoneIndices[2];
    real32 Normal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwnt3232_vertex) extern data_type_definition PWNT3232VertexType[];

EXPTYPE struct pwngbt323332_vertex
{
    real32 Position[3];
    uint8 BoneWeights[2];
    uint8 BoneIndices[2];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwngbt323332_vertex) extern data_type_definition PWNGBT323332VertexType[];


EXPTYPE struct pwn343_vertex
{
    real32 Position[3];
    uint8 BoneWeights[4];
    uint8 BoneIndices[4];
    real32 Normal[3];
};
EXPCONST EXPGROUP(pwn343_vertex) extern data_type_definition PWN343VertexType[];

EXPTYPE struct pwng3433_vertex
{
    real32 Position[3];
    uint8 BoneWeights[4];
    uint8 BoneIndices[4];
    real32 Normal[3];
    real32 Tangent[3];
};
EXPCONST EXPGROUP(pwng3433_vertex) extern data_type_definition PWNG3433VertexType[];

EXPTYPE struct pwngt34332_vertex
{
    real32 Position[3];
    uint8 BoneWeights[4];
    uint8 BoneIndices[4];
    real32 Normal[3];
    real32 Tangent[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwngt34332_vertex) extern data_type_definition PWNGT34332VertexType[];

EXPTYPE struct pwngb34333_vertex
{
    real32 Position[3];
    uint8 BoneWeights[4];
    uint8 BoneIndices[4];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
};
EXPCONST EXPGROUP(pwngb34333_vertex) extern data_type_definition PWNGB34333VertexType[];

EXPTYPE struct pwnt3432_vertex
{
    real32 Position[3];
    uint8 BoneWeights[4];
    uint8 BoneIndices[4];
    real32 Normal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwnt3432_vertex) extern data_type_definition PWNT3432VertexType[];

EXPTYPE struct pwngbt343332_vertex
{
    real32 Position[3];
    uint8 BoneWeights[4];
    uint8 BoneIndices[4];
    real32 Normal[3];
    real32 Tangent[3];
    real32 Binormal[3];
    real32 UV[2];
};
EXPCONST EXPGROUP(pwngbt343332_vertex) extern data_type_definition PWNGBT343332VertexType[];


EXPTYPE struct vertex_weight_arrays
{
    real32 BoneWeights[MaximumWeightCount];
    uint32 BoneIndices[MaximumWeightCount];
};
EXPCONST EXPGROUP(vertex_weight_arrays) extern data_type_definition VertexWeightArraysType[];

END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_VERTEX_DATA_H
#endif
