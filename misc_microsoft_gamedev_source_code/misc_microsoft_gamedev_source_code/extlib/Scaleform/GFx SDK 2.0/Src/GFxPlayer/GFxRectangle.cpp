/**********************************************************************

Filename    :   GFxRectangle.cpp
Content     :   flash.geom.Rectangle reference class for ActionScript 2.0
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
#include "GFxRectangle.h"

// default parameters
static const GASValue GFxRectangle_DefaultParams[GFxRectangle_NumProperties] =
{
    0,  // x:Number
    0,  // y:Number
    0,  // w:Number
    0   // h:Number
};

// NaN parameters
static const GASValue GFxRectangle_NaNParams[GFxRectangle_NumProperties] = 
{
    GASNumberUtil::NaN(),
    GASNumberUtil::NaN(),
    GASNumberUtil::NaN(),
    GASNumberUtil::NaN()
};

// ****************************************************************************
// Helper function to retrieve the point properties from an object
//
static void GFxObject_GetRectangleProperties(GASEnvironment *penv, GASObject *pobj, GASValue params[GFxRectangle_NumProperties])
{
    GASStringContext* psc = penv->GetSC();
    pobj->GetConstMemberRaw(psc, "x", &params[GFxRectangle_X]);
    pobj->GetConstMemberRaw(psc, "y", &params[GFxRectangle_Y]);
    pobj->GetConstMemberRaw(psc, "width", &params[GFxRectangle_Width]);
    pobj->GetConstMemberRaw(psc, "height", &params[GFxRectangle_Height]);
}

/*
static void GFxObject_GetRectangleProperties(GASEnvironment *penv, GASObject *pobj, GASRect &r)
{
    GASStringContext* psc = penv->GetSC();
    GASValue params[GFxRectangle_NumProperties];
    pobj->GetConstMemberRaw(psc, "x", &params[GFxRectangle_X]);
    pobj->GetConstMemberRaw(psc, "y", &params[GFxRectangle_Y]);
    pobj->GetConstMemberRaw(psc, "width", &params[GFxRectangle_Width]);
    pobj->GetConstMemberRaw(psc, "height", &params[GFxRectangle_Height]);
    r.SetRect(params[GFxRectangle_X].ToNumber(penv),
              params[GFxRectangle_Y].ToNumber(penv),
              GSize<GASNumber>(params[GFxRectangle_Width].ToNumber(penv),
                               params[GFxRectangle_Height].ToNumber(penv)));
}
*/

// ****************************************************************************
// Helper function to compute the top left (GASPointObject)
//
static GASValue GFxRectangle_ComputeTopLeft(GASEnvironment *penv, GASRect r)
{
    GPtr<GASPointObject> gpt = *new GASPointObject(penv);
    GASPointObject* pt = gpt.GetPtr();
    pt->SetProperties(penv, r.TopLeft());
    return pt;
}

// ****************************************************************************
// Helper function to compute the bottom right (GASPointObject)
//
static GASValue GFxRectangle_ComputeBottomRight(GASEnvironment *penv, GASRect r)
{
    GPtr<GASPointObject> gpt = *new GASPointObject(penv);
    GASPointObject* pt = gpt.GetPtr();
    pt->SetProperties(penv, r.BottomRight());
    return pt;
}

// ****************************************************************************
// Helper function to compute the size (GASPointObject)
//
static GASValue GFxRectangle_ComputeSize(GASEnvironment *penv, GASRect r)
{
    GPtr<GASPointObject> gpt = *new GASPointObject(penv);
    GASPointObject *pt = gpt.GetPtr();    
    pt->SetProperties(penv, GASPoint(r.Width(), r.Height()));
    return pt;
}

// ****************************************************************************
// Helper function to recalculate dependent properties of 'topLeft'
//
static void GFxRectangle_SetTopLeft(GASEnvironment *penv, const GASValue &val, GASRect &r) 
{
    if (val.ToObject() != NULL)
    {
        GASObject* o = val.ToObject();
        GASPoint pt;
        GFxObject_GetPointProperties(penv, o, pt);
        r.SetTopLeft(pt);
    }
}

