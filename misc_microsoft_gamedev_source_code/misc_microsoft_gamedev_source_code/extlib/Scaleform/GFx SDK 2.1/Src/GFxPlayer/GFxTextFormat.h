/**********************************************************************

Filename    :   GFxTextFormat.h
Content     :   TextFormat object functinality
Created     :   April 17, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTEXTFORMAT_H
#define INC_GFXTEXTFORMAT_H

#include "GFxAction.h"
#include "GFxStyledText.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASTextFormatObject;
class GASTextFormatProto;
class GASTextFormatCtorFunction;


class GASTextFormatObject : public GASObject
{
public:
    GFxTextFormat           TextFormat;
    GFxTextParagraphFormat  ParagraphFormat;

    GASTextFormatObject(GASStringContext* psc = 0) { GUNUSED(psc); }
    GASTextFormatObject(GASEnvironment* penv);

    ObjectType      GetObjectType() const   { return Object_TextFormat; }

    virtual bool    SetMember(GASEnvironment *penv, const GASString& name, 
        const GASValue& val, const GASPropFlags& flags = GASPropFlags());

    void SetTextFormat(GASStringContext* psc, const GFxTextFormat& textFmt);
    void SetParagraphFormat(GASStringContext* psc, const GFxTextParagraphFormat& paraFmt);
};

class GASTextFormatProto : public GASPrototype<GASTextFormatObject>
{
public:
    GASTextFormatProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

    static void GetTextExtent(const GASFnCall& fn);
};

class GASTextFormatCtorFunction : public GASFunctionObject
{
public:
    GASTextFormatCtorFunction(GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
};

#endif //TEXTFORMAT

