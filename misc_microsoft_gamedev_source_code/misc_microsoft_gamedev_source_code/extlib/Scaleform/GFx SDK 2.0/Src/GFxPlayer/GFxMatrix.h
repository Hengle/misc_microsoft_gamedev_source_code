/**********************************************************************

Filename    :   GFxMatrix.h
Content     :   Matrix class implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXMATRIX_H
#define INC_GFXMATRIX_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "GFxObject.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASMatrixObject;
class GASMatrixProto;

// ***** External Classes
class GASValue;

// Some magic number used for scaling coefficients in CreateGradientBox.
// This number was deduced empirically; in reality it may be more complex
// and depend on something else
const Float GASGradientBoxMagicNumber = 1.0f/1638.4f;

// ActionScript Matrix object

class GASMatrixObject : public GASObject
{
    friend class GASMatrixProto;
protected:
    GASMatrixObject(GASStringContext *psc = 0) { GUNUSED(psc); }
public:
    // a - [0]
    // b - [1]
    // c - [2]
    // d - [3]
    // tx - [4]
    // ty - [5]
    typedef GASValue GASMatrixArray[6];

    GASMatrixObject(GASEnvironment* penv);

    virtual ObjectType          GetObjectType() const   { return Object_Matrix; }

    // marr should be at least GASValue[6]
    GASMatrixArray* GetMatrixAsValuesArray(GASStringContext *psc, GASMatrixArray* marr);

    GMatrix2D GetMatrix(GASEnvironment *env);
    void SetMatrix(GASEnvironment *env, const GMatrix2D& m);

    void SetMatrixTwips(GASStringContext *psc, const GMatrix2D& m);
};

class GASMatrixProto : public GASPrototype<GASMatrixObject>
{
public:
    GASMatrixProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static void ToString           (const GASFnCall& fn);
    static void Clone              (const GASFnCall& fn);
    static void Concat             (const GASFnCall& fn);
    static void CreateBox          (const GASFnCall& fn);
    static void CreateGradientBox  (const GASFnCall& fn);
    static void DeltaTransformPoint(const GASFnCall& fn);
    static void Identity           (const GASFnCall& fn);
    static void Invert             (const GASFnCall& fn);
    static void Rotate             (const GASFnCall& fn);
    static void Scale              (const GASFnCall& fn);
    static void TransformPoint     (const GASFnCall& fn);
    static void Translate          (const GASFnCall& fn);
};


#endif // INC_GFXMATRIX_H