// ****************************************************************************
// Helper function to recalculate dependent properties of 'bottomRight'
//
static void GFxRectangle_SetBottomRight(GASEnvironment *penv, const GASValue &val, GASRect &r) 
{
    if (val.ToObject() != NULL)
    {
        GASObject* o = val.ToObject();
        GASPoint pt;
        GFxObject_GetPointProperties(penv, o, pt);
        r.SetBottomRight(pt);
    }
}

// ****************************************************************************
// Helper function to recalculate dependent properties of 'size'
//
static void GFxRectangle_SetSize(GASEnvironment *penv, const GASValue &val, GASRect &r) 
{
    if (val.ToObject() != NULL)
    {
        GASObject* o = val.ToObject();
        GASPoint pt;
        GFxObject_GetPointProperties(penv, o, pt);
        r.SetSize(pt.x, pt.y);
    }
}

// ****************************************************************************
// GASRectangle  constructor
//
GASRectangleObject::GASRectangleObject(GASEnvironment *penv)
{
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Rectangle));
    SetProperties(penv->GetSC(), GFxRectangle_DefaultParams);
}

// ****************************************************************************
// Handle get method for properties computed on the fly
//
bool GASRectangleObject::GetMember(GASEnvironment *penv, const GASString &name, GASValue* val)
{
    // try to handle this property
    // NOTE: Flash 8 is always case sensitive (Rectangle was introduced in Flash 8)
    if (name == "left") 
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = r.Left;
        return true;
    }
    else if (name == "right") 
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = r.Right;
        return true;
    }
    else if (name == "top")
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = r.Top;
        return true;
    }
    else if (name == "bottom")
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = r.Bottom;
        return true;
    }
    else if (name == "topLeft")
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = GFxRectangle_ComputeTopLeft(penv, r);
        return true;
    }
    else if (name == "bottomRight")
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = GFxRectangle_ComputeBottomRight(penv, r);
        return true;
    }
    else if (name == "size")
    { 
        GASRect r;
        GetProperties(penv, r);
        *val = GFxRectangle_ComputeSize(penv, r);
        return true;
    }
    // if property wasn't gobbled up by the custom handler, pass it along to the base class
    return GASObject::GetMember(penv, name, val);
}

/******************************************************************************
* Intercept the set command so that dependent properties can be recalculated
*/
bool GASRectangleObject::SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags)
{
    if (flags.GetFlags() & GASPropFlags::PropFlag_ReadOnly)
    {
        return false;
    }

    // every property of Rectangle can be set and each has its own dependent properties
    // handle the cases individually
    if  (name == "left")        
    { 
        GASRect r;
        GetProperties(penv, r);
        r.Left = val.ToNumber(penv);
        SetProperties(penv, r);
        return true;
    }
    else if (name == "top")
    { 
        GASRect r;
        GetProperties(penv, r);
        r.Top = val.ToNumber(penv);
        SetProperties(penv, r);
        return true;
    }
    else if (name == "right")
    { 
        GASRect r;
        GetProperties(penv, r);
        r.Right = val.ToNumber(penv);
        SetProperties(penv, r);
        return true;
    }
    else if (name == "bottom")  
    { 
        GASRect r;
        GetProperties(penv, r);
        r.Bottom = val.ToNumber(penv);
        SetProperties(penv, r);
        return true;
    }
    else if (name == "topLeft")
    { 
        GASRect r;
        GetProperties(penv, r);
        GFxRectangle_SetTopLeft(penv, val, r);
        SetProperties(penv, r);
        return true;
    }
    else if (name == "bottomRight")
    { 
        GASRect r;
        GetProperties(penv, r);
        GFxRectangle_SetBottomRight(penv, val, r);
        SetProperties(penv, r);
        return true;
    }
    else if (name == "size")
    { 
        GASRect r;
        GetProperties(penv, r);
        GFxRectangle_SetSize(penv, val, r);
        SetProperties(penv, r);
        return true;
    }
    return GASObject::SetMember(penv, name, val, flags);
}

// ****************************************************************************
// Get the Rectangle properties
//
void GASRectangleObject::GetProperties(GASStringContext *psc, GASValue params[GFxRectangle_NumProperties])
{
    GetConstMemberRaw(psc, "x", &(params[GFxRectangle_X]));
    GetConstMemberRaw(psc, "y", &(params[GFxRectangle_Y]));
    GetConstMemberRaw(psc, "width", &(params[GFxRectangle_Width]));
    GetConstMemberRaw(psc, "height", &(params[GFxRectangle_Height]));
}

