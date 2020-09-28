/**********************************************************************

Filename    :   GFxExternalInterface.h
Content     :   ExternalInterface AS class implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXEXTINTF_H
#define INC_GFXEXTINTF_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "GFxObject.h"
#include "GFxPlayerImpl.h"


// ***** Declared Classes
class GASExternalInterface;
class GASExternalInterfaceProto;
class GASExternalInterfaceCtorFunction;

// ***** External Classes
class GASValue;


// ActionScript Stage objects.

class GASExternalInterface : public GASObject
{
    friend class GASExternalInterfaceProto;
protected:
    GASExternalInterface() {}
    GASExternalInterface(GASStringContext*) {}
public:

    GASExternalInterface(GASEnvironment* penv);

    virtual ObjectType          GetObjectType() const   { return Object_Stage; }
};

class GASExternalInterfaceProto : public GASPrototype<GASExternalInterface>
{
public:
    GASExternalInterfaceProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
};

class GASExternalInterfaceCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];
public:
    GASExternalInterfaceCtorFunction(GASStringContext *psc);

    bool    GetMember(GASEnvironment *penv, const GASString& name, GASValue* val);
    
    static void AddCallback(const GASFnCall& fn);
    static void Call(const GASFnCall& fn);
};

#endif // INC_GFXEXTINTF_H

