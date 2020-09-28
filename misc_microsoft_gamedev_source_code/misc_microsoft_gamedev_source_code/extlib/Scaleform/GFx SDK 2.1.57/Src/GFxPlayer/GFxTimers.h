/**********************************************************************

Filename    :   GFxTimers.h
Content     :   SWF (Shockwave Flash) player library
Created     :   August, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTIMERS_H
#define INC_GFXTIMERS_H

#include "GFxLog.h"
#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GTimer.h"

class GASIntervalTimer : public GNewOverrideBase
{
    GASFunctionRef              Function;
    GWeakPtr<GASObject>         Object;
    GWeakPtr<GFxASCharacter>    Character;
    GASString                   MethodName;
    GASValueArray               Params;
    int                         Interval;
    Float                       InvokeTime;
    int                         Id;
    GPtr<GFxCharacterHandle>    LevelHandle; // used to store handle on _levelN to get environment
    bool                        Active;
    bool                        Timeout; // indicates to invoke timer only once

    Float           GetNextInterval(Float currentTime, Float frameTime) const;
public:
    GASIntervalTimer(const GASFunctionRef& function, GASStringContext* psc);
    GASIntervalTimer(GASObject* object, const GASString& methodName);
    GASIntervalTimer(GFxASCharacter* character, const GASString& methodName);

    void            Start(GFxMovieRoot* proot);
    bool            Invoke(GFxMovieRoot* proot, Float frameTime);
    inline bool     IsActive() const { return Active; }
    void            Clear();
    inline Float    GetNextInvokeTime() const { return InvokeTime; }

    inline void SetId(int id) { Id = id; }
    inline int  GetId() const { return Id; }

    static void SetInterval(const GASFnCall& fn);
    static void ClearInterval(const GASFnCall& fn);

    static void SetTimeout(const GASFnCall& fn);
    static void ClearTimeout(const GASFnCall& fn);
};

#endif // INC_GFXTIMERS_H