void GASRectangleObject::GetProperties(GASEnvironment *penv, GASRect &r)
{
    GASStringContext *psc = penv->GetSC();
    GASValue params[GFxRectangle_NumProperties];
    GetConstMemberRaw(psc, "x", &(params[GFxRectangle_X]));
    GetConstMemberRaw(psc, "y", &(params[GFxRectangle_Y]));
    GetConstMemberRaw(psc, "width", &(params[GFxRectangle_Width]));
    GetConstMemberRaw(psc, "height", &(params[GFxRectangle_Height]));
    r.SetRect(params[GFxRectangle_X].ToNumber(penv),
                params[GFxRectangle_Y].ToNumber(penv),
                GSize<GASNumber>(params[GFxRectangle_Width].ToNumber(penv),
                                params[GFxRectangle_Height].ToNumber(penv)));
}

// ****************************************************************************
// Set the Rectangle properties
//
void GASRectangleObject::SetProperties(GASStringContext *psc, const GASValue params[GFxRectangle_NumProperties])
{
    SetConstMemberRaw(psc, "x", params[GFxRectangle_X]);
    SetConstMemberRaw(psc, "y", params[GFxRectangle_Y]);
    SetConstMemberRaw(psc, "width", params[GFxRectangle_Width]);
    SetConstMemberRaw(psc, "height", params[GFxRectangle_Height]);
}

void GASRectangleObject::SetProperties(GASEnvironment *penv, const GASRect &r)
{
    GASStringContext *psc = penv->GetSC();
    SetConstMemberRaw(psc, "x", r.Left);
    SetConstMemberRaw(psc, "y", r.Top);
    SetConstMemberRaw(psc, "width", r.Width());
    SetConstMemberRaw(psc, "height", r.Height());
}

// ****************************************************************************
// AS to GFx function mapping
//
const GASNameFunction GASRectangleProto::FunctionTable[] = 
{
    { "clone", &GASRectangleProto::Clone },
    { "contains", &GASRectangleProto::Contains },
    { "containsPoint", &GASRectangleProto::ContainsPoint },
    { "containsRectangle", &GASRectangleProto::ContainsRectangle },
    { "equals", &GASRectangleProto::Equals },
    { "inflate", &GASRectangleProto::Inflate },
    { "inflatePoint", &GASRectangleProto::InflatePoint },
    { "intersection", &GASRectangleProto::Intersection },
    { "intersects", &GASRectangleProto::Intersects },
    { "isEmpty", &GASRectangleProto::IsEmpty },
    { "offset", &GASRectangleProto::Offset },
    { "offsetPoint", &GASRectangleProto::OffsetPoint },
    { "setEmpty", &GASRectangleProto::SetEmpty },
    { "toString", &GASRectangleProto::ToString },
    { "union", &GASRectangleProto::Union },
    { 0, 0 }
};

// ****************************************************************************
// GASRectangle Prototype constructor
//
GASRectangleProto::GASRectangleProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) :
    GASPrototype<GASRectangleObject>(psc, pprototype, constructor)
{
    // we make the functions enumerable
    for(int i = 0; FunctionTable[i].Name; i++)
    {
        GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString(FunctionTable[i].Name), 
            GASFunctionRef (*new GASFunctionObject(psc, pprototype, FunctionTable[i].Function)),
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete);
    }
    // add the on-the-fly calculated properties
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("left"), 0, GASPropFlags::PropFlag_DontDelete);
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("top"), 0, GASPropFlags::PropFlag_DontDelete);
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("right"), 0, GASPropFlags::PropFlag_DontDelete);
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("bottom"), 0, GASPropFlags::PropFlag_DontDelete);  
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("topLeft"), 0, GASPropFlags::PropFlag_DontDelete);
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("bottomRight"), 0, GASPropFlags::PropFlag_DontDelete);
    GASRectangleObject::SetMemberRaw(psc, psc->CreateConstString("size"), 0, GASPropFlags::PropFlag_DontDelete);
    //InitFunctionMembers(psc, FunctionTable, pprototype);
}

