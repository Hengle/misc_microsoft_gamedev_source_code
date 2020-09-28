/**********************************************************************

Filename    :   GFxStringObject.h
Content     :   SWF (Shockwave Flash) player library
Created     :   July 7, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXSTRINGOBJECT_H
#define INC_GFXSTRINGOBJECT_H

#include "GFxAction.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASStringObject;
class GASStringProto;
class GASStringCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASStringObject : public GASObject
{
protected:
    GASString Value;

    void commonInit (GASEnvironment* penv);
    
public:
    GASStringObject(GASStringContext *psc)
        : GASObject(), Value(psc->GetBuiltin(GASBuiltin_empty_)) { }
    GASStringObject(GASEnvironment* penv);
    GASStringObject(GASEnvironment* penv, const GASString& val);

    bool                GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val);
    const char*         GetTextValue(GASEnvironment* =0) const;
    ObjectType          GetObjectType() const   { return Object_String; }

    const GASString&    GetString() const               { return Value; }
    void                SetString(const GASString& val) { Value = val; }

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;
};

class GASStringProto : public GASPrototype<GASStringObject>
{
public:
    GASStringProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static void StringCharAt(const GASFnCall& fn);
    static void StringCharCodeAt(const GASFnCall& fn);
    static void StringConcat(const GASFnCall& fn);
    static void StringIndexOf(const GASFnCall& fn);
    static void StringLastIndexOf(const GASFnCall& fn);
    static void StringSlice(const GASFnCall& fn);
    static void StringSplit(const GASFnCall& fn);        
    static void StringSubstr(const GASFnCall& fn);
    static void StringSubstring(const GASFnCall& fn);
    static void StringToLowerCase(const GASFnCall& fn);
    static void StringToString(const GASFnCall& fn);
    static void StringToUpperCase(const GASFnCall& fn);
    static void StringValueOf(const GASFnCall& fn);

    // Helper methods.
    static GASString StringSubstring(const GASString& str, 
                                     int start, int length);
    static GPtr<GASArrayObject> StringSplit(GASEnvironment* penv, const GASString& str,
                                            const char* delimiters, int limit = 0x3FFFFFFF);

    // Creates a ASString based on two char* pointers
    static GASString CreateStringFromCStr(GASStringContext* psc, const char* start, const char* end = 0);
};

class GASStringCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void StringFromCharCode(const GASFnCall& fn);
public:
    GASStringCtorFunction(GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext *psc) const;

    static void GlobalCtor(const GASFnCall& fn);
};


#endif // INC_GFXSTRINGOBJECT_H
