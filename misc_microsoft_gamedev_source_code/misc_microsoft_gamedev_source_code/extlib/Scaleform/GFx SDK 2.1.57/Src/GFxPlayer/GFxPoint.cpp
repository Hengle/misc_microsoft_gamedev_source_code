/**********************************************************************

Filename    :   GFxPoint.cpp
Content     :   flash.geom.Point reference class for ActionScript 2.0
Created     :   3/7/2007
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
#include "GFxPoint.h"
#include "GFxStringBuiltins.h"

// GFC_NO_FXPLAYER_AS_POINT disables Point class
#ifndef GFC_NO_FXPLAYER_AS_POINT

// default parameters
static const GASValue GFxPoint_DefaultParams[GFxPoint_NumProperties] = 
{
    0,  // x:Number
    0   // y:Number
};

// NaN parameters
static const GASValue GFxPoint_NanParams[GFxPoint_NumProperties] =
{
    GASNumberUtil::NaN(),
    GASNumberUtil::NaN()
};

// ****************************************************************************
// Helper function to retrieve the point properties from an object
//
void GFxObject_GetPointProperties(GASEnvironment *penv, GASObject *pobj, GASValue params[GFxPoint_NumProperties])
{
    GASStringContext* psc = penv->GetSC();
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &params[GFxPoint_X]);
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &params[GFxPoint_Y]);
}

void GFxObject_GetPointProperties(GASEnvironment *penv, GASObject *pobj, GASPoint &pt)
{
    GASStringContext* psc = penv->GetSC();
    GASValue params[GFxPoint_NumProperties];
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &(params[GFxPoint_X]));
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &(params[GFxPoint_Y]));
    pt.SetPoint(params[GFxPoint_X].ToNumber(penv), params[GFxPoint_Y].ToNumber(penv));
}

// ****************************************************************************
// GASPoint constructors
//
GASPointObject::GASPointObject(GASEnvironment *penv)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Point));
    SetProperties(penv->GetSC(), GFxPoint_DefaultParams);
}

GASPointObject::GASPointObject(GASEnvironment* penv, GASPointObject *po)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Point));
    GASValue params[GFxPoint_NumProperties];
    po->GetProperties(penv->GetSC(), params);
    SetProperties(penv->GetSC(), params);
}

// ****************************************************************************
// Handle get method for properties computed on the fly
//
bool GASPointObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    bool ret = false;
    GASStringContext* psc = penv->GetSC();
    // try to handle this property
    // NOTE: Flash 8 is always case sensitive (Point was introduced in Flash 8)
    if (name == psc->GetBuiltin(GASBuiltin_length))
    {
        GASPoint pt;
        GetProperties(penv, pt);
        *val = pt.Distance();
        ret = true;
    }
    // if property wasn't gobbled up by the custom handler, pass it along to the base class
    else
    {
        ret = GASObject::GetMember(penv, name, val);
    }
    return ret;
}

// ****************************************************************************
// Get the Point
//
void GASPointObject::GetProperties(GASStringContext *psc, GASValue params[GFxPoint_NumProperties])
{
    GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &(params[GFxPoint_X]));
    GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &(params[GFxPoint_Y]));

}

void GASPointObject::GetProperties(GASEnvironment *penv, GASPoint &pt)
{
    GASStringContext *psc = penv->GetSC();
    GASValue params[GFxPoint_NumProperties];
    GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &(params[GFxPoint_X]));
    GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &(params[GFxPoint_Y]));
    pt.SetPoint(params[GFxPoint_X].ToNumber(penv), params[GFxPoint_Y].ToNumber(penv));
}

// ****************************************************************************
// Set the Point
//
void GASPointObject::SetProperties(GASStringContext *psc, const GASValue params[GFxPoint_NumProperties])
{
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), params[GFxPoint_X]);
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), params[GFxPoint_Y]);

}

void GASPointObject::SetProperties(GASEnvironment *penv, const GASPoint &pt)
{
    GASStringContext *psc = penv->GetSC();
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), pt.x);
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), pt.y);
}

// ****************************************************************************
// AS to GFx function mapping
//
const GASNameFunction GASPointProto::FunctionTable[] = 
{
    { "add", &GASPointProto::Add },
    { "clone", &GASPointProto::Clone },
    { "equals", &GASPointProto::Equals },
    { "normalize", &GASPointProto::Normalize },
    { "offset", &GASPointProto::Offset },
    { "subtract", &GASPointProto::Subtract },
    { "toString", &GASPointProto::ToString },
    { 0, 0 }
};

// ****************************************************************************
// GASPoint prototype constructor
//
GASPointProto::GASPointProto(GASStringContext *psc, GASObject *pprototype, const GASFunctionRef &constructor) : 
    GASPrototype<GASPointObject>(psc, pprototype, constructor)
{
    // we make the functions enumerable
    for(int i = 0; FunctionTable[i].Name; i++)
    {
        GASPointObject::SetMemberRaw(psc, psc->CreateConstString(FunctionTable[i].Name), 
            GASFunctionRef (*new GASFunctionObject(psc, pprototype, FunctionTable[i].Function)),
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);
    }
    // add the on-the-fly calculated properties
    GASPointObject::SetMemberRaw(psc, psc->CreateConstString("length"), 0, GASPropFlags::PropFlag_DontDelete);
    //InitFunctionMembers(psc, FunctionTable, pprototype);
}

// ****************************************************************************
// Called when the constructor is invoked for the Point class (new Point())
//
void GASPointProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASPointObject> ppt = *new GASPointObject(fn.Env);
    fn.Result->SetAsObject(ppt.GetPtr());

    // handle constructor parameters
    if (fn.NArgs > 0)
    {
        GASValue params[GFxPoint_NumProperties];
        params[GFxPoint_X] = fn.Arg(0);
        if (fn.NArgs > 1)
        {
            params[GFxPoint_Y] = fn.Arg(1);
        }
        // set the parameters
        ppt->SetProperties(fn.Env->GetSC(), params);
    }
}

// ****************************************************************************
// Point.add method
//
void GASPointProto::Add(const GASFnCall& fn)
{
    GPtr<GASPointObject> retpt = *new GASPointObject(fn.Env);
    bool set = false;
    if (fn.NArgs > 0) 
    {
        GASObject* p = fn.Arg(0).ToObject();
        if (NULL != p)
        {
            CHECK_THIS_PTR(fn, Point, "Point");
            GASPointObject* pthis = (GASPointObject*)fn.ThisPtr;
            GASSERT(pthis);
            GASPoint pt1, pt2;
            // get properties for this
            pthis->GetProperties(fn.Env, pt1);
            // get properties for parameter
            GFxObject_GetPointProperties(fn.Env, p, pt2);
            // begin add logic
            pt1 += pt2;
            retpt->SetProperties(fn.Env, pt1);
            // end add logic
            set = true;
        }
    }
    if (!set)
    {
        retpt->SetProperties(fn.Env->GetSC(), GFxPoint_NanParams);
    }
    fn.Result->SetAsObject(retpt.GetPtr());
}

// ****************************************************************************
// Point.clone method
//
void GASPointProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Point, "Point");
    GASPointObject* pthis = (GASPointObject*)fn.ThisPtr;
    GASSERT(pthis);
    GPtr<GASPointObject> ppt = *new GASPointObject(fn.Env);

    GASStringContext* psc = fn.Env->GetSC();
    GASValue params[GFxPoint_NumProperties];
    pthis->GetProperties(psc, params);
    // begin copy of properties
    ppt.GetPtr()->SetProperties(psc, params);
    // end copy of properties

    fn.Result->SetAsObject(ppt.GetPtr());
}

// ****************************************************************************
// Point.equals method
//
void GASPointProto::Equals(const GASFnCall& fn)
{
    bool ret = false;
    if (fn.NArgs > 0)
    {
        GASObject* p = fn.Arg(0).ToObject();
        if ((NULL != p) && (p->GetObjectType() == Object_Point))
        {       
            CHECK_THIS_PTR(fn, Point, "Point");
            GASPointObject* pthis = (GASPointObject*)fn.ThisPtr;
            GASSERT(pthis);
            GASPoint pt1, pt2;
            // get properties for this
            pthis->GetProperties(fn.Env, pt1);
            // get properties for parameter
            GFxObject_GetPointProperties(fn.Env, p, pt2);
            // begin equals logic
            ret = (pt1 == pt2);
            // end equals logic
        }
    }
    fn.Result->SetBool(ret);
}

// ****************************************************************************
// Point.normalize method
//
void GASPointProto::Normalize(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Point, "Point");
    GASPointObject* pthis = (GASPointObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (fn.NArgs > 0)
    {
        GASValue p = fn.Arg(0);
        GASPoint pt1;
        pthis->GetProperties(fn.Env, pt1);
        // begin normalize logic
        GASNumber m = (p.ToNumber(fn.Env) / pt1.Distance());
        pthis->SetProperties(fn.Env, pt1.Mul(m));
        // end normalize logic
    }
    else
    {
        pthis->SetProperties(fn.Env->GetSC(), GFxPoint_NanParams);
    }
}

// ****************************************************************************
// Point.offset method
//
void GASPointProto::Offset(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Point, "Point");
    GASPointObject* pthis = (GASPointObject*)fn.ThisPtr;
    GASSERT(pthis);
    GASValue dx, dy;
    if (fn.NArgs > 0)
    {
        dx = fn.Arg(0);
        if (fn.NArgs > 1)
        {
            dy = fn.Arg(1); 
        }
    }
    GASPoint pt1;
    pthis->GetProperties(fn.Env, pt1);
    // begin offset logic   
    pthis->SetProperties(fn.Env, pt1.Offset(dx.ToNumber(fn.Env), dy.ToNumber(fn.Env)));
    // end offset logic
}

// ****************************************************************************
// Point.subtract method
//
void GASPointProto::Subtract(const GASFnCall& fn)
{
    GPtr<GASPointObject> retpt = *new GASPointObject(fn.Env);
    bool set = false;
    if (fn.NArgs > 0) 
    {
        GASObject* p = fn.Arg(0).ToObject();
        if ( NULL != p)
        {
            CHECK_THIS_PTR(fn, Point, "Point");
            GASPointObject* pthis = (GASPointObject*)fn.ThisPtr;
            GASSERT(pthis);
            GASPoint pt1, pt2;
            // get properties for this
            pthis->GetProperties(fn.Env, pt1);
            // get properties for parameter
            GFxObject_GetPointProperties(fn.Env, p, pt2);
            // begin add logic
            pt1 -= pt2;
            retpt->SetProperties(fn.Env, pt1);
            // end add logic
            set = true;
        }
    }
    if (!set)
    {
        retpt->SetProperties(fn.Env->GetSC(), GFxPoint_NanParams);
    }
    fn.Result->SetAsObject(retpt.GetPtr());
}

// ****************************************************************************
// Point.toString method
//
void GASPointProto::ToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Point, "Point");
    GASPointObject* pthis = (GASPointObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GASValue params[GFxPoint_NumProperties];
    pthis->GetProperties(fn.Env->GetSC(), params);

    GASString ps[2] = 
    {
        params[GFxPoint_X].ToString(fn.Env, 6),
        params[GFxPoint_Y].ToString(fn.Env, 6)
    };

    GFxString str;
    str += "(x=";
    str += ps[GFxPoint_X].ToCStr();
    str += ", y=";
    str += ps[GFxPoint_Y].ToCStr();
    str += ")";

    fn.Result->SetString(fn.Env->CreateString(str));
}


// ****************************************************************************
// AS to GFx static function mapping
//
const GASNameFunction GASPointCtorFunction::StaticFunctionTable[] = 
{
    { "interpolate", &GASPointCtorFunction::Interpolate },
    { "distance", &GASPointCtorFunction::Distance },
    { "polar", &GASPointCtorFunction::Polar },
    { 0, 0 }
};

// ****************************************************************************
// GASPoint ctor function constructor
//
GASPointCtorFunction::GASPointCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GASPointProto::GlobalCtor)
{
    for (int i=0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue (StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

// ****************************************************************************
// (Static) Point.distance method
//
void GASPointCtorFunction::Distance(const GASFnCall& fn)
{
    // handle weird behavior in AS 2.0..
    // if {x:10,y:10} and {x:20,y:20} are passed in as params, then the return value is undefined
    // if Point(10,10) and {x:20,y:20} are passed in as params, then the return value IS defined
    fn.Result->SetNumber(GASNumberUtil::NaN());
    if (fn.NArgs > 1) 
    {
        GASObject* p1 = fn.Arg(0).ToObject();
        GASObject* p2 = fn.Arg(1).ToObject();
        if (( NULL != p1) && ( NULL != p2))
        {
            if ((p1->GetObjectType() != Object_Point) &&
               (p2->GetObjectType() != Object_Point))
            {
                fn.Result->SetUndefined();
            }
            else
            {
                GASValue o1[GFxPoint_NumProperties], o2[GFxPoint_NumProperties];
                // get the first param's properties
                GFxObject_GetPointProperties(fn.Env, p1, o1);
                // get the second param's properties
                GFxObject_GetPointProperties(fn.Env, p2, o2);
                // begin distance logic
                GASValue xd = o2[GFxPoint_X];
                xd.Sub(fn.Env, o1[GFxPoint_X]);
                xd.Mul(fn.Env, xd);
                GASValue yd = o2[GFxPoint_Y];
                yd.Sub(fn.Env, o1[GFxPoint_Y]);
                yd.Mul(fn.Env, yd);
                xd.Add(fn.Env, yd);
                GASNumber d = GASValue(GASNumber(sqrt(xd.ToNumber(fn.Env)))).ToNumber(fn.Env);
                fn.Result->SetNumber(d);
                // end distance logic
            }
        }
    }
}

// ****************************************************************************
// (Static) Point.interpolate method
//
void GASPointCtorFunction::Interpolate(const GASFnCall& fn)
{
    // handle weird behavior in AS 2.0..
    // both params can be of the form {x:x_val, y:y_val} and the return value is a valid point object
    GPtr<GASPointObject> ppt = *new GASPointObject(fn.Env);
    bool set = false;
    if (fn.NArgs > 2) 
    {
        GASObject* p1 = fn.Arg(0).ToObject();
        GASObject* p2 = fn.Arg(1).ToObject();
        GASValue f = fn.Arg(2);
        if (( NULL != p1) && ( NULL != p2))
        {
            GASPoint pt1, pt2;
            // get the first param's properties
            GFxObject_GetPointProperties(fn.Env, p1, pt1);
            // get the second param's properties
            GFxObject_GetPointProperties(fn.Env, p2, pt2);
            GASNumber ptf = f.ToNumber(fn.Env);
            // begin interpolate logic
            // x = ((pt1.x - pt2.x) * ptf) + pt2.x;
            // y = ((pt1.y - pt2.y) * ptf) + pt2.y;
            GASPoint ip( ((pt1.x - pt2.x) * ptf) + pt2.x, ((pt1.y - pt2.y) * ptf) + pt2.y );
            ppt->SetProperties(fn.Env, ip);
            // end interpolate logic
            set = true;
        }       
    }   
    if (!set)
    {
        ppt->SetProperties(fn.Env->GetSC(), GFxPoint_NanParams);
    }
    fn.Result->SetAsObject(ppt.GetPtr());
}

// ****************************************************************************
// (Static) Point.polar method
//
void GASPointCtorFunction::Polar(const GASFnCall& fn)
{
    GPtr<GASPointObject> ppt = *new GASPointObject(fn.Env);
    bool set = false;
    if (fn.NArgs > 1) 
    {
        GASValue length = fn.Arg(0);
        GASValue angle = fn.Arg(1);
        // begin polar logic
        GASNumber l = length.ToNumber(fn.Env);
        GASNumber a = angle.ToNumber(fn.Env);
        Double sa = sin(a);
        Double ca = cos(a);
        ppt->SetProperties(fn.Env, GASPoint(l*ca, l*sa));
        // end polar logic
        set = true;
    }   
    if (!set)
    {
        ppt->SetProperties(fn.Env->GetSC(), GFxPoint_NanParams);
    }
    fn.Result->SetAsObject(ppt.GetPtr());
}

#else

void GASPoint_DummyFunction() {}   // Exists to quelch compiler warning

#endif  //  GFC_NO_FXPLAYER_AS_POINT

