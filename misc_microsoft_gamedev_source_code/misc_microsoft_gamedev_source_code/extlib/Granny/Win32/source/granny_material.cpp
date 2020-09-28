// ========================================================================
// $File: //jeffr/granny/rt/granny_material.cpp $
// $DateTime: 2007/11/10 13:28:10 $
// $Change: 16510 $
// $Revision: #15 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_MATERIAL_H)
#include "granny_material.h"
#endif

#if !defined(GRANNY_TEXTURE_H)
#include "granny_texture.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

data_type_definition MaterialMapType[] =
{
    {StringMember, "Usage"},
    {ReferenceMember, "Map", MaterialType},         // Yes, this name should be "Material", but that would break existing files.

    {EndMember},
};

data_type_definition MaterialType[] =
{
    {StringMember, "Name"},

    {ReferenceToArrayMember, "Maps", MaterialMapType},

    {ReferenceMember, "Texture", TextureType},

    {VariantReferenceMember, "ExtendedData"},

    {EndMember},
};

struct usage_code
{
    material_texture_type Type;
    char const *Name;
};

usage_code const UsageCodes[] =
{
    // Maya texture codes
    {DiffuseColorTexture, "color"},
    {SpecularColorTexture, "specularColor"},
    {BumpHeightTexture, "normalCamera"},
    {SelfIlluminationTexture, "incandescence"},

    // MAX texture codes
    {AmbientColorTexture, "Ambient Color"},
    {DiffuseColorTexture, "Diffuse Color"},

    // The DirectX material reports this as the usage code for the "display material".
    // Try that to find the diffuse channel
    {DiffuseColorTexture, "RenderMaterial"},
    // Max has both specular colour and specular level, which are theoretically multiplied together,
    // but actually you usually only have one of the two present.
    {SpecularColorTexture, "Specular Color"},
    {SpecularColorTexture, "Specular Level"},
    {SelfIlluminationTexture, "Self-Illumination"},
    {OpacityTexture, "Opacity"},
    {BumpHeightTexture, "Bump"},
    {ReflectionTexture, "Reflection"},
    {RefractionTexture, "Refraction"},
    {DisplacementTexture, "Displacement"},

    // LightWave texture codes
    {AmbientColorTexture, "BaseColor"},
    {DiffuseColorTexture, "BaseColor"},
    {SpecularColorTexture, "Specularity"},
    {SelfIlluminationTexture, "Luminosity"},
    {OpacityTexture, "Transparency"},
    {BumpHeightTexture, "Bump"},
    {ReflectionTexture, "ReflectionImage"},
    {RefractionTexture, "RefractionImage"},

    // XSI texture codes
    {AmbientColorTexture, "ambience"},
    {AmbientColorTexture, "ambient"},
    {DiffuseColorTexture, "diffuse"},
    {SpecularColorTexture, "specular"},
    {SelfIlluminationTexture, "incandescence"},
    {SelfIlluminationTexture, "radiance"},
    {OpacityTexture, "transparency"},
    {BumpHeightTexture, "Bump"},
    {ReflectionTexture, "reflectivity"},
    {RefractionTexture, "index_of_refraction"},

    // Custom codes.
    {BumpHeightTexture, "normspec"},   // Hero
};
int32x const UsageCodeCount = SizeOf(UsageCodes)/SizeOf(UsageCodes[0]);

struct material_recursion_chain
{
    material*                 Material;
    material_recursion_chain* Previous;
};

END_GRANNY_NAMESPACE;

static bool
TextureTypeMatches(material_map &MaterialMap, material_texture_type Type)
{
    {for(int32x UsageCodeIndex = 0;
         UsageCodeIndex < UsageCodeCount;
         ++UsageCodeIndex)
    {
        usage_code const &UsageCode = UsageCodes[UsageCodeIndex];
        if(StringsAreEqualLowercase(UsageCode.Name, MaterialMap.Usage) &&
           (UsageCode.Type == Type))
        {
            return(true);
        }
    }}

    return(false);
}

static material *
FindTexturedMaterial(material *Material,
                     material_recursion_chain* RecursionChain)
{
    material *Result = 0;

    if(Material)
    {
        if(Material->Texture)
        {
            Result = Material;
        }
        else
        {
            {for(int32x MapIndex = 0;
                 (MapIndex < Material->MapCount) && (!Result);
                 ++MapIndex)
            {
                material* Descend = Material->Maps[MapIndex].Material;
                if (Descend == Material)
                    continue;

                // Make sure we're not about to enter a loop...
                material_recursion_chain* Walk = RecursionChain;
                while (Walk)
                {
                    if (Walk->Material == Descend)
                        break;
					Walk = Walk->Previous;
                }

                if (!Walk)
                {
                    material_recursion_chain NewElement = { Descend, RecursionChain };
                    Result = FindTexturedMaterial(Descend, &NewElement);
                }
            }}
        }
    }

    return(Result);
}

static texture *
FindTexture(material *Material)
{
    texture *Texture = 0;

    material *SubMaterial = FindTexturedMaterial(Material, NULL);
    if(SubMaterial)
    {
        Texture = SubMaterial->Texture;
    }

    return(Texture);
}

texture *GRANNY
GetMaterialTextureByType(material const *Material, material_texture_type Type)
{
    if(Material)
    {
        {for(int32x MapIndex = 0;
             MapIndex < Material->MapCount;
             ++MapIndex)
        {
            texture *SourceTexture = FindTexture(
                Material->Maps[MapIndex].Material);
            if(SourceTexture)
            {
                if(TextureTypeMatches(Material->Maps[MapIndex], Type))
                {
                    return(SourceTexture);
                }
            }
        }}
    }

    return(0);
}

material *GRANNY
GetTexturedMaterialByChannelName(material const *Material,
                                 char const *ChannelName)
{
    if(Material)
    {
        {for(int32x MapIndex = 0;
             MapIndex < Material->MapCount;
             ++MapIndex)
        {
            material *SourceMaterial = FindTexturedMaterial(
                Material->Maps[MapIndex].Material, NULL);
            if(SourceMaterial)
            {
                if(StringsAreEqualLowercase(
                       Material->Maps[MapIndex].Usage, ChannelName))
                {
                    return(SourceMaterial);
                }
            }
        }}
    }

    return(0);
}

texture *GRANNY
GetMaterialTextureByChannelName(material const *Material,
                                char const *ChannelName)
{
    material *Find = GetTexturedMaterialByChannelName(Material, ChannelName);
    if(Find)
    {
        return(Find->Texture);
    }

    return(0);
}
