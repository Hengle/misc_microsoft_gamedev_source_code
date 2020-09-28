/**********************************************************************

Filename    :   GFxColor.h
Content     :   Text character implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXCOLOR_H
#define INC_GFXCOLOR_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "GFxObject.h"

#include "GFxAction.h"
#include "GFxObjectProto.h"

// ***** Declared Classes
class GASColorObject;
class GASColorProto;


// ActionScript Color object

class GASColorObject : public GASObject
{
    friend class GASColorProto;
    GWeakPtr<GFxASCharacter> pCharacter;
protected:
    GASColorObject(GASStringContext *psc = 0) { GUNUSED(psc); }
public:
    GASColorObject(GASEnvironment* penv, GFxASCharacter *pcharacter)
    {
        pCharacter = pcharacter;
        Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Color));
    }
    virtual ObjectType          GetObjectType() const   { return Object_Color; }
};

class GASColorProto : public GASPrototype<GASColorObject>
{
public:
    GASColorProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    // NOTE: SetTransform allows for negative color transforms (inverse mul/color subtract?).
    // TODO: Such transforms probably won't work in renderer -> needs testing!

    static void SetTransform(const GASFnCall& fn);
    static void GetTransform(const GASFnCall& fn);
    static void SetRGB(const GASFnCall& fn);
    static void GetRGB(const GASFnCall& fn);
};


#endif // INC_GFXCOLOR_H

