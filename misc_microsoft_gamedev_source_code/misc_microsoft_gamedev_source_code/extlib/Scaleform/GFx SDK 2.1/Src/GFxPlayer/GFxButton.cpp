/**********************************************************************

Filename    :   GFxButton.cpp
Content     :   
Created     :   
Authors     :   
Notes       :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxButton.h"

#include "GRenderer.h"

#include "GFxAction.h"
//#include "GFxSound.h" //AB no sound
#include "GFxStream.h"
#include "GFxFontResource.h"
#include "GFxPlayerImpl.h"
#include "GFxLoaderImpl.h"
#include "GFxSprite.h"
#include "GFxRectangle.h"

#include "GFxLoadProcess.h"
#include "GFxDisplayContext.h"

/*

Observations about button & mouse behavior

Entities that receive mouse events: only buttons and sprites, AFAIK

When the mouse button goes down, it becomes "captured" by whatever
element is topmost, directly below the mouse at that moment.  While
the mouse is captured, no other entity receives mouse events,
regardless of how the mouse or other elements move.

The mouse remains captured until the mouse button goes up.  The mouse
remains captured even if the element that captured it is removed from
the display list.

If the mouse isn't above a button or sprite when the mouse button goes
down, then the mouse is captured by the Background (i.E. mouse events
just don't get sent, until the mouse button goes up again).

MA: The only exception to this is Menu mode button, which will get
dragOver/dragOut and release even if the inital click was outside
of the button (unless it was in a NON-menu button initally, in
which case that button would capture the mouse).

Mouse events:

+------------------+---------------+-------------------------------------+
| Event            | Mouse Button  | description                         |
=========================================================================
| onRollOver       |     up        | sent to topmost entity when mouse   |
|                  |               | cursor initially goes over it       |
+------------------+---------------+-------------------------------------+
| onRollOut        |     up        | when mouse leaves entity, after     |
|                  |               | onRollOver                          |
+------------------+---------------+-------------------------------------+
| onPress          |  up -> down   | sent to topmost entity when mouse   |
|                  |               | button goes down.  onRollOver       |
|                  |               | always precedes onPress.  Initiates |
|                  |               | mouse capture.                      |
+------------------+---------------+-------------------------------------+
| onRelease        |  down -> up   | sent to active entity if mouse goes |
|                  |               | up while over the element           |
+------------------+---------------+-------------------------------------+
| onDragOut        |     down      | sent to active entity if mouse      |
|                  |               | is no longer over the entity        |
+------------------+---------------+-------------------------------------+
| onReleaseOutside |  down -> up   | sent to active entity if mouse goes |
|                  |               | up while not over the entity.       |
|                  |               | onDragOut always precedes           |
|                  |               | onReleaseOutside                    |
+------------------+---------------+-------------------------------------+
| onDragOver       |     down      | sent to active entity if mouse is   |
|                  |               | dragged back over it after          |
|                  |               | onDragOut                           |
+------------------+---------------+-------------------------------------+

There is always one active entity at any given Time (considering NULL to
be an active entity, representing the background, and other objects that
don't receive mouse events).

When the mouse button is up, the active entity is the topmost element
directly under the mouse pointer.

When the mouse button is down, the active entity remains whatever it
was when the button last went down.

The active entity is the only object that receives mouse events.

!!! The "trackAsMenu" property alters this behavior!  If trackAsMenu
is set on the active entity, then onReleaseOutside is filtered out,
and onDragOver from another entity is Allowed (from the background, or
another trackAsMenu entity). !!!

*/


void    GFx_GenerateMouseButtonEvents(GFxMouseButtonState* ms)
{
    GPtr<GFxASCharacter>    ActiveEntity = ms->ActiveEntity;
    GPtr<GFxASCharacter>    TopmostEntity = ms->TopmostEntity;

    if (ms->MouseButtonStateLast & 1)
    {
        // Mouse button was down.


        // Handle onDragOut, onDragOver
        if (!ms->MouseInsideEntityLast)
        {
            if (TopmostEntity == ActiveEntity)
            {
                // onDragOver
                if (ActiveEntity)                   
                    ActiveEntity->OnButtonEvent(GFxEventId::Event_DragOver);
                
                ms->MouseInsideEntityLast = true;
            }
        }
        else
        {
            // MouseInsideEntityLast == true
            if (TopmostEntity != ActiveEntity)
            {
                // onDragOut
                if (ActiveEntity)
                {
                    ActiveEntity->OnButtonEvent(GFxEventId::Event_DragOut);
                }
                ms->MouseInsideEntityLast = false;
            }
        }
        // Handle trackAsMenu dragOver
        if (!ActiveEntity
            || ActiveEntity->GetTrackAsMenu())
        {
            if (TopmostEntity
                && TopmostEntity != ActiveEntity
                && TopmostEntity->GetTrackAsMenu())
            {
                // Transfer to topmost entity, dragOver
                ActiveEntity = TopmostEntity;
                ActiveEntity->OnButtonEvent(GFxEventId::Event_DragOver);
                ms->MouseInsideEntityLast = true;
            }
        }

        // Handle onRelease, onReleaseOutside
        if ((ms->MouseButtonStateCurrent & 1) == 0)
        {
            // Mouse button just went up.
            ms->MouseButtonStateLast &= (~1u);

            if (ActiveEntity)
            {
                if (ms->MouseInsideEntityLast)
                {
                    // onRelease
                    ActiveEntity->OnButtonEvent(GFxEventId::Event_Release);
                }
                else
                {
                    // onReleaseOutside
                    if (!ActiveEntity->GetTrackAsMenu())
                        ActiveEntity->OnButtonEvent(GFxEventId::Event_ReleaseOutside);                       
                    // Clear, to prevent Event_RollOut event below.
                    ActiveEntity = 0;
                }
            }
        }
    }

    if ((ms->MouseButtonStateLast & 1) == 0)
    {
        // Mouse button was up.

        // New active entity is whatever is below the mouse right now.
        if (TopmostEntity != ActiveEntity)
        {
            // onRollOut
            if (ActiveEntity)               
                ActiveEntity->OnButtonEvent(GFxEventId::Event_RollOut);              

            ActiveEntity = TopmostEntity;

            // onRollOver
            if (ActiveEntity)
                ActiveEntity->OnButtonEvent(GFxEventId::Event_RollOver);             

            ms->MouseInsideEntityLast = true;
        }

        // mouse button press
        if (ms->MouseButtonStateCurrent & 1)
        {
            // onPress
            if (ActiveEntity)
                ActiveEntity->OnButtonEvent(GFxEventId::Event_Press);
            
            ms->MouseInsideEntityLast = true;
            ms->MouseButtonStateLast |= 1;
        }
    }
    // copy states of right and middle buttons
    ms->MouseButtonStateLast = (ms->MouseButtonStateCurrent & (~1u)) | (ms->MouseButtonStateLast & 1); 

    // Write The (possibly modified) GPtr copies back
    // into the state struct.
    ms->ActiveEntity = ActiveEntity;
    ms->TopmostEntity = TopmostEntity;
}


