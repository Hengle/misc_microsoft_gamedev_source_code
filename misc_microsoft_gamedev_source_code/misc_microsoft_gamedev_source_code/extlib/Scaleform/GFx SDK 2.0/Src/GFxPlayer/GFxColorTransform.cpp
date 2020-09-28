/**********************************************************************

Filename    :   GFxColorTransform.cpp
Content     :   flash.geom.Rectangle reference class for ActionScript 2.0
Created     :   3/19/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxAction.h"
#include "GFxNumber.h"
#include "GUTF8Util.h"
#include "GFxColorTransform.h"


// ****************************************************************************
// Helper function to retrieve the point properties from an object
//
void GFxObject_GetColorTransformProperties(GASEnvironment *penv, GASObject *pobj, GASValue params[GFxColorTransform_NumProperties])
{
    GASStringContext* psc = penv->GetSC();
    pobj->GetConstMemberRaw(psc, "redMultiplier", &(params[GFxColorTransform_RedMultiplier]));
    pobj->GetConstMemberRaw(psc, "greenMultiplier", &(params[GFxColorTransform_GreenMultiplier]));
    pobj->GetConstMemberRaw(psc, "blueMultiplier", &(params[GFxColorTransform_BlueMultiplier]));
    pobj->GetConstMemberRaw(psc, "alphaMultiplier", &(params[GFxColorTransform_AlphaMultiplier]));
    pobj->GetConstMemberRaw(psc, "redOffset", &(params[GFxColorTransform_RedOffset]));
    pobj->GetConstMemberRaw(psc, "greenOffset", &(params[GFxColorTransform_GreenOffset]));
    pobj->GetConstMemberRaw(psc, "blueOffset", &(params[GFxColorTransform_BlueOffset]));
    pobj->GetConstMemberRaw(psc, "alphaOffset", &(params[GFxColorTransform_AlphaOffset]));
}

// ****************************************************************************
// GASColorTransform  constructor
//
GASColorTransformObject::GASColorTransformObject(GASEnvironment *penv)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_ColorTransform));
}

/******************************************************************************
* Intercept the get command so that dependent properties can be recalculated
*/
bool GASColorTransformObject::GetMember(GASEnvironment *penv, const GASString &name, GASValue* val)
{
    if (name == "redMultiplier")
    {
        *val = ColorTransform.M_[0][0];
        return true;
    }
    else if (name == "greenMultiplier")
    {
        *val = ColorTransform.M_[1][0];
        return true;
    }
    else if (name == "blueMultiplier")
    {
        *val = ColorTransform.M_[2][0];
        return true;
    }
    else if (name == "alphaMultiplier")
    {
        *val = ColorTransform.M_[3][0];
        return true;
    }
    else if (name == "redOffset")
    {
        *val = ColorTransform.M_[0][1];
        return true;
    }
    else if (name == "greenOffset")
    {
        *val = ColorTransform.M_[1][1];
        return true;
    }
    else if (name == "blueOffset")
    {
        *val = ColorTransform.M_[2][1];
        return true;
    }
    else if (name == "alphaOffset")
    {
        *val = ColorTransform.M_[3][1];
        return true;
    }
    else if (name == "rgb")
    {
        GASValue ro, go, bo;
        UInt8 rc = (UInt8) ColorTransform.M_[0][1];
        UInt8 gc = (UInt8) ColorTransform.M_[1][1];
        UInt8 bc = (UInt8) ColorTransform.M_[2][1];
        UInt32 p = (rc << 16) | (gc << 8) | bc;
        *val = GASNumber(p);
        return true;
    }
    return GASObject::GetMember(penv, name, val);
}

