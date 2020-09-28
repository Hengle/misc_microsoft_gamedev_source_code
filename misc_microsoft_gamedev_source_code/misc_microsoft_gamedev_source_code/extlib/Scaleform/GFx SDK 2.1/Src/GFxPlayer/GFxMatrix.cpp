/**********************************************************************

Filename    :   GFxMatrix.h
Content     :   Matrix class implementation
Created     :   
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxMatrix.h"
#include "GFxFunction.h"
#include "GFxValue.h"
#include "GFxPoint.h"

#include "GFxAction.h"

#include <stdio.h>
#include <stdlib.h>


// GFC_NO_FXPLAYER_AS_MATRIX disables Matrix support
#ifndef GFC_NO_FXPLAYER_AS_MATRIX


GASMatrixObject::GASMatrixObject(GASEnvironment* penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Matrix));
    SetMatrix(penv, GMatrix2D());
}

GASMatrixObject::GASMatrixArray* GASMatrixObject::GetMatrixAsValuesArray(GASStringContext *psc, GASMatrixObject::GASMatrixArray* marr)
{
    // a  = Sx         (_M[0][0])
    // b  = Sky        (_M[1][0])
    // c  = Skx        (_M[0][1])
    // d  = Sy         (_M[1][1])
    // tx = Tx         (_M[0][2])
    // ty = Ty         (_M[1][2])
    if (!GetConstMemberRaw(psc, "a",  &(*marr)[0])) (*marr)[0] = 1;
    if (!GetConstMemberRaw(psc, "b",  &(*marr)[1])) (*marr)[1] = 0;
    if (!GetConstMemberRaw(psc, "c",  &(*marr)[2])) (*marr)[2] = 0;
    if (!GetConstMemberRaw(psc, "d",  &(*marr)[3])) (*marr)[3] = 1;
    if (!GetConstMemberRaw(psc, "tx", &(*marr)[4])) (*marr)[4] = 0;
    if (!GetConstMemberRaw(psc, "ty", &(*marr)[5])) (*marr)[5] = 0;
    return marr;
}


GMatrix2D GASMatrixObject::GetMatrix(GASEnvironment *penv)
{
    // a  = Sx         (_M[0][0])
    // b  = Sky        (_M[1][0])
    // c  = Skx        (_M[0][1])
    // d  = Sy         (_M[1][1])
    // tx = Tx         (_M[0][2])
    // ty = Ty         (_M[1][2])
    GMatrix2D m;
    GASValue v;
    GASStringContext *psc = penv->GetSC();
    if (GetConstMemberRaw(psc, "a",  &v)) m.M_[0][0] = (Float)v.ToNumber(penv); else m.M_[0][0] = 1;
    if (GetConstMemberRaw(psc, "b",  &v)) m.M_[1][0] = (Float)v.ToNumber(penv); else m.M_[1][0] = 0;
    if (GetConstMemberRaw(psc, "c",  &v)) m.M_[0][1] = (Float)v.ToNumber(penv); else m.M_[0][1] = 0;
    if (GetConstMemberRaw(psc, "d",  &v)) m.M_[1][1] = (Float)v.ToNumber(penv); else m.M_[1][1] = 1;
    if (GetConstMemberRaw(psc, "tx", &v)) m.M_[0][2] = (Float)v.ToNumber(penv); else m.M_[0][2] = 0;
    if (GetConstMemberRaw(psc, "ty", &v)) m.M_[1][2] = (Float)v.ToNumber(penv); else m.M_[1][2] = 0;
    return m;
}

void GASMatrixObject::SetMatrix(GASEnvironment* penv, const GMatrix2D& m)
{
    // a  = Sx         (_M[0][0])
    // b  = Sky        (_M[1][0])
    // c  = Skx        (_M[0][1])
    // d  = Sy         (_M[1][1])
    // tx = Tx         (_M[0][2])
    // ty = Ty         (_M[1][2])
    GASStringContext *psc = penv->GetSC();
    SetConstMemberRaw(psc, "a",  GASValue(m.M_[0][0]));
    SetConstMemberRaw(psc, "b",  GASValue(m.M_[1][0]));
    SetConstMemberRaw(psc, "c",  GASValue(m.M_[0][1]));
    SetConstMemberRaw(psc, "d",  GASValue(m.M_[1][1]));
    SetConstMemberRaw(psc, "tx", GASValue(m.M_[0][2]));
    SetConstMemberRaw(psc, "ty", GASValue(m.M_[1][2]));
}

void GASMatrixObject::SetMatrixTwips(GASStringContext* psc, const GMatrix2D& m)
{
    // a  = Sx         (_M[0][0])
    // b  = Sky        (_M[1][0])
    // c  = Skx        (_M[0][1])
    // d  = Sy         (_M[1][1])
    // tx = Tx         (_M[0][2])
    // ty = Ty         (_M[1][2])
    SetConstMemberRaw(psc, "a",  GASValue(m.M_[0][0]));
    SetConstMemberRaw(psc, "b",  GASValue(m.M_[1][0]));
    SetConstMemberRaw(psc, "c",  GASValue(m.M_[0][1]));
    SetConstMemberRaw(psc, "d",  GASValue(m.M_[1][1]));
    SetConstMemberRaw(psc, "tx", GASValue(TwipsToPixels(Double(m.M_[0][2]))));
    SetConstMemberRaw(psc, "ty", GASValue(TwipsToPixels(Double(m.M_[1][2]))));
}

void GASMatrixProto::ToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;     
    GASSERT(pthis);
    if (!pthis) return;

    GASMatrixArray matrix;
    pthis->GetMatrixAsValuesArray(fn.Env->GetSC(), &matrix);
    
    // AddRef strings since we can't call ToCStr() on a temp primitive.
    GASString mxs[6] =
    {
        matrix[0].ToPrimitive(fn.Env).ToString(fn.Env, 6),
        matrix[1].ToPrimitive(fn.Env).ToString(fn.Env, 6),
        matrix[2].ToPrimitive(fn.Env).ToString(fn.Env, 6),
        matrix[3].ToPrimitive(fn.Env).ToString(fn.Env, 6),
        matrix[4].ToPrimitive(fn.Env).ToString(fn.Env, 6),
        matrix[5].ToPrimitive(fn.Env).ToString(fn.Env, 6)
    };

    GFxString str;
    str += "(a=";
    str += mxs[0].ToCStr();
    str += ", b=";
    str += mxs[1].ToCStr();
    str += ", c=";
    str += mxs[2].ToCStr();
    str += ", d=";
    str += mxs[3].ToCStr();
    str += ", tx=";
    str += mxs[4].ToCStr();
    str += ", ty=";
    str += mxs[5].ToCStr();
    str += ")";

    fn.Result->SetString(fn.Env->CreateString(str));
}

void GASMatrixProto::Clone(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    // clone() : Matrix
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) 
    {
        fn.Result->SetUndefined();
        return;
    }

    GPtr<GASMatrixObject> m = *new GASMatrixObject(fn.Env);

    m->SetMatrix(fn.Env, pthis->GetMatrix(fn.Env));
    fn.Result->SetAsObject(m.GetPtr());
}

void GASMatrixProto::Concat(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    // concat(m:Matrix) : Void
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs > 0)
    {
        GASObject* arg = fn.Arg(0).ToObject();
        if(arg->GetObjectType() == Object_Matrix)
        {
            GMatrix2D m = pthis->GetMatrix(fn.Env);
            m.Append(((GASMatrixObject*)arg)->GetMatrix(fn.Env));
            pthis->SetMatrix(fn.Env, m);
        }
    }
}

void GASMatrixProto::CreateBox(const GASFnCall& fn)
{
    // createBox(scaleX:Number, scaleY:Number, [rotation:Number], [tx:Number], [ty:Number]) : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs > 1)
    {
        GMatrix2D m;
        Float sx  = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float sy  = (Float)(fn.Arg(1).ToNumber(fn.Env));
        Float r   = 0;
        Float tx  = 0;
        Float ty  = 0;
        if (fn.NArgs > 2)
        {
            r = (Float)(fn.Arg(2).ToNumber(fn.Env));
            if (fn.NArgs > 3)
            {
                tx = (Float)(fn.Arg(3).ToNumber(fn.Env));
                if (fn.NArgs > 4)
                {
                    ty = (Float)(fn.Arg(4).ToNumber(fn.Env));
                }
            }
        }

        m.AppendRotation(r);
        m.AppendScaling(sx, sy);
        m.AppendTranslation(tx, ty);

        pthis->SetMatrix(fn.Env, m);
    }
}

void GASMatrixProto::CreateGradientBox(const GASFnCall& fn)
{
    // createGradientBox(width:Number, height:Number, [rotation:Number], [tx:Number], [ty:Number]) : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs > 1)
    {
        GMatrix2D m;
        Float w  = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float h  = (Float)(fn.Arg(1).ToNumber(fn.Env));
        Float r  = 0;
        Float tx = w / 2;
        Float ty = h / 2;
        if (fn.NArgs > 2)
        {
            r = (Float)(fn.Arg(2).ToNumber(fn.Env));
            if (fn.NArgs > 3)
            {
                tx += (Float)(fn.Arg(3).ToNumber(fn.Env));
                if (fn.NArgs > 4)
                {
                    ty += (Float)(fn.Arg(4).ToNumber(fn.Env));
                }
            }
        }

        w *= GASGradientBoxMagicNumber; 
        h *= GASGradientBoxMagicNumber;

        m.AppendRotation(r);
        m.AppendScaling(w, h);
        m.AppendTranslation(tx, ty);

        pthis->SetMatrix(fn.Env, m);
    }
}

void GASMatrixProto::Identity(const GASFnCall& fn)
{
    // identity() : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    pthis->SetMatrix(fn.Env, GMatrix2D());
}

void GASMatrixProto::Invert(const GASFnCall& fn)
{
    // invert() : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    pthis->SetMatrix(fn.Env, pthis->GetMatrix(fn.Env).Invert());
}

void GASMatrixProto::Rotate(const GASFnCall& fn)
{
    // rotate(angle:Number) : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs > 0)
    {
        Float r = (Float)(fn.Arg(0).ToNumber(fn.Env));

        GMatrix2D m(pthis->GetMatrix(fn.Env));
        m.AppendRotation(r);

        pthis->SetMatrix(fn.Env, m);
    }
}

void GASMatrixProto::Scale(const GASFnCall& fn)
{
    // scale(sx:Number, sy:Number) : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs > 1)
    {
        Float sx = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float sy = (Float)(fn.Arg(1).ToNumber(fn.Env));

        GMatrix2D m(pthis->GetMatrix(fn.Env));
        m.AppendScaling(sx, sy);

        pthis->SetMatrix(fn.Env, m);
    }

// Just some tests of GMatrix2D.
//GMatrix2D m1(0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
//m1.Rotate(1);
//m1.Scale(1.3, 1.8);
//m1.Rotate(-1.2);
//
//GMatrix2D m2 = m1;
//m1.CustomTranslate(-3.3, 4.4);
//m2.PrependTranslation(-3.3, 4.4);
//
//char b1[1024];
//char b2[1024];
//sprintf(b1, "%f %f %f %f %f %f", m1.M_[0][0],m1.M_[0][1],m1.M_[0][2],m1.M_[1][0],m1.M_[1][1],m1.M_[1][2]);
//sprintf(b2, "%f %f %f %f %f %f", m2.M_[0][0],m2.M_[0][1],m2.M_[0][2],m2.M_[1][0],m2.M_[1][1],m2.M_[1][2]);
//GMatrix2D m1(0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
//GMatrix2D m2;
//m2.Rotate(1);
//m1.Multiply(m2);
//
//GMatrix2D m1(0.1, 0.2, 0.3, 0.4, 0.5, 0.6);
//GMatrix2D m2;
//m2.Rotate(1);
//m2.Multiply(m1);
//
//m1.M_[0][0] = 0;
}

void GASMatrixProto::Translate(const GASFnCall& fn)
{
    // translate(tx:Number, ty:Number) : Void
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    if (fn.NArgs > 1)
    {
        Float tx = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float ty = (Float)(fn.Arg(1).ToNumber(fn.Env));

        GMatrix2D m(pthis->GetMatrix(fn.Env));
        m.AppendTranslation(tx, ty);

        pthis->SetMatrix(fn.Env, m);
    }
}

void GASMatrixProto::TransformPoint(const GASFnCall& fn)
{
#ifndef GFC_NO_FXPLAYER_AS_POINT

    // transformPoint(pt:Point) : Point
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GASObject* arg = 0;
    if (fn.NArgs > 0 && 
       (arg = fn.Arg(0).ToObject())->GetObjectType() == GASObjectInterface::Object_Point)
    {
        GMatrix2D m = pthis->GetMatrix(fn.Env);
        GASValue ptprop[GFxPoint_NumProperties];
        ((GASPointObject*)arg)->GetProperties(fn.Env->GetSC(), ptprop);

        GPtr<GASPointObject> ppt = *new GASPointObject(fn.Env);
        GASValue nptprop[GFxPoint_NumProperties];
        // begin transform point logic
        GASValue x1 = GASValue(m.M_[0][0]);
        x1.Mul(fn.Env, ptprop[GFxPoint_X]);
        GASValue x2 = GASValue(m.M_[0][1]);
        x2.Mul(fn.Env, ptprop[GFxPoint_Y]);
        nptprop[GFxPoint_X] = GASValue(m.M_[0][2]);
        nptprop[GFxPoint_X].Add(fn.Env, x1);
        nptprop[GFxPoint_X].Add(fn.Env, x2);
        
        x1 = GASValue(m.M_[1][0]);
        x1.Mul(fn.Env, ptprop[GFxPoint_X]);
        x2 = GASValue(m.M_[1][1]);
        x2.Mul(fn.Env, ptprop[GFxPoint_Y]);
        nptprop[GFxPoint_Y] = GASValue(m.M_[1][2]);
        nptprop[GFxPoint_Y].Add(fn.Env, x1);
        nptprop[GFxPoint_Y].Add(fn.Env, x2);
        
        /** Maxim's code
        ppt->SetPoint(fn.Env, 
                      GPoint2D(m.M_[0][0] * p.GetX() + m.M_[0][1] * p.GetY() + m.M_[0][2],
                               m.M_[1][0] * p.GetX() + m.M_[1][1] * p.GetY() + m.M_[1][2]));
        **/
        // end transform point logic
        ppt->SetProperties(fn.Env->GetSC(), nptprop);
        fn.Result->SetAsObject(ppt.GetPtr());
    }
