/**********************************************************************

Filename    :   GFxDate.cpp
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

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxASString.h"
#include "GFxAction.h"
#include "GFxArray.h"
#include "GFxNumber.h"
#include "GFxDate.h"

// GFC_NO_FXPLAYER_AS_DATE disables Date class.
#ifndef GFC_NO_FXPLAYER_AS_DATE

#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
#include <sys/timeb.h>
# if defined(GFC_OS_WIN32)
#include <windows.h>
# endif
#elif defined(GFC_OS_WINCE)
#include <windows.h>
#elif defined(GFC_OS_PS3)
#include <cell/rtc.h>
#elif defined(GFC_OS_PS2)
#include <sifdev.h>
#elif defined(GFC_OS_PSP)
#include <rtcsvc.h>
#elif defined(GFC_OS_WII)
#include <revolution/os.h>
#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)
#include <dk_time.h>
#else
#include <sys/time.h>
#endif

GASDate::GASDate(GASEnvironment* penv)
{
    commonInit(penv);
}

GASDate::GASDate(GASEnvironment* penv, SInt64 val)
{
    SetDate(val);
    commonInit(penv);
}

void GASDate::commonInit (GASEnvironment* penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Date));
}

const char* GASDate::GetTextValue(GASEnvironment*) const
{
    return "Date";
}

//////////////////////////////////////////
//

static inline bool IsLeapYear(SInt y)
{
    return 0 == (y % 4) && ((y % 100) || 0 == (y % 400));
}

static inline SInt StartOfYear (SInt y)
{
    y -= 1970;
    return 365 * y + ((y+1) / 4) - ((y+69) / 100) + ((y+369) / 400);
}

void GASDate::UpdateLocal()
{
    LTime = Time + LocalOffset;
    LDate = Date + LocalOffset;
    LJDate = JDate;
    LYear = Year;
    if (LTime >= 86400000 || LTime < 0)
    {
        SInt dd = ((LTime + 864000000) / 86400000) - 10;
        LJDate += dd;
        LTime -= dd * 86400000;
        if (LJDate >= (IsLeapYear(LYear) ? 366 : 365))
        {
            LJDate -= IsLeapYear(LYear) ? 366 : 365;
            LYear++;
        }
        else if (LJDate < 0)
        {
            LYear--;
            LJDate += IsLeapYear(LYear) ? 366 : 365;
        }
    }
}

void GASDate::UpdateGMT()
{
    Time = LTime - LocalOffset;
    Date = LDate - LocalOffset;
    JDate = LJDate;
    Year = LYear;
    if (Time >= 86400000 || Time < 0)
    {
        SInt dd = ((Time + 864000000) / 86400000) - 10;
        JDate += dd;
        Time -= dd * 86400000;
        if (JDate >= (IsLeapYear(Year) ? 366 : 365))
        {
            JDate -= IsLeapYear(Year) ? 366 : 365;
            Year++;
        }
        else if (JDate < 0)
        {
            Year--;
            JDate += IsLeapYear(Year);
        }
    }
}

void GASDate::SetDate(SInt64 val)
{
    SInt64 days = val / 86400000;
    Time = SInt32(val % 86400000);
    SInt64 qcents = days / (400 * 365 + 97);
    SInt64 qcdays = days % (400 * 365 + 97);
    Year = 1970 + SInt32(qcents * 400);
    while (qcdays >= (IsLeapYear(Year) ? 366 : 365))
    {
        qcdays -= IsLeapYear(Year) ? 366 : 365;
        Year++;
    }
    JDate = SInt(qcdays);
    Date = val;
    UpdateLocal();
}

void GASDateProto::DateGetTimezoneOffset(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(-pThis->LocalOffset / 60000) );
}

static const int months[2][12] = {{31,59,90,120,151,181,212,243,273,304,334,365},
                                  {31,60,91,121,152,182,213,244,274,305,335,366}};

void GASDateProto::DateGetMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    for (int i = 0; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            fn.Result->SetNumber( GASNumber(i) );
            return;
        }
    fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][0])
    {
        fn.Result->SetNumber( GASNumber(1 + pThis->LJDate) );
        return;
    }

    for (int i = 1; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            fn.Result->SetNumber( GASNumber(1 + pThis->LJDate - months[IsLeapYear(pThis->LYear)][i-1]) );
            return;
        }
    fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetDay(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber((4 + SInt(pThis->LDate / 86400000)) % 7) );
}

void GASDateProto::DateGetTime(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber((GASNumber)pThis->Date);
}

void GASDateProto::DateGetHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LTime / 3600000) );
}

void GASDateProto::DateGetMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber((pThis->LTime % 3600000) / 60000) );
}

void GASDateProto::DateGetSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber((pThis->LTime % 60000) / 1000) );
}

void GASDateProto::DateGetMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LTime % 1000) );
}

void GASDateProto::DateGetYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LYear - 1900) );
}

void GASDateProto::DateGetFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber( GASNumber(pThis->LYear) );
}

void GASDateProto::DateSetYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (newVal < 100 && newVal >= 0)
        newVal += 1900;
    if (pThis->LJDate >= 60)
        pThis->LJDate += IsLeapYear(newVal) - IsLeapYear(pThis->LYear);
    pThis->LDate = SInt64(pThis->LTime) + SInt64(StartOfYear(newVal) + pThis->LJDate) * SInt64(86400000);
    pThis->LYear = newVal;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (pThis->LJDate >= 60)
        pThis->LJDate += IsLeapYear(newVal) - IsLeapYear(pThis->LYear);
    pThis->LDate = SInt64(pThis->LTime) + SInt64(StartOfYear(newVal) + pThis->LJDate) * SInt64(86400000);
    pThis->LYear = newVal;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
       if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
       {
                int d = months[IsLeapYear(pThis->LYear)][newVal] - months[IsLeapYear(pThis->LYear)][i];
                pThis->LJDate += d;
                pThis->LDate += SInt64(d) * SInt64(86400000);
                pThis->UpdateGMT();
                return;
       }
}

void GASDateProto::DateSetDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            int d = newVal - (pThis->LJDate - (i ? months[IsLeapYear(pThis->LYear)][i-1] : 0)) - 1;
            pThis->LJDate += d;
            pThis->LDate += SInt64(d) * SInt64(86400000);
            pThis->UpdateGMT();
            return;
        }
}

void GASDateProto::DateSetTime(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    pThis->SetDate((SInt64) fn.Arg(0).ToNumber(fn.Env));
}

void GASDateProto::DateSetHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - pThis->LTime / 3600000) * 3600000;
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->LTime % 3600000) / 60000)) * 60000;
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->LTime % 60000) / 1000)) * 1000;
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}

void GASDateProto::DateSetMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = newVal - (pThis->LTime % 1000);
    pThis->LDate += d;
    pThis->LTime += d;
    pThis->UpdateGMT();
}


void GASDateProto::DateGetUTCMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    for (int i = 0; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            fn.Result->SetNumber(GASNumber(i));
            return;
        }
        fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetUTCDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (pThis->JDate < months[IsLeapYear(pThis->Year)][0])
    {
        fn.Result->SetNumber(GASNumber(1 + pThis->JDate));
        return;
    }

    for (int i = 1; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            fn.Result->SetNumber(GASNumber(1 + pThis->JDate - months[IsLeapYear(pThis->Year)][i-1]));
            return;
        }
        fn.Result->SetNumber(-1);
}

void GASDateProto::DateGetUTCDay(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber((4 + SInt(pThis->Date / 86400000)) % 7));
}

void GASDateProto::DateGetUTCHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Time / 3600000));
}

void GASDateProto::DateGetUTCMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber((pThis->Time % 3600000) / 60000));
}

void GASDateProto::DateGetUTCSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber((pThis->Time % 60000) / 1000));
}

void GASDateProto::DateGetUTCMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Time % 1000));
}

void GASDateProto::DateGetUTCYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Year - 1900));
}

void GASDateProto::DateGetUTCFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber(GASNumber(pThis->Year));
}

void GASDateProto::DateSetUTCYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (newVal < 100 && newVal >= 0)
        newVal += 1900;
    if (pThis->JDate >= 60)
        pThis->JDate += IsLeapYear(newVal) - IsLeapYear(pThis->Year);
    pThis->Date = SInt64(pThis->Time) + SInt64(StartOfYear(newVal) + pThis->JDate) * SInt64(86400000);
    pThis->Year = newVal;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCFullYear(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    if (pThis->JDate >= 60)
        pThis->JDate += IsLeapYear(newVal) - IsLeapYear(pThis->Year);
    pThis->Date = SInt64(pThis->Time) + SInt64(StartOfYear(newVal) + pThis->JDate) * SInt64(86400000);
    pThis->Year = newVal;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCMonth(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            int d = months[IsLeapYear(pThis->Year)][newVal] - months[IsLeapYear(pThis->Year)][i];
            pThis->JDate += d;
            pThis->Date += SInt64(d) * SInt64(86400000);
            pThis->UpdateLocal();
            return;
        }
}

void GASDateProto::DateSetUTCDate(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    for (int i = 0; i < 12; i++)
        if (pThis->JDate < months[IsLeapYear(pThis->Year)][i])
        {
            int d = newVal - (pThis->JDate - (i ? months[IsLeapYear(pThis->Year)][i-1] : 0)) - 1;
            pThis->JDate += d;
            pThis->Date += SInt64(d) * SInt64(86400000);
            pThis->UpdateLocal();
            return;
        }
}
void GASDateProto::DateSetUTCHours(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - pThis->Time / 3600000) * 3600000;
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCMinutes(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->Time % 3600000) / 60000)) * 60000;
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCSeconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = (newVal - ((pThis->Time % 60000) / 1000)) * 1000;
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

void GASDateProto::DateSetUTCMilliseconds(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    if (fn.NArgs < 1)
        return;
    int newVal = (int) fn.Arg(0).ToNumber(fn.Env);
    int d = newVal - (pThis->Time % 1000);
    pThis->Date += d;
    pThis->Time += d;
    pThis->UpdateLocal();
}

static const char *daynames[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *monthnames[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

void GASDateProto::DateToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);
    SInt month=0, day=0;

    for (int i = 0; i < 12; i++)
        if (pThis->LJDate < months[IsLeapYear(pThis->LYear)][i])
        {
            month = i;
            day = 1 + pThis->LJDate - (i ? months[IsLeapYear(pThis->LYear)][i-1] : 0);
            break;
        }

    char out[128];
    gfc_sprintf(out, sizeof(out), 
        "%s %s %2d %02d:%02d:%02d GMT%+03d%02d %d", daynames[(4 + pThis->LDate / 86400000) % 7], monthnames[month], day, 
        pThis->LTime / 3600000, (pThis->LTime % 3600000) / 60000, (pThis->LTime % 60000) / 1000,
        pThis->LocalOffset / 3600000, (pThis->LocalOffset % 3600000) / 60000, pThis->LYear);

    fn.Result->SetString(fn.Env->CreateString(out));
}

void GASDateProto::DateValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Date, "Date");
    GASDate* pThis = (GASDate*) fn.ThisPtr;
    GASSERT(pThis);

    fn.Result->SetNumber((GASNumber)pThis->Date);
}

static const GASNameFunction GAS_DateFunctionTable[] = 
{
    { "getDate",            &GASDateProto::DateGetDate             },
    { "getDay",             &GASDateProto::DateGetDay              },
    { "getFullYear",        &GASDateProto::DateGetFullYear         },
    { "getHours",           &GASDateProto::DateGetHours            },
    { "getMilliseconds",    &GASDateProto::DateGetMilliseconds     },
    { "getMinutes",         &GASDateProto::DateGetMinutes          },
    { "getMonth",           &GASDateProto::DateGetMonth            },
    { "getSeconds",         &GASDateProto::DateGetSeconds          },
    { "getTime",            &GASDateProto::DateGetTime             },
    { "getTimezoneOffset",  &GASDateProto::DateGetTimezoneOffset   },
    { "getYear",            &GASDateProto::DateGetYear             },
    { "getUTCDate",         &GASDateProto::DateGetUTCDate          },
    { "getUTCDay",          &GASDateProto::DateGetUTCDay           },
    { "getUTCFullYear",     &GASDateProto::DateGetUTCFullYear      },
    { "getUTCHours",        &GASDateProto::DateGetUTCHours         },
    { "getUTCMilliseconds", &GASDateProto::DateGetUTCMilliseconds  },
    { "getUTCMinutes",      &GASDateProto::DateGetUTCMinutes       },
    { "getUTCMonth",        &GASDateProto::DateGetUTCMonth         },
    { "getUTCSeconds",      &GASDateProto::DateGetUTCSeconds       },
    { "getUTCYear",         &GASDateProto::DateGetUTCYear          },

    { "setDate",            &GASDateProto::DateSetDate             },
    { "setFullYear",        &GASDateProto::DateSetFullYear         },
    { "setHours",           &GASDateProto::DateSetHours            },
    { "setMilliseconds",    &GASDateProto::DateSetMilliseconds     },
    { "setMinutes",         &GASDateProto::DateSetMinutes          },
    { "setMonth",           &GASDateProto::DateSetMonth            },
    { "setSeconds",         &GASDateProto::DateSetSeconds          },
    { "setTime",            &GASDateProto::DateSetTime             },
    { "setYear",            &GASDateProto::DateSetYear             },
    { "setUTCDate",         &GASDateProto::DateSetUTCDate          },
    { "setUTCFullYear",     &GASDateProto::DateSetUTCFullYear      },
    { "setUTCHours",        &GASDateProto::DateSetUTCHours         },
    { "setUTCMilliseconds", &GASDateProto::DateSetUTCMilliseconds  },
    { "setUTCMinutes",      &GASDateProto::DateSetUTCMinutes       },
    { "setUTCSeconds",      &GASDateProto::DateSetUTCSeconds       },
    { "setUTCMonth",        &GASDateProto::DateSetUTCMonth         },
    { "setUTCYear",         &GASDateProto::DateSetUTCYear          },

    { "toString",     &GASDateProto::DateToString     },
    { "valueOf",      &GASDateProto::DateValueOf      },
    { 0, 0 }
};

GASDateProto::GASDateProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor)
    : GASPrototype<GASDate>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, GAS_DateFunctionTable, pprototype);
}

//////////////////
const GASNameFunction GASDateCtorFunction::StaticFunctionTable[] = 
{
    { "UTC", &GASDateCtorFunction::DateUTC },
    { 0, 0 }
};

GASDateCtorFunction::GASDateCtorFunction(GASStringContext *psc) : GASFunctionObject(GlobalCtor)
{
    for(int i = 0; StaticFunctionTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, StaticFunctionTable[i].Name, GASValue (StaticFunctionTable[i].Function), 
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASDateCtorFunction::DateUTC (const GASFnCall& fn)
{
    if (fn.NArgs >= 2)
    {
        SInt i = (SInt)fn.Arg(0).ToNumber(fn.Env);
        SInt y = i > 99 || i < 0 ? i : 1900 + i;
        GASNumber days = GASNumber(StartOfYear(y));
        GASNumber ms = 0;
        i = (SInt)fn.Arg(1).ToNumber(fn.Env);
        if (i)
            days += months[IsLeapYear(y)][i-1];
        if (fn.NArgs >= 3)
            days += (SInt)fn.Arg(2).ToNumber(fn.Env)-1;

        if (fn.NArgs >= 4)
            ms += fn.Arg(3).ToNumber(fn.Env) * 3600000;
        if (fn.NArgs >= 5)
            ms += fn.Arg(4).ToNumber(fn.Env) * 60000;
        if (fn.NArgs >= 6)
            ms += fn.Arg(5).ToNumber(fn.Env) * 1000;
        if (fn.NArgs >= 7)
            ms += fn.Arg(6).ToNumber(fn.Env);

        fn.Result->SetNumber(ms + days * 86400000);
    }
    else
        fn.Result->SetNumber(0);
}

void GASDateCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    GPtr<GASDate> DateObj = *new GASDate(fn.Env);
    fn.Result->SetAsObject(DateObj.GetPtr());

    if (fn.NArgs == 1)
        DateObj->SetDate((SInt64)fn.Arg(0).ToNumber(fn.Env));
    else if (fn.NArgs >= 2)
    {
        SInt i = (SInt)fn.Arg(0).ToNumber(fn.Env);
        SInt y = i > 99 || i < 0 ? i : 1900 + i;
        SInt32 days = StartOfYear(y), ms = 0;
        i = (SInt)fn.Arg(1).ToNumber(fn.Env);
        if (i)
            days += months[IsLeapYear(y)][i-1];
        if (fn.NArgs >= 3)
            days += (SInt)fn.Arg(2).ToNumber(fn.Env)-1;
        
        if (fn.NArgs >= 4)
            ms += (SInt)fn.Arg(3).ToNumber(fn.Env) * 3600000;
        if (fn.NArgs >= 5)
            ms += (SInt)fn.Arg(4).ToNumber(fn.Env) * 60000;
        if (fn.NArgs >= 6)
            ms += (SInt)fn.Arg(5).ToNumber(fn.Env) * 1000;
        if (fn.NArgs >= 7)
            ms += (SInt)fn.Arg(6).ToNumber(fn.Env);

        DateObj->LJDate = days - StartOfYear(y);
        DateObj->LYear = y;
        DateObj->LTime = ms;
        DateObj->LDate = SInt64(days) * SInt64(86400000) + ms;
        DateObj->UpdateGMT();
        DateObj->SetDate(DateObj->Date);
    }
    else
    {
#if defined(GFC_OS_WIN32) || defined(GFC_OS_XBOX) || defined(GFC_OS_XBOX360)
        struct timeb t;
        ftime(&t);
        TIME_ZONE_INFORMATION tz;
        DWORD r = GetTimeZoneInformation(&tz);
        long bias = tz.Bias;
        if( r == TIME_ZONE_ID_STANDARD )
        { 
            bias += tz.StandardBias; 
        } 
        else if( r == TIME_ZONE_ID_DAYLIGHT)
        { 
            bias += tz.DaylightBias; 
        } 
        // else leave the bias alone
        DateObj->LocalOffset = -60000 * bias;
        DateObj->SetDate(SInt64(t.time) * 1000 + t.millitm);
#elif defined(GFC_OS_WINCE)
        TIME_ZONE_INFORMATION tz;
        GetTimeZoneInformation(&tz);
        DateObj->LocalOffset = -60000 * tz.Bias;
        DateObj->SetDate(time(0));

#elif defined(GFC_OS_PS3)
        CellRtcTick t, localt;
        cellRtcGetCurrentTick(&t);
        cellRtcConvertUtcToLocalTime(&t, &localt);
        DateObj->LocalOffset = ((localt.tick/1000)-(t.tick/1000));
        DateObj->SetDate(SInt64(t.tick/1000 - 62135596800000ull));

#elif defined(GFC_OS_PS2)
        unsigned char t[8];
        sceDevctl("cdrom0:", CDIOC_READCLOCK, NULL, 0, t, 8);

        SInt y = 2000 + (t[7]&0xf) + 10*((t[7]>>4)&0xf);
        SInt32 days = StartOfYear(y), ms = 0;
        SInt i = (t[6]&0xf) + 10*((t[6]>>4)&0xf);
        if (i > 1)
            days += months[IsLeapYear(y)][i-2];

        days += (t[5]&0xf) + 10*((t[5]>>4)&0xf) - 1;
        ms += ((t[3]&0xf) + 10*((t[3]>>4)&0xf)) * 3600000;
        ms += ((t[2]&0xf) + 10*((t[2]>>4)&0xf)) * 60000;
        ms += ((t[1]&0xf) + 10*((t[1]>>4)&0xf)) * 1000;

        DateObj->LocalOffset = 0;
        DateObj->LJDate = days - StartOfYear(y);
        DateObj->LYear = y;
        DateObj->LTime = ms;
        DateObj->LDate = SInt64(days) * SInt64(86400000) + ms;
        DateObj->UpdateGMT();
        DateObj->SetDate(DateObj->Date);

#elif defined(GFC_OS_PSP)
        SceRtcTick t, localt;
        sceRtcGetCurrentTick(&t);
        sceRtcConvertUtcToLocalTime(&t, &localt);
        DateObj->LocalOffset = ((localt.tick/1000)-(t.tick/1000));
        DateObj->SetDate(SInt64(t.tick/1000 - 62135596800000ull));

#elif defined(GFC_OS_WII)
        SInt64 t = OSTicksToMilliseconds(OSGetTime());
        DateObj->LocalOffset = 0;
        DateObj->SetDate(t + 946684800000LL);

#elif defined(GFC_CC_RENESAS) && defined(DMP_SDK)
        DateObj->LocalOffset = 0;
        DateObj->SetDate(1000 * SInt64(dk_tm_getCurrentTime()));
#else
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        long bias = tz.tz_minuteswest - (tz.tz_dsttime ? 60 : 0);
        DateObj->LocalOffset = -60000 * bias;
        DateObj->SetDate(SInt64(tv.tv_sec) * 1000 + SInt64(tv.tv_usec/1000));
#endif
    }
}

#else

void GASDate_DummyFunction() {}   // Exists to quelch compiler warning

#endif // GFC_NO_FXPLAYER_AS_DATE