// ****************************************************************************
// Called when the constructor is invoked for the Rectangle class
//
void GASRectangleProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASRectangleObject> prect = *new GASRectangleObject(fn.Env);
    fn.Result->SetAsObject(prect.GetPtr());

    // handle the constructor parameters
    if (fn.NArgs > 0)
    {
        GASValue params[GFxRectangle_NumProperties];
        params[GFxRectangle_X] = fn.Arg(0);
        if (fn.NArgs > 1)
        {
            params[GFxRectangle_Y] = fn.Arg(1);
            if (fn.NArgs > 2)
            {
                params[GFxRectangle_Width] = fn.Arg(2);
                if (fn.NArgs > 3)
                {
                    params[GFxRectangle_Height] = fn.Arg(3);
                }
            }
        }

        // set the parameters
        prect->SetProperties(fn.Env->GetSC(), params);
    }
}

// ****************************************************************************
// Rectangle.clone method
//
void GASRectangleProto::Clone(const GASFnCall &fn)
{
    GASRectangleObject* p_this = (GASRectangleObject*) fn.ThisPtr;
    GASSERT(p_this);
    GPtr<GASRectangleObject> p_clone = *new GASRectangleObject(fn.Env);
    GASValue params[GFxRectangle_NumProperties];
    p_this->GetProperties(fn.Env->GetSC(), params);
    // begin copy of properties
    p_clone.GetPtr()->SetProperties(fn.Env->GetSC(), params);
    // end copy of properties
    fn.Result->SetAsObject(p_clone.GetPtr());
}

// ****************************************************************************
// Rectangle.clone method
//
void GASRectangleProto::Contains(const GASFnCall& fn)
{
    if (fn.NArgs > 1)
    {
        GASValue px = fn.Arg(0);
        GASValue py = fn.Arg(1);
        GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
        GASSERT(pthis);
        GASRect r;
        // get properties for this
        pthis->GetProperties(fn.Env, r);
        // begin contains logic
        GASNumber pxn = px.ToNumber(fn.Env);
        GASNumber pyn = py.ToNumber(fn.Env);
        // AS 2.0 contains method is not closed on the right and bottom boundaries
        // handle that case
        if (pxn == r.Right)
        {
            pxn += 1.0;
        }
        if (pyn == r.Bottom)
        {
            pyn += 1.0;
        }
        fn.Result->SetBool(r.Contains(pxn, pyn));
        // end contains logic
    }
    else
    {
        fn.Result->SetBool(false);
    }
}

// ****************************************************************************
// Rectangle.containsPoint method
//
void GASRectangleProto::ContainsPoint(const GASFnCall& fn)
{
    if (fn.NArgs > 0)
    {
        GASObject* p = fn.Arg(0).ToObject();
        if ( NULL != p )
        {
            GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
            GASSERT(pthis);
            GASRect r;
            GASValue ptv[GFxPoint_NumProperties];
            // get properties for this
            pthis->GetProperties(fn.Env, r);
            // get properties for parameter
            GFxObject_GetPointProperties(fn.Env, p, ptv);
            // if param is not a point, then x and y both must be defined, else return undefined
            if ( (p->GetObjectType() != Object_Point) && (ptv[GFxPoint_X].IsUndefined() || ptv[GFxPoint_Y].IsUndefined()) )
            {
                return;
            }
            // begin contains logic
            GASPoint pt;
            ((GASPointObject*)p)->GetProperties(fn.Env, pt);
            // AS 2.0 contains method is not closed on the right and bottom boundaries
            // handle that case
            if (pt.x == r.Right)
            {
                pt.x += 1.0;
            }
            if (pt.y == r.Bottom)
            {
                pt.y += 1.0;
            }
            fn.Result->SetBool(r.Contains(pt));
            // end contains logic
        }
    }
}

// ****************************************************************************
// Rectangle.containsRectangle method
//
void GASRectangleProto::ContainsRectangle(const GASFnCall& fn)
{
    if (fn.NArgs > 0)
    {
        GASObject* p = fn.Arg(0).ToObject();
        if ( NULL != p )
        {
            GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
            GASSERT(pthis);
            GASRect r1;
            GASValue r2v[GFxRectangle_NumProperties];
            // get properties for this
            pthis->GetProperties(fn.Env, r1);
            // get properties for parameter
            GFxObject_GetRectangleProperties(fn.Env, p, r2v);
            // x, y, w, h must be defined, else return undefined
            if ( r2v[GFxRectangle_X].IsUndefined() || r2v[GFxRectangle_Y].IsUndefined() ||
                 r2v[GFxRectangle_Width].IsUndefined() || r2v[GFxRectangle_Height].IsUndefined() )
            {
                return;
            }
            // begin contains logic
            GASRect r2(r2v[GFxRectangle_X].ToNumber(fn.Env),
                       r2v[GFxRectangle_Y].ToNumber(fn.Env),
                       GSize<GASNumber>(r2v[GFxRectangle_Width].ToNumber(fn.Env),
                                        r2v[GFxRectangle_Height].ToNumber(fn.Env)));           
            fn.Result->SetBool(r1.Contains(r2));
            // end contains logic
        }
    }
}

