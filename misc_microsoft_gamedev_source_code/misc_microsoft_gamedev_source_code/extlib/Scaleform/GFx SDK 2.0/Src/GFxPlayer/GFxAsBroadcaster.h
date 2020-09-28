/**********************************************************************

Filename    :   GFxAsBroadcaster.h
Content     :   Implementation of AsBroadcaster class
Created     :   October, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXASBROADCASTER_H
#define INC_GFXASBROADCASTER_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASAsBroadcaster;
class GASAsBroadcasterProto;
class GASAsBroadcasterCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASAsBroadcaster : public GASObject
{
protected:
    struct ListenerObject
    {
        GPtr<GASObject>         Object;
        GPtr<GFxASCharacter>    Character;
    };
    GTL::garray<ListenerObject> Listeners;

    void commonInit (GASEnvironment* penv);
    
public:
    GASAsBroadcaster (GASStringContext *psc = 0) : GASObject() { GUNUSED(psc); }
    GASAsBroadcaster (GASEnvironment* penv);

    static bool Initialize(GASStringContext* psc, GASObjectInterface* pobj);
    static bool InitializeProto(GASStringContext* psc, GASObjectInterface* pobj);
    static bool InitializeInstance(GASStringContext* psc, GASObjectInterface* pobj);
    static bool AddListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener);
    static bool RemoveListener(GASEnvironment* penv, GASObjectInterface* pthis, GASObjectInterface* plistener);
    static bool BroadcastMessage(GASEnvironment* penv, GASObjectInterface* pthis, const GASString& eventName, int nArgs, int firstArgBottomIndex);
};

class GASAsBroadcasterProto : public GASPrototype<GASAsBroadcaster>
{
public:
    GASAsBroadcasterProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static void AddListener(const GASFnCall& fn);
    static void BroadcastMessage(const GASFnCall& fn);
    static void RemoveListener(const GASFnCall& fn);
};

class GASAsBroadcasterCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void Initialize (const GASFnCall& fn);
public:
    GASAsBroadcasterCtorFunction (GASStringContext *psc);
};


#endif // INC_GFXASBROADCASTER_H