#else

    GFxLog* log = fn.GetLog();
    if (log != NULL)
        log->LogScriptError("Point ActionScript class was not included in this GFx build.\n");

#endif  // GFC_NO_FXPLAYER_AS_POINT
}

void GASMatrixProto::DeltaTransformPoint(const GASFnCall& fn)
{
#ifndef GFC_NO_FXPLAYER_AS_POINT

    // deltaTransformPoint(pt:Point) : Point
    CHECK_THIS_PTR(fn, Matrix, "Matrix");
    GASMatrixObject* pthis = (GASMatrixObject*) fn.ThisPtr;
    GASSERT(pthis);
    if (!pthis) return;

    GASObject* arg = 0;
    if (fn.NArgs > 0 && 
       (arg = fn.Arg(0).ToObject())->GetObjectType() == GASObjectInterface::Object_Point)
    {
        GMatrix2D m = pthis->GetMatrix(fn.Env);
        GASValue ptprop[GFxPoint_NumProperties];
        ((GASPointObject*)arg)->GetProperties(fn.Env->GetSC(), ptprop);

        GPtr<GASPointObject> ppt = *new GASPointObject(fn.Env);
        GASValue nptprop[GFxPoint_NumProperties];
        // begin delta transform point logic
        GASValue x1 = GASValue(m.M_[0][0]);
        x1.Mul(fn.Env, ptprop[GFxPoint_X]);
        nptprop[GFxPoint_X] = GASValue(m.M_[0][1]);
        nptprop[GFxPoint_X].Mul(fn.Env, ptprop[GFxPoint_Y]);
        nptprop[GFxPoint_X].Add(fn.Env, x1);

        x1 = GASValue(m.M_[1][0]);
        x1.Mul(fn.Env, ptprop[GFxPoint_X]);
        nptprop[GFxPoint_Y] = GASValue(m.M_[1][1]);
        nptprop[GFxPoint_Y].Mul(fn.Env, ptprop[GFxPoint_Y]);
        nptprop[GFxPoint_Y].Add(fn.Env, x1);
        /** Maxim's code
        ppt->SetPoint(fn.Env, 
                      GPoint2D(m.M_[0][0] * p.GetX() + m.M_[0][1] * p.GetY(),
                               m.M_[1][0] * p.GetX() + m.M_[1][1] * p.GetY()));
        **/
        // end delta transform point logic
        ppt->SetProperties(fn.Env->GetSC(), nptprop);
        fn.Result->SetAsObject(ppt.GetPtr());
    }
#else

    GFxLog* log = fn.GetLog();
    if (log != NULL)
        log->LogScriptError("Point ActionScript class was not included in this GFx build.\n");

#endif  // GFC_NO_FXPLAYER_AS_POINT
}

