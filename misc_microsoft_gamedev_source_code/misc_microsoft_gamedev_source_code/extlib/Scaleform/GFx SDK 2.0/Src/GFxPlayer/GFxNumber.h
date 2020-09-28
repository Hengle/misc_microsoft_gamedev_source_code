/**********************************************************************

Filename    :   GFxNumber.h
Content     :   Number object functinality
Created     :   March 10, 2006
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXNUMBER_H
#define INC_GFXNUMBER_H

#include "GFxAction.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASNumberUtil;
class GASNumberObject;
class GASNumberProto;
class GASNumberCtorFunction;



class GASNumberUtil
{
public:
    static GASNumber NaN();
    static GASNumber POSITIVE_INFINITY();
    static GASNumber NEGATIVE_INFINITY();
    static GASNumber MIN_VALUE();
    static GASNumber MAX_VALUE();
    static GASNumber POSITIVE_ZERO();
    static GASNumber NEGATIVE_ZERO();

#ifndef GFC_NO_DOUBLE
    static inline bool IsNaN(GASNumber v)
    {
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return ((u.I & GUINT64(0x7FF0000000000000)) == GUINT64(0x7FF0000000000000) && (u.I & GUINT64(0xFFFFFFFFFFFFF)));
    }
    static inline bool IsPOSITIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return (u.I == GUINT64(0x7FF0000000000000));
    }
    static inline bool IsNEGATIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return (u.I == GUINT64(0xFFF0000000000000));
    }
    static inline bool IsNaNOrInfinity(GASNumber v)
    {
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt64));
        union
        {
            UInt64  I;
            Double  D;
        } u;
        u.D = v;
        return ((u.I & GUINT64(0x7FF0000000000000)) == GUINT64(0x7FF0000000000000));
    }
#else
    static inline bool IsNaN(GASNumber v)
    {
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return ((u.I & 0x7F800000u) == 0x7F800000u && (u.I & 0x7FFFFFu));
    }
    static inline bool IsPOSITIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return (u.I == 0x7F800000u);
    }
    static inline bool IsNEGATIVE_INFINITY(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return (u.I == 0xFF800000u);
    }
    static inline bool IsNaNOrInfinity(GASNumber v) 
    { 
        GCOMPILER_ASSERT(sizeof(GASNumber) == sizeof(UInt32));
        union
        {
            UInt32  I;
            Float   F;
        } u;
        u.F = v;
        return ((u.I & 0x7F800000u) == 0x7F800000u);
    }
#endif

    static bool IsPOSITIVE_ZERO(GASNumber v);
    static bool IsNEGATIVE_ZERO(GASNumber v);

    // radixPrecision: > 10 - radix
    //                 < 0  - precision (= -radixPrecision)
    static const char* ToString(GASNumber value, GFxString& destStr, int radixPrecision = 10);
};

class GASNumberObject : public GASObject
{
protected:
    GASNumber           Value;
    mutable GFxString   StringValue;

    void CommonInit(GASEnvironment* penv);

public:
    GASNumberObject() : GASObject(), Value(0) { }
    GASNumberObject(GASStringContext*) : GASObject(), Value(0) { }

    GASNumberObject (GASEnvironment* penv);
    GASNumberObject (GASEnvironment* penv, GASNumber val);
    virtual const char* GetTextValue(GASEnvironment* penv = 0) const;
    ObjectType          GetObjectType() const   { return Object_Number; }

    const char* ToString (GFxString& destStr, int radix) const;

    virtual void        SetValue(GASEnvironment* penv, const GASValue&);
    virtual GASValue    GetValue() const;

    //
    // This method is used to invoke some methods for primitive number,
    // like it is possible to invoke toString (radix) or valueOf even for non-object Number.
    //
    static bool InvokePrimitiveMethod (const GASFnCall& fnCall, const GASString& methodName);
};

class GASNumberProto : public GASPrototype<GASNumberObject>
{
public:
    GASNumberProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);
};

class GASNumberCtorFunction : public GASFunctionObject
{
public:
    GASNumberCtorFunction (GASStringContext *psc);

    virtual GASObject* CreateNewObject(GASStringContext*) const { return new GASNumberObject(); }

    static void GlobalCtor(const GASFnCall& fn);
};

#endif

