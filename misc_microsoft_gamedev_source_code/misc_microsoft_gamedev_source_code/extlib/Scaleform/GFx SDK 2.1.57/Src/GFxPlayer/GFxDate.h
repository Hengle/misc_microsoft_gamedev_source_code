/**********************************************************************

Filename    :   GFxDate.h
Content     :   SWF (Shockwave Flash) player library
Created     :   October 24, 2006
Authors     :   Andrew Reisse

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXDATE_H
#define INC_GFXDATE_H

#include "GFxAction.h"

// GFC_NO_FXPLAYER_AS_DATE disables Date class.
#ifndef GFC_NO_FXPLAYER_AS_DATE


// ***** Declared Classes
class GASDate;
class GASDateProto;
class GASDateCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASDate : public GASObject
{
protected:
    // GMT
    SInt64  Date; // ms +/- 1 Jan 1970
    SInt    Time; // ms within day
    SInt    Year;
    SInt    JDate; // days within year

    // local time
    SInt64  LDate; // ms +/- 1 Jan 1970
    SInt    LTime; // ms within day
    SInt    LYear;
    SInt    LJDate; // days within year

    // time zone
    SInt    LocalOffset;

    void commonInit (GASEnvironment* penv);

    void UpdateLocal();
    void UpdateGMT();

    friend class GASDateProto;
    friend class GASDateCtorFunction;

public:
    
    //GASDate () : GASObject() { GUNUSED(psc); }
    GASDate (GASStringContext *psc = 0) : GASObject() { GUNUSED(psc); }    
    GASDate (GASEnvironment* penv);
    GASDate (GASEnvironment* penv, SInt64 val);

    const char*         GetTextValue(GASEnvironment* =0) const;
    ObjectType          GetObjectType() const   { return Object_Date; }

    void                SetDate(SInt64 utc);
};

class GASDateProto : public GASPrototype<GASDate>
{
public:
    GASDateProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void DateGetDate(const GASFnCall& fn);
    static void DateGetDay(const GASFnCall& fn);
    static void DateGetFullYear(const GASFnCall& fn);
    static void DateGetHours(const GASFnCall& fn);
    static void DateGetMilliseconds(const GASFnCall& fn);
    static void DateGetMinutes(const GASFnCall& fn);
    static void DateGetMonth(const GASFnCall& fn);
    static void DateGetSeconds(const GASFnCall& fn);
    static void DateGetTime(const GASFnCall& fn);
    static void DateGetTimezoneOffset(const GASFnCall& fn);
    static void DateGetYear(const GASFnCall& fn);

    static void DateSetDate(const GASFnCall& fn);
    static void DateSetFullYear(const GASFnCall& fn);
    static void DateSetHours(const GASFnCall& fn);
    static void DateSetMilliseconds(const GASFnCall& fn);
    static void DateSetMinutes(const GASFnCall& fn);
    static void DateSetMonth(const GASFnCall& fn);
    static void DateSetSeconds(const GASFnCall& fn);
    static void DateSetTime(const GASFnCall& fn);
    static void DateSetYear(const GASFnCall& fn);

    static void DateGetUTCDate(const GASFnCall& fn);
    static void DateGetUTCDay(const GASFnCall& fn);
    static void DateGetUTCFullYear(const GASFnCall& fn);
    static void DateGetUTCHours(const GASFnCall& fn);
    static void DateGetUTCMilliseconds(const GASFnCall& fn);
    static void DateGetUTCMinutes(const GASFnCall& fn);
    static void DateGetUTCMonth(const GASFnCall& fn);
    static void DateGetUTCSeconds(const GASFnCall& fn);
    static void DateGetUTCYear(const GASFnCall& fn);

    static void DateSetUTCDate(const GASFnCall& fn);
    static void DateSetUTCFullYear(const GASFnCall& fn);
    static void DateSetUTCHours(const GASFnCall& fn);
    static void DateSetUTCMilliseconds(const GASFnCall& fn);
    static void DateSetUTCMinutes(const GASFnCall& fn);
    static void DateSetUTCMonth(const GASFnCall& fn);
    static void DateSetUTCSeconds(const GASFnCall& fn);
    static void DateSetUTCYear(const GASFnCall& fn);

    static void DateToString(const GASFnCall& fn);
    static void DateValueOf(const GASFnCall& fn);
};

class GASDateCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];

    static void DateUTC(const GASFnCall& fn);

public:
    GASDateCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
};


#endif // GFC_NO_FXPLAYER_AS_DATE
#endif // INC_GFXSTRINGOBJECT_H