class GFxButtonCharacter : public GFxASCharacter
{
public:
    GFxButtonCharacterDef*              pDef;
    GFxScale9Grid*                      pScale9Grid;

    GTL::garray< GPtr<GFxCharacter> >   RecordCharacter;

    enum MouseFlags
    {
        IDLE = 0,
        FLAG_OVER = 1,
        FLAG_DOWN = 2,
        OVER_DOWN = FLAG_OVER|FLAG_DOWN,

        // aliases
        OVER_UP = FLAG_OVER,
        OUT_DOWN = FLAG_DOWN
    };

    int             LastMouseFlags, MouseFlags;
    GFxMouseState   MouseState;

    GPtr<GASButtonObject> ASButtonObj;

    GFxButtonCharacter(GFxButtonCharacterDef* def, GFxMovieDefImpl *pbindingDefImpl,
                       GFxASCharacter* parent, GFxResourceId id)
        :
        GFxASCharacter(pbindingDefImpl, parent, id),
        pDef(def),
        pScale9Grid(0),
        LastMouseFlags(IDLE),
        MouseFlags(IDLE),
        MouseState(Event_MouseUp)
    {
        GASSERT(pDef);
        
        SetScale9Grid(def->GetScale9Grid());
        SetTrackAsMenuFlag(pDef->Menu);

        UPInt r, R_num = pDef->ButtonRecords.size();
        RecordCharacter.resize(R_num);
    
        for (r = 0; r < R_num; r++)
        {
            GFxButtonRecord*    bdef = &pDef->ButtonRecords[r];

            // Resolve the GFxCharacter id.
            GFxCharacterCreateInfo ccinfo = pbindingDefImpl->GetCharacterCreateInfo(bdef->CharacterId);
            GASSERT(ccinfo.pCharDef && ccinfo.pBindDefImpl);

            if (ccinfo.pCharDef)
            {
                const GRenderer::Matrix&    mat = pDef->ButtonRecords[r].ButtonMatrix;
                const GRenderer::Cxform&    cx  = pDef->ButtonRecords[r].ButtonCxform;

                GPtr<GFxCharacter>  ch = *ccinfo.pCharDef->CreateCharacterInstance(this, id, ccinfo.pBindDefImpl);
                RecordCharacter[r] = ch;
                ch->SetMatrix(mat);
                ch->SetCxform(cx);
                ch->Restart();
                ch->SetBlendMode(bdef->BlendMode);

                ch->SetScale9GridExists(false);
                const GFxASCharacter* parent = ch->GetParent();
                while (parent)
                {
                    if (parent->GetScale9Grid())
                    {
                        ch->SetScale9GridExists(true);
                        ch->PropagateScale9GridExists();
                        break;
                    }
                    parent = parent->GetParent();
                }
            }            
        }
        ASButtonObj = *new GASButtonObject(GetGC(), this);
        // let the base class know what's going on
        pProto = ASButtonObj->Get__proto__();
    }

    ~GFxButtonCharacter()
    {
        delete pScale9Grid;
    }

    inline void SetDirtyFlag()
    {
        GetMovieRoot()->SetDirtyFlag();
    }

    virtual GFxCharacterDef* GetCharacterDef() const
    {
        return pDef;
    }

    // Default implementation of these is ok for buttons.
    // In the future, however, objects will have to consider _lockroot (although this may not affect buttons).
    //virtual GFxMovieDef*  GetResourceMovieDef() const         { return pParent->GetResourceMovieDef(); }
    //virtual GFxMovieRoot* GetMovieRoot() const                { return pParent->GetMovieRoot(); }
    //virtual GFxMovieSub*  GetLevelMovie(SInt level) const     { return pParent->GetLevelMovie(level); }
    //virtual GFxMovieSub*  GetRootMovie2() const               { return pParent->GetRootMovie2(); }



    void SetScale9Grid(const GFxScale9Grid* gr)
    {
        bool propagate = (gr != 0) != (pScale9Grid != 0);
        if (gr == 0)
        {
            delete pScale9Grid;
            pScale9Grid = 0;
            SetScale9GridExists(false);
        }
        else
        {
            if (pScale9Grid == 0) 
                pScale9Grid = new GFxScale9Grid;
            *pScale9Grid = *gr;
            SetScale9GridExists(true);
        }
        SetDirtyFlag();
        if (propagate)
            PropagateScale9GridExists();
    }

