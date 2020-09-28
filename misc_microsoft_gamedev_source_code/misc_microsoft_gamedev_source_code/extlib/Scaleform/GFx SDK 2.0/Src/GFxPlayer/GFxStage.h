/**********************************************************************

Filename    :   GFxStage.h
Content     :   Stage class implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSTAGE_H
#define INC_GFXSTAGE_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "GFxObject.h"
#include "GFxPlayerImpl.h"


// ***** Declared Classes
class GASStageObject;
class GASStageProto;
class GASStageCtorFunction;

// ***** External Classes
class GASValue;


// ActionScript Stage objects.

class GASStageObject : public GASObject
{
    friend class GASStageProto;
protected:
    GASStageObject() {}
    GASStageObject(GASStringContext*) {}
public:

    GASStageObject(GASEnvironment* penv);

    virtual ObjectType          GetObjectType() const   { return Object_Stage; }
};

class GASStageProto : public GASPrototype<GASStageObject>
{
public:
    GASStageProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
};

class GASStageCtorFunction : public GASFunctionObject
{
    GWeakPtr<GFxMovieRoot> MovieRoot;
public:
    GASStageCtorFunction(GASStringContext *psc, GFxMovieRoot* movieRoot);

    bool    GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);
    bool    SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags);
    bool    SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags);

    void    NotifyOnResize(GASEnvironment* penv);
    static void NotifyOnResize(const GASFnCall& fn);
};

#endif // INC_GFXSTAGE_H

