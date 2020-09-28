/**********************************************************************

Filename    :   GFxTransform.h
Content     :   flash.geom.Transform reference class for ActionScript 2.0
Created     :   6/22/2006
Authors     :   Artyom Bolgar, Prasad Silva
Copyright   :   (c) 2001-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_GFXTRANSFORM_H
#define INC_GFXTRANSFORM_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "GFxObject.h"
#include "GFxPlayerImpl.h"
#include "GFxMatrix.h"
#include "GFxColorTransform.h"
#include "GFxRectangle.h"
#include "GFxObjectProto.h"

// GFC_NO_FXPLAYER_AS_TRANSFORM disables Transform class
#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM

// ***** Declared Classes
class GASTransformObject;
class GASTransformProto;
class GASEnvironment;

// ****************************************************************************
// GAS Transform class
//
class GASTransformObject : public GASObject
{
    friend class GASTransformProto;
    GWeakPtr<GFxMovieRoot>          MovieRoot;
    GPtr<GFxCharacterHandle>        TargetHandle;

#ifndef GFC_NO_FXPLAYER_AS_MATRIX
    GPtr<GASMatrixObject>           Matrix;
#endif

#ifndef GFC_NO_FXPLAYER_AS_COLORTRANSFORM
    GPtr<GASColorTransformObject>   ColorTransform;
#endif 

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
    GPtr<GASRectangleObject>        PixelBounds;
#endif 

protected:
    GASTransformObject(GASStringContext *psc) { GUNUSED(psc); }
public:
    GASTransformObject(GASEnvironment* penv, GFxASCharacter *pcharacter);

    virtual ObjectType GetObjectType() const   { return Object_Transform; }

    // getters and setters
    virtual bool GetMember(GASEnvironment *penv, const GASString &name, GASValue* val);
    virtual bool SetMember(GASEnvironment *penv, const GASString &name, const GASValue &val, const GASPropFlags& flags = GASPropFlags());
};

// ****************************************************************************
// GAS Transform prototype class
//
class GASTransformProto : public GASPrototype<GASTransformObject>
{
public:
    GASTransformProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

    static const GASNameFunction FunctionTable[];
};


#endif // GFC_NO_FXPLAYER_AS_TRANSFORM
#endif // INC_GFXTRANSFORM_H