    void PropagateScale9GridExists()
    {
        bool actualGrid = GetScale9Grid() != 0;
        // Stop cleaning up scale9Grid if actual one exists in the node
        if (!DoesScale9GridExist() && actualGrid)
            return; 
        
        for (UInt i = 0; i < RecordCharacter.size(); i++)
        {
            GFxCharacter* ch = RecordCharacter[i];
            if (!ch)
                continue;

            ch->SetScale9GridExists(DoesScale9GridExist() || actualGrid);
            ch->PropagateScale9GridExists();
        }   
    }



    void    Restart()
    {
        LastMouseFlags = IDLE;
        MouseFlags = IDLE;
        MouseState = Event_MouseUp;
        UPInt r, R_num = RecordCharacter.size();
        for (r = 0; r < R_num; r++)
        {
            RecordCharacter[r]->Restart();
        }
        SetDirtyFlag();
    }

    // Obtains an active button record based on state, or -1.
    // Note: not quite correct, since multiple records can apply to a state.
    int     GetActiveRecordIndex() const
    {
        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];
            if (!RecordCharacter[i])                
                continue;               
            if (rec.MatchMouseState(MouseState))
            {
                return i;
            }
        }
        // Not found
        return -1;
    }


    virtual void    AdvanceFrame(bool nextFrame, Float framePos)
    {
        // Implement mouse-drag.
        GFxASCharacter::DoMouseDrag();

        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];
            
            if (RecordCharacter[i] && rec.MatchMouseState(MouseState))
                RecordCharacter[i]->AdvanceFrame(nextFrame, framePos);
        }
    }


    void    Display(GFxDisplayContext& context, StackData stackData)
    {
        if (!pDef->ButtonRecords.size())
            return;

        // We must apply a new binding table if the character needs to
        // use resource data from the loaded/imported GFxMovieDefImpl.
        GFxResourceBinding *psave = context.pResourceBinding;
        context.pResourceBinding = &pDefImpl->GetResourceBinding();

        GRenderer* prenderer = context.GetRenderer();

        // Button records already sorted by depth, so display them in order.
        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord&    rec = pDef->ButtonRecords[i];
            GFxCharacter*       pch = RecordCharacter[i];
            
            if (pch && rec.MatchMouseState(MouseState))
            {
				   // Pass matrix on the stack
				   StackData newStackData;
				   newStackData.stackMatrix = stackData.stackMatrix * this->Matrix_1;
				   newStackData.stackColor.Concatenate( this->ColorTransform );				   

                prenderer->PushBlendMode(pch->GetBlendMode());
                pch->Display(context,newStackData);
                prenderer->PopBlendMode();
            }
        }

        context.pResourceBinding = psave;

        DoDisplayCallback();
    }

    // Combine the flags to avoid a conditional. It would be faster with a macro.
    GINLINE int     Transition(int a, int b) const
    {
        return (a << 2) | b;
    }

    virtual bool    PointTestLocal(const GPointF &pt, UInt8 hitTestMask = 0) const
    {
        if (IsHitTestDisableFlagSet())
            return false;

        if ((hitTestMask & HitTest_IgnoreInvisible) && !GetVisible())
            return false;

        if (!DoesScale9GridExist())
        {
            if (!GetBounds(GRenderer::Matrix()).Contains(pt))
                return false;
            else if (!(hitTestMask & HitTest_TestShape))
                return true;
        }

        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord&    rec = pDef->ButtonRecords[i];
            if ((rec.CharacterId == GFxResourceId::InvalidId) || !rec.HitTest)
                continue;

            GFxCharacter* ch = RecordCharacter[i];
            if (!ch)
                continue;

            if ((hitTestMask & HitTest_IgnoreInvisible) && !ch->GetVisible())
                continue;

            GRenderer::Matrix   m = ch->GetMatrix();
            GPointF             p = m.TransformByInverse(pt);   

            if (ch->PointTestLocal (p, hitTestMask))
                return true;
        }   
        return false;
    }

    // Return the topmost entity that the given point covers.  NULL if none.
    // I.E. check against ourself.
    virtual GFxASCharacter* GetTopMostMouseEntity(const GPointF &pt, bool, const GFxASCharacter* ignoreMC = NULL)
    {
        if (!GetVisible())
            return 0;

        if (ignoreMC == this)
            return 0;

        GRenderer::Matrix   m = GetMatrix();
        GPointF             p;
        m.TransformByInverse(&p, pt);

        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord&    rec = pDef->ButtonRecords[i];
            if ((rec.CharacterId == GFxResourceId::InvalidId) || !rec.HitTest)
                continue;

            // Find the mouse position in button-record space.
            GPointF SubP;
            rec.ButtonMatrix.TransformByInverse(&SubP, p);

            if (i < RecordCharacter.size())
            {
                GFxCharacter* pcharacter = RecordCharacter[i];

                if (pcharacter && pcharacter->GetCharacterDef()->DefPointTestLocal(SubP, true, this))
                {
                    // The mouse is inside the shape.
                    return this;
                }
            }
        }
        return NULL;
    }

    bool IsTabable() const
    {
        if (!GetVisible()) return false;
        if (!IsTabEnabledFlagDefined())
            return true;
        else
            return IsTabEnabledFlagTrue();
    }

    virtual ObjectType      GetObjectType() const
    {
        return Object_Button;
    }

    virtual const GFxScale9Grid* GetScale9Grid() const { return pScale9Grid; }

    // Return a single character bounds
    virtual GRectF  GetBoundsOfRecord(const Matrix &transform, UInt recNumber) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        Matrix  m;

        if (RecordCharacter[recNumber])
        {
            m = transform;
            m *= RecordCharacter[recNumber]->GetMatrix();
            bounds = RecordCharacter[recNumber]->GetBounds(m);
        }
        
        return bounds;
    }

    // Return a single character "pure rectangle" bounds (not considering the stroke)
    GRectF  GetRectBounds(const Matrix &transform, UInt recNumber) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        Matrix  m;

        if (RecordCharacter[recNumber])
        {
            m = transform;
            m *= RecordCharacter[recNumber]->GetMatrix();
            bounds = RecordCharacter[recNumber]->GetRectBounds(m);
        }
        
        return bounds;
    }


    // Get bounds. This is used,
    // among other things, to calculate button width & height.
    virtual GRectF  GetBounds(const Matrix &transform) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        GRectF  tempRect;
        bool    boundsInit = 0;

        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];

            if (rec.MatchMouseState(MouseState))
            {
                tempRect = GetBoundsOfRecord(transform, i);
                if (!tempRect.IsNull())
                {
                    if (!boundsInit)
                    {
                        bounds = tempRect;
                        boundsInit = 1;
                    }
                    else
                    {
                        bounds.Union(tempRect);
                    }
                }
            }
        }
        return bounds;
    }

    // "transform" matrix describes the transform applied to parent and us,
    // including the object's matrix itself. This means that if transform is
    // identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
    virtual GRectF GetRectBounds(const Matrix &transform) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        GRectF  tempRect;
        bool    boundsInit = 0;

        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            // TO DO: Clarify this, whether or not we should match the state.
            // If yes, it may change the scale9grid configuration and the appearance.
            //GFxButtonRecord& rec = pDef->ButtonRecords[i];
            //if (rec.MatchMouseState(MouseState)) 
            {
                tempRect = GetRectBounds(transform, i);
                if (!tempRect.IsNull())
                {
                    if (!boundsInit)
                    {
                        bounds = tempRect;
                        boundsInit = 1;
                    }
                    else
                    {
                        bounds.Union(tempRect);
                    }
                }
            }
        }
        return bounds;
    }


    enum ButtonState
    {
        Up,
        Over,
        Down,
        Hit
    };
    // returns the local boundaries of whole state
    virtual GRectF  GetBoundsOfState(const Matrix &transform, ButtonState state) const
    {
        // Custom based on state.
        GRectF  bounds(0);
        GRectF  tempRect;

        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            GFxButtonRecord& rec = pDef->ButtonRecords[i];

            if ((state == Hit && rec.HitTest) || 
                (state == Down && rec.Down) ||
                (state == Over && rec.Over) ||
                (state == Up && rec.Up))
            {
                tempRect = GetBoundsOfRecord(transform, i);
                if (!tempRect.IsNull())
                {
                    if (bounds.IsNull())
                        bounds = tempRect;
                    else
                        bounds.Union(tempRect);
                }
            }
        }
        
        return bounds;
    }

    // focus rect for buttons is calculated as follows:
    // 1) if "hit" state exists - boundary rect of "hit" shape
    // 2) if "down" state exists  - boundary rect of "down" shape
    // 3) if "over" state exists  - boundary rect of "over" shape
    // 4) otherwise - boundary rect of "up" shape
    virtual GRectF GetFocusRect() const 
    {
        GRenderer::Matrix m;
        GRectF tempRect;
        if (!(tempRect = GetBoundsOfState(m, Hit)).IsNull())
            return tempRect;
        else if (!(tempRect = GetBoundsOfState(m, Down)).IsNull())
            return tempRect;
        else if (!(tempRect = GetBoundsOfState(m, Over)).IsNull())
            return tempRect;
        else if (!(tempRect = GetBoundsOfState(m, Up)).IsNull())
            return tempRect;
        return GetBounds(m); // shouldn't reach this point, actually
    }

    // invoked when item is going to get focus (Selection.setFocus is invoked, or TAB is pressed)
    void OnGettingKeyboardFocus()
    {
        if (!GetMovieRoot()->DisableFocusRolloverEvent.IsTrue())
            OnButtonEvent(GFxEventId::Event_RollOver);
    }

    // invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
    bool OnLosingKeyboardFocus(GFxASCharacter*, GFxFocusMovedType) 
    {
        if (GetMovieRoot()->IsFocusRectShown() && !GetMovieRoot()->DisableFocusRolloverEvent.IsTrue())
            OnButtonEvent(GFxEventId::Event_RollOut);
        return true;
    }

    /* returns true, if event fired */
    virtual bool    OnButtonEvent(const GFxEventId& event)
    {
        // Enabled buttons are not manipulated by events.
        if (!IsEnabledFlagSet())
            return false;
        bool handlerFound = false;

        // Set our mouse State (so we know how to render).
        switch (event.Id)
        {
        case GFxEventId::Event_RollOut:
        case GFxEventId::Event_ReleaseOutside:
            MouseState = Event_MouseUp;
            break;

        case GFxEventId::Event_Release:
        case GFxEventId::Event_RollOver:
            MouseState = MOUSE_OVER;
            break;
        case GFxEventId::Event_DragOut:
            if (GetTrackAsMenu())
                MouseState = Event_MouseUp;
            else
                MouseState = MOUSE_OVER;
            break;

        case GFxEventId::Event_Press:
        case GFxEventId::Event_DragOver:
            MouseState = Event_MouseDown;
            break;

        case GFxEventId::Event_KeyPress:
            break; 
        default:
            GASSERT(0); // missed a case?
            break;
        };

        // Button transition sounds.
        if (pDef->Sound != NULL)
        {
        // MA: Commented out sound for now
        #if 0

            int bi; // button sound GTL::garray index [0..3]
            GFxSoundHandler* s = GetSoundHandler();

            // Check if there is a sound handler
            if (s != NULL) {
                switch (event.Id)
                {
                case GFxEventId::Event_RollOut:
                    bi = 0;
                    break;
                case GFxEventId::Event_RollOver:
                    bi = 1;
                    break;
                case GFxEventId::Event_Press:
                    bi = 2;
                    break;
                case GFxEventId::Event_Release:
                    bi = 3;
                    break;
                default:
                    bi = -1;
                    break;
                }
                if (bi >= 0)
                {
                    GFxButtonCharacterDef::GFxButtonSoundInfo& bs = pDef->Sound->ButtonSounds[bi];
                    // GFxCharacter zero is considered as null GFxCharacter
                    /*
                    if (bs.SoundId > 0)
                    {
                        GASSERT(pDef->Sound->ButtonSounds[bi].Sam != NULL);
                        if (bs.SoundStyle.StopPlayback)
                        {
                            s->StopSound(bs.Sam->SoundHandlerId);
                        }
                        else
                        {
                            s->PlaySound(bs.Sam->SoundHandlerId, bs.SoundStyle.LoopCount);
                        }
                    }
                    */
                }
            }
        #endif
        }

        // @@ eh, should just be a lookup table.
        int c = 0, kc = 0;
        if (event.Id == GFxEventId::Event_RollOver) c |= (GFxButtonAction::IDLE_TO_OVER_UP);
        else if (event.Id == GFxEventId::Event_RollOut) c |= (GFxButtonAction::OVER_UP_TO_IDLE);
        else if (event.Id == GFxEventId::Event_Press) c |= (GFxButtonAction::OVER_UP_TO_OVER_DOWN);
        else if (event.Id == GFxEventId::Event_Release) c |= (GFxButtonAction::OVER_DOWN_TO_OVER_UP);
        else if (event.Id == GFxEventId::Event_DragOut) c |= (GFxButtonAction::OVER_DOWN_TO_OUT_DOWN);
        else if (event.Id == GFxEventId::Event_DragOver) c |= (GFxButtonAction::OUT_DOWN_TO_OVER_DOWN);
        else if (event.Id == GFxEventId::Event_ReleaseOutside) c |= (GFxButtonAction::OUT_DOWN_TO_IDLE);
        else if (event.Id == GFxEventId::Event_KeyPress) 
        {
            // convert keycode/ascii to button's keycode
            kc = event.ConvertToButtonKeyCode();
        }
            //IDLE_TO_OVER_DOWN = 1 << 7,
        //OVER_DOWN_TO_IDLE = 1 << 8,

        // restart the characters of the new state.
        RestartCharacters(c);

        // Add appropriate actions to the GFxMovieSub's execute list...
        for (UInt i = 0; i < pDef->ButtonActions.size(); i++)
        {
            if (((pDef->ButtonActions[i].Conditions & (~0xFE00)) & c) ||     //!AB??
                (kc > 0 && ((pDef->ButtonActions[i].Conditions >> 9) & 0x7F) == kc))
            {
                // Matching action.
                GFxSprite* pparentSprite = GetParent()->ToSprite();                
                if (pparentSprite)
                {
                    GASStringContext* psc = pparentSprite->GetASEnvironment()->GetSC();
                    const UPInt n = pDef->ButtonActions[i].Actions.size();
                    for (UPInt j = 0; j < n; ++j)
                    {
                        if (!pDef->ButtonActions[i].Actions[j]->IsNull())
                        {
                            GPtr<GASActionBuffer> pbuff = 
                                *new GASActionBuffer(psc, pDef->ButtonActions[i].Actions[j]);
                            pparentSprite->AddActionBuffer(pbuff);
                        }
                    }
                    if (n > 0) handlerFound = true;
                }
            }
        }

        // Call conventional attached method.
        // Check for member function, it is called after onClipEvent(). 
        // In ActionScript 2.0, event method names are CASE SENSITIVE.
        // In ActionScript 1.0, event method names are CASE INSENSITIVE.
        GASEnvironment *penv = GetASEnvironment();
        GASString       methodName(event.GetFunctionName(penv->GetSC()));

        if (methodName.GetSize() > 0)
        {
            GASValue method;
            if (GetMember(penv, methodName, &method))
            {
                // do actual dispatch
                GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
                if (pe) pe->SetAction(this, event);

                handlerFound = true;
            }
        }
        return handlerFound;
    }


    void RestartCharacters(int condition)
    {
        // Restart our relevant characters
        for (UInt i = 0; i < pDef->ButtonRecords.size(); i++)
        {
            bool    restart = false;
            GFxButtonRecord* rec = &pDef->ButtonRecords[i];

            switch (MouseState)
            {
            case MOUSE_OVER:
                {
                    if ((rec->Over) && (condition & GFxButtonAction::IDLE_TO_OVER_UP))
                    {
                        restart = true;
                    }
                    break;
                }
            // @@ are there other cases where we restart stuff?
            default:
                {
                    break;
                }
            }

            if (restart == true)
            {
                RecordCharacter[i]->Restart();
            }
        }
    }



    //
    // ActionScript overrides
    //

    
    // GFxASCharacter override to indicate which standard members are handled for us.
    virtual UInt32  GetStandardMemberBitMask() const
    {
        // MovieClip lets base handle all members it supports.
        return  UInt32(
                M_BitMask_PhysicalMembers |         
                M_BitMask_CommonMembers |
                (1 << M_target) |
                (1 << M_url) |
                (1 << M_parent) |
                (1 << M_blendMode) |
                (1 << M_cacheAsBitmap) |
                (1 << M_filters) |
                (1 << M_focusrect) |
                (1 << M_enabled) |
                (1 << M_trackAsMenu) |                                  
                (1 << M_tabEnabled) |
                (1 << M_tabIndex) |
                (1 << M_useHandCursor) |
                (1 << M_quality) |
                (1 << M_highquality) |
                (1 << M_soundbuftime) |
                (1 << M_xmouse) |
                (1 << M_ymouse)                                                                 
                );
    // MA Verified: _lockroot does not exist/carry over from buttons, so don't include it.
    // If a movie is loaded into button, local _lockroot state is lost.
    }

    virtual GASObject*  GetASObject () { return ASButtonObj; }
    virtual GASObject*  GetASObject () const { return ASButtonObj; }

    virtual bool    GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
    {
        if (name.IsStandardMember())                    
            if (GetStandardMember(GetStandardMemberConstant(name), pval, 0))
                return true;        

        if (ASButtonObj)
        {
            return ASButtonObj->GetMember(penv, name, pval);
        }
        // Looks like _global is accessible from any character
        if (penv && (name == penv->GetBuiltin(GASBuiltin__global)))
        {
            pval->SetAsObject(penv->GetGC()->pGlobal);
            return true;
        }
        return false;
    }


    virtual bool GetStandardMember(StandardMember member, GASValue* pval, bool opcodeFlag) const
    {
        if (GFxASCharacter::GetStandardMember (member, pval, opcodeFlag))
            return true;

        // Handle MovieClip specific "standard" members.
        switch(member)
        {
            case M_scale9Grid:
                if (GetASEnvironment()->GetVersion() >= 8)
                {
                    if (pScale9Grid)
                    {
                        GASEnvironment* penv = const_cast<GASEnvironment*>(GetASEnvironment());

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
                        GPtr<GASRectangleObject> rectObj = *new GASRectangleObject(penv);
                        GASRect gr(TwipsToPixels(pScale9Grid->x), 
                                   TwipsToPixels(pScale9Grid->y), 
                                   TwipsToPixels(pScale9Grid->x+pScale9Grid->w), 
                                   TwipsToPixels(pScale9Grid->y+pScale9Grid->h)); 
                        rectObj->SetProperties(penv, gr);
#else
                        GPtr<GASObject> rectObj = *new GASObject(penv);
                        GASStringContext *psc = penv->GetSC();
                        rectObj->SetConstMemberRaw(psc, "x", TwipsToPixels(pScale9Grid->x));
                        rectObj->SetConstMemberRaw(psc, "y", TwipsToPixels(pScale9Grid->y));
                        rectObj->SetConstMemberRaw(psc, "width", TwipsToPixels(pScale9Grid->x+pScale9Grid->w));
                        rectObj->SetConstMemberRaw(psc, "height", TwipsToPixels(pScale9Grid->y+pScale9Grid->h));
#endif

                        pval->SetAsObject(rectObj);
                    }
                    else
                    {
                        pval->SetUndefined();
                    }
                    return true;
                }
                break;

            // extension
            case M_hitTestDisable:
                if (GetASEnvironment()->CheckExtensions())
                {
                    pval->SetBool(IsHitTestDisableFlagSet());
                    return 1;
                }
                break;

            default:
            break;
        }
        return false;
    }

    virtual bool SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag)
    {   
        if (GFxASCharacter::SetStandardMember (member, val, opcodeFlag))
            return true;

        // Handle MovieClip specific "standard" members.
        switch(member)
        {   
            case M_scale9Grid:
                if (GetASEnvironment()->GetVersion() >= 8)
                {
                    GASEnvironment* penv = GetASEnvironment();
                    GASObject* pobj = val.ToObject();

#ifndef GFC_NO_FXPLAYER_AS_RECTANGLE
                    if (pobj && pobj->GetObjectType() == Object_Rectangle)
                    {
                        GASRectangleObject* prect = (GASRectangleObject*)pobj;
                        GASRect gr;
                        prect->GetProperties(penv, gr);
                        GFxScale9Grid sg;
                        sg.x = PixelsToTwips(Float(gr.Left));
                        sg.y = PixelsToTwips(Float(gr.Top));
                        sg.w = PixelsToTwips(Float(gr.Width()));
                        sg.h = PixelsToTwips(Float(gr.Height()));
                        SetScale9Grid(&sg);
                    }
#else
                    if (pobj)
                    {
                        GASStringContext *psc = penv->GetSC();
                        GASValue params[4];
                        pobj->GetConstMemberRaw(psc, "x", &params[0]);
                        pobj->GetConstMemberRaw(psc, "y", &params[1]);
                        pobj->GetConstMemberRaw(psc, "width", &params[2]);
                        pobj->GetConstMemberRaw(psc, "height", &params[3]);
                        GFxScale9Grid sg;
                        sg.x = PixelsToTwips(Float(params[0].ToNumber(penv)));
                        sg.y = PixelsToTwips(Float(params[1].ToNumber(penv)));
                        sg.w = PixelsToTwips(Float(params[2].ToNumber(penv)));
                        sg.h = PixelsToTwips(Float(params[3].ToNumber(penv)));
                        SetScale9Grid(&sg);
                    }
#endif
                    else
                        SetScale9Grid(0);
                    return true;
                }
                break;

            // extension
            case M_hitTestDisable:
                if (GetASEnvironment()->CheckExtensions())
                {
                    SetHitTestDisableFlag(val.ToBool(GetASEnvironment()));
                    return 1;
                }
                break;
            // No other custom properties to set for now.
            default:
            break;
        }
        return false;
    }



    virtual bool OnKeyEvent(const GFxEventId& id, int* pkeyMask) 
    {
        // Check for member function.   
        // In ActionScript 2.0, event method names are CASE SENSITIVE.
        // In ActionScript 1.0, event method names are CASE INSENSITIVE.
        GASEnvironment *penv = GetASEnvironment();
        GASString       methodName(id.GetFunctionName(penv->GetSC()));

        if (methodName.GetSize() > 0)
        {           
            GASValue    method;
            if ((id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp) &&
                GetMemberRaw(penv->GetSC(), methodName, &method)) 
            {
                // onKeyDown/onKeyUp are available only in Flash 6 and later
                // (don't mess with onClipEvent (keyDown/keyUp)!)
                if (penv->GetVersion() >= 6 && GetMovieRoot()->IsKeyboardFocused(this))
                {
                    // also, onKeyDown/onKeyUp should be invoked only if focus
                    // is enabled and set to this button

                    // do actual dispatch
                    GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
                    if (pe) pe->SetAction(this, id);
                }
            }
        }

        if (id.Id == GFxEventId::Event_KeyDown)
        {
            // covert Event_KeyDown to Event_KeyPress
            GASSERT (pkeyMask != 0);

            // check if keyPress already was handled then do not handle it again
            if (!((*pkeyMask) & GFxCharacter::KeyMask_KeyPress))
            {
                if (OnButtonEvent(GFxEventId (GFxEventId::Event_KeyPress, id.KeyCode, id.AsciiCode)))
                {
                    *pkeyMask |= GFxCharacter::KeyMask_KeyPress;
                }
            }

            if (GetMovieRoot()->IsKeyboardFocused(this) && (id.KeyCode == GFxKey::Return || id.KeyCode == GFxKey::Space))
            {
                // if focused and enter - simulate on(press)/onPress and on(release)/onRelease
                OnButtonEvent(GFxEventId::Event_Press);

                GPtr<GFxASCharacter> thisHolder = this;
                GetMovieRoot()->Advance(0, 0); //??AB, allow on(press) to be executed 
                    // completely before on(release). Otherwise, these events may affect each other
                    // (see focusKB_test.swf). Need further investigations.

                OnButtonEvent(GFxEventId::Event_Release);
            }
        }
        return true;
    }

    UInt            GetCursorType() const 
    { 
        const GASEnvironment* penv = GetASEnvironment();
        GASValue val;

        if (const_cast<GFxButtonCharacter*>(this)->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_useHandCursor), &val))
        {
            if (val.ToBool(penv))
                return GFxMouseCursorEvent::HAND;
        }
        return GFxASCharacter::GetCursorType();
    }
};


