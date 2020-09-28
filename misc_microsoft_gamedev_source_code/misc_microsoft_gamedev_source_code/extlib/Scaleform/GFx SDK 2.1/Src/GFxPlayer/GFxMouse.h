/**********************************************************************

Filename    :   GFxMouse.h
Content     :   Implementation of Mouse class
Created     :   November, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXMOUSE_H
#define INC_GFXMOUSE_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASAsBroadcaster;
class GASAsBroadcasterProto;
class GASAsBroadcasterCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASMouse : public GASObject
{
protected:

    void commonInit (GASEnvironment* penv);
    
public:
    GASMouse (GASStringContext *psc = 0) : GASObject() { GUNUSED(psc); }
    GASMouse (GASEnvironment* penv);
};

class GASMouseProto : public GASPrototype<GASMouse>
{
public:
    GASMouseProto (GASStringContext* psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
};

class GASMouseCtorFunction : public GASFunctionObject, public GFxASMouseListener
{
    void CleanupListeners();
    void NotifyListeners(GASEnvironment *penv, GASBuiltinType eventName, const GASString* ptargetName = 0, UInt button = 0, int delta = 0) const;
protected:
    enum 
    {
        Mouse_LEFT = 1,
        Mouse_RIGHT = 2,
        Mouse_MIDDLE = 3
    };
    // Hack to store both WeakPtrs and correct interfaces.
    // If weak ptr is null, the interface pointer is unusable.
    GTL::garray<GWeakPtr<GRefCountBaseImpl> >   ListenerWeakRefs;
    GTL::garray<GASObjectInterface*>            Listeners;
    GASFunctionRef                              SetCursorTypeFunc;
    bool                                        MouseExtensions;

    virtual void OnMouseMove(GASEnvironment *penv) const;
    virtual void OnMouseDown(GASEnvironment *penv, UInt button, GFxASCharacter* ptarget) const;
    virtual void OnMouseUp(GASEnvironment *penv, UInt button, GFxASCharacter* ptarget) const;
    virtual void OnMouseWheel(GASEnvironment *penv, int sdelta, GFxASCharacter* ptarget) const;
    virtual bool IsEmpty() const { return Listeners.size() == 0; }

    static const GASNameFunction StaticFunctionTable[];

    static void AddListener(const GASFnCall& fn);
    static void RemoveListener(const GASFnCall& fn);
    static void Hide(const GASFnCall& fn);
    static void Show(const GASFnCall& fn);

    // extension methods
    static void SetCursorType(const GASFnCall& fn);
    // Returns a character at the specified location. Four versions of this method: no params, 
    // 1 param, 2 params and 3 params. Method with no params just look for ANY character under 
    // the mouse pointer (with or without button handlers). Method with 1 parameter takes a boolean 
    // value as the parameter. It also look for character under the mouse pointer, though the 
    // parameter indicates type of characters: true - ANY character, false - only with button 
    // handlers. Version with two parameters takes custom X and Y coordinates in _root coordinate 
    // space and look for ANY character at these coordinates. Version with three params is same 
    // as version with two, the third parameter (boolean) indicates the type of character
    // (any/with button handler).
    static void GetTopMostEntity(const GASFnCall& fn);
public:
    GASMouseCtorFunction (GASStringContext *psc, GFxMovieRoot* proot);

    virtual bool    GetMember(GASEnvironment *penv, const GASString& name, GASValue* val);
    virtual bool    SetMember(GASEnvironment *penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());

    static bool     SetCursorType(GFxMovieRoot* proot, UInt cursorType);
};


#endif // INC_GFXMOUSE_H
