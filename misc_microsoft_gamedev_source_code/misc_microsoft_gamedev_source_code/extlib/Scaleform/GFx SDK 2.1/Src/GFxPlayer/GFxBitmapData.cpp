/**********************************************************************

Filename    :   GFxBitmapData.cpp
Content     :   Implementation of BitmapData class
Created     :   March, 2007
Authors     :   Artyom Bolgar

Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxASString.h"
#include "GFxAction.h"
#include "GFxPlayerImpl.h"
#include "GFxBitmapData.h"
#include "GFxRectangle.h"
#include "GUTF8Util.h"

GASBitmapData::GASBitmapData(GASEnvironment* penv)
{
    commonInit(penv);
}

void GASBitmapData::commonInit (GASEnvironment* penv)
{    
    Set__proto__ (penv->GetSC(), penv->GetPrototype(GASBuiltin_BitmapData));
}

template <typename StringType>
GASBitmapData* GFx_LoadBitmap(GASEnvironment* penv, const StringType& linkageId)
{
    GASBitmapData* pobj = NULL;
    GPtr<GFxMovieDefImpl> md = penv->GetTarget()->GetResourceMovieDef();
    if (md) // can it be NULL?
    {
        const char* plinkageIdStr = linkageId.ToCStr();
        GASSERT(plinkageIdStr);
        bool    userImageProtocol = 0;

        // Check to see if URL is a user image substitute.
        if (plinkageIdStr[0] == 'i' || plinkageIdStr[0] == 'I')
        {
            StringType urlLowerCase = linkageId.ToLower();

            if (urlLowerCase.Substring(0, 6) == "img://")
                userImageProtocol = true;
            else if (urlLowerCase.Substring(0, 8) == "imgps://")
                userImageProtocol = true;
        }

        if (userImageProtocol)
        {
            // Create image through image callback
            GFxSharedState* pss = penv->GetMovieRoot()->pSharedState;
            GPtr<GFxImageResource> pimageRes = 
                *GFxLoaderImpl::LoadMovieImage(plinkageIdStr, pss->GetImageLoader(), pss->GetLog());

            if (pimageRes)
            {
                pobj = new GASBitmapData(penv);
                pobj->SetImage(penv, pimageRes);
            }
            else
            {
                penv->LogScriptWarning(
                    "GASBitmapData::LoadBitmap: LoadMovieImageCallback failed to load image \"%s\"\n", plinkageIdStr);
            }
        }
        else
        {
            // Get exported resource for linkageId and verify that it is an image.
            GFxResourceBindData resBindData;
            if (!md->GetExportedResource(&resBindData, linkageId.ToCStr()))
                return NULL;
            GASSERT(resBindData.pResource.GetPtr() != 0);
            // Must check resource type, since users can theoretically pass other resource ids.
            if (resBindData.pResource->GetResourceType() != GFxResource::RT_Image)
                return NULL;

            GFxImageResource* pbmpRes = (GFxImageResource*)resBindData.pResource.GetPtr();
            if (pbmpRes)
            {
                pobj = new GASBitmapData(penv);
                pobj->SetImage(penv, pbmpRes);
            }
        }
    }
    return pobj;
}

GASBitmapData* GASBitmapData::LoadBitmap(GASEnvironment* penv, const GFxString& linkageId)
{
    return GFx_LoadBitmap<>(penv, linkageId);
}

void GASBitmapData::SetImage(GASEnvironment *penv, GFxImageResource* pimg) 
{ 
    pImageRes = pimg; 

    GASValue params[4];
    params[0] = GASValue(0);
    params[1] = GASValue(0);
    params[2] = GASValue((Float)pimg->GetWidth());
    params[3] = GASValue((Float)pimg->GetHeight());

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    GPtr<GASRectangleObject> bounds = *new GASRectangleObject(penv);
    bounds->SetProperties(penv->GetSC(), params);
#else
    GPtr<GASObject> bounds = *new GASObject(penv);
    GASStringContext *psc = penv->GetSC();
    bounds->SetConstMemberRaw(psc, "x", params[0]);
    bounds->SetConstMemberRaw(psc, "y", params[1]);
    bounds->SetConstMemberRaw(psc, "width", params[2]);
    bounds->SetConstMemberRaw(psc, "height", params[3]);
#endif 
    SetMember(penv, penv->CreateConstString("rectangle"), GASValue(bounds), GASPropFlags::PropFlag_ReadOnly);
}

//////////////////////////////////////////
//
static void GFx_BitmapDataFuncStub(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
}

static const GASNameFunction GFx_BitmapDataFunctionTable[] = 
{
    { "loadBitmap",        &GFx_BitmapDataFuncStub },
    { 0, 0 }
};

GASBitmapDataProto::GASBitmapDataProto(GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASBitmapData>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GFx_BitmapDataFunctionTable, pprototype);
}

//////////////////
const GASNameFunction GASBitmapDataCtorFunction::StaticFunctionTable[] = 
{
    { "loadBitmap",         &GASBitmapDataCtorFunction::LoadBitmap    },
    { 0, 0 }
};

GASBitmapDataCtorFunction::GASBitmapDataCtorFunction(GASStringContext* psc) :
    GASFunctionObject(GASBitmapDataCtorFunction::GlobalCtor)
{
    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue (StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASBitmapDataCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASBitmapData> ab = *new GASBitmapData(fn.Env);
    fn.Result->SetAsObject(ab.GetPtr());
}

void GASBitmapDataCtorFunction::LoadBitmap(const GASFnCall& fn)
{
    fn.Result->SetNull();
    if (fn.NArgs < 1)
        return;

    GASSERT(fn.ThisPtr);

    GASString linkageId(fn.Arg(0).ToString(fn.Env));
    GPtr<GASBitmapData> pobj = *GFx_LoadBitmap<>(fn.Env, linkageId);
    if (pobj)
        fn.Result->SetAsObject(pobj.GetPtr());
}

