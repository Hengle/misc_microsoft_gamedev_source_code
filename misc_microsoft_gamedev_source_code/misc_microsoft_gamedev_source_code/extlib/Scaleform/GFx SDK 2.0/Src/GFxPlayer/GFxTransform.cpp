/**********************************************************************

Filename    :   GFxTransform.cpp
Content     :   flash.geom.Transform reference class for ActionScript 2.0
Created     :   6/22/2006
Authors     :   Artyom Bolgar, Prasad Silva
Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxTransform.h"
#include "GFxFunction.h"
#include "GFxMatrix.h"
#include "GFxRectangle.h"
#include "GFxColorTransform.h"

#include <stdio.h>
#include <stdlib.h>

// ****************************************************************************
// GASTransform  constructor
//
// Why are we storing the movie root and target handle? Flash resolves 
// characters using it's handle (name). The behavior for the following code in
// ActionScript..
// Frame 1 => (Create an instance mc1)
//            var t1:Transform = new Transform(mc1);
//            var m1:Matrix = new Matrix();
//            m2 = new Matrix(0.5, 0, 0, 1.4, 0, 0);
//            m1.concat(m2);
//            t1.matrix = m1;
// Frame 2 => (Delete mc1)
// Frame 3 => (Create another instance mc1)
//            m1 = t1.matrix;
//            m2 = new Matrix(0.5, 0, 0, 1.4, 0, 0);
//            m1.concat(m2);
//            t1.matrix = m1;
//
// ..is that the second mc1 instance will have the transform t1 applied to it.
//
GASTransformObject::GASTransformObject(GASEnvironment* penv, GFxASCharacter *pcharacter)
{
    TargetHandle    = pcharacter->GetCharacterHandle();
    MovieRoot       = pcharacter->GetMovieRoot();
    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Transform));

    Matrix = *new GASMatrixObject(penv);
    ColorTransform = *new GASColorTransformObject(penv);
    PixelBounds = *new GASRectangleObject(penv);
}

// ****************************************************************************
// Intercept the get command so that dependent properties can be recalculated
//
bool GASTransformObject::GetMember(GASEnvironment *penv, const GASString& name, GASValue* val)
{
    if (name == "pixelBounds")
    {
        bool rv = true;
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                GRectF bounds = ch->GetBounds(ch->GetMatrix());
                // Flash has 0.05 granularity and will cause the values below to
                // be off.
                GASRect r(gfrnd(TwipsToPixels(bounds.Left)), gfrnd(TwipsToPixels(bounds.Top)), 
                    GSize<Double>(gfrnd(TwipsToPixels(bounds.Width())), gfrnd(TwipsToPixels(bounds.Height()))));
                PixelBounds->SetProperties(penv, r);
                val->SetAsObject(PixelBounds);
            }
            else 
                rv = false;
        }
        else 
            rv = false;
        if (!rv) val->SetUndefined();
        return rv;
    }
    else if (name == "colorTransform")
    {
        bool rv = true;
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                GASColorTransform ct = ch->GetCxform();
                ColorTransform->SetColorTransform(ct);
                val->SetAsObject(ColorTransform);
            }
            else 
                rv = false;
        }
        else 
            rv = false;
        if (!rv) val->SetUndefined();
        return rv;
    }
    else if (name == "matrix")
    {
        bool rv = true;
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                GMatrix2D m = ch->GetMatrix();
                Matrix->SetMatrixTwips(penv->GetSC(), m);
                val->SetAsObject(Matrix);
            }
            else 
                rv = false;
        }
        else 
            rv = false;
        if (!rv) val->SetUndefined();
        return rv;
    }
    else if (name == "concatenatedColorTransform")
    {
        // concatenatedColorTransform should be computed on the fly:
        // "A ColorTransform object representing the combined color transformations 
        // applied to this object and all of its parent objects, back to the root 
        // level. If different color transformations have been applied at different 
        // levels, each of those transformations will be concatenated into one ColorTransform 
        // object for this property."        
        GASColorTransform ct;
        // get the ASCharacter for this transform
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                // run through the scene hierarchy and cat the color transforms
                GFxASCharacter* curr = ch.GetPtr();
                while (curr != NULL)
                {
                    ct.Concatenate(curr->GetCxform());
                    curr = curr->GetParent();
                }
            }
        }
        // return a GASObject
        GPtr<GASColorTransformObject> pct = *new GASColorTransformObject(penv);
        pct->SetColorTransform(ct);
        *val = GASValue(pct.GetPtr());
        return true;
    }
    else if (name == "concatenatedMatrix")
    {
        // concatenatedMatrix should be computed on the fly:
        // "A Matrix object representing the combined transformation matrices of this 
        // object and all of its parent objects, back to the root level. If different 
        // transformation matrices have been applied at different levels, each of those 
        // matrices will be concatenated into one matrix for this property."
        GMatrix2D mt;
        // get the ASCharacter for this transform
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                // run through the scene hierarchy and cat the color transforms
                GFxASCharacter* curr = ch.GetPtr();
                while (curr != NULL)
                {
                    mt.Prepend(curr->GetMatrix());  // or Append?
                    curr = curr->GetParent();
                }
            }
        }
        // return a GASObject
        GPtr<GASMatrixObject> pmt = *new GASMatrixObject(penv);
        pmt->SetMatrixTwips(penv->GetSC(), mt);
        *val = GASValue(pmt.GetPtr());
        return true;
    }
    return GASObject::GetMember(penv, name, val);
}

// ****************************************************************************
// Intercept the set command so that dependent properties can be recalculated
//
bool    GASTransformObject::SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    if (name == "pixelBounds")
    {
        // Does nothing
        return true;
    }
    else if (name == "colorTransform")
    {
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                // get GASColorTransform from val
                GPtr<GASObject> obj = val.ToObject();
                if (obj->GetObjectType() == GASObject::Object_ColorTransform)
                {
                    GASColorTransformObject* ctobj = (GASColorTransformObject*)obj.GetPtr();
                    if (ctobj)
                    {
                        GASColorTransform* ct = ctobj->GetColorTransform();
                        ch->SetCxform(*ct);
                        ch->SetAcceptAnimMoves(false);
                    }
                }
            }
        }
        return true;
    }
    else if (name == "matrix")
    {
        GPtr<GFxMovieRoot> movieRoot = MovieRoot;
        if (movieRoot)
        {
            GPtr<GFxASCharacter> ch = TargetHandle->ResolveCharacter(movieRoot);
            if (ch)
            {
                // get GMatrix2D from val
                GPtr<GASObject> obj = val.ToObject();
                if (obj->GetObjectType() == GASObject::Object_Matrix)
                {
                    GASMatrixObject* mobj = (GASMatrixObject*)obj.GetPtr();
                    if (mobj)
                    {
                        GMatrix2D m = mobj->GetMatrix(penv);
                        m.M_[0][2] = PixelsToTwips(m.M_[0][2]);
                        m.M_[1][2] = PixelsToTwips(m.M_[1][2]);
                        ch->SetMatrix(m);
                    }
                }
            }
        }
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

// ****************************************************************************
// AS to GFx function mapping
//
const GASNameFunction GASTransformProto::FunctionTable[] = 
{
    // no functions
    { 0, 0 }
};

// ****************************************************************************
// GASTransform Prototype constructor
//
GASTransformProto::GASTransformProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASTransformObject>(psc, pprototype, constructor)
{
    // we make the functions enumerable
    for(int i = 0; FunctionTable[i].Name; i++)
    {
        GASTransformObject::SetMemberRaw(psc, psc->CreateConstString(FunctionTable[i].Name), 
            GASFunctionRef (*new GASFunctionObject(psc, pprototype, FunctionTable[i].Function)),
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);
    }
    // add the on-the-fly calculated properties
    GASValue undef;
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("matrix"), undef, GASPropFlags::PropFlag_DontDelete);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("concatenatedMatrix"), undef, GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("colorTransform"), undef, GASPropFlags::PropFlag_DontDelete);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("concatenatedColorTransform"), undef, GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_ReadOnly);
    GASTransformObject::SetMemberRaw(psc, psc->CreateConstString("pixelBounds"), undef, GASPropFlags::PropFlag_DontDelete);
    //InitFunctionMembers(psc, FunctionTable, pprototype);
}

// ****************************************************************************
// Called when the constructor is invoked for the Transform class
//
void GASTransformProto::GlobalCtor(const GASFnCall& fn)
{
    GFxASCharacter* ptarget = NULL;
    if (fn.NArgs >= 1)
    {
        ptarget = fn.Env->FindTargetByValue(fn.Arg(0));
        if (NULL == ptarget)
        {
            fn.Result->SetUndefined();
        }
        else
        {
            GPtr<GASTransformObject> ptransform = *new GASTransformObject(fn.Env, ptarget);
            fn.Result->SetAsObject(ptransform.GetPtr());
        }
    }
}