// ****************************************************************************
// Rectangle.equals method
//
void GASRectangleProto::Equals(const GASFnCall& fn)
{
    bool ret = false;
    if ( fn.NArgs > 0 )
    {
        GASObject *p = fn.Arg(0).ToObject();
        if (( NULL != p) && (p->GetObjectType() == Object_Rectangle))
        {
            GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
            GASSERT(pthis);
            GPtr<GASRectangleObject> parr = (GASRectangleObject*)p;
            GASRect r1, r2;
            // get properties for this
            pthis->GetProperties(fn.Env, r1);
            // get properties for parameter
            parr->GetProperties(fn.Env, r2);
            ret = (r1 == r2);
        }
    }
    fn.Result->SetBool(ret);
}

// ****************************************************************************
// Rectangle.inflate method
//
void GASRectangleProto::Inflate(const GASFnCall& fn)
{
    GASValue dw, dh;
    if (fn.NArgs > 0)
    {
        dw = fn.Arg(0);
        if (fn.NArgs > 1)
        {
            dh = fn.Arg(1);
        }
    }
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    GASRect o1;
    // get properties for this
    pthis->GetProperties(fn.Env, o1);
    // begin inflate logic
    o1.Expand(dw.ToNumber(fn.Env), dh.ToNumber(fn.Env));
    pthis->SetProperties(fn.Env, o1);
    // end inflate logic        
}

// ****************************************************************************
// Rectangle.inflatePoint method
//
void GASRectangleProto::InflatePoint(const GASFnCall& fn)
{
    if (fn.NArgs > 0)
    {
        GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
        GASSERT(pthis);
        GASObject* p = fn.Arg(0).ToObject();
        if ( NULL == p )
        {
            pthis->SetProperties(fn.Env->GetSC(), GFxRectangle_NaNParams);
            return;
        }   
        GASRect o1;
        GASPoint o2;
        // get properties for this
        pthis->GetProperties(fn.Env, o1);
        // get properties for parameter
        GFxObject_GetPointProperties(fn.Env, p, o2);
        // begin contains logic
        o1.Expand(o2.x, o2.y);
        // end contains logic
        pthis->SetProperties(fn.Env, o1);
    }
}

// ****************************************************************************
// Rectangle.intersection method
//
void GASRectangleProto::Intersection(const GASFnCall& fn)
{
    GASRect ret(0,0,0,0);
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (fn.NArgs > 0)
    {
        GASObject* p = fn.Arg(0).ToObject();
        if (p != NULL)
        {
            GASRect o1;
            GASValue o2v[GFxRectangle_NumProperties];
            // get properties for this
            pthis->GetProperties(fn.Env, o1);
            // get properties for parameter
            GFxObject_GetRectangleProperties(fn.Env, p, o2v);
            GASRect o2(o2v[GFxRectangle_X].ToNumber(fn.Env),
                o2v[GFxRectangle_Y].ToNumber(fn.Env),
                GSize<GASNumber>(o2v[GFxRectangle_Width].ToNumber(fn.Env),
                o2v[GFxRectangle_Height].ToNumber(fn.Env)));
            // begin intersection logic
            o1.IntersectRect(&ret, o2);
            // AS 2.0 returns an empty rect if width or height is 0, NaN or undefined
            // handle that case
            if ( (ret.Width() == 0) || (ret.Height() == 0) )
            {
                ret.SetRect(0, 0, 0, 0);
            }
            // end intersection logic
        }
    }
    GPtr<GASRectangleObject> obj = *new GASRectangleObject(fn.Env);
    obj.GetPtr()->SetProperties(fn.Env, ret);
    fn.Result->SetAsObject(obj.GetPtr());
}

