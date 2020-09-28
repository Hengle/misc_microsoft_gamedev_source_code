/**********************************************************************

Filename    :   GFxBoolean.h
Content     :   Boolean object functinality
Created     :   April 10, 2006
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXBOOLEAN_H
#define INC_GFXBOOLEAN_H

#include "GFxAction.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASBooleanObject;
class GASBooleanProto;
class GASBooleanCtorFunction;


class GASBooleanObject : public GASObject
{
protected:
    bool Value;
    
    void CommonInit (GASEnvironment* penv);


public:
    //GASBooleanObject() : GASObject(), Value (0) {}
    GASBooleanObject(GASStringContext* psc = 0) : GASObject(), Value (0) { GUNUSED(psc); }
    GASBooleanObject(GASEnvironment* penv);
    GASBooleanObject(GASEnvironment* penv, bool val);
    virtual const char* GetTextValue(GASEnvironment* penv = 0) const;

    ObjectType      GetObjectType() const   { return Object_Boolean; }

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;

    //
    // This method is used to invoke some methods for primitive number,
    // like it is possible to invoke toString (radix) or valueOf even for non-object Boolean.
    //
    static bool InvokePrimitiveMethod(const GASFnCall& fnCall, const GASString& methodName);
};

class GASBooleanProto : public GASPrototype<GASBooleanObject>
{
public:
    GASBooleanProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);
};

class GASBooleanCtorFunction : public GASFunctionObject
{
public:
    GASBooleanCtorFunction(GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext*) const { return new GASBooleanObject(); }

    static void GlobalCtor(const GASFnCall& fn);
};

#endif //BOOLEAN

