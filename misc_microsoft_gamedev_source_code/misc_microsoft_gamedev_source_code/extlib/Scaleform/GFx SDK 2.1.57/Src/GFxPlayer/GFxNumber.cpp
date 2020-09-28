/**********************************************************************

Filename    :   GFxNumber.cpp
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

#include "GFxNumber.h"
#include "GFxFunction.h"

#include <stdio.h>
#include <stdlib.h>


#if GFC_BYTE_ORDER == GFC_LITTLE_ENDIAN

#ifdef GFC_NO_DOUBLE
static const UByte GFxNaN_Bytes[]               = {0x00, 0x00, 0xC0, 0xFF};
static const UByte GFxQNaN_Bytes[]              = {0x00, 0x00, 0xC0, 0x7F};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x80, 0x7F};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x80, 0xFF};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x00, 0x80, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0xFF, 0xFF, 0x7F, 0x7F};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x80};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00};
#else
static const UByte GFxNaN_Bytes[]               = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xFF};
static const UByte GFxQNaN_Bytes[]              = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x7F};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x7F};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0xFF};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

#else

#ifdef GFC_NO_DOUBLE
static const UByte GFxNaN_Bytes[]               = {0xFF, 0xC0, 0x00, 0x00};
static const UByte GFxQNaN_Bytes[]              = {0x7F, 0xC0, 0x00, 0x00};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x7F, 0x80, 0x00, 0x00};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0xFF, 0x80, 0x00, 0x00};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x80, 0x00, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0x7F, 0x7F, 0xFF, 0xFF};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x80, 0x00, 0x00, 0x00};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00};
#else
static const UByte GFxNaN_Bytes[]               = {0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxQNaN_Bytes[]              = {0x7F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxPOSITIVE_INFINITY_Bytes[] = {0x7F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxNEGATIVE_INFINITY_Bytes[] = {0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxMIN_VALUE_Bytes[]         = {0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxMAX_VALUE_Bytes[]         = {0x7F, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const UByte GFxNEGATIVE_ZERO_Bytes[]     = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const UByte GFxPOSITIVE_ZERO_Bytes[]     = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

#endif

GASNumber GASNumberUtil::NaN()               { GASNumber v; memcpy(&v, GFxNaN_Bytes              , sizeof(v)); return v; }
GASNumber GASNumberUtil::POSITIVE_INFINITY() { GASNumber v; memcpy(&v, GFxPOSITIVE_INFINITY_Bytes, sizeof(v)); return v; }
GASNumber GASNumberUtil::NEGATIVE_INFINITY() { GASNumber v; memcpy(&v, GFxNEGATIVE_INFINITY_Bytes, sizeof(v)); return v; }
GASNumber GASNumberUtil::POSITIVE_ZERO()     { GASNumber v; memcpy(&v, GFxPOSITIVE_ZERO_Bytes    , sizeof(v)); return v; }
GASNumber GASNumberUtil::NEGATIVE_ZERO()     { GASNumber v; memcpy(&v, GFxNEGATIVE_ZERO_Bytes    , sizeof(v)); return v; }
GASNumber GASNumberUtil::MIN_VALUE()         { GASNumber v; memcpy(&v, GFxMIN_VALUE_Bytes        , sizeof(v)); return v; }
GASNumber GASNumberUtil::MAX_VALUE()         { GASNumber v; memcpy(&v, GFxMAX_VALUE_Bytes        , sizeof(v)); return v; }

bool GASNumberUtil::IsPOSITIVE_ZERO(GASNumber v)
{
    return (memcmp(&v, GFxPOSITIVE_ZERO_Bytes,  sizeof(v)) == 0);
}

bool GASNumberUtil::IsNEGATIVE_ZERO(GASNumber v)
{
    return (memcmp(&v, GFxNEGATIVE_ZERO_Bytes,  sizeof(v)) == 0);
}

// radixPrecision: > 10 - radix
//                 < 0  - precision (= -radixPrecision)
const char* GASNumberUtil::ToString(GASNumber value, GFxString& destStr, int radix)
{
    char        buffer[129];
    char*       pbuf = buffer;
    // MA: not sure about precision, but "%.14g" 
    // makes test_rotation.swf display too much rounding            
    const char* const defaultFmtStr = "%.14g";
    const char* fmtStr;
    char        fmtStrBuf[20];

    if(radix > 0)
        fmtStr = defaultFmtStr;
    else
    {
   
        gfc_sprintf(fmtStrBuf, 20,
        "%%.%dg", (radix >= -14) ? -radix : 14);
        
        fmtStr = fmtStrBuf;
        radix = 10;
    }

    if(GASNumberUtil::IsNaN(value))
    {
        destStr = "NaN";
    }
    else if(GASNumberUtil::IsPOSITIVE_INFINITY(value))
    {
        destStr = "Infinity";
    }
    else if(GASNumberUtil::IsNEGATIVE_INFINITY(value))
    {
        destStr = "-Infinity";
    }
    else
    {
        switch (radix) {
            case 10:
                gfc_sprintf(buffer, sizeof (buffer), fmtStr, (double)value);
                break;
            case 16:  
            case 8:
            case 2:
                return IntToString((int)value, destStr, radix);
        }
        destStr = pbuf;
    }
    return destStr.ToCStr();
}

// radixPrecision: > 10 - radix
//                 < 0  - precision (= -radixPrecision)
const char* GASNumberUtil::IntToString(SInt32 value, GFxString& destStr, int radix)
{
    char        buffer[129];
    char*       pbuf = buffer;

    switch (radix) {
        case 16:  
            gfc_sprintf(buffer, sizeof (buffer), "%x", (int)value);
            break;
        case 8:
            gfc_sprintf(buffer, sizeof (buffer), "%o", (int)value);
            break;
        case 2:
            {
                unsigned ival = (unsigned)value, i;
                unsigned mask = 1;
                pbuf = &buffer[sizeof (buffer) - 1];
                *pbuf = '\0';
                char* plastSignificantBit = 0;
                for (i = 0; i < sizeof (int)*8; i++) {
                    pbuf--;
                    if (ival & mask) {
                        *pbuf = '1';
                        plastSignificantBit = pbuf;
                    }
                    else {
                        *pbuf = '0';
                    }
                    mask <<= 1;
                    if (mask == 0) mask = 1;
                }
                if (plastSignificantBit == 0)
                    pbuf = &buffer[sizeof (buffer) - 2];
                else
                    pbuf = plastSignificantBit;
            }
            break;
        default:
            gfc_sprintf(buffer, sizeof (buffer), "%d", (int)value);
            break;
    }
    destStr = pbuf;
    return destStr.ToCStr();
}

////////////
GASNumberObject::GASNumberObject(GASEnvironment* penv) : Value(0)
{
    CommonInit (penv);
}

GASNumberObject::GASNumberObject(GASEnvironment* penv, GASNumber val) : Value(val)
{
    CommonInit(penv);
}

void GASNumberObject::CommonInit(GASEnvironment* penv)
{    
    Set__proto__(penv->GetSC(), penv->GetPrototype(GASBuiltin_Number));
}

const char* GASNumberObject::GetTextValue(GASEnvironment* penv) const
{
    GUNUSED(penv);
    return ToString(StringValue, 10);
}

void        GASNumberObject::SetValue(GASEnvironment* penv, const GASValue& v)
{
    Value = v.ToNumber(penv);
}

GASValue    GASNumberObject::GetValue() const
{
    return GASValue(Value);
}

const char* GASNumberObject::ToString(GFxString& destStr, int radix) const
{
    return GASNumberUtil::ToString(Value, destStr, radix);
}

bool GASNumberObject::InvokePrimitiveMethod(const GASFnCall& fnCall, const GASString& methodName)
{
    GASNumberObject* pthis = (GASNumberObject*) fnCall.ThisPtr;
    GASSERT(pthis);

    // if (methodName == "toString" || methodName == "valueOf")
    if (fnCall.Env->GetBuiltin(GASBuiltin_toString).CompareBuiltIn_CaseCheck(methodName, fnCall.Env->IsCaseSensitive()) ||
        fnCall.Env->GetBuiltin(GASBuiltin_valueOf).CompareBuiltIn_CaseCheck(methodName, fnCall.Env->IsCaseSensitive()) )
    {
        GASValue method;
        if (pthis->GetMemberRaw(fnCall.Env->GetSC(), methodName, &method)) 
        {
            GASFunctionRef  func = method.ToFunction();
            if (!func.IsNull())
            {
                // It's a C function.  Call it.
                func(fnCall);
                return true;
            }
        }
    }
    fnCall.Result->SetUndefined();
    return false;
}

////////////////////////
static void GAS_NumberToString(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Number, "Number");
    GASNumberObject* pthis = (GASNumberObject*) fn.ThisPtr;
    GASSERT(pthis);
    int radix = 10;
    GFxString str;

    if (fn.NArgs > 0) {
        // special case if optional parameter (radix) is specified.
        radix = (int) fn.Arg(0).ToNumber(fn.Env);
    }

    fn.Result->SetString(fn.Env->CreateString(pthis->ToString(str, radix)));
}

static void GAS_NumberValueOf(const GASFnCall& fn)
{
    CHECK_THIS_PTR(fn, Number, "Number");
    GASNumberObject* pthis = (GASNumberObject*) fn.ThisPtr;
    GASSERT(pthis);

    fn.Result->SetNumber (pthis->GetValue ().ToNumber(fn.Env));
}


static const GASNameFunction NumberFunctionTable[] = 
{
    { "toString",       &GAS_NumberToString  },
    { "valueOf",        &GAS_NumberValueOf  },
    { 0, 0 }
};

GASNumberProto::GASNumberProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor) : 
    GASPrototype<GASNumberObject>(psc, pprototype, constructor)
{
    InitFunctionMembers(psc, NumberFunctionTable, pprototype);
}

////////////////////////////////////////
//
struct GASNameNumberFunc
{
    const char* Name;
    GASNumber (*Function)();
};

static const GASNameNumberFunc GASNumberConstTable[] = 
{
    { "MAX_VALUE",         GASNumberUtil::MAX_VALUE          },
    { "MIN_VALUE",         GASNumberUtil::MIN_VALUE          },
    { "NaN",               GASNumberUtil::NaN                },
    { "NEGATIVE_INFINITY", GASNumberUtil::NEGATIVE_INFINITY  },
    { "POSITIVE_INFINITY", GASNumberUtil::POSITIVE_INFINITY  },
    { 0, 0 }
};

GASNumberCtorFunction::GASNumberCtorFunction(GASStringContext *psc) :
    GASFunctionObject(GlobalCtor)
{
    for(int i = 0; GASNumberConstTable[i].Name; i++)
    {
        SetConstMemberRaw(psc, GASNumberConstTable[i].Name, GASValue(GASNumberConstTable[i].Function()),  
            GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
    }
}

void GASNumberCtorFunction::GlobalCtor(const GASFnCall& fn)
{
    if (fn.NArgs == 0)
    {
        fn.Result->SetNumber(0);
    }
    else
    {
        fn.Result->SetNumber(fn.Arg(0).ToNumber(fn.Env));
    }
}