// ****************************************************************************
// Rectangle.intersects method
//
void GASRectangleProto::Intersects(const GASFnCall& fn)
{
    bool ret = false;
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    if (fn.NArgs > 0)
    {
        GASObject* p = fn.Arg(0).ToObject();
        if (p != NULL)
        {
            GASRect o1;
            GASValue o2v[GFxRectangle_NumProperties];
            // get properties for this
            pthis->GetProperties(fn.Env, o1);
            // get properties for parameter
            GFxObject_GetRectangleProperties(fn.Env, p, o2v);
            GASRect o2(o2v[GFxRectangle_X].ToNumber(fn.Env),
                o2v[GFxRectangle_Y].ToNumber(fn.Env),
                GSize<GASNumber>(o2v[GFxRectangle_Width].ToNumber(fn.Env),
                o2v[GFxRectangle_Height].ToNumber(fn.Env)));
            // begin intersection logic
            GASRect r(0,0,0,0);
            o1.IntersectRect(&r, o2);
            ret = true;
            // AS 2.0 returns an empty rect if width or height is 0, NaN or undefined
            // handle that case
            if ( (r.Width() == 0) || (r.Height() == 0) )
            {
                ret = false;
            }
            // end intersection logic
        }
    }
    fn.Result->SetBool(ret);
}

// ****************************************************************************
// Rectangle.isEmpty method
///
void GASRectangleProto::IsEmpty(const GASFnCall& fn)
{
    bool ret = false;
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    GASValue params[GFxRectangle_NumProperties];
    pthis->GetProperties(fn.Env->GetSC(), params);
    // begin isempty logic
    // AS 2.0 isempty method return true iff w==h==0 or (w||h)==NaN
    if ( GASNumberUtil::IsNaN(params[GFxRectangle_Width].ToNumber(fn.Env)) || 
        GASNumberUtil::IsNaN(params[GFxRectangle_Height].ToNumber(fn.Env)) )
    {
        ret = true;
    }
    else
    {
        GASRect o1 = (params[GFxRectangle_X].ToNumber(fn.Env),
            params[GFxRectangle_Y].ToNumber(fn.Env),
            GSize<GASNumber>(params[GFxRectangle_Width].ToNumber(fn.Env),
            params[GFxRectangle_Height].ToNumber(fn.Env)));
        ret = o1.IsEmpty();
    }
    // end isempty logic
    fn.Result->SetBool(ret);
}

// ****************************************************************************
// Rectangle.offset method
//
void GASRectangleProto::Offset(const GASFnCall& fn)
{
    GASValue dx, dy;
    if (fn.NArgs > 0)
    {
        dx = fn.Arg(0);
        if (fn.NArgs > 1)
        {
            dy = fn.Arg(1);
        }
    }
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    GASValue o1[GFxRectangle_NumProperties];
    // get properties for this
    pthis->GetProperties(fn.Env->GetSC(), o1);
    // begin offset logic
    // in AS 2.0, width and height do not change
    // handle that
    o1[GFxRectangle_X] = o1[GFxRectangle_X].ToNumber(fn.Env) + dx.ToNumber(fn.Env);
    o1[GFxRectangle_Y] = o1[GFxRectangle_Y].ToNumber(fn.Env) + dy.ToNumber(fn.Env);
    pthis->SetProperties(fn.Env->GetSC(), o1);
    // end offset logic     
}