//
// GFxButtonRecord
//

// Return true if we read a record; false if this is a null record.
bool    GFxButtonRecord::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    int flags = p->ReadU8();
    if (flags == 0) 
        return false;

    GFxStream*  pin = p->GetStream();
    
    HitTest     = flags & 8 ? true : false;
    Down        = flags & 4 ? true : false;
    Over        = flags & 2 ? true : false;
    Up          = flags & 1 ? true : false; 
    
    CharacterId     = GFxResourceId(p->ReadU16());   
    ButtonLayer     = p->ReadU16(); 
    pin->ReadMatrix(&ButtonMatrix);     

    if (tagType == 34)
    {
        pin->ReadCxformRgba(&ButtonCxform);
    }

    // SWF 8 features.

    // Has filters.
    if (flags & 0x10)
    {       
        // This skips filters for now.
        GFx_LoadFilters(pin, 0);
    }
    // Has custom blending.
    if (flags & 0x20)
    {       
        UByte   blendMode = pin->ReadU8();
        if ((blendMode < 1) || (blendMode>14))
        {
            GFC_DEBUG_WARNING(1, "ButtonRecord::Read - loaded blend mode out of range");
            blendMode = 1;
        }
        BlendMode = (GRenderer::BlendType) blendMode;
    }
    else
    {
        BlendMode = GRenderer::Blend_None;
    }

    // Note: 'Use Bitmap Caching' toggle does not seem to be serialized to flags, perhaps 
    // because it makes no sense in button records, since they cannot be animated (?).

    return true;
}


