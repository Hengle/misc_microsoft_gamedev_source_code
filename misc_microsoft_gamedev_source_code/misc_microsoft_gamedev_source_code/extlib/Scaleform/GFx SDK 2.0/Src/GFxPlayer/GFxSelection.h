/**********************************************************************

Filename    :   GFxSelection.h
Content     :   Implementation of Selection class
Created     :   February, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXSELECTION_H
#define INC_GFXSELECTION_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"


// ***** Declared Classes
class GASAsBroadcaster;
class GASAsBroadcasterProto;
class GASAsBroadcasterCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;

class GASSelection : public GASObject
{
protected:

    void commonInit (GASEnvironment* penv);

    static void DoTransferFocus(const GASFnCall& fn);
public:
    GASSelection(GASStringContext *psc = 0) : GASObject() { GUNUSED(psc); }
    GASSelection(GASEnvironment* penv);

    static void QueueSetFocus(GASEnvironment* penv, GFxASCharacter* pNewFocus);
    static void BroadcastOnSetFocus(GASEnvironment* penv, GFxASCharacter* pOldFocus, GFxASCharacter* pNewFocus);
};

class GASSelectionProto : public GASPrototype<GASSelection>
{
public:
    GASSelectionProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
};

//
// Selection static class
//
// A constructor function object for Object
class GASSelectionCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void GetFocus(const GASFnCall& fn);
    static void SetFocus(const GASFnCall& fn);
    static void SetSelection(const GASFnCall& fn);
    static void GetCaretIndex(const GASFnCall& fn);

    // extension methods
    // one parameter, boolean, true - to capture focus, false - to release one.
    // Returns currently focus character.
    static void CaptureFocus(const GASFnCall& fn);
public:
    GASSelectionCtorFunction (GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext*) const { return 0; }

    virtual bool GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval);
    virtual bool SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
};


#endif // INC_GFXSELECTION_H