static const GASNameFunction GAS_MatrixFunctionTable[] = 
{
    { "toString",            &GASMatrixProto::ToString   },
    { "valueOf",             &GASMatrixProto::ToString   },

    { "clone",               &GASMatrixProto::Clone },
    { "concat",              &GASMatrixProto::Concat },
    { "createBox",           &GASMatrixProto::CreateBox },
    { "createGradientBox",   &GASMatrixProto::CreateGradientBox },
    { "deltaTransformPoint", &GASMatrixProto::DeltaTransformPoint },
    { "identity",            &GASMatrixProto::Identity },
    { "invert",              &GASMatrixProto::Invert },
    { "rotate",              &GASMatrixProto::Rotate },
    { "scale",               &GASMatrixProto::Scale },
    { "transformPoint",      &GASMatrixProto::TransformPoint },
    { "translate",           &GASMatrixProto::Translate },

    { 0, 0 }
};

 



GASMatrixProto::GASMatrixProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASMatrixObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_MatrixFunctionTable, pprototype);
}

void GASMatrixProto::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASMatrixObject> pmatrix = *new GASMatrixObject(fn.Env);
    fn.Result->SetAsObject(pmatrix.GetPtr());

    if (fn.NArgs > 0)
    {   
        GASStringContext* psc = fn.Env->GetSC();

        pmatrix->SetConstMemberRaw(psc, "a", fn.Arg(0));
        if (fn.NArgs > 1)
        {
            pmatrix->SetConstMemberRaw(psc, "b", fn.Arg(1));
            if (fn.NArgs > 2)
            {
                pmatrix->SetConstMemberRaw(psc, "c", fn.Arg(2));
                if (fn.NArgs > 3)
                {
                    pmatrix->SetConstMemberRaw(psc, "d", fn.Arg(3));
                    if (fn.NArgs > 4)
                    {
                        pmatrix->SetConstMemberRaw(psc, "tx", fn.Arg(4));
                        if (fn.NArgs > 5)
                        {
                            pmatrix->SetConstMemberRaw(psc, "ty", fn.Arg(5));
                        }
                    }
                }
            }
        }
    }
}


#else

void GASMatrix_DummyFunction() {}   // Exists to quelch compiler warning

#endif  // GFC_NO_FXPLAYER_AS_MATRIX

