/**********************************************************************

Filename    :   GFxBitmapData.h
Content     :   Implementation of BitmapData class
Created     :   March, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXBITMAPDATA_H
#define INC_GFXBITMAPDATA_H

#include "GFxAction.h"
#include "GFxCharacter.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GASBitmapData;
class GASBitmapDataProto;
class GASBitmapDataCtorFunction;

// ***** External Classes
class GASArrayObject;
class GASEnvironment;



class GASBitmapData : public GASObject
{
    GPtr<GFxImageResource> pImageRes;

    void commonInit (GASEnvironment* penv);
public:
    GASBitmapData(GASStringContext* psc = 0) : GASObject() { GUNUSED(psc); }
    GASBitmapData(GASEnvironment* penv);

    ObjectType      GetObjectType() const   { return Object_BitmapData; }

    void              SetImage(GASEnvironment* penv, GFxImageResource* pimg);
    GFxImageResource* GetImage() const                 { return pImageRes; }

    bool            SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, 
                              const GASPropFlags& flags = GASPropFlags());
    bool            GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);

    static GASBitmapData* LoadBitmap(GASEnvironment* penv, const GFxString& linkageId);
};

class GASBitmapDataProto : public GASPrototype<GASBitmapData>
{
public:
    GASBitmapDataProto(GASStringContext *psc, GASObject* pprototype, const GASFunctionRef& constructor);

};

class GASBitmapDataCtorFunction : public GASFunctionObject
{
    static const GASNameFunction StaticFunctionTable[];
public:
    GASBitmapDataCtorFunction (GASStringContext *psc);

    static void GlobalCtor(const GASFnCall& fn);
    static void LoadBitmap(const GASFnCall& fn);
};


#endif // INC_GFXBITMAPDATA_H