// ****************************************************************************
// Rectangle.offsetPoint method
///
void GASRectangleProto::OffsetPoint(const GASFnCall& fn)
{
    if (fn.NArgs > 0)
    {
        GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
        GASSERT(pthis);
        GASObject* p = fn.Arg(0).ToObject();
        GASValue o1v[GFxRectangle_NumProperties];
        if ( NULL == p )
        {
            // get properties for this
            pthis->GetProperties(fn.Env->GetSC(), o1v);
            o1v[GFxRectangle_X] = GASNumberUtil::NaN();
            o1v[GFxRectangle_Y] = GASNumberUtil::NaN();
            pthis->SetProperties(fn.Env->GetSC(), o1v);
            return;
        }
        // get properties for parameter
        GASValue o2v[GFxPoint_NumProperties];
        GFxObject_GetPointProperties(fn.Env, p, o2v);
        // if param is not a point, then x and y both must be defined, else return undefined
        if ( (p->GetObjectType() != Object_Point) && (o2v[GFxPoint_X].IsUndefined() || o2v[GFxPoint_Y].IsUndefined()) )
        {
            // get properties for this
            pthis->GetProperties(fn.Env->GetSC(), o1v);
            o1v[GFxRectangle_X] = GASNumberUtil::NaN();
            o1v[GFxRectangle_Y] = GASNumberUtil::NaN();         
            pthis->SetProperties(fn.Env->GetSC(), o1v);
            return;
        }
        // get properties for this
        pthis->GetProperties(fn.Env->GetSC(), o1v);
        // begin contains logic
        o1v[GFxRectangle_X] = o1v[GFxRectangle_X].ToNumber(fn.Env) + o2v[GFxPoint_X].ToNumber(fn.Env);
        o1v[GFxRectangle_Y] = o1v[GFxRectangle_Y].ToNumber(fn.Env) + o2v[GFxPoint_Y].ToNumber(fn.Env);        
        // end contains logic
        pthis->SetProperties(fn.Env->GetSC(), o1v);
    }
}

// ****************************************************************************
// Rectangle.setEmpty method
//
void GASRectangleProto::SetEmpty(const GASFnCall& fn)
{
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    pthis->SetProperties(fn.Env, GASRect(0, 0, 0, 0));
}

// ****************************************************************************
// Rectangle.toString method
//
void GASRectangleProto::ToString(const GASFnCall& fn)
{
    GASRectangleObject* pthis = (GASRectangleObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GASValue params[GFxRectangle_NumProperties];
    pthis->GetProperties(fn.Env->GetSC(), params);

    GASString ps[4] = 
    {
        params[GFxRectangle_X].ToString(fn.Env, 6),
        params[GFxRectangle_Y].ToString(fn.Env, 6),
        params[GFxRectangle_Width].ToString(fn.Env, 6),
        params[GFxRectangle_Height].ToString(fn.Env, 6)
    };

    GFxString str;
    str += "(x=";
    str += ps[0].ToCStr();
    str += ", y=";
    str += ps[1].ToCStr();
    str += ", width=";
    str += ps[2].ToCStr();
    str += ", height=";
    str += ps[3].ToCStr();
    str += ")";

    fn.Result->SetString(fn.Env->CreateString(str));
}

// ****************************************************************************
// Rectangle.union method
//
void GASRectangleProto::Union(const GASFnCall& fn)
{
    GASRectangleObject *pthis = (GASRectangleObject*)fn.ThisPtr;
    GASSERT(pthis);
    GPtr<GASRectangleObject> obj = *new GASRectangleObject(fn.Env);
    fn.Result->SetAsObject(obj.GetPtr());
    if (fn.NArgs > 0)
    {           
        GASRect ret(GASNumberUtil::NaN(), GASNumberUtil::NaN(), GASNumberUtil::NaN(), GASNumberUtil::NaN());
        GASObject* p = fn.Arg(0).ToObject();
        if (p != NULL)
        {
            GASRect o1;
            GASValue o2v[GFxRectangle_NumProperties];
            // get properties for this
            pthis->GetProperties(fn.Env, o1);
            // get properties for parameter
            GFxObject_GetRectangleProperties(fn.Env, p, o2v);
            GASRect o2(o2v[GFxRectangle_X].ToNumber(fn.Env),
                o2v[GFxRectangle_Y].ToNumber(fn.Env),
                GSize<GASNumber>(o2v[GFxRectangle_Width].ToNumber(fn.Env),
                o2v[GFxRectangle_Height].ToNumber(fn.Env)));
            // begin union logic
            o1.UnionRect(&ret, o2);
            if (GASNumberUtil::IsNaN(o2v[GFxRectangle_X].ToNumber(fn.Env)))
            {
                ret.Left = GASNumberUtil::NaN();
            }
            if (GASNumberUtil::IsNaN(o2v[GFxRectangle_Y].ToNumber(fn.Env)))
            {
                ret.Top = GASNumberUtil::NaN();
            }
            // end union logic
        }
        obj.GetPtr()->SetProperties(fn.Env, ret);
    }
    else
    {
        obj.GetPtr()->SetProperties(fn.Env->GetSC(), GFxRectangle_NaNParams);
    }   
}
