/**********************************************************************

Filename    :   GFxSoundObject.h
Content     :   Number object functinality
Created     :   October 20, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSOUNDOBJECT_H
#define INC_GFXSOUNDOBJECT_H

#include "GFxAction.h"
#include "GFxObjectProto.h"

//
// sound object
//

class GASSoundObject : public GASObject
{
protected:
    GASSoundObject() : SoundId (0) { }
    GASSoundObject(GASStringContext* ) : SoundId (0) { }
public:
    GFxString   Sound;
    int         SoundId;

    GASSoundObject(GASEnvironment* penv) : SoundId(0)
    {        
        GASStringContext* psc = penv->GetSC();
        Set__proto__(psc, penv->GetPrototype(GASBuiltin_Sound));
    }
};

class GASSoundProto : public GASPrototype<GASSoundObject>
{
public:
    GASSoundProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static void Attach(const GASFnCall& fn);
    static void GetBytesLoaded(const GASFnCall& fn);
    static void GetBytesTotal(const GASFnCall& fn);
    static void GetPan(const GASFnCall& fn);
    static void GetTransform(const GASFnCall& fn);
    static void GetVolume(const GASFnCall& fn);
    static void LoadSound(const GASFnCall& fn);
    static void SetPan(const GASFnCall& fn);
    static void SetTransform(const GASFnCall& fn);
    static void SetVolume(const GASFnCall& fn);
    static void Start(const GASFnCall& fn);
    static void Stop(const GASFnCall& fn);
};

#endif // INC_GFXSOUNDOBJECT_H