//
// GFxButtonAction
//


GFxButtonAction::~GFxButtonAction()
{
    for (UPInt i = 0, n = Actions.size(); i < n; i++)
    {
        Actions[i]->Release();
    }
    Actions.resize(0);
}

void    GFxButtonAction::Read(GFxStream* pin, GFxTagType tagType)
{
    // Read condition flags.
    if (tagType == GFxTag_ButtonCharacter)
    {
        Conditions = OVER_DOWN_TO_OVER_UP;
    }
    else
    {
        GASSERT(tagType == GFxTag_ButtonCharacter2);
        Conditions = pin->ReadU16();
    }

    // Read actions.
    pin->LogParseAction("-- actions in button\n"); // @@ need more info about which actions
    GASActionBufferData*    a = new GASActionBufferData;
    a->Read(pin);
    Actions.push_back(a);
}


//
// GFxButtonCharacterDef
//

GFxButtonCharacterDef::GFxButtonCharacterDef()
    :
    Sound(NULL),
    pScale9Grid(NULL)
// Constructor.
{
}

GFxButtonCharacterDef::~GFxButtonCharacterDef()
{
    delete Sound;
    delete pScale9Grid;
}


void GFxButtonCharacterDef::GFxSoundInfo::Read(GFxStream* in)
{
    InPoint = OutPoint = LoopCount = 0;
    in->ReadUInt(2);    // skip reserved bits.
    StopPlayback = in->ReadUInt(1) ? true : false;
    NoMultiple = in->ReadUInt(1) ? true : false;
    HasEnvelope = in->ReadUInt(1) ? true : false;
    HasLoops = in->ReadUInt(1) ? true : false;
    HasOutPoint = in->ReadUInt(1) ? true : false;
    HasInPoint = in->ReadUInt(1) ? true : false;
    if (HasInPoint) InPoint = in->ReadU32();
    if (HasOutPoint) OutPoint = in->ReadU32();
    if (HasLoops) LoopCount = in->ReadU16();
    if (HasEnvelope)
    {
        int nPoints = in->ReadU8();
        Envelopes.resize(nPoints);
        for (int i=0; i < nPoints; i++)
        {
            Envelopes[i].Mark44 = in->ReadU32();
            Envelopes[i].Level0 = in->ReadU16();
            Envelopes[i].Level1 = in->ReadU16();
        }
    }
    else
    {
        Envelopes.resize(0);
    }
    
    // Loggin output
    in->LogParse("  HasEnvelope = %d\n", HasEnvelope);
    in->LogParse("  HasLoops = %d\n", HasLoops);
    in->LogParse("  HasOutPoint = %d\n", HasOutPoint);
    in->LogParse("  HasInPoint = %d\n", HasInPoint);
    in->LogParse("  InPoint = %d\n", (int)InPoint);
    in->LogParse("  OutPoint = %d\n", (int)OutPoint);
    in->LogParse("  LoopCount = %d\n", LoopCount);
    in->LogParse("  envelope size = %d\n", (int)Envelopes.size());       

}