/******************************************************************************
* Intercept the set command so that dependent properties can be recalculated
*/
bool GASColorTransformObject::SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags)
{
    if (name == "redMultiplier")
    {
        ColorTransform.M_[0][0] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "greenMultiplier")
    {
        ColorTransform.M_[1][0] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "blueMultiplier")
    {
        ColorTransform.M_[2][0] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "alphaMultiplier")
    {
        ColorTransform.M_[3][0] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "redOffset")
    {
        ColorTransform.M_[0][1] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "greenOffset")
    {
        ColorTransform.M_[1][1] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "blueOffset")
    {
        ColorTransform.M_[2][1] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "alphaOffset")
    {
        ColorTransform.M_[3][1] = static_cast<Float>(val.ToNumber(penv));
        return true;
    }
    else if (name == "rgb")
    {
        // When you set this property, it changes the three color offset values 
        // (redOffset, greenOffset, and blueOffset) accordingly, and it sets the 
        // three color multiplier values (redMultiplier, greenMultiplier, and 
        // blueMultiplier) to 0. The alpha transparency multiplier and offset 
        // values do not change.
        ColorTransform.M_[0][0] = 0;
        ColorTransform.M_[1][0] = 0;
        ColorTransform.M_[2][0] = 0;
        GASNumber n = val.ToNumber(penv);
        UInt32 p = 0, rc = 0, gc = 0, bc = 0;
        if (!GASNumberUtil::IsNaN(n))
        {
            p = (UInt32) val.ToNumber(penv);
            rc = (p >> 16) & 0x000000ff;
            gc = (p >> 8) & 0x000000ff;
            bc = p & 0x000000ff;
        }
        ColorTransform.M_[0][1] = static_cast<Float>(rc);
        ColorTransform.M_[1][1] = static_cast<Float>(gc);
        ColorTransform.M_[2][1] = static_cast<Float>(bc);
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

// ****************************************************************************
// AS to GFx function mapping
//
const GASNameFunction GASColorTransformProto::FunctionTable[] = 
{
    { "concat", &GASColorTransformProto::Concat },
    { "toString", &GASColorTransformProto::ToString },
    { 0, 0 }
};

// ****************************************************************************
// GASColorTransform Prototype constructor
//
GASColorTransformProto::GASColorTransformProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) :
GASPrototype<GASColorTransformObject>(psc, pprototype, constructor)
{
    // we make the functions enumerable
    for(int i = 0; FunctionTable[i].Name; i++)
    {
        GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString(FunctionTable[i].Name), 
            GASFunctionRef (*new GASFunctionObject(psc, pprototype, FunctionTable[i].Function)),
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);
    }
    // add the on-the-fly calculated properties
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("redMultiplier"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("greenMultiplier"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("blueMultiplier"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("alphaMultiplier"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("redOffset"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("greenOffset"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("blueOffset"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("alphaOffset"), 0, GASPropFlags::PropFlag_DontDelete);
    GASColorTransformObject::SetMemberRaw(psc, psc->CreateConstString("rgb"), 0, GASPropFlags::PropFlag_DontDelete);
    //InitFunctionMembers(psc, FunctionTable, pprototype);
}

// ****************************************************************************
// Called when the constructor is invoked for the ColorTransform class
//
void GASColorTransformProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASColorTransformObject> pct = *new GASColorTransformObject(fn.Env);
    fn.Result->SetAsObject(pct.GetPtr());
    // handle the constructor parameters
    if (fn.NArgs > 7)
    {
        GASColorTransform* ct = pct->GetColorTransform();
        ct->M_[0][0] = static_cast<Float>(fn.Arg(0).ToNumber(fn.Env));
        ct->M_[1][0] = static_cast<Float>(fn.Arg(1).ToNumber(fn.Env));
        ct->M_[2][0] = static_cast<Float>(fn.Arg(2).ToNumber(fn.Env));
        ct->M_[3][0] = static_cast<Float>(fn.Arg(3).ToNumber(fn.Env));
        ct->M_[0][1] = static_cast<Float>(fn.Arg(4).ToNumber(fn.Env));
        ct->M_[1][1] = static_cast<Float>(fn.Arg(5).ToNumber(fn.Env));
        ct->M_[2][1] = static_cast<Float>(fn.Arg(6).ToNumber(fn.Env));
        ct->M_[3][1] = static_cast<Float>(fn.Arg(7).ToNumber(fn.Env));
    }
}

// ****************************************************************************
// ColorTransform.Concat method
//
void GASColorTransformProto::Concat(const GASFnCall& fn)
{
    if (fn.NArgs > 0)
    {
        GASObject* p = fn.Arg(0).ToObject();
        if (NULL != p)
        {
            GASColorTransformObject *pthis = (GASColorTransformObject*)fn.ThisPtr;
            GASSERT(pthis);
            // begin concat logic
            if (p->GetObjectType() == Object_ColorTransform)
            {
                // the properties aren't stored if it is a ColorTransform obj
                // so, manually get the properties
                GASColorTransformObject *pparam = (GASColorTransformObject*)p;
                pthis->GetColorTransform()->Concatenate(*pparam->GetColorTransform());
            }
            else
            {
                GASValue c[GFxColorTransform_NumProperties];
                // get properties for parameter
                GFxObject_GetColorTransformProperties(fn.Env, p, c); 
                GASColorTransform ct;
                ct.M_[0][0] = static_cast<Float>(c[GFxColorTransform_RedMultiplier].ToNumber(fn.Env));
                ct.M_[1][0] = static_cast<Float>(c[GFxColorTransform_GreenMultiplier].ToNumber(fn.Env));
                ct.M_[2][0] = static_cast<Float>(c[GFxColorTransform_BlueMultiplier].ToNumber(fn.Env));
                ct.M_[3][0] = static_cast<Float>(c[GFxColorTransform_AlphaMultiplier].ToNumber(fn.Env));
                ct.M_[0][1] = static_cast<Float>(c[GFxColorTransform_RedOffset].ToNumber(fn.Env));
                ct.M_[1][1] = static_cast<Float>(c[GFxColorTransform_GreenOffset].ToNumber(fn.Env));
                ct.M_[2][1] = static_cast<Float>(c[GFxColorTransform_BlueOffset].ToNumber(fn.Env));
                ct.M_[3][1] = static_cast<Float>(c[GFxColorTransform_AlphaOffset].ToNumber(fn.Env));
                pthis->GetColorTransform()->Concatenate(ct);
            }
            // end concat logic
        }
    }
}

// ****************************************************************************
// ColorTransform.toString method
//
void GASColorTransformProto::ToString(const GASFnCall& fn)
{
    GASColorTransformObject* pthis = (GASColorTransformObject*) fn.ThisPtr;
    GASSERT(pthis);
    GASColorTransform *ct = pthis->GetColorTransform();

    GASString ps[8] = 
    {
        GASValue(ct->M_[0][0]).ToString(fn.Env, 6),
        GASValue(ct->M_[1][0]).ToString(fn.Env, 6),
        GASValue(ct->M_[2][0]).ToString(fn.Env, 6),
        GASValue(ct->M_[3][0]).ToString(fn.Env, 6),
        GASValue(ct->M_[0][1]).ToString(fn.Env, 6),
        GASValue(ct->M_[1][1]).ToString(fn.Env, 6),
        GASValue(ct->M_[2][1]).ToString(fn.Env, 6),
        GASValue(ct->M_[3][1]).ToString(fn.Env, 6)
    };

    GFxString str;
    str += "(redMultiplier=";
    str += ps[0].ToCStr();
    str += ", greenMultiplier=";
    str += ps[1].ToCStr();
    str += ", blueMultiplier=";
    str += ps[2].ToCStr();
    str += ", alphaMultiplier=";
    str += ps[3].ToCStr();
    str += ", redOffset=";
    str += ps[4].ToCStr();
    str += ", greenOffset=";
    str += ps[5].ToCStr();
    str += ", blueOffset=";
    str += ps[6].ToCStr();
    str += ", alphaOffset=";
    str += ps[7].ToCStr();
    str += ")";

    fn.Result->SetString(fn.Env->CreateString(str));
}
