/**********************************************************************

Filename    :   GFxButton.h
Content     :   Button character implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXBUTTON_H
#define INC_GFXBUTTON_H

#include "GRefCount.h"

#include "GFxTags.h"
#include "GFxCharacter.h"
#include "GFxAction.h"
#include "GFxObjectProto.h"


// ***** Declared Classes
class GFxMouseButtonState;
class GFxButtonRecord;
class GFxButtonAction;
class GFxButtonCharacterDef;
class GASButtonObject;
class GASButtonProto;
class GFxSoundSampleImpl;

class GFxLoadProcess;


//
// Helper to generate mouse events, given mouse state & history.
//

class GFxMouseButtonState : public GNewOverrideBase
{
public:
    GWeakPtr<GFxASCharacter>    ActiveEntity;   // entity that currently owns the mouse pointer
    GWeakPtr<GFxASCharacter>    TopmostEntity;  // what's underneath the mouse right now

    UInt    MouseButtonStateLast;    // previous state of mouse buttons
    UInt    MouseButtonStateCurrent; // current state of mouse buttons

    bool    MouseInsideEntityLast;  // whether mouse was inside the ActiveEntity last frame

    GFxMouseButtonState()
        :
        MouseButtonStateLast(0),
        MouseButtonStateCurrent(0),
        MouseInsideEntityLast(false)
    {
    }
};

void    GFx_GenerateMouseButtonEvents(GFxMouseButtonState* ms);


//
// button characters
//
enum GFxMouseState
{
    Event_MouseUp,
    Event_MouseDown,
    MOUSE_OVER
};

class GFxButtonRecord
{
public:
    bool                HitTest;
    bool                Down;
    bool                Over;
    bool                Up; 

    GFxResourceId       CharacterId;    
    int                 ButtonLayer;

    GRenderer::BlendType BlendMode;

    GRenderer::Matrix   ButtonMatrix;
    GRenderer::Cxform   ButtonCxform;

    bool    MatchMouseState(GFxMouseState mouseState) const
    {
        if ((mouseState == Event_MouseUp && Up)      ||
            (mouseState == Event_MouseDown && Down)  ||
            (mouseState == MOUSE_OVER && Over) )
            return 1;
        return 0;
    }

    bool    Read(GFxLoadProcess* p, GFxTagType tagType);
};


class GFxButtonAction
{
public:
    enum ConditionType
    {
        IDLE_TO_OVER_UP = 1 << 0,
        OVER_UP_TO_IDLE = 1 << 1,
        OVER_UP_TO_OVER_DOWN = 1 << 2,
        OVER_DOWN_TO_OVER_UP = 1 << 3,
        OVER_DOWN_TO_OUT_DOWN = 1 << 4,
        OUT_DOWN_TO_OVER_DOWN = 1 << 5,
        OUT_DOWN_TO_IDLE = 1 << 6,
        IDLE_TO_OVER_DOWN = 1 << 7,
        OVER_DOWN_TO_IDLE = 1 << 8,
    };
    int Conditions;
    GTL::garray<GASActionBufferData*>   Actions;

    ~GFxButtonAction();
    void    Read(GFxStream* in, GFxTagType tagType);
};

class GFxButtonCharacterDef : public GFxCharacterDef
{
public:
    class GFxSoundEnvelope
    {
    public:
        UInt32 Mark44;
        UInt16 Level0;
        UInt16 Level1;
    };

    class GFxSoundInfo
    {
    public:
        void Read(GFxStream* in);

        bool NoMultiple;
        bool StopPlayback;
        bool HasEnvelope;
        bool HasLoops;
        bool HasOutPoint;
        bool HasInPoint;
        UInt32 InPoint;
        UInt32 OutPoint;
        UInt16 LoopCount;
        GTL::garray<GFxSoundEnvelope> Envelopes;
    };

    class GFxButtonSoundInfo
    {
    public:
        UInt16 SoundId;
        GFxSoundSampleImpl* Sam;
        GFxSoundInfo SoundStyle;
    };

    class GFxButtonSoundDef : public GNewOverrideBase
    {
    public:
        void    Read(GFxLoadProcess* p);
        GFxButtonSoundInfo ButtonSounds[4];
    };


    bool Menu;
    GTL::garray<GFxButtonRecord>    ButtonRecords;
    GTL::garray<GFxButtonAction>    ButtonActions;
    GFxButtonSoundDef*              Sound;
    GFxScale9Grid*                  pScale9Grid;


    GFxButtonCharacterDef();
    virtual ~GFxButtonCharacterDef();

    GFxCharacter*   CreateCharacterInstance(GFxASCharacter* pparent, GFxResourceId id,
                                            GFxMovieDefImpl *pbindingImpl);
    void    Read(GFxLoadProcess* p, GFxTagType tagType);

    // Obtains character bounds in local coordinate space.
    virtual GRectF  GetBoundsLocal() const
    {
        // This is not useful, because button bounds always depend on its state.
        GASSERT(0);
        return GRectF(0);
        /*
        Float h = 0;
        for(UInt i=0; i<ButtonRecords.size(); i++)
            if (!ButtonRecords[i].HitTest)
                h = GTL::gmax<Float>(h, ButtonRecords[i].pCharacterDef->GetHeightLocal());
        return h; */
    }


    void SetScale9Grid(const GRectF& r) 
    {
        if (pScale9Grid == 0) 
            pScale9Grid = new GFxScale9Grid;
        *pScale9Grid = r;
    }

    const GFxScale9Grid* GetScale9Grid() const { return pScale9Grid; }

    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_ButtonDef); }
    
};

// ActionScript objects. Should be separated later.

class GASButtonObject : public GASObject
{
    friend class GASButtonProto;

    GWeakPtr<GFxASCharacter> pButton;   // weak ref on button obj

    void commonInit ();
protected:

    GASButtonObject(GASStringContext *psc = 0) : GASObject() { GUNUSED(psc); commonInit(); }
public:
    GASButtonObject(GASGlobalContext* gCtxt, GFxASCharacter* pbutton) : pButton(pbutton)
    {
        commonInit ();
        GASStringContext* psc = pbutton->GetASEnvironment()->GetSC();
        Set__proto__ (psc, gCtxt->GetPrototype(GASBuiltin_Button));
    }

    virtual ObjectType      GetObjectType() const   { return Object_ButtonASObject; }

    GFxASCharacter* GetASCharacter() { return GPtr<GFxASCharacter>(pButton).GetPtr(); }
};

class GASButtonProto : public GASPrototype<GASButtonObject>
{
public:
    GASButtonProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

};


#endif // INC_GFXBUTTON_H