// Initialize from the given GFxStream.
void    GFxButtonCharacterDef::Read(GFxLoadProcess* p, GFxTagType tagType)
{
    GASSERT(tagType == GFxTag_ButtonCharacter ||
            tagType == GFxTag_ButtonSound ||
            tagType == GFxTag_ButtonCharacter2);

    if (tagType == GFxTag_ButtonCharacter)
    {
        // Old button tag.
            
        // Read button GFxCharacter records.
        for (;;)
        {
            GFxButtonRecord r;
            if (r.Read(p, tagType) == false)
            {
                // Null record; marks the end of button records.
                break;
            }

            // Search for the depth and insert in the right location. This ensures
            // that buttons are always drawn correctly.
            UInt i;
            for(i=0; i<ButtonRecords.size(); i++)
                if (ButtonRecords[i].ButtonLayer > r.ButtonLayer)
                    break;
            ButtonRecords.insert(i, r);
        }

        // Read actions.
        ButtonActions.resize(ButtonActions.size() + 1);
        ButtonActions.back().Read(p->GetStream(), tagType);
    }

    else if (tagType == GFxTag_ButtonSound)
    {
        GASSERT(Sound == NULL); // redefinition button sound is error
        Sound = new GFxButtonSoundDef();
        p->LogParse("button sound options:\n");
        for (int i = 0; i < 4; i++)
        {
            GFxButtonSoundInfo& bs = Sound->ButtonSounds[i];
            bs.SoundId = p->ReadU16();
            if (bs.SoundId > 0)
            {
             //   bs.Sam = (GFxSoundSampleImpl*) p->GetResource(GFxResourceId(bs.SoundId),
             //                                                 GFxResource::RT_SoundSample);
                if (bs.Sam == NULL)
                {
//                      printf("sound tag not found, SoundId=%d, button state #=%i", bs.SoundId, i);
                }
                p->LogParse("\n    SoundId = %d\n", bs.SoundId);
                bs.SoundStyle.Read(p->GetStream());
            }
        }
    }

    else if (tagType == GFxTag_ButtonCharacter2)
    {
        // Read the menu flag.
        Menu = p->ReadU8() != 0;

        int Button2_actionOffset = p->ReadU16();
        int NextActionPos = p->Tell() + Button2_actionOffset - 2;

        // Read button records.
        for (;;)
        {
            GFxButtonRecord r;
            if (r.Read(p, tagType) == false)
            {
                // Null record; marks the end of button records.
                break;
            }
            // Search for the depth and insert in the right location.
            UInt i;
            for(i=0; i<ButtonRecords.size(); i++)
                if (ButtonRecords[i].ButtonLayer > r.ButtonLayer)
                    break;
            ButtonRecords.insert(i, r);
        }

        if (Button2_actionOffset > 0)
        {
            p->SetPosition(NextActionPos);

            // Read Button2ActionConditions
            for (;;)
            {
                int NextActionOffset = p->ReadU16();
                NextActionPos = p->Tell() + NextActionOffset - 2;

                ButtonActions.resize(ButtonActions.size() + 1);
                ButtonActions.back().Read(p->GetStream(), tagType);

                if (NextActionOffset == 0
                    || p->Tell() >= p->GetTagEndPosition())
                {
                    // done.
                    break;
                }

                // seek to next action.
                p->SetPosition(NextActionPos);
            }
        }
    }
}


// Create a mutable instance of our definition.
GFxCharacter*   GFxButtonCharacterDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                               GFxMovieDefImpl *pbindingImpl)
{
    GFxCharacter*   ch = new GFxButtonCharacter(this, pbindingImpl, parent, id);
    return ch;
}


void GASButtonObject::commonInit ()
{
}

static const GASNameFunction GAS_ButtonFunctionTable[] = 
{
    { "getDepth",       &GFxASCharacter::CharacterGetDepth },

    { 0, 0 }
};

GASButtonProto::GASButtonProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor) :
    GASPrototype<GASButtonObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, GAS_ButtonFunctionTable, prototype);
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_useHandCursor), GASValue(true), 
        GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASButtonProto::GlobalCtor(const GASFnCall& fn) 
{
    fn.Result->SetAsObject(GPtr<GASObject>(*new GASButtonObject()));
}
