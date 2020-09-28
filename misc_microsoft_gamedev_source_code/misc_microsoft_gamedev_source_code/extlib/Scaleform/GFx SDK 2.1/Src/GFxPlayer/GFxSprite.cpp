/**********************************************************************

Filename    :   GFxSprite.cpp
Content     :   Implementation of MovieClip character
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxSprite.h"
#include "GFxAction.h"
#include "GFxButton.h"
#include "GFxLoaderImpl.h"
#include "GFxTransform.h"
#include "GFxFontResource.h"
#include "GFxLog.h"
#include "GFxMorphCharacter.h"
#include "GFxShape.h"
#include "GFxArray.h"
#include "GFxMatrix.h"
#include "GFxStream.h"
#include "GFxStyles.h"
#include "GFxDlist.h"
//#include "GFxTimers.h"

#include "GFxPlayer.h"

#include "GFxLoadProcess.h"
#include "GFxDisplayContext.h"

#include "GFxBitmapData.h"

#include <string.h> // for memset
#include <float.h>
#include <stdlib.h>
#ifdef GFC_MATH_H
#include GFC_MATH_H
#else
#include <math.h>
#endif

// Version numbers for $version and getVersion()
// TBD: Perhaps these should be moved into a different header?
#if defined(GFC_OS_WIN32)
#define GFX_FLASH_VERSION "WIN 8,0,0,0"
#elif defined(GFC_OS_MAC)
#define GFX_FLASH_VERSION "MAC 8,0,0,0"
#elif defined(GFC_OS_LINUX)
#define GFX_FLASH_VERSION "LINUX 8,0,0,0"
#elif defined(GFC_OS_XBOX360)
#define GFX_FLASH_VERSION "XBOX360 8,0,0,0"
#elif defined(GFC_OS_XBOX)
#define GFX_FLASH_VERSION "XBOX 8,0,0,0"
#elif defined(GFC_OS_PSP)
#define GFX_FLASH_VERSION "PSP 8,0,0,0"
#elif defined(GFC_OS_PS2)
#define GFX_FLASH_VERSION "PS2 8,0,0,0"
#elif defined(GFC_OS_PS3)
#define GFX_FLASH_VERSION "PS3 8,0,0,0"
#else
#define GFX_FLASH_VERSION "GFX 8,0,0,0"
#endif

//
// ***** GFxSpriteDef
//

GFxSpriteDef::GFxSpriteDef(GFxMovieDataDef* pmd)
    :
    pMovieDef(pmd),
    FrameCount(0),
    LoadingFrame(0),
    pScale9Grid(0)
{   
    GASSERT(pMovieDef);
}

GFxSpriteDef::~GFxSpriteDef()
{
    // Destroy frame data; the actual de-allocation of tag memory
    // takes place in the GFxMovieDataDef's tag block allocator.
    UInt i;
    for(i=0; i<Playlist.size(); i++)
        Playlist[i].DestroyTags();        
    delete pScale9Grid;
}

// Create A (mutable) instance of our definition.  The instance is created to
// live (temporarily) on some level on the parent GFxASCharacter's display list.
GFxCharacter*   GFxSpriteDef::CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                      GFxMovieDefImpl *pbindingImpl)
{
    // Most of the time pbindingImpl will be ResourceMovieDef come from parents scope.
    // In some cases we call CreateCharacterInstance with a different binding context
    // then the parent sprite; this can happen for import-bound characters, for example.
    GFxSprite*  si = new GFxSprite(this, pbindingImpl,
                                   parent->GetMovieRoot(), parent, id);
    return si;
}


// Labels the frame currently being loaded with the
// given name.  A copy of the name string is made and
// kept in this object.
void    GFxSpriteDef::AddFrameName(const char* name, GFxLog *plog)
{
    GASSERT(LoadingFrame >= 0 && LoadingFrame < FrameCount);

    GFxString   n = name;
    UInt        currentlyAssigned = 0;
    if (NamedFrames.get(n, &currentlyAssigned) == true)
    {
        if (plog)
            plog->LogError("AddFrameName(%d, '%s') -- frame name already assigned to frame %d; overriding\n",
              LoadingFrame, name, currentlyAssigned);
    }
    NamedFrames.set(n, LoadingFrame);   // stores 0-based frame #
}


// Read the sprite info.  Consists of a series of tags.
void    GFxSpriteDef::Read(GFxLoadProcess* p, GFxResourceId charId)
{
    GFxStream*  pin     = p->GetStream();
    UInt32      tagEnd = pin->GetTagEndPosition();


    p->EnterSpriteDef(this);

    FrameCount = pin->ReadU16();

    // ALEX: some SWF files have been seen that have 0-frame sprites.
    // The Macromedia player behaves as if they have 1 frame.
    if (FrameCount < 1)    
        FrameCount = 1;    
    Playlist.resize(FrameCount);    // need a playlist for each frame

    pin->LogParse("  frames = %d\n", FrameCount);

    LoadingFrame = 0;

    while ((UInt32) pin->Tell() < tagEnd)
    {
        GFxTagInfo  tagInfo;
        GFxTagType  tagType = pin->OpenTag(&tagInfo);
        GFxLoaderImpl::LoaderFunction lf = NULL;

        if (tagType == GFxTag_EndFrame)
        {
            // NOTE: In very rare cases FrameCount can LIE, containing
            // more frames then was reported in sprite header (wizardy.swf).
            if (LoadingFrame == (int)Playlist.size())
                Playlist.resize(Playlist.size()+1);

            p->CommitFrameTags();

            // show frame tag -- advance to the next frame.
            pin->LogParse("  ShowFrame (sprite, char id = %d)\n", charId.GetIdIndex());
            LoadingFrame++;
        }
        else if (GFxLoaderImpl::GetTagLoader(tagType, &lf))            
        {
            // call the tag loader.  The tag loader should add
            // characters or tags to the GFxASCharacter data structure.
            (*lf)(p, tagInfo);
        }
        else
        {
            // no tag loader for this tag type.
            pin->LogParse("*** no tag loader for type %d\n", tagType);
        }

        pin->CloseTag();
    }

    // In general, we should have EndFrame
    if (p->FrameTagsAvailable())
    {
        p->CommitFrameTags();
        // ??
        // LoadingFrame++;
    }


    p->LeaveSpriteDef();

    pin->LogParse("  -- sprite END, char id = %d --\n", charId.GetIdIndex());
}

// Initialize an empty clip.
void GFxSpriteDef::InitEmptyClipDef()
{
    // Set FrameCount = 1; that is the default for an empty clip.
    FrameCount = 1;
    Playlist.resize(FrameCount);
}

bool GFxSpriteDef::DefPointTestLocal(const GPointF &pt, bool testShape, const GFxCharacter* pinst) const  
{ 
    GUNUSED3(pt, testShape, pinst);
    return false;
}


//
// ***** GFxSprite Implementation
//


GFxSprite::GFxSprite(GFxTimelineDef* pdef, GFxMovieDefImpl* pdefImpl,
                     GFxMovieRoot* pr, GFxASCharacter* pparent,
                     GFxResourceId id, bool loadedSeparately)
    :
    GFxASCharacter(pdefImpl, pparent, id),
    pDef(pdef),    
    pRoot(pr),
    pScale9Grid(0),
    PlayStatePriv(GFxMovie::Playing),
    CurrentFrame(0),    
    Level(-1),
    pRootNode(0),
    Flags(0),
    MouseStatePriv(UP)
{
    GASSERT(pDef && pDefImpl);
    GASSERT(pRoot != NULL);

    if (pdef->GetResourceType() == GFxResource::RT_SpriteDef)
    {
        GFxSpriteDef* sp = static_cast<GFxSpriteDef*>(pdef);
        SetScale9Grid(sp->GetScale9Grid());
    }
   
    ASEnvironment.SetTargetOnConstruct(this);

    SetUpdateFrame(true);
    SetHasLoopedPriv(true);

    // By default LockRoot is false.
    SetLockRoot(false);
    SetLoadedSeparately(loadedSeparately);
    
    // Since loadedSeparately flag does not get set for imports (because it only
    // enables _lockroot behavior not related to imports), we need to check for
    // pMovieDefImpl specifically. Imports receive root node so that they can
    // have their own font manager.
    bool importFlag = pparent && !loadedSeparately && 
                      (pparent->GetResourceMovieDef() != pdefImpl);

    if (loadedSeparately || !pparent || importFlag)
    {
        // We share the GFxMovieDefRootNode with other root sprites based on a ref 
        // count. Try to find the root node for the same MovieDef and import flag
        // as ours, if not found create one.
       
        // An exhaustive search should be ok here since we don't expect too
        // many different MovieDefs and root sprite construction happens rarely;
        // furthermore, a list is cheap to traverse in Advance where updates need
        // to take place. If this becomes a bottleneck we could introduce a
        // hash table in movie root.    
        GFxMovieDefRootNode *pnode = pRoot->RootMovieDefNodes.GetFirst();

        while(!pRoot->RootMovieDefNodes.IsNull(pnode))
        {
            if ((pnode->pDefImpl == pDefImpl) && (pnode->ImportFlag == importFlag))
            {
                // Found node identical to us.
                pnode->SpriteRefCount++;
                pRootNode = pnode;
                break;
            }
            pnode = pnode->pNext;
        }

        // If compatible root node not found, create one.
        if (!pRootNode)
        {
            pRootNode = new GFxMovieDefRootNode(pDefImpl, importFlag);
            // Bytes loaded must be grabbed first due to data race.        
            pRootNode->BytesLoaded  = pDefImpl->GetBytesLoaded();
            pRootNode->LoadingFrame = importFlag ? 0 : pDefImpl->GetLoadingFrame();

            // Create a local font manager.
            // Fonts created for sprite will match bindings of our DefImpl.
            pRootNode->pFontManager = *new GFxFontManager(pDefImpl, pr->pSharedState);
            pRoot->RootMovieDefNodes.PushFront(pRootNode);
        }
    }

    // Initialize the flags for init action executed.
    UInt frameCount = pDef->GetFrameCount();
    GASSERT(frameCount != 0);

    InitActionsExecuted.resize(frameCount);
    memset(&InitActionsExecuted[0], 0, sizeof(InitActionsExecuted[0]) * frameCount);

    ASMovieClipObj = *new GASMovieClipObject(GetGC(), this);

    // let the base class know what's going on
    pProto = ASMovieClipObj->Get__proto__();

    // NOTE: DoInitActions are executed separately in Advance.
}


GFxSprite::~GFxSprite()
{
    if (pRootNode)    
    {
        pRootNode->SpriteRefCount--;
        if (pRootNode->SpriteRefCount == 0)
        {
            pRoot->RootMovieDefNodes.Remove(pRootNode);
            delete pRootNode;
        }
    }

    ClearDisplayList();
    delete pScale9Grid;
    //Root->DropRef();
}

void GFxSprite::SetDirtyFlag() 
{ 
    pRoot->SetDirtyFlag(); 
}

void GFxSprite::SetRootNodeLoadingStat(UInt bytesLoaded, UInt loadingFrame)
{
    if (pRootNode)
    {
        pRootNode->BytesLoaded = bytesLoaded;
        pRootNode->LoadingFrame = pRootNode->ImportFlag ? 0 : loadingFrame;
    }
}

void GFxSprite::ForceShutdown ()
{
    //ClearDisplayList();
    //pRoot->DropRef();

    // to prevent memory leak we need to traverse through all variables in environment and members
    // and release all local frames in GASFunctionRef. This is necessary until we have 
    // garbage collection. (!AB)
    ASEnvironment.ForceReleaseLocalFrames();

    if (ASMovieClipObj)
        ASMovieClipObj->ForceShutdown();

}

void    GFxSprite::ClearDisplayList()
{
    DisplayList.Clear();
    SetDirtyFlag();
}


// These accessor methods have custom implementation in GFxSprite.
GFxMovieDefImpl* GFxSprite::GetResourceMovieDef() const
{
    // Return the definition where binding takes place.
    return pDefImpl.GetPtr();
}

GFxFontManager*  GFxSprite::GetFontManager() const
{
    if (pRootNode)
        return pRootNode->pFontManager;
    return GetParent()->GetFontManager();
}

GFxMovieRoot*   GFxSprite::GetMovieRoot() const
{
    return pRoot;
}
GFxASCharacter* GFxSprite::GetLevelMovie(SInt level) const
{   
    return pRoot->GetLevelMovie(level);
}

GFxASCharacter* GFxSprite::GetASRootMovie() const
{
    // Return _root, considering _lockroot. 
    if (!GetParent() ||
        (IsLoadedSeparately() && IsLockRoot()) )
    {
        // If no parent, we ARE the root.
        return const_cast<GFxSprite*>(this);
    }

    // HACK!
    if (IsUnloaded())
    {
        GFC_DEBUG_WARNING(1, "GetASRootMovie called on unloaded sprite");
        return pRoot->GetLevelMovie(0);
    }

    return GetParent()->GetASRootMovie();
}

UInt32  GFxSprite::GetBytesLoaded() const
{  
    const GFxASCharacter *pchar = this;
    // Find a load root sprite and grab bytes loaded from it.
    while(pchar)
    {
        const GFxSprite* psprite = pchar->ToSprite();
        if (psprite && psprite->pRootNode)
            return psprite->pRootNode->BytesLoaded;
        pchar = pchar->GetParent();
    }
    // We should never end up here.
    return 0;
}

void    GFxSprite::SetLevel(SInt level)
{
    Level = level;

    // Assign the default name to level.
    // Note that for levels, Flash allows changing these names later,
    // but such new names can not be used for path lookup.
    char nameBuff[64] = "";
    gfc_sprintf(nameBuff, 64, "_level%d", level);
    SetName(ASEnvironment.CreateString(nameBuff));
}


// Returns frames loaded, sensitive to background loading.
UInt    GFxSprite::GetLoadingFrame() const
{
    return (pRootNode && !pRootNode->ImportFlag) ?
            pRootNode->LoadingFrame : GetFrameCount();
}

// Checks that load was called and does so if necessary.
// Used for root-level sprites.
void    GFxSprite::ExecuteFrame0Events()
{
    if (!IsOnEventLoadCalled())
    {       
        // Must do loading events for root level sprites.
        // For child sprites this is done by the dlist,
        // but root movies don't get added to a dlist, so we do it here.
        // Also, this queues up tags and actions for frame 0.
        SetOnEventLoadCalled();
        // Can't call OnLoadEvent here because implementation of onLoad handler is different
        // for the root: it seems to be called after the first frame as a special case, presumably
        // to allow for load actions to exits. That, however, is NOT true for nested movie clips.
        ExecuteFrameTags(0);
        pRoot->InsertEmptyAction(GFxMovieRoot::AP_Load)->SetAction(this, GFxEventId::Event_Load);
    }
}

void    GFxSprite::OnIntervalTimer(void *timer)
{
    GUNUSED(timer);
}   


// "transform" matrix describes the transform applied to parent and us,
// including the object's matrix itself. This means that if transform is
// identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
GRectF  GFxSprite::GetBounds(const Matrix &transform) const
{
    GFxCharacter*   ch;
    UInt            i, n = DisplayList.GetCharacterCount();
    GRectF          r(0);   
    GRectF          tempRect;   
    Matrix          m;

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL)
        {
            // Build transform to target.       
            m = transform;
            m *= ch->GetMatrix();
            // get bounds transformed by matrix 
            tempRect = ch->GetBounds(m);                        

            if (!tempRect.IsEmpty ())
            {
                if (!r.IsEmpty ())
                    r.Union(tempRect);
                else
                    r = tempRect;
            }
        }
    }

    if (pDrawingAPI)
    {
        pDrawingAPI->ComputeBound(&tempRect);
        tempRect = transform.EncloseTransform(tempRect);
        if (!r.IsEmpty())
            r.Union(tempRect);
        else
            r = tempRect;
    }

    return r;
}


// "transform" matrix describes the transform applied to parent and us,
// including the object's matrix itself. This means that if transform is
// identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
GRectF  GFxSprite::GetRectBounds(const Matrix &transform) const
{
    GFxCharacter*   ch;
    UInt            i, n = DisplayList.GetCharacterCount();
    GRectF          r(0);   
    GRectF          tempRect;   
    Matrix          m;

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL)
        {
            // Build transform to target.       
            m = transform;
            m *= ch->GetMatrix();
            // get bounds transformed by matrix 
            tempRect = ch->GetRectBounds(m);                        

            if (!tempRect.IsEmpty ())
            {
                if (!r.IsEmpty ())
                    r.Union(tempRect);
                else
                    r = tempRect;
            }
        }
    }
    return r;
}


void    GFxSprite::Restart()
{
    CurrentFrame = 0;   
    SetUpdateFrame();
    SetHasLoopedPriv(false);
    PlayStatePriv = GFxMovie::Playing;

    ExecuteFrameTags(CurrentFrame);
    DisplayList.RemoveMarkedObjects();
    SetDirtyFlag();
}
    
void    GFxSprite::CalcDisplayListHitTestMaskArray(GTL::garray<UByte> *phitTest, const GPointF &pt, bool testShape) const
{
    GUNUSED(testShape);

    int i, n = DisplayList.GetCharacterCount();

    // Mask support
    for (i = 0; i < n; i++)
    {
        GFxCharacter* pmaskch = DisplayList.GetCharacter(i);

        if (pmaskch->GetClipDepth() > 0)
        {
            if (phitTest->size()==0)
            {
                phitTest->resize(n);
                memset(&(*phitTest)[0], 1, n);
            }

            GRenderer::Matrix   m = pmaskch->GetMatrix();
            GPointF             p = m.TransformByInverse(pt);

            (*phitTest)[i] = pmaskch->PointTestLocal(p, HitTest_TestShape);

            int k = i+1;            
            while (k < n)
            {
                GFxCharacter* pch = DisplayList.GetCharacter(k);
                if (pch && (pch->GetDepth() > pmaskch->GetClipDepth()))
                    break;
                (*phitTest)[k] = (*phitTest)[i];
                k++;
            }
            i = k-1;
        }
    }
}

// Return the topmost entity that the given point covers.  NULL if none.
// Coords are in parent's frame.
GFxASCharacter* GFxSprite::GetTopMostMouseEntity(const GPointF &pt, bool testAll, const GFxASCharacter* ignoreMC)
{
    // Invisible sprites/buttons don't receive mouse events (i.e. are disabled).
    // Masks also shouldn't receive mouse events (!AB)
    if (IsHitTestDisableFlagSet() || !GetVisible() || IsUsedAsMask())
        return 0;

    if (ignoreMC == this)
        return 0;

    GRenderer::Matrix   m = GetMatrix();
    GPointF             p = m.TransformByInverse(pt);   

    int i, n = DisplayList.GetCharacterCount();
    
    GFxSprite* pmask = GetMask();  
    if (pmask)
    {
        if (pmask->IsUsedAsMask() && !pmask->IsUnloaded())
        {
            GRenderer::Matrix matrix;
            matrix.SetInverse(pmask->GetWorldMatrix());
            matrix *= GetWorldMatrix();
            GPointF pp = matrix.Transform(p);

            if (!pmask->PointTestLocal(pp, HitTest_TestShape))
                return NULL;
        }
    }

    GTL::garray<UByte> hitTest;
    CalcDisplayListHitTestMaskArray(&hitTest, p, 1);

    // Go backwards, to check higher objects first.
    for (i = n - 1; i >= 0; i--)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);

        if (hitTest.size() && (!hitTest[i] || ch->GetClipDepth()>0))
            continue;

        if (ch->IsTopmostLevelFlagSet()) // do not check children w/topmostLevel
            continue;

        // MA: This should consider submit masks in the display list,
        // such masks can obscure/clip out buttons for the purpose of
        // hit-testing as well.
        
        if (ch != NULL)
        {           
            GFxASCharacter* te = ch->GetTopMostMouseEntity(p, testAll, ignoreMC);

            if (te && testAll) // needed for mouse wheel
                return te;

            // If we found anything and we are in button mode, this refers to us.
            if (ActsAsButton())
            {
                // It is either child or us; no matter - button mode takes precedence.
                if (te)
                    return this;
            }
            else if (te && te != this)
            {
                // Found one.
                // @@ TU: GetVisible() needs to be recursive here!
                
                // character could be _root, in which case it would have no parent.
                if (te->GetParent() && te->GetParent()->GetVisible())
                //if (te->GetVisible()) // TODO move this check to the base case(s) only
                {
                    // @@
                    // Vitaly suggests "return ch" here, but it breaks
                    // samples/test_button_functions.swf
                    //
                    // However, it fixes samples/clip_as_button.swf
                    //
                    // What gives?
                    //
                    // Answer: a button event must be passed up to parent until
                    // somebody handles it.
                    //
                    return te;
                }
                else
                {
                    return 0;
                }
            }           
        }
    }
    if (pDrawingAPI && ActsAsButton())
    {
        if (pDrawingAPI->DefPointTestLocal(pt, true, this))
            return this;
    }
    return 0;
}

// Increment CurrentFrame, and take care of looping.
void    GFxSprite::IncrementFrameAndCheckForLoop()
{
    CurrentFrame++;
        
    UInt loadingFrame = GetLoadingFrame();
    UInt frameCount   = pDef->GetFrameCount();

    if ((loadingFrame < frameCount) && (CurrentFrame >= loadingFrame))
    {
        // We can not advance past the loading frame, since those tags are not yet here.        
        CurrentFrame = (loadingFrame > 0) ? loadingFrame - 1 : 0;
    }
    else if (CurrentFrame >= frameCount)
    {
        // Loop.
        CurrentFrame = 0;
        SetHasLoopedPriv(true);
        if (frameCount > 1)
            DisplayList.MarkAllEntriesForRemoval();     
    }
    SetDirtyFlag();
}


void    GFxSprite::AdvanceFrame(bool nextFrame, Float framePos)
{
    if (IsAdvanceDisabled())
        return;

    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);

    GASSERT(pDef && pRoot != NULL);

    // Adjust x,y of this character if it is being dragged.
    if (pRoot->MouseSupportEnabled)
        GFxASCharacter::DoMouseDrag();

    // So, here is the problem. EnterFrame should be processed in order reverse to order of tags processing.
    // That is why we need to dance with saving/restoring action queue positions....

    // Save insert location for child actions. Child actions have to be
    // queued before frame actions; however, frame tags affect their presence.
    GFxMovieRoot::ActionEntry *pchildInitClipPos    = pRoot->ActionQueue.GetInsertEntry(GFxMovieRoot::AP_InitClip);
    GFxMovieRoot::ActionEntry *pchildActionPos      = pRoot->ActionQueue.GetInsertEntry(GFxMovieRoot::AP_Frame);

    // Check for the end of frame
    if (nextFrame)
    {   
        // Update current and next frames.
        if (PlayStatePriv == GFxMovie::Playing)
        {
            UInt    CurrentFrame0 = CurrentFrame;
            IncrementFrameAndCheckForLoop();
                
            // Execute the current frame's tags.
            if (CurrentFrame != CurrentFrame0)
            {

#if 0
    // Shape dump logic for external debugging.
    FILE* fd = fopen("shapes", "at");
    fprintf(fd, "Frame %d\n", CurrentFrame);
    fclose(fd);
#endif 

                // Init action tags must come before Event_EnterFrame
                ExecuteInitActionFrameTags(CurrentFrame);

                // Post OnEnterFrame event, so that it arrives before other
                // frame actions generated by frame tags.
                OnEvent(GFxEventId::Event_EnterFrame);

                ExecuteFrameTags(CurrentFrame);
            }
            else
            {
                // Post OnEnterFrame event, so that it arrives before other
                // frame actions generated by frame tags.
                OnEvent(GFxEventId::Event_EnterFrame);
            }
        }
        else
        {
            OnEvent(GFxEventId::Event_EnterFrame);
        }

        // Clean up display List (remove dead objects). Such objects
        // can only exist if we looped around to frame 0.
        if (CurrentFrame == 0)
            DisplayList.RemoveMarkedObjects();
    }
    GFxMovieRoot::ActionEntry *psaveInitClipPos = pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_InitClip, pchildInitClipPos);
    GFxMovieRoot::ActionEntry *psaveActionPos   = pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_Frame, pchildActionPos);

    // Advance everything in the display list.
    // This call will automatically skip advancing children that were just loaded.
    DisplayList.AdvanceFrame(nextFrame, framePos);

    // Restore to tail location, but only if there were some actions generated by
    // the current frame logic at tail after children.
    // If no such actions existed, the new insert location is correct.
    if (psaveInitClipPos != pchildInitClipPos)
        pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_InitClip, psaveInitClipPos);
    if (psaveActionPos != pchildActionPos)
        pRoot->ActionQueue.SetInsertEntry(GFxMovieRoot::AP_Frame, psaveActionPos);
}


// Execute the tags associated with the specified frame.
// frame is 0-based
void    GFxSprite::ExecuteFrameTags(UInt frame, bool stateOnly, bool noRemove, GFxActionPriority::Priority prio)
{
    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);

    GASSERT(frame != UInt(~0));
    //GASSERT(frame < GetLoadingFrame());
    if (frame >= GetLoadingFrame())
        return;

    // Execute this frame's init actions, if necessary.
    ExecuteInitActionFrameTags(frame);
    
    const Frame playlist = pDef->GetPlaylist(frame);
    for (UInt i = 0; i < playlist.GetTagCount(); i++)
    {
        GASExecuteTag*  e = playlist.GetTag(i);

        if (noRemove && e->IsRemoveTag())
            continue;

        if (stateOnly)
        {
            e->ExecuteState(this);
        }
        else
        {
            //e->Execute(this);
            e->ExecuteWithPriority(this, prio);
        }
    }
    SetDirtyFlag();
}

// Executes init action tags only for the frame.
void    GFxSprite::ExecuteInitActionFrameTags(UInt frame)
{       
    // Execute this frame's init actions, if necessary.
    if (InitActionsExecuted[frame] == false)
    {
        // Keep this (particularly GASEnvironment) alive during execution!
        GPtr<GFxSprite> thisPtr(this);

        bool markFrameAsVisited = false;
   
        /*
        // execute init clip actions for each first time used import movies.
        const GTL::garray<GFxMovieDataDef*>* pimportMoviesSet = pDef->GetImportSourceMoviesForFrame(frame);
        if (pimportMoviesSet && pimportMoviesSet->size() > 0)
        {
            // iterate through all import source movies for the current frame
            for(UInt f = 0, nf = pimportMoviesSet->size(); f < nf; ++f)
            {
                GFxMovieDefSub* psrcImportMovie = (*pimportMoviesSet)[f];
                GASSERT(psrcImportMovie);
                if (psrcImportMovie->HasInitActions())
                {
                    ExecuteImportedInitActions(psrcImportMovie);
                }
            }
            markFrameAsVisited = true;
        }
        */

        GFxTimelineDef::Frame initActionsFrame;        
       
        if (pDef->GetInitActions(&initActionsFrame, frame) &&
            initActionsFrame.GetTagCount() > 0)
        {
            // Need to execute these actions.
            for (UInt i= 0; i < initActionsFrame.GetTagCount(); i++)
            {
                GASExecuteTag*  e = initActionsFrame.GetTag(i);
                e->Execute(this);
            }
            markFrameAsVisited = true;
        }
        if (markFrameAsVisited)
        {
            // Mark this frame done, so we never execute these init actions again.
            InitActionsExecuted[frame] = true;
        }
    }
}

void GFxSprite::ExecuteImportedInitActions(GFxMovieDef* psourceMovie)
{
    GASSERT(psourceMovie);

    GFxMovieDefImpl* pdefImpl = static_cast<GFxMovieDefImpl*>(psourceMovie);
    GFxMovieDataDef* pdataDef = pdefImpl->GetDataDef();

    for(UPInt f = 0, fc = pdataDef->GetInitActionListSize(); f < fc; ++f)
    {
        Frame actionsList;
        if (pdataDef->GetInitActions(&actionsList, (int)f))
        {
            for(UInt i = 0; i < actionsList.GetTagCount(); ++i)
            {
                GASExecuteTag*  e = actionsList.GetTag(i);
                // InitImportActions must get the right binding context since
                // their binding scope does not match that of sprite.
                if (e->IsInitImportActionsTag())
                    ((GFxInitImportActions*)e)->ExecuteInContext(this, pdefImpl);
                else
                    e->ExecuteWithPriority(this, GFxMovieRoot::AP_Highest);
            }
        }        
    }
    SetDirtyFlag();
}


// Add the given action buffer to the list of action
// buffers to be processed at the end of the next frame advance in root.    
void    GFxSprite::AddActionBuffer(GASActionBuffer* a, GFxActionPriority::Priority prio) 
{
    GFxMovieRoot::ActionEntry* pe = pRoot->InsertEmptyAction(prio);
    if (pe) pe->SetAction(this, a);
}



// Execute the tags associated with the specified frame, IN REVERSE.
// I.E. if it's an "add" tag, then we do a "remove" instead.
// Only relevant to the display-list manipulation tags: add, move, remove, replace.
//
// frame is 0-based
void    GFxSprite::ExecuteFrameTagsReverse(UInt frame)
{
    // Keep this (particularly GASEnvironment) alive during execution!
    GPtr<GFxSprite> thisPtr(this);

    GASSERT(frame != UInt(~0));
    GASSERT(frame < GetLoadingFrame());

    const Frame playlist = pDef->GetPlaylist(frame);
    // Playlist needs to be executed backwards here, so that Add/Remove tags don't conflict.    
    for (int i = (int)playlist.GetTagCount()-1; i >= 0; i--)
    {
        GASExecuteTag*  e = playlist.GetTag(i);
        e->ExecuteStateReverse(this, frame);
    }
    SetDirtyFlag();
}

GFxPlaceObject2* GFxSprite::FindPreviousPlaceObjectInfo(GFxCharPosInfo *ppos, UInt *pframe, int depth, GFxResourceId id, UInt findPrevFlags)
{
    GFxPlaceObject2* ptag;
    GFxPlaceObject2* pprevTag;
    UInt             currentFrame = *pframe;

    // Find the tag.
    ptag = FindPreviousPlaceObject2(&currentFrame, depth, id);
    if (!ptag) return 0;
    
    if (!(findPrevFlags & FindPrev_AllowMove))
    {
        // When servicing a 'Remove', we may be looking for Add/Replace tag, so
        // allowMove would be disabled. However, we still need to record the new move position.

        while(ptag->PlaceType == GFxPlaceObject2::Place_Move)
        {
            // Store matrix and cxform overrides.
            if (!ppos->HasMatrix && ptag->Pos.HasMatrix)
            {
                ppos->HasMatrix = 1;
                ppos->Matrix_1 = ptag->Pos.Matrix_1;
            }
            if (!ppos->HasCxform && ptag->Pos.HasCxform)
            {
                ppos->HasCxform = 1;
                ppos->ColorTransform = ptag->Pos.ColorTransform;
            }
            if (!ppos->BlendMode && ptag->Pos.BlendMode)
            {
                ppos->BlendMode = ptag->Pos.BlendMode;
            }

            ptag = FindPreviousPlaceObject2(&currentFrame, depth, id);
            if (!ptag) return 0;
        }
    }
    // Tag was found, record its frame.
    *pframe = currentFrame;
    
    // Store position and other info.
    ppos->CharacterId   = ptag->Pos.CharacterId;
    ppos->ClipDepth     = ptag->Pos.ClipDepth;
    ppos->Depth         = ptag->Pos.Depth;
    ppos->Ratio         = ptag->Pos.Ratio;

    if (!ppos->HasCxform)
    {
        ppos->HasCxform      = ptag->Pos.HasCxform;
        ppos->ColorTransform = ptag->Pos.ColorTransform;
    }
    if (!ppos->HasMatrix)
    {
        ppos->HasMatrix = ptag->Pos.HasMatrix;
        ppos->Matrix_1  = ptag->Pos.Matrix_1;
    }
    if (!ppos->BlendMode)
    {
        ppos->BlendMode = ptag->Pos.BlendMode;
    }   

    // Search for Matrix/CXForm/Blend tags from even earlier frames if necessary.

    // If this is a move tag, CXForm + Matrix values that are not in it should
    // have already been established, so don't do further searching.
    if (ptag->PlaceType == GFxPlaceObject2::Place_Move)
    {
        if ( (ppos->HasCxform || !(findPrevFlags & FindPrev_NeedCXForm)) && 
             (ppos->HasMatrix || !(findPrevFlags & FindPrev_NeedMatrix)) &&
             (ppos->BlendMode || !(findPrevFlags & FindPrev_NeedBlend)) )
        {           
            return ptag;
        }   
    }

    pprevTag = ptag;
    while ((pprevTag->PlaceType != GFxPlaceObject2::Place_Add) &&
           (!ppos->HasMatrix || !ppos->HasCxform || !ppos->BlendMode))
    {
        pprevTag = FindPreviousPlaceObject2(&currentFrame, depth, id);
        if (!pprevTag)
            break;

        if (!ppos->HasCxform && pprevTag->Pos.HasCxform)
        {
            ppos->ColorTransform = pprevTag->Pos.ColorTransform;
            ppos->HasCxform      = 1;
        }
        if (!ppos->HasMatrix && pprevTag->Pos.HasMatrix)
        {
            ppos->Matrix_1  = pprevTag->Pos.Matrix_1;
            ppos->HasMatrix = 1;
        }
        if (!ppos->BlendMode && pprevTag->Pos.BlendMode)
        {
            ppos->BlendMode = pprevTag->Pos.BlendMode;
        }
    }

    // If we needed to find CXForm/Matrix and did not find one, that means that
    // its value must have been identity. So force the identity to be applied anyway.   
    if (findPrevFlags & FindPrev_NeedCXForm)
        ppos->HasCxform = 1;
    if (findPrevFlags & FindPrev_NeedMatrix)
        ppos->HasMatrix = 1;
    if ((findPrevFlags & FindPrev_NeedBlend) && !ppos->BlendMode)
        ppos->BlendMode = GRenderer::Blend_Normal;

    // Done.
    return ptag;
}

GFxPlaceObject2* GFxSprite::FindPreviousPlaceObject2(UInt *pframe, int depth, GFxResourceId id)
{
    GASExecuteTag::DepthResId depthId(id, depth);  

    int f;
    for (f = *pframe - 1; f >= 0; f--)
    {
        const Frame playlist = pDef->GetPlaylist(f);
        for (int i = playlist.GetTagCount() - 1; i >= 0; i--)
        {
            GASExecuteTag*  e = playlist.GetTag(i);
            if (e->GetPlaceObject2_DepthId() == depthId)
            {
                // Update frame and return tag pointer.
                *pframe = (UInt) f;
                return (GFxPlaceObject2*) e;
            }
        }
    }
    
    *pframe = (UInt) f;
    return NULL;
}


// Execute any remove-object tags associated with the specified frame.
// frame is 0-based
void    GFxSprite::ExecuteRemoveTags(UInt frame)
{
    GASSERT(frame != UInt(~0));
    GASSERT(frame < GetLoadingFrame());

    const Frame playlist = pDef->GetPlaylist(frame);
    for (UInt i = 0; i < playlist.GetTagCount(); i++)
    {
        GASExecuteTag*  e = playlist.GetTag(i);
        if (e->IsRemoveTag())
        {
            e->ExecuteState(this);
        }
    }
    SetDirtyFlag();
}


// Set the sprite state at the specified frame number.
// 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
void    GFxSprite::GotoFrame(UInt targetFrameNumber)
{
//  IF_VERBOSE_DEBUG(LogMsg("sprite::GotoFrame(%d)\n", targetFrameNumber));//xxxxx

    targetFrameNumber = (UInt)GTL::gclamp<SInt>(targetFrameNumber, 0, (SInt)GetLoadingFrame() - 1);

    if (targetFrameNumber < CurrentFrame)
    {
#if 0
        DisplayList.MarkAllEntriesForRemoval();
        for (UInt f = 0; f < targetFrameNumber; f++)
        {
            // MA TBD: This incorrectly executes Event_Load/Event_Unload events on intermediate tags.
            // Should not happen? Needs fixing.
            ExecuteFrameTags(f, true);
        }
#endif
        for (UInt f = CurrentFrame; f > targetFrameNumber; f--)
        {
            ExecuteFrameTagsReverse(f);
        }

        // Set CurrentFrame, so that add object control tags in frame 1, potentially processed by
        // ExecuteFrameTags, do not re-create nested clips (important for preserving internal state).
        CurrentFrame = targetFrameNumber;
        // No remove tags after backwards seek
        //??ExecuteFrameTags(targetFrameNumber, false);

        ExecuteFrameTags(targetFrameNumber, false, true);
        GetMovieRoot()->ActionQueue.FlushOnLoadQueues();
        DisplayList.RemoveMarkedObjects();
    }
    else if (targetFrameNumber > CurrentFrame)
    {
        for (UInt f = CurrentFrame + 1; f < targetFrameNumber; f++)
        {
            // MA TBD: This incorrectly executes Event_Load/Event_Unload events on intermediate tags.
            // Should not happen? Needs fixing.
            ExecuteFrameTags(f, true);
        }

        CurrentFrame = targetFrameNumber;
        ExecuteFrameTags(targetFrameNumber, false);
        GetMovieRoot()->ActionQueue.FlushOnLoadQueues();
        DisplayList.RemoveMarkedObjects();
    }

    // GotoFrame stops by default.
    PlayStatePriv = GFxMovie::Stopped;
}


bool    GFxSprite::GotoLabeledFrame(const char* label, SInt offset)
{
    UInt    targetFrame = GFC_MAX_UINT;
    if (pDef->GetLabeledFrame(label, &targetFrame, 0))
    {
        GotoFrame((UInt)((SInt)targetFrame + offset));
        return true;
    }
    else
    {       
        LogWarning("Error: MovieImpl::GotoLabeledFrame('%s') unknown label\n", label);
        return false;
    }
}

        
void    GFxSprite::Display(GFxDisplayContext &context, StackData stackData)
{
    // We're invisible, so don't display!
    //!AB: but mask should be drawn even if it is invisible!
    if (!IsDisplayable())
        return;

    // We must apply a new binding table if the character needs to
    // use resource data from the loaded/imported GFxMovieDefImpl.
    GFxResourceBinding *psave = context.pResourceBinding;
    context.pResourceBinding = &pDefImpl->GetResourceBinding();

	// Pass matrix on the stack
	StackData newStackData;
	newStackData.stackMatrix = stackData.stackMatrix * this->Matrix_1;
	newStackData.stackColor = stackData.stackColor * this->ColorTransform;

    GFxSprite* pmask = GetMask();  
    bool       masked = false;
    if (pmask)
    {
        if (!pmask->IsUsedAsMask() || pmask->IsUnloaded())
        {
            // if mask character was unloaded or unmarked as a mask - remove it from the sprite
            SetMask(NULL);
        }
        else
        {
            context.PushAndDrawMask(pmask,newStackData);
            masked = true;
        }
    }

    if (pDrawingAPI)
    {
        //GRenderer::Matrix       mat        = GetWorldMatrix();
        //GRenderer::Cxform       wcx        = GetWorldCxform();
        GRenderer::BlendType    blend      = GetActiveBlendMode();
		pDrawingAPI->Display(context, newStackData.stackMatrix , newStackData.stackColor , blend, GetClipDepth() > 0);
    }

    DisplayList.Display(context,newStackData);

    context.pResourceBinding = psave;

    DoDisplayCallback();

    if (masked)
    {
        context.PopMask();
    }
}



void    GFxSprite::PropagateMouseEvent(const GFxEventId& id)
{
    // MA: Hidden clips don't receive mouse events.
    // Tested by assigning _visible property to false,
    // but the object didn't hide in Flash.
    if (GetVisible() == false)  
        return;

    DisplayList.PropagateMouseEvent(id);
    // Notify us after children.
    OnEvent(id);
}

void    GFxSprite::PropagateKeyEvent(const GFxEventId& id, int* pkeyMask)
{
    // AB: Hidden clips don't receive key events.
    // Tested by assigning _visible property to false,
    // but the object didn't hide in Flash. nEed to test more:
    // what about hidden by timeline?
    if (!GetVisible())  
        return;

    DisplayList.PropagateKeyEvent (id, pkeyMask);
    // Notify us after children.
    OnKeyEvent(id, pkeyMask);
}

void    GFxSprite::CloneInternalData(const GFxASCharacter* src)
{
    GASSERT(src);
    GFxASCharacter::CloneInternalData(src);
    if (src->IsSprite())
    {
        const GFxSprite* pSrcSprite = src->ToSprite();
        if (pSrcSprite->ActsAsButton())
            SetHasButtonHandlers (true);
    }
    SetDirtyFlag();
}

// ThisPtr - sprite
// Arg(0) - constructor function of the class
static void GFx_InitializeClassInstance(const GASFnCall& fn)
{
    GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
    GASSERT(pscriptCh);

    GASFunctionRef ctorFunc = fn.Arg(0).ToFunction();
    GASSERT(!ctorFunc.IsNull());

    pscriptCh->SetProtoToPrototypeOf(ctorFunc.GetObjectPtr());
}

// ThisPtr - sprite
// Arg(0) - String, name of instance
// Arg(1) - Boolean, true, if construct event should be fired
static void GFx_FindClassAndInitializeClassInstance(const GASFnCall& fn)
{
    GASSERT(fn.Env);

    GASGlobalContext* gctxt = fn.Env->GetGC();
    GASSERT(gctxt);

    GASFunctionRef ctorFunc;
    const GASString symbolName(fn.Arg(0).ToString(fn.Env));
    if (!symbolName.IsEmpty())
    {
        if (gctxt->FindRegisteredClass(fn.Env->GetSC(), symbolName, &ctorFunc))
        {
            GASSERT(!ctorFunc.IsNull());

            GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
            GASSERT(pscriptCh);

            pscriptCh->SetProtoToPrototypeOf(ctorFunc.GetObjectPtr());

            // now, check for 'construct' event and fire it if Arg(1) is set to TRUE
            if (fn.Arg(1).ToBool(fn.Env))
            {
                GFxMovieRoot::ActionEntry e(pscriptCh, GFxEventId::Event_Construct);
                e.Execute(pscriptCh->GetMovieRoot());
            }

            // now, invoke a constructor
            GFxMovieRoot::ActionEntry e(pscriptCh, ctorFunc);
            e.Execute(pscriptCh->GetMovieRoot());
        }
        else
        {
            // a special case, when a component doesn't have a class (it is possible if Linkage->AS 2.0 Class field left empty)
            // but there is a construct event for its instance.

            // now, check for 'construct' event and fire it if Arg(1) is set to TRUE
            if (fn.Arg(1).ToBool(fn.Env))
            {
                GPtr<GFxSprite> pscriptCh = fn.ThisPtr->ToSprite();
                GASSERT(pscriptCh);

                GFxMovieRoot::ActionEntry e(pscriptCh, GFxEventId::Event_Construct);
                e.Execute(pscriptCh->GetMovieRoot());
            }

        }
    }
}

// Add an object to the display list.
GFxCharacter*   GFxSprite::AddDisplayObject(
                                const GFxCharPosInfo &pos,
                                const GASString &name,
                                const GTL::garray<GFxSwfEvent*>& eventHandlers,
                                const GASObjectInterface *pinitSource,
                                bool replaceIfDepthIsOccupied,  UInt createFrame,
                                bool placeObject,
                                GFxCharacterCreateInfo* pcharCreateOverride,
                                GFxASCharacter* origChar)
{
    GASSERT(pDef);

    // TBD:
    // Here we look up a character but do not return it's binding.
    // This means that while we will have the right character created,
    // its pDefImpl will correspond OUR table and not THEIR table.
    // However, imported characters MUST reference their own tables for resources!

    GFxCharacterCreateInfo ccinfo = pcharCreateOverride ? *pcharCreateOverride :    
                                    pDefImpl->GetCharacterCreateInfo(pos.CharacterId);
    if (!ccinfo.pCharDef)
    {
        LogError("GFxSprite::AddDisplayObject(): unknown cid = %d\n", pos.CharacterId.GetIdIndex());
        return NULL;
    }

    // If we already have this object on this plane, then move it instead of replacing it.
    // This can happen when wrapping around from last frame to first one, where
    // the first frame fill bring back to life objects marked for removal after last frame.
    // It can also happen when seeking back.
    GFxCharacter* pexistingChar = DisplayList.GetCharacterAtDepth(pos.Depth);

    if (placeObject)
    {
        // If the object was moved explicitly in Action script, leave it there.
        if (pexistingChar && !pexistingChar->GetAcceptAnimMoves())
            return NULL;        

        if ( pexistingChar && 
            (pexistingChar->GetId() == pos.CharacterId) &&
            ( (name.IsEmpty() && pexistingChar->GetName().IsEmpty()) ||
              (name.IsEmpty() && pexistingChar->IsASCharacter() && ((GFxASCharacter*)pexistingChar)->HasInstanceBasedName()) ||
              (!name.IsEmpty() && pexistingChar->GetName() == name)) )
        {
            // Move will bring the object back by clearing its MarkForRemove flag
            // in display list. We only do so if we are talking about THE SAME object,
            // i.e. created in the same frame. Flash considers objects created in
            // different frames to be different, even if they share a name.
            if (pexistingChar->GetCreateFrame() == CurrentFrame)
            {
                if (!pos.HasCxform && !pexistingChar->GetCxform().IsIdentity())
                {
                    // if the object is moved by AddDisplayObject and no color transform suppose to 
                    // to be used then we need to reset the color transform for the existing
                    // character, since it may have it changed earlier. 
                    // This is actual when the character has changed Cxform matrix and
                    // then time-line goes to the first frame, where this Cxform doesn't
                    // exist. (!AB)
                    GFxCharPosInfo newPos = pos;
                    newPos.HasCxform = true;
                    newPos.ColorTransform.SetIdentity();
                    MoveDisplayObject(newPos);             
                }
                else
                    MoveDisplayObject(pos);             
                return NULL;
            }
            // Otherwise replace object with a new-state fresh instance of it.
            replaceIfDepthIsOccupied = 1;
        }
    }
    else
    {
        replaceIfDepthIsOccupied = 1;
    }
    SetDirtyFlag();

    GPtr<GFxCharacter>  ch          = *ccinfo.pCharDef->CreateCharacterInstance(this, pos.CharacterId,
                                                                                ccinfo.pBindDefImpl);
    GASSERT(ch);
    GFxASCharacter*     pscriptCh   = ch->ToASCharacter();
    GFxSprite*          pspriteCh   = pscriptCh ? pscriptCh->ToSprite() : 0;
    bool                nameSet     = 0;

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

    if (pscriptCh)
    {
        if (!name.IsEmpty())
        {
            ch->SetName(name);          
            nameSet = 1;         
        }

        if (origChar)
        {
            // if we are duplicating char - call CloneInternalData
            pscriptCh->CloneInternalData(origChar);
        }
    }
    
    ch->SetCreateFrame((createFrame == GFC_MAX_UINT) ? CurrentFrame : createFrame);

    // Attach event Handlers (if any).
    
    bool hasConstructEvent = false;
    GFxMovieRoot* proot = GetMovieRoot();
    if (pscriptCh)
    {
        for (UPInt i = 0, n = eventHandlers.size(); i < n; i++)   
        {       
            eventHandlers[i]->AttachTo(pscriptCh);
            // If we installed a button handler, sprite will work in button mode...
            if (pspriteCh && eventHandlers[i]->Event.IsButtonEvent())
                pspriteCh->SetHasButtonHandlers (true);

            if (placeObject)
            {
                if (eventHandlers[i]->Event.Id == GFxEventId::Event_Initialize)
                {
                    GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Initialize);
                    GASActionBufferData* pactionOpData = eventHandlers[i]->pActionOpData;
                    if (pe && pactionOpData && !pactionOpData->IsNull())
                    {
                        GPtr<GASActionBuffer> pbuff =
                                *new GASActionBuffer(pscriptCh->GetASEnvironment()->GetSC(),
                                                     pactionOpData);
                        pe->SetAction(pscriptCh, pbuff);
                    }
                }
                if (eventHandlers[i]->Event.Id == GFxEventId::Event_Construct)
                {
                    hasConstructEvent = true;
                }
            }
        }
    }
    
    // Check for registered classes
    UInt sessionId = 0;
    if (pscriptCh)
    {
        GASGlobalContext* gctxt = this->GetGC();
        GASSERT(gctxt);
        
        GASFunctionRef ctorFunc;    
        const GFxString* psymbolName = ch->GetResourceMovieDef()->GetNameOfExportedResource(ccinfo.pCharDef->GetId());
        
        UInt oldSessionId;
        sessionId = proot->ActionQueue.StartNewSession(&oldSessionId);

        if (psymbolName)
        {
            GASString symbolName = GetASEnvironment()->CreateString(*psymbolName);

            if (gctxt->FindRegisteredClass(GetASEnvironment()->GetSC(), symbolName, &ctorFunc))
            {
                // first - schedule initializer of class instance. It will
                // assign proper prototype to the pscriptCh.
                GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Initialize);
                GASValueArray params;
                params.push_back(ctorFunc);
                if (pe) pe->SetAction(pscriptCh, GFx_InitializeClassInstance, &params);
                
                // schedule "construct" event, if any
                if (hasConstructEvent)
                {
                    GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                    if (pe) pe->SetAction(pscriptCh, GFxEventId::Event_Construct);
                    hasConstructEvent = false;
                }

                // now, schedule constructor invocation
                pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                if (pe) pe->SetAction(pscriptCh, ctorFunc);
            }
            else if (placeObject)
            {
                GASValueArray params;
                params.push_back(GASValue(symbolName)); //[0]
                params.push_back(GASValue(hasConstructEvent)); //[1]

                GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
                if (pe) pe->SetAction(pscriptCh, GFx_FindClassAndInitializeClassInstance, &params);
                hasConstructEvent = false;
            }
        }
        if (placeObject && hasConstructEvent)
        {
            // if no class is registered, only onClipEvent(construct)

            GFxMovieRoot::ActionEntry* pe = proot->InsertEmptyAction(GFxMovieRoot::AP_Construct);
            if (pe) pe->SetAction(pscriptCh, GFxEventId::Event_Construct);
        }
        proot->ActionQueue.RestoreSession(oldSessionId);
    }

    DisplayList.AddDisplayObject(pos, ch.GetPtr(), replaceIfDepthIsOccupied);   

    // Assign sticky variables (only applies if there is an identifiable name).
    if (nameSet)
        proot->ResolveStickyVariables(pscriptCh);

    // Copy init properties if they exist (for SWF 6+).
    if (GetVersion() >= 6 && pinitSource && pscriptCh)
    {
        struct InitVisitor : public MemberVisitor
        {
            GASEnvironment *pEnv;
            GFxASCharacter *pCharacter;
            bool           IsInsideAS;

            InitVisitor(GASEnvironment *penv, GFxASCharacter *pchar, bool isInsideAS)
                : pEnv(penv), pCharacter(pchar), IsInsideAS(isInsideAS)
            { }

            virtual void    Visit(const GASString& name, const GASValue& val, UByte flags)
            {
                GUNUSED(flags);                
                if (IsInsideAS)
                    pCharacter->SetMember(pEnv, name, val);
                else
                    pCharacter->SetMemberRaw(pEnv->GetSC(), name, val);
            }
        };
        InitVisitor memberVisitor(GetASEnvironment(), pscriptCh, !placeObject);
        pinitSource->VisitMembers(memberVisitor.pEnv->GetSC(), &memberVisitor);
    }

    GASSERT(!ch || ch->GetRefCount() > 1);

    // here is a special case for attachMovie. If we have queued up constructors' invocations before 
    // DisplayList.AddDisplayObject then we need to execute them right now.
    if (pscriptCh && !placeObject)
    {
        proot->DoActionsForSession(sessionId);
    }

    return ch.GetPtr();
}


void    GFxSprite::MoveDisplayObject(const GFxCharPosInfo &pos)
{
    DisplayList.MoveDisplayObject(pos);
    SetDirtyFlag();
}


void    GFxSprite::ReplaceDisplayObject(const GFxCharPosInfo &pos, const GASString& name)
{
    GASSERT(pDef);

    GFxCharacterCreateInfo ccinfo = pDefImpl->GetCharacterCreateInfo(pos.CharacterId);    
    if (ccinfo.pCharDef == NULL)
    {
        LogError("Sprite::ReplaceDisplayObject(): unknown cid = %d\n", pos.CharacterId.GetIdIndex());
        return;
    }
    GASSERT(ccinfo.pCharDef && ccinfo.pBindDefImpl);

    GPtr<GFxCharacter> ch = *ccinfo.pCharDef->CreateCharacterInstance(this, pos.CharacterId,
                                                                      ccinfo.pBindDefImpl);
    GASSERT(ch);

    ReplaceDisplayObject(pos, ch, name);
}


void    GFxSprite::ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, const GASString& name)
{
    GASSERT(ch != NULL);
    if (!name.IsEmpty())
        ch->SetName(name);  

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

    DisplayList.ReplaceDisplayObject(pos, ch);

    if (!name.IsEmpty() && ch->IsASCharacter())
        GetMovieRoot()->ResolveStickyVariables(ch->ToASCharacter());
    SetDirtyFlag();
}


void    GFxSprite::RemoveDisplayObject(SInt depth, GFxResourceId id)
{
    DisplayList.RemoveDisplayObject(depth, id);
    SetDirtyFlag();
}


// For debugging -- return the id of the GFxCharacter at the specified depth.
// Return -1 if nobody's home.
GFxResourceId GFxSprite::GetIdAtDepth(int depth)
{
    GFxCharacter* pch = GetCharacterAtDepth(depth);
    return pch ? pch->GetId() : GFxResourceId();
}

GFxCharacter*   GFxSprite::GetCharacterAtDepth(int depth)
{
    int index = DisplayList.GetDisplayIndex(depth);
    if (index == -1)    
        return 0;   
    return DisplayList.GetDisplayObject(index).pCharacter.GetPtr();
}


// Movie Loading support
bool        GFxSprite::ReplaceChildCharacter(GFxASCharacter *poldChar, GFxASCharacter *pnewChar)
{
    GASSERT(poldChar != 0);
    GASSERT(pnewChar != 0);
    GASSERT(poldChar->GetParent() == this);

    // Get the display entry.
    SInt index = DisplayList.GetDisplayIndex(poldChar->GetDepth());
    if (index == -1)
        return 0;
    GFxDisplayList::DisplayEntry &e = DisplayList.GetDisplayObject(index);
        
    // Inform old character of unloading.
    poldChar->OnEventUnload();
    GetMovieRoot()->DoActions();

    // Copy physical properties & re-link all ActionScript references.
    pnewChar->CopyPhysicalProperties(poldChar);
    // For Sprites, we also copy _lockroot.
    if (pnewChar->IsSprite() && poldChar->IsSprite())
    {
        pnewChar->ToSprite()->SetLockRoot(poldChar->ToSprite()->IsLockRoot());
    }

    // Alter the display list.
    e.SetCharacter(pnewChar);
    SetDirtyFlag();
    return true;
}

bool        GFxSprite::ReplaceChildCharacterOnLoad(GFxASCharacter *poldChar, GFxASCharacter *pnewChar)
{
    if (!ReplaceChildCharacter(poldChar, pnewChar))
        return false;

    // And call load.
    pnewChar->OnEventLoad();
    GetMovieRoot()->DoActions();
    SetDirtyFlag();
    return true;
}



//
// *** Sprite ActionScript support
//

// GFxASCharacter override to indicate which standard members are handled for us.
UInt32  GFxSprite::GetStandardMemberBitMask() const
{
    // MovieClip lets base handle all members it supports.
    return UInt32(
            M_BitMask_PhysicalMembers |
            M_BitMask_CommonMembers |           
            (1 << M_target) |
            (1 << M_droptarget) |
            (1 << M_url) |          
            (1 << M_blendMode) |
            (1 << M_cacheAsBitmap) |
            (1 << M_filters) |
            (1 << M_focusrect) |
            (1 << M_enabled) |
            (1 << M_trackAsMenu) |
            (1 << M_lockroot) |
            (1 << M_tabEnabled) |
            (1 << M_tabIndex) |
            (1 << M_useHandCursor) |        
            (1 << M_quality) |
            (1 << M_highquality) |
            (1 << M_soundbuftime) |
            (1 << M_xmouse) |
            (1 << M_ymouse) |

            // GFxASCharacter will report these as read-only for us,
            // but we still need to handle them in GetMember.
            (1 << M_currentframe) |
            (1 << M_totalframes) |
            (1 << M_framesloaded) 
            );
}

bool    GFxSprite::GetStandardMember(StandardMember member, GASValue* pval, bool opcodeFlag) const
{
    if (GFxASCharacter::GetStandardMember (member, pval, opcodeFlag))
        return true;

    // Handle MovieClip specific "standard" members.
    switch(member)
    {
        case M_currentframe:
            pval->SetInt(CurrentFrame + 1);
            return true;

        case M_totalframes:             
            pval->SetInt(pDef->GetFrameCount());
            return true;

        case M_framesloaded:
            pval->SetInt(GetLoadingFrame());
            return true;

        case M_lockroot:
            pval->SetBool(IsLockRoot());            
            return true;

        case M_focusEnabled:   
            {
                if (FocusEnabled.IsDefined())
                    pval->SetBool(FocusEnabled.IsTrue());
                else
                    pval->SetUndefined();
                return 1;
            }

        case M_tabChildren:   
            {
                // Is a yellow rectangle visible around a focused GFxASCharacter Clip (?)
                if (TabChildren.IsDefined())
                    pval->SetBool(TabChildren.IsTrue());
                else
                    pval->SetUndefined();
                return 1;
            }

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

bool    GFxSprite::SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag)
{   
    if (GFxASCharacter::SetStandardMember (member, val, opcodeFlag))
        return true;

    // Handle MovieClip specific "standard" members.
    switch(member)
    {   
        case M_currentframe:
        case M_totalframes:
        case M_framesloaded:
            // Already handled as Read-Only in base,
            // so we won't get here.
            return true;

        case M_lockroot:
            SetLockRoot(val.ToBool(GetASEnvironment()));
            return true;

        case M_focusEnabled:
            FocusEnabled = val.ToBool(GetASEnvironment());
            return 1;

        case M_tabChildren:
            TabChildren = val.ToBool(GetASEnvironment());
            return 1;

        // extension
        case M_hitTestDisable:
            if (GetASEnvironment()->CheckExtensions())
            {
                SetHitTestDisableFlag(val.ToBool(GetASEnvironment()));
                return 1;
            }
            break;

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

        // No other custom properties to set for now.
        default:
        break;
    }
    return false;
}

// Set the named member to the value.  Return true if we have
// that member; false otherwise.
bool    GFxSprite::SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{    
    if (name.IsStandardMember())
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (SetStandardMember(member, val, 0))
            return true;
    }

    if (ASMovieClipObj)
    {
        // Note that MovieClipObject will also track setting of button
        // handlers, i.e. 'onPress', etc.
        return ASMovieClipObj->SetMemberRaw(psc, name, val, flags);
    }
    return false;
}

// internal method, a common implementation for both GetMember/GetMemberRaw.
// Only either penv or psc should be not NULL. If penv is not null, then this method
// works like GetMember, if psc is not NULL - it is like GetMemberRaw
bool    GFxSprite::GetMember(GASEnvironment* penv, GASStringContext *psc, const GASString& name, GASValue* pval)
{
    if (name.IsStandardMember())
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (GetStandardMember(member, pval, 0))
            return true;

        switch(member)
        {
        case M_transform:
            {

#ifndef GFC_NO_FXPLAYER_AS_TRANSFORM
                // create a GASTransformObject
                GPtr<GASTransformObject> transfObj = *new GASTransformObject(GetASEnvironment(), this);
                pval->SetAsObject(transfObj);
                return true;
#else
                GFxLog* log = penv->GetLog();
                if (log != NULL)
                    log->LogScriptError("Transform ActionScript class was not included in this GFx build.\n");                
                return true;
#endif
            }

        default:
            break;
        }
    }

    // Looks like timeline variables have higher priority than display list
    if (ASMovieClipObj)
    {
        if (penv && ASMovieClipObj->GetMember(penv, name, pval))    
            return true;
        else if (psc && ASMovieClipObj->GetMemberRaw(psc, name, pval))    
            return true;
    }

    // Try variables.
    if (ASEnvironment.GetMember(name, pval))
        return true;    

    // Not a built-in property.  Check items on our display list.
    GFxASCharacter* pch = DisplayList.GetCharacterByName(ASEnvironment.GetSC(), name);
    if (pch)
    {
        // Found object.
        pval->SetAsCharacter(pch);
        return true;
    }
    // Looks like _global is accessible from any character
    // if (name == _global)
    if (ASEnvironment.GetBuiltin(GASBuiltin__global).CompareBuiltIn_CaseCheck(name, ASEnvironment.IsCaseSensitive()))
    {
        pval->SetAsObject(ASEnvironment.GetGC()->pGlobal);
        return true;
    }
    return false;
}

// Find the GFxASCharacter which is one degree removed from us,
// given the relative pathname.
//
// If the pathname is "..", then return our parent.
// If the pathname is ".", then return ourself.  If
// the pathname is "_level0" or "_root", then return
// the root pMovie.
//
// Otherwise, the name should refer to one our our
// named characters, so we return it.
//
// NOTE: In ActionScript 2.0, top level Names (like
// "_root" and "_level0") are CASE SENSITIVE.
// GFxCharacter names in a display list are CASE
// SENSITIVE. Member names are CASE INSENSITIVE.  Gah.
//
// In ActionScript 1.0, everything seems to be CASE
// INSENSITIVE.
GFxASCharacter* GFxSprite::GetRelativeTarget(const GASString& name, bool first_call)
{
    bool           caseSensitive = IsCaseSensitive();
    GASEnvironment &e = ASEnvironment;
    
    if (name.IsBuiltin())
    {    
        // Special branches for case-sensitivity for efficiency.
        if (caseSensitive)
        {
            if (e.GetBuiltin(GASBuiltin_dot_) == name || // "."
                e.GetBuiltin(GASBuiltin_this) == name)
            {
                return this;
            }
            else if (e.GetBuiltin(GASBuiltin_dotdot_) == name || // ".."
                    e.GetBuiltin(GASBuiltin__parent) == name)
            {
                return GetParent();
            }
            else if (e.GetBuiltin(GASBuiltin__root) == name)
            {
                return GetASRootMovie();
            }
        }
        else
        {   // Case-insensitive version.
            name.ResolveLowercase();
            if (e.GetBuiltin(GASBuiltin_dot_) == name ||
                e.GetBuiltin(GASBuiltin_this).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {
                return this;
            }
            else if (e.GetBuiltin(GASBuiltin_dotdot_) == name || // ".."
                    e.GetBuiltin(GASBuiltin__parent).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {
                return GetParent();
            }
            else if (e.GetBuiltin(GASBuiltin__root).CompareBuiltIn_CaseInsensitive_Unchecked(name))
            {
                return GetASRootMovie();
            }
        }
    }

    if (name[0] == '_' && first_call)
    {
        // Parse level index.
        const char* ptail = 0;
        SInt        levelIndex = pRoot->ParseLevelName(name.ToCStr(), &ptail,
                                                       caseSensitive);
        if ((levelIndex != -1) && !*ptail)
            return pRoot->GetLevelMovie(levelIndex);
    }

    // See if we have a match on the display list.
    return DisplayList.GetCharacterByName(e.GetSC(), name);
}



// Execute the actions for the specified frame.  The
// FrameSpec could be an integer or a string.
void    GFxSprite::CallFrameActions(const GASValue& FrameSpec)
{
    UInt    FrameNumber = GFC_MAX_UINT;

    // Figure out what frame to call.
    if (FrameSpec.GetType() == GASValue::STRING)
    {
        GASString frameLabel(FrameSpec.ToString(&ASEnvironment));
        if (!pDef->GetLabeledFrame(frameLabel.ToCStr(), &FrameNumber))
        {
            // GetLabeledFrame does number translation too, so if it fails
            // there is no frame to call.
            return;
        }
    }
    else
    {
        // convert from 1-based to 0-based
        FrameNumber = (UInt)(int) FrameSpec.ToNumber(&ASEnvironment) - 1;
    }

    if (FrameNumber == UInt(~0) || FrameNumber >= GetLoadingFrame())
    {
        // No dice.
        LogError("Error: CallFrame('%s') - unknown frame\n", FrameSpec.ToString(&ASEnvironment).ToCStr());
        return;
    }

    // These actions will be executed immediately. Such out-of-order execution
    // only happens due to a CallFrame instruction. We implement this by
    // simply assigning a different session id to actions being queued during
    // the execution.

    UInt aqOldSession, aqCurSession = pRoot->ActionQueue.StartNewSession(&aqOldSession);

    // Execute the actions.
    const Frame playlist = pDef->GetPlaylist(FrameNumber);
    for (UInt i = 0; i < playlist.GetTagCount(); i++)
    {
        GASExecuteTag*  e = playlist.GetTag(i);
        if (e->IsActionTag())
        {
            e->Execute(this);
        }
    }
    pRoot->ActionQueue.RestoreSession(aqOldSession);

    // Execute any new actions triggered by the tag.    
    pRoot->DoActionsForSession(aqCurSession);

    // If some other actions were queued during the DoAction (like gotoAnd<>)- execute them 
    // as usual, at the next Advance. (AB)
}



// Handle a button event.
bool    GFxSprite::OnButtonEvent(const GFxEventId& id)
{
    if (!IsEnabledFlagSet())
        return false;

    // Handle it ourself?
    bool            handled = OnEvent(id);
    GFxASCharacter* pparent;
    
    if (!handled && ((pparent=GetParent()) != 0))
    {
        // We couldn't handle it ourself, so
        // pass it up to our parent.
        return pparent->OnButtonEvent(id);
    }
    return false;
}

// Special event handler; ensures that unload is called on child items in timely manner.
void    GFxSprite::OnEventUnload()
{
    SetDirtyFlag();

    // Cause all children to be unloaded.
    DisplayList.Clear();
    GFxASCharacter::OnEventUnload();
}

bool    GFxSprite::OnKeyEvent(const GFxEventId& id, int* pkeyMask)
{ 
    bool rv = false;
    if (id.Id == GFxEventId::Event_KeyDown)
    {
        // run onKeyDown first.
        OnEvent(id);

        // also simmulate on(keyPress)
        // covert Event_KeyDown to Event_KeyPress
        GASSERT(pkeyMask != 0);

        // check if keyPress already was handled then do not handle it again
        if (!((*pkeyMask) & GFxCharacter::KeyMask_KeyPress))
        {
            int kc = id.ConvertToButtonKeyCode();
            if (kc && (rv = OnEvent(GFxEventId(GFxEventId::Event_KeyPress, short(kc), 0)))!=0)
            {
                *pkeyMask |= GFxCharacter::KeyMask_KeyPress;
            }
        }

        if (GetMovieRoot()->IsKeyboardFocused(this) && (id.KeyCode == GFxKey::Return || id.KeyCode == GFxKey::Space))
        {
            // if focused and enter - simulate on(press)/onPress and on(release)/onRelease
            OnEvent (GFxEventId::Event_Press);
            OnEvent (GFxEventId::Event_Release);
        }
        return rv;
    }
    else
        return OnEvent(id); 
}

// Dispatch event Handler(s), if any.
bool    GFxSprite::OnEvent(const GFxEventId& id)
{
    /*  
    if(GetName().ToCStr() && GetName().ToCStr()[0])
    {
        // IF unload executed, clear pparent
        if (id.Id == GFxEventId::Event_Unload)                
            GFC_DEBUG_MESSAGE1(1, "Event_Unload event - %s", GetName().ToCStr());
        if (id.Id == GFxEventId::Event_Load)              
            GFC_DEBUG_MESSAGE1(1, "Event_Load event - %s", GetName().ToCStr());
    } */

    GASValue    method;

    do {

        // First, check for built-in event onClipEvent() handler.   
        if (HasEventHandler(id))
        {
            // Dispatch.
            continue;
        }
        
        // Check for member function.   
        // In ActionScript 2.0, event method names are CASE SENSITIVE.
        // In ActionScript 1.0, event method names are CASE INSENSITIVE.
        GASStringContext *psc = GetASEnvironment()->GetSC();
        GASString         methodName(id.GetFunctionName(psc));
        if (!methodName.IsEmpty())
        {
            // Event methods can only be in MovieClipObject or Environment.
            // It's important to not call GFx::GetMemberRaw here to save on overhead 
            // of checking display list and member constants, which will cost
            // us a lot when this is called in Advance.
            bool hasMethod = false;

            // Resolving of event handlers should NOT involve __resolve and properties handles. Thus,
            // we need to use GetMemberRaw instead of GetMember. (AB)
            if (ASMovieClipObj && ASMovieClipObj->GetMemberRaw(ASEnvironment.GetSC(), methodName, &method))    
                hasMethod = true;            
            //!AB, now we put named functions as a member of target, thus they should be found by the
            // statement above.
            //else if (ASEnvironment.GetMember(methodName, &method))
            //    hasMethod = true;            

            //bool hasMethod = GetMemberRaw(psc, methodName, &method);

            if (id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp) 
            {
                // onKeyDown/onKeyUp are available only in Flash 6 and later
                // (don't mess with onClipEvent (keyDown/keyUp)!)
                if (ASEnvironment.GetVersion() >= 6)
                {
                    // also, onKeyDown/onKeyUp should be invoked only if focus
                    // is enabled and set to this movieclip
                    if (!GetMovieRoot()->IsKeyboardFocused(this))
                        hasMethod = false;
                }
                else
                {
                    // Flash 5 and below doesn't have onKeyDown/Up function handlers.
                    hasMethod = false;
                }
            }

            if (hasMethod)
                continue;
        }   
        
        // No handler: mark us as unloaded. This means that no more
        // execution will take place on this object.
        if (id.Id == GFxEventId::Event_Unload)
            SetUnloaded(true);
        return false;
    } while (0);

    // do actual dispatch
    GFxMovieRoot::ActionEntry* pe = GetMovieRoot()->InsertEmptyAction();
    if (pe) pe->SetAction(this, id);
    return true;
}

bool    GFxSprite::ExecuteBuffer(GASActionBuffer* pactionbuffer)
{
    if (!IsUnloaded()) 
    {
        pactionbuffer->Execute(GetASEnvironment());
        return true;
    }
    else
    {
        // This happens due to untimely Event_Load/Event_Unload during seek
        //GFC_DEBUG_WARNING1(1, "Action execute after unload: %s", pSprite->GetName().ToCStr());
    }
    return false;
}

// Execute this even immediately (called for processing queued event actions).
bool    GFxSprite::ExecuteEvent(const GFxEventId& id)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    if (GFxASCharacter::ExecuteEvent (id))
    {       
        // Handler done, mark us as unloaded.
        if (id == GFxEventId::Event_Unload)
            SetUnloaded(true);
        return true;
    }
    return false;
}

// Do the events That (appear to) happen as the GFxASCharacter
// loads. The OnLoad event is generated first. Then,
//  "constructor" frame1 tags and actions are Executed (even
// before Advance() is called). 
void    GFxSprite::OnEventLoad()
{   
    //AB: we need to queue up the onload event. At this moment we don't know do we really have
    // onLoad handler or not, because it might be installed later. So, we need to queue it always.
    // If sprite has onClipEvent(load) then priority is higher than if it has just onLoad-function 
    const GFxActionPriority::Priority prio = HasEventHandler(GFxEventId::Event_Load) ?
                                             GFxActionPriority::AP_ClipEventLoad : GFxActionPriority::AP_Load;
    pRoot->InsertEmptyAction(prio)->SetAction(this, GFxEventId::Event_Load);

    // execution of frame 0 tag should be done with the same priority as load event
    ExecuteFrameTags(0, false, false, prio);

    pRoot->ActionQueue.FlushOnLoadQueues();
}

// Do the events that happen when there is XML data waiting
// on the XML socket connection.
void    GFxSprite::OnEventXmlsocketOnxml()
{
    GFC_DEBUG_WARNING(1, "OnEventXmlsocketOnxml: unimplemented");  
    OnEvent(GFxEventId::Event_SockXML);
}

// Do the events That (appear to) happen on a specified interval.
void    GFxSprite::OnEventIntervalTimer()
{
    //GFC_DEBUG_WARNING(1, "OnEventIntervalTimer: unimplemented");   
    //OnEvent(GFxEventId::TIMER);
}

// Do the events that happen as a MovieClip (swf 7 only) loads.
void    GFxSprite::OnEventLoadProgress()
{   
    GFC_DEBUG_WARNING(1, "OnEventLoadProgress: unimplemented");
    OnEvent(GFxEventId::Event_LoadProgress);
}

bool GFxSprite::Invoke(const char* methodName, GASValue* presult, UInt numArgs)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    return GAS_Invoke(methodName, presult, this, &ASEnvironment, numArgs, ASEnvironment.GetTopIndex());
}

bool GFxSprite::InvokeArgs(const char* methodName, GASValue* presult, const char* methodArgFmt, va_list args)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    return GAS_InvokeParsed(methodName, presult, this, &ASEnvironment, methodArgFmt, args);
}

/*const char* GFxSprite::InvokeArgs(const char* methodName, const char* methodArgFmt, 
                                  const void* const* methodArgs, int numArgs)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxSprite> thisPtr(this);

    return GAS_InvokeParsed(&ASEnvironment, this, methodName, methodArgFmt, methodArgs, numArgs);
}*/


bool GFxSprite::ActsAsButton() const    
{ 
    if (ASMovieClipObj) 
        return ASMovieClipObj->ActsAsButton();  
    return false;
}

void GFxSprite::SetHasButtonHandlers(bool has)
{
    if (ASMovieClipObj) 
        ASMovieClipObj->SetHasButtonHandlers(has);  
}

bool GFxSprite::PointTestLocal(const GPointF &pt, UInt8 hitTestMask) const 
{ 
    if (IsHitTestDisableFlagSet())
        return false;

    if (!DoesScale9GridExist() && !GetBounds(GRenderer::Matrix()).Contains(pt))
        return false;

    if ((hitTestMask & HitTest_IgnoreInvisible) && !GetVisible())
        return false;

    int i, n = DisplayList.GetCharacterCount();

    GFxSprite* pmask = GetMask();  
    if (pmask)
    {
        if (pmask->IsUsedAsMask() && !pmask->IsUnloaded())
        {
            GRenderer::Matrix matrix;
            matrix.SetInverse(pmask->GetWorldMatrix());
            matrix *= GetWorldMatrix();
            GPointF p = matrix.Transform(pt);

            if (!pmask->PointTestLocal(p, hitTestMask))
                return false;
        }
    }

    GTL::garray<UByte> hitTest;
    CalcDisplayListHitTestMaskArray(&hitTest, pt, hitTestMask & HitTest_TestShape);

    GRenderer::Matrix m;
    GPointF           p = pt;

    // Go backwards, to check higher objects first.
    for (i = n - 1; i >= 0; i--)
    {
        GFxCharacter* pch = DisplayList.GetCharacter(i);

        if ((hitTestMask & HitTest_IgnoreInvisible) && !pch->GetVisible())
            continue;

        if (hitTest.size() && (!hitTest[i] || pch->GetClipDepth()>0))
            continue;

        m = pch->GetMatrix();
        p = m.TransformByInverse(pt);   

        if (pch->PointTestLocal(p, hitTestMask))
            return true;
    }   

    if (pDrawingAPI)
    {
        if (pDrawingAPI->DefPointTestLocal(pt, hitTestMask & HitTest_TestShape, this))
            return true;
    }

    return false;
}

void    GFxSprite::VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const
{
    if (visitFlags & VisitMember_ChildClips)
        DisplayList.VisitMembers(pvisitor, visitFlags); 
    GFxASCharacter::VisitMembers(psc, pvisitor, visitFlags);
    ASEnvironment.VisitMembers(pvisitor, visitFlags);
}


// Delete a member field, if not read-only. Return true if deleted.
bool    GFxSprite::DeleteMember(GASStringContext *psc, const GASString& name)
{
    if (GFxASCharacter::DeleteMember(psc, name))
        return true;
    return ASEnvironment.DeleteMember(name);
}

GASObject* GFxSprite::GetASObject ()          
{ 
    return ASMovieClipObj; 
}

GASObject* GFxSprite::GetASObject () const
{ 
    return ASMovieClipObj; 
}

bool    GFxSprite::IsTabable() const
{
    if (!GetVisible()) return false;
    if (!IsTabEnabledFlagDefined())
        return (ActsAsButton() || GetTabIndex() > 0);
    else
        return IsTabEnabledFlagTrue();
}

bool     GFxSprite::IsFocusEnabled() const
{
    if (!FocusEnabled.IsDefined() || FocusEnabled.IsFalse())
        return (ActsAsButton());
    else
        return FocusEnabled.IsTrue();
}

void    GFxSprite::FillTabableArray(GTL::garray<GPtr<GFxASCharacter> >& array, bool* tabIndexed)
{
    if (TabChildren.IsFalse()) return;
    GASSERT(tabIndexed);

    GFxCharacter*   ch;
    UInt            i, n = DisplayList.GetCharacterCount();

    for (i = 0; i < n; i++)
    {
        ch = DisplayList.GetCharacter(i);
        if (ch != NULL && ch->IsASCharacter() && ch->GetVisible())
        {
            GPtr<GFxASCharacter> asch = ch->ToASCharacter();
            if (asch->IsTabIndexed() && !(*tabIndexed))
            {
                // the first char with tabIndex: release the current array and
                // start to fill it again according to tabIndex.
                array.release();
                *tabIndexed = true;
            }
            // append for now; if tabIndexed - later it will be sorted by tabIndex
            if (asch->IsTabable() && (!(*tabIndexed) || asch->IsTabIndexed()))
                array.push_back(asch);
            asch->FillTabableArray(array, tabIndexed);
        }
    }
}

void GFxSprite::OnGettingKeyboardFocus()
{
    if (ActsAsButton() && !GetMovieRoot()->DisableFocusRolloverEvent.IsTrue()) 
        OnButtonEvent(GFxEventId::Event_RollOver);
}

// invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
bool GFxSprite::OnLosingKeyboardFocus(GFxASCharacter*, GFxFocusMovedType) 
{
    if (ActsAsButton() && GetMovieRoot()->IsFocusRectShown() && 
        !GetMovieRoot()->DisableFocusRolloverEvent.IsTrue())
    {
        OnButtonEvent(GFxEventId::Event_RollOut);
    }
    return true;
}

void GFxSprite::OnInsertionAsLevel(int level)
{
    if (level == 0)
        SetFocusRectFlag(); // _focusrect in _root is true by default
    else if (level > 0)
    {
        // looks like levels above 0 inherits _focusrect from _level0. Probably
        // another properties are also inherited, need investigate.
        GFxASCharacter* _level0 = GetLevelMovie(0);
        if (_level0)
        {
            SetFocusRectFlag(_level0->IsFocusRectEnabled());
        }
    }
    SetConstMemberRaw(GetASEnvironment()->GetSC(), "$version", 
        GASValue(GetASEnvironment()->CreateConstString(GFX_FLASH_VERSION)));
}

UInt GFxSprite::GetCursorType() const
{
    if (ActsAsButton())
    {
        const GASEnvironment* penv = GetASEnvironment();
        GASValue val;
       
        if (const_cast<GFxSprite*>(this)->GetMemberRaw(penv->GetSC(), penv->GetBuiltin(GASBuiltin_useHandCursor), &val))
        {
            if (!val.ToBool(penv))
                return GFxASCharacter::GetCursorType();
        }
        return GFxMouseCursorEvent::HAND;
    }
    return GFxASCharacter::GetCursorType();
}

void GFxSprite::SetMask(GFxSprite* pmaskSprite) 
{ 
    // clear "used-as-mask" flag for old MaskCharacter
    if (MaskCharacter)
    {
        MaskCharacter->SetUsedAsMask(false);
    }
    MaskCharacter = pmaskSprite;

    // the sprite being masked cannot be a mask for another sprite
    SetUsedAsMask(false);
    
    if(pmaskSprite) 
    {
        // setMask method override layers' masking (by zeroing ClipDepth).
        // (Do we need to do same for the mask sprite too? !AB)
        SetClipDepth(0); 

        // the mask sprite cannot have its own mask
        pmaskSprite->SetMask(NULL);

        // mark the mask sprite as a mask.
        pmaskSprite->SetUsedAsMask();
    }
    SetDirtyFlag();
}

void GFxSprite::SetScale9Grid(const GFxScale9Grid* gr)
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
    if (propagate)
        PropagateScale9GridExists();
    SetDirtyFlag();
}

void GFxSprite::PropagateScale9GridExists()
{
    bool actualGrid = GetScale9Grid() != 0;
    // Stop cleaning up scale9Grid if actual one exists in the node
    if (!DoesScale9GridExist() && actualGrid)
        return; 

    for (int i = 0; i < DisplayList.GetCharacterCount(); ++i)
    {
        GFxCharacter* ch = DisplayList.GetCharacter(i);
        ch->SetScale9GridExists(DoesScale9GridExist() || actualGrid);
        ch->PropagateScale9GridExists();
    }   
}

void GFxSprite::SetVisible(bool visible)            
{ 
    SetVisibleFlag(visible); 
    SetNoAdvanceGlobalFlag(!visible && pRoot->IsNoInvisibleAdvanceFlagSet());
    SetDirtyFlag(); 
}


// GFx_SpriteGetTarget can return NULL in the case when
// sprite-related method (such as 'attachMovie', etc) is being
// called for MovieClip instance without attached sprite, for 
// example:
// var m = new MovieClip;
// m.attachMovie(...);
// Here GFx_SpriteGetTarget wiil return NULL.
static GFxSprite* GFx_SpriteGetTarget(const GASFnCall& fn)
{
    GFxSprite* psprite = NULL;
    if (fn.ThisPtr == NULL)
    {
        psprite = (GFxSprite*) fn.Env->GetTarget();
    }
    else
    {
        if (fn.ThisPtr->IsSprite())
            psprite = (GFxSprite*) fn.ThisPtr;
    }
    //GASSERT(psprite);
    return psprite;
}

static GFxASCharacter* GFx_CharacterGetTarget(const GASFnCall& fn)
{
    GFxASCharacter* psprite = NULL;
    if (fn.ThisPtr == NULL)
    {
        psprite = (GFxASCharacter*) fn.Env->GetTarget();
    }
    else
    {
        if (fn.ThisPtr->IsASCharacter())
            psprite = (GFxASCharacter*) fn.ThisPtr;
    }
    //GASSERT(psprite);
    return psprite;
}

//
// *** Sprite Built-In ActionScript methods
//

static void GFx_SpritePlay(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->SetPlayState(GFxMovie::Playing);
}

static void GFx_SpriteStop(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->SetPlayState(GFxMovie::Stopped);
}

static void GFx_SpriteGotoAndPlay(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs < 1)
    {
        psprite->LogScriptError("Error: GFx_SpriteGotoAndPlay needs one arg\n");
        return;
    }

    const GASValue& arg = fn.Arg(0);
    UInt            targetFrame = GFC_MAX_UINT;
    if (arg.GetType() == GASValue::STRING)
    {   // Frame label or string frame number
        GASString sa0(arg.ToString(fn.Env));
        if (!psprite->GetLabeledFrame(sa0.ToCStr(), &targetFrame))
            return;
    }
    else
    {   // Convert to 0-based
        targetFrame = ((UInt)arg.ToNumber(fn.Env)) - 1;
    }

    psprite->GotoFrame(targetFrame);
    psprite->SetPlayState(GFxMovie::Playing);
}

static void GFx_SpriteGotoAndStop(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs < 1)
    {
        psprite->LogScriptError("Error: GFx_SpriteGotoAndStop needs one arg\n");
        return;
    }

    const GASValue& arg = fn.Arg(0);
    UInt            targetFrame = GFC_MAX_UINT;
    if (arg.GetType() == GASValue::STRING)
    {   // Frame label or string frame number
        GASString sa0(arg.ToString(fn.Env));
        if (!psprite->GetLabeledFrame(sa0.ToCStr(), &targetFrame))
            return;
    }
    else
    {   // Convert to 0-based
        targetFrame = ((UInt)arg.ToNumber(fn.Env)) - 1;
    }

    psprite->GotoFrame(targetFrame);
    psprite->SetPlayState(GFxMovie::Stopped);
}

static void GFx_SpriteNextFrame(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    int availableFrameCount = psprite->GetLoadingFrame();
    int currentFrame = psprite->GetCurrentFrame();
    if (currentFrame < availableFrameCount)
        psprite->GotoFrame(currentFrame + 1);      
    psprite->SetPlayState(GFxMovie::Stopped);
}

static void GFx_SpritePrevFrame(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    
    int currentFrame = psprite->GetCurrentFrame();
    if (currentFrame > 0)
        psprite->GotoFrame(currentFrame - 1);
    psprite->SetPlayState(GFxMovie::Stopped);
}


static void GFx_SpriteGetBytesLoaded(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Value of getBytesLoaded must always match the frames loaded,
    // because many Flash files rely on it. In particular, if this value
    // is equal to getTotalBytes, they assume that all frames must be
    // available. If that does not hold, many pre-loader algorithms
    // will end up in infinite AS loops.

    fn.Result->SetInt(psprite->GetBytesLoaded());
}

static void GFx_SpriteGetBytesTotal(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    fn.Result->SetInt(psprite->GetResourceMovieDef()->GetFileBytes());
}


// Duplicate / remove clip.
static void GFx_SpriteDuplicateMovieClip(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 2) return;

    GPtr<GFxASCharacter> newCh = psprite->CloneDisplayObject(
            fn.Arg(0).ToString(fn.Env), ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
            (fn.NArgs == 3) ? fn.Arg(2).ToObjectInterface(fn.Env) : 0);

    //!AB: duplicateMovieClip in Flash 6 and above should return newly created clip
    if (psprite->GetVersion() >= 6)
    {
        fn.Result->SetAsCharacter(newCh.GetPtr());
    }
}

static void GFx_SpriteAttachMovie(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 3) return;

    // Get resource id, looked up locally based on  
    GASString           sa0(fn.Arg(0).ToString(fn.Env));

    GFxResourceBindData resBindData;
    if (!psprite->GetResourceMovieDef()->GetExportedResource(&resBindData, sa0.ToCStr()))
        return;
    
    GASSERT(resBindData.pResource.GetPtr() != 0); // MA TBD: Could this be true?     
    if (!(resBindData.pResource->GetResourceType() & GFxResource::RT_CharacterDef_Bit))
        return;

    GFxCharacterCreateInfo ccinfo;
    ccinfo.pCharDef     = (GFxCharacterDef*) resBindData.pResource.GetPtr();
    ccinfo.pBindDefImpl = resBindData.pBinding->GetOwnerDefImpl();
    
    // Create a new object and add it.
    GTL::garray<GFxSwfEvent*>   DummyEventHandlers;
    GFxCharPosInfo pos( ccinfo.pCharDef->GetId(), ((SInt)fn.Arg(2).ToNumber(fn.Env)) + 16384,
                        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return;

    // We pass pchDef to make sure that symbols from nested imports use
    // the right definition scope, which might be different from that of psprite.
    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
            pos, fn.Arg(1).ToString(fn.Env), DummyEventHandlers,
            (fn.NArgs == 4) ? fn.Arg(3).ToObjectInterface(fn.Env) : 0, true, GFC_MAX_UINT, false, &ccinfo);

    //!AB: attachMovie in Flash 6 and above should return newly created clip
    if (psprite->GetVersion() >= 6)
    {
        GFxASCharacter* pspriteCh = newCh->ToASCharacter(); 
        GASSERT (pspriteCh != 0);
        fn.Result->SetAsCharacter(pspriteCh);
    }
}

static void GFx_SpriteAttachBitmap(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 2) 
        return;
    if (psprite->GetVersion() < 8) // supported only in Flash 8
        return;

    // Arg(0) - BitmapData
    // Arg(1) - depth
    // Arg(2) - String, pixelSnapping, optional (auto, always, never)
    // Arg(3) - Boolean, smoothing, optional

    GPtr<GASObject> pobj = fn.Arg(0).ToObject();
    if (!pobj || pobj->GetObjectType() != GASObject::Object_BitmapData)
        return;

    GASBitmapData*    pbmpData  = static_cast<GASBitmapData*>(pobj.GetPtr());
    GFxImageResource* pimageRes = pbmpData->GetImage();
    if (!pimageRes)
        return;    

    // Create position info
    GFxCharPosInfo pos = GFxCharPosInfo(GFxResourceId(GFxCharacterDef::CharId_ImageMovieDef_ShapeDef),
                       ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
                       1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return;

    bool                    smoothing = (fn.NArgs >= 4) ? fn.Arg(3).ToBool(fn.Env) : false;
    GFxMovieRoot*           pmroot = fn.Env->GetMovieRoot();
    GPtr<GFxMovieDefImpl>   pimageMovieDef = *pmroot->CreateImageMovieDef(pimageRes, smoothing, "");

    if (pimageMovieDef)
    {           
        // Empty sprite based on new Def. New Def gives correct Version, URL, and symbol lookup behavior.
        GPtr<GFxSprite> pchildSprite = *new GFxSprite(pimageMovieDef->GetDataDef(), pimageMovieDef, pmroot, psprite,
                                                      GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip), true);

        if (pchildSprite)
        {    

            // Verified: Flash assigns depth -16383 to image shapes (depth value = 1 in our list).
            SInt                        depth = 1;
            GFxCharPosInfo              locpos = GFxCharPosInfo(GFxResourceId(GFxCharacterDef::CharId_ImageMovieDef_ShapeDef),
                                               depth, 0, GRenderer::Cxform(), 1, GRenderer::Matrix());

            GASString                   emptyName(fn.Env->GetBuiltin(GASBuiltin_empty_));
            GTL::garray<GFxSwfEvent*>   emptyEvents;

            pchildSprite->AddDisplayObject(locpos, emptyName, emptyEvents, 0, 1);
            psprite->ReplaceDisplayObject(pos, pchildSprite, emptyName);
        }
    }
}

static void GFx_SpriteCreateEmptyMovieClip(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 2) return;

    // Create a new object and add it.
    GTL::garray<GFxSwfEvent*>   DummyEventHandlers;
    GFxCharPosInfo pos = GFxCharPosInfo( GFxResourceId(GFxCharacterDef::CharId_EmptyMovieClip),
                        ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
                        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return;

    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
                pos, fn.Arg(0).ToString(fn.Env), DummyEventHandlers, 0, true);  

    // Return newly created clip.
    GFxASCharacter* pspriteCh = newCh->ToASCharacter(); 
    GASSERT (pspriteCh != 0);
    fn.Result->SetAsCharacter(pspriteCh);
}

static void GFx_SpriteCreateTextField(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 6) return; //?AB, could it work with less than 6 params specified?

    // Create a new object and add it.
    GTL::garray<GFxSwfEvent*>   DummyEventHandlers;
    GFxCharPosInfo pos = GFxCharPosInfo( GFxResourceId(GFxCharacterDef::CharId_EmptyTextField),
        ((SInt)fn.Arg(1).ToNumber(fn.Env)) + 16384,
        1, GRenderer::Cxform::Identity, 1, GRenderer::Matrix::Identity);
    // Bounds check depth.
    if ((pos.Depth < 0) || (pos.Depth > (2130690045 + 16384)))
        return;

    GPtr<GFxCharacter> newCh = psprite->AddDisplayObject(
        pos, fn.Arg(0).ToString(fn.Env), DummyEventHandlers, 0, true);  

    // Return newly created clip.
    GFxASCharacter* ptextCh = newCh->ToASCharacter(); 
    GASSERT (ptextCh != 0);
    ptextCh->SetStandardMember(GFxSprite::M_x, fn.Arg(2), false);
    ptextCh->SetStandardMember(GFxSprite::M_y, fn.Arg(3), false);
    ptextCh->SetStandardMember(GFxSprite::M_width,  fn.Arg(4), false);
    ptextCh->SetStandardMember(GFxSprite::M_height, fn.Arg(5), false);
    fn.Result->SetAsCharacter(ptextCh);
}


static void GFx_SpriteRemoveMovieClip(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (psprite->GetDepth() < 16384)
    {
        psprite->LogScriptWarning("%s.removeMovieClip() failed - depth must be >= 0\n",
                                  psprite->GetName().ToCStr());
        return;
    }
    psprite->RemoveDisplayObject();
}

// Implementation of MovieClip.setMask method. 
static void GFx_SpriteSetMask(const GASFnCall& fn)
{
    fn.Result->SetUndefined();

    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 1)
    {
        if (fn.Arg(0).IsNull())
        {
            // remove mask
            psprite->SetMask(NULL);
        }
        else
        {
            GFxASCharacter* pobj = fn.Arg(0).ToASCharacter(fn.Env);
            if (pobj)
            {
                GFxSprite* pmaskSpr = pobj->ToSprite();
                psprite->SetMask(pmaskSpr);
            }
            else
                psprite->SetMask(NULL);
        }
    }
}

static void GFx_SpriteGetBounds(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Get target sprite.
    GFxASCharacter*     pobj    = (fn.NArgs > 0) ? fn.Arg(0).ToASCharacter(fn.Env) : psprite;
    GFxSprite*          ptarget = pobj ? pobj->ToSprite() : 0;

    GRectF              b(0);
    GRenderer::Matrix   matrix;

    if (ptarget)
    {
        // Transform first by sprite's matrix, then by inverse of target.
        if (ptarget != psprite) // Optimize identity case.
        {
            matrix.SetInverse(ptarget->GetWorldMatrix());
            matrix *= psprite->GetWorldMatrix();
        }
        
        // A "perfect" implementation would be { b = psprite->GetBounds(matrix); },
        // however, Flash always gets the local bounding box before a transform,
        // as follows.
        matrix.EncloseTransform(&b, psprite->GetBounds(GRenderer::Matrix()));
    }

    // Store into result object.    
    GPtr<GASObject>     presult = *new GASObject;
    GASStringContext*   psc = fn.Env->GetSC();
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_xMin), TwipsToPixels(Double(b.Left)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_xMax), TwipsToPixels(Double(b.Right)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_yMin), TwipsToPixels(Double(b.Top)));
    presult->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_yMax), TwipsToPixels(Double(b.Bottom)));

    fn.Result->SetAsObject(presult.GetPtr());   
}

static void GFx_SpriteLocalToGlobal(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 1) return;

    // Get target object.
    GASStringContext*   psc = fn.Env->GetSC();
    GASObjectInterface* pobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!pobj) return;

    // Object must have x & y members, which are numbers; otherwise 
    // the function does nothing. This is the expected behavior.
    GASValue            xval, yval;  
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &xval);
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &yval);
    if (!xval.IsNumber() || !yval.IsNumber())
        return;

    // Compute transformation and set members.
    GPointF pt((Float)GFC_PIXELS_TO_TWIPS(xval.ToNumber(fn.Env)), (Float)GFC_PIXELS_TO_TWIPS(yval.ToNumber(fn.Env)));
    pt = psprite->GetWorldMatrix().Transform(pt);
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), TwipsToPixels(Double(pt.x)));
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), TwipsToPixels(Double(pt.y)));
}

static void GFx_SpriteGlobalToLocal(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL || fn.NArgs < 1) return;

    // Get target object.
    GASStringContext*   psc = fn.Env->GetSC();
    GASObjectInterface* pobj = fn.Arg(0).ToObjectInterface(fn.Env);
    if (!pobj) return;

    // Object must have x & y members, which are numbers; otherwise 
    // the function does nothing. This is the expected behavior.
    GASValue xval, yval;
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), &xval);
    pobj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), &yval);
    if (!xval.IsNumber() || !yval.IsNumber())
        return;

    // Compute transformation and set members.
    GPointF pt((Float)GFC_PIXELS_TO_TWIPS(xval.ToNumber(fn.Env)), (Float)GFC_PIXELS_TO_TWIPS(yval.ToNumber(fn.Env)));
    pt = psprite->GetWorldMatrix().TransformByInverse(pt);
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_x), TwipsToPixels(Double(pt.x)));
    pobj->SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_y), TwipsToPixels(Double(pt.y)));
}


// Hit-test implementation.
static void GFx_SpriteHitTest(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    fn.Result->SetBool(0);

    GRectF spriteLocalBounds = psprite->GetBounds(GRenderer::Matrix());
    if (spriteLocalBounds.IsNull())
        return;

    // Hit-test has either 1, 2 or 3 arguments.
    if (fn.NArgs >= 2)
    {
        UInt8 hitTestMask = 0;
        // x, y, shapeFlag version of hitTest.
        GPointF     pt( (Float)GFC_PIXELS_TO_TWIPS(fn.Arg(0).ToNumber(fn.Env)),
                        (Float)GFC_PIXELS_TO_TWIPS(fn.Arg(1).ToNumber(fn.Env)) );
        if (fn.NArgs >= 3) // optional parameter shapeFlag
            hitTestMask |= (fn.Arg(2).ToBool(fn.Env)) ? GFxCharacter::HitTest_TestShape : 0;
        if (fn.NArgs >= 4) // optional parameter shapeFlag
            hitTestMask |= (fn.Arg(3).ToBool(fn.Env)) ? GFxCharacter::HitTest_IgnoreInvisible : 0;

        GPointF ptSpr = psprite->GetLevelMatrix().TransformByInverse(pt);
        // pt is already in root's coordinates!

        if (psprite->DoesScale9GridExist())
        {
            fn.Result->SetBool(psprite->PointTestLocal (ptSpr, hitTestMask));
        }
        else
        {
            if (spriteLocalBounds.Contains(ptSpr))
            {
                if (!(hitTestMask & GFxCharacter::HitTest_TestShape))
                    fn.Result->SetBool(true);
                else
                    fn.Result->SetBool(psprite->PointTestLocal (ptSpr, hitTestMask));
            }
            else
                fn.Result->SetBool(false);
        }
    }
    else if (fn.NArgs == 1)
    {
        // Target sprite version of hit-test.
        const GASValue&     arg = fn.Arg(0);
        GFxSprite*          ptarget = NULL;
        if (arg.IsCharacter())
        {
            GFxASCharacter* pchar   = arg.ToASCharacter(fn.Env);
            ptarget = pchar ? pchar->ToSprite() : 0;
        }
        else
        {
            // if argument is not a character, then convert it to string
            // and try to resolve as variable (AB)
            GASString varName = arg.ToString(fn.Env);
            GASValue result;
            if (fn.Env->GetVariable(varName, &result))
            {
                GFxASCharacter* pchar = result.ToASCharacter(fn.Env);
                ptarget = pchar ? pchar->ToSprite() : 0;
            }
        }
        if (!ptarget)
            return;

        GRectF targetLocalRect  = ptarget->GetBounds(GRenderer::Matrix());
        if (targetLocalRect.IsNull())
            return;
        // Rect rectangles in same coordinate space & check intersection.
        GRectF spriteWorldRect  = psprite->GetWorldMatrix().EncloseTransform(spriteLocalBounds);
        GRectF targetWorldRect  = ptarget->GetWorldMatrix().EncloseTransform(targetLocalRect);
        fn.Result->SetBool(spriteWorldRect.Intersects(targetWorldRect));
    }
}

// Implemented as member function so that it can access Sprite's private methods.
void    GFxSprite::SpriteSwapDepths(const GASFnCall& fn)
{
    GFxASCharacter* pchar = GFx_CharacterGetTarget(fn);
    if (pchar == NULL || fn.NArgs < 1) return;

    GFxSprite* pparent = (GFxSprite*)pchar->GetParent();
    const GASValue& arg     = fn.Arg(0);
    int             depth2  = 0;
    GFxASCharacter* ptarget = 0;
    GFxSprite* psprite = (pchar->IsSprite()) ? (GFxSprite*)pchar : 0;

    // Depth can be a number at which the character is inserted.
    if (arg.IsNumber())
    {
        depth2 = ((int)arg.ToNumber(fn.Env)) + 16384;
        // Verified: Flash will discard smaller values.
        if (depth2 < 0)
            return;
        // Documented depth range is -16384 to 1048575,
        // but Flash does not seem to enforce that upper constraint. Instead
        // manually determined upper constraint is 2130690045.
        if (depth2 > (2130690045 + 16384))
            return;
    }
    else
    {
        // Need to search for target in our scope (environment scope might be different).
        if (psprite)
        {
            GFxASCharacter* poldTarget = fn.Env->GetTarget();
            fn.Env->SetTarget(psprite);
            ptarget = fn.Env->FindTargetByValue(arg);
            fn.Env->SetTarget(poldTarget);
        }
        else
            ptarget = fn.Env->FindTargetByValue(arg); //???

        if (!ptarget || ptarget == pchar)
            return;         
        // Must have same parent.
        if (pparent != ptarget->GetParent())
            return;

        // Can we swap with text fields, etc? Yes.
        depth2 = ptarget->GetDepth();
    }

    pchar->SetAcceptAnimMoves(0);
    // Do swap or depth change.
    if (pparent && pparent->DisplayList.SwapDepths(pchar->GetDepth(), depth2))
    {
        pparent->SetDirtyFlag();
        if (ptarget)
            ptarget->SetAcceptAnimMoves(0);
    }
}

static void GFx_SpriteGetNextHighestDepth(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Depths is always at least 0. No upper bound constraint is done
    // (i.e. 2130690046 will be returned in Flash, although invalid).
    SInt depth = GTL::gmax<SInt>(0, psprite->GetLargestDepthInUse() - 16384 + 1);
    fn.Result->SetInt(depth);
}

static void GFx_SpriteGetInstanceAtDepth(const GASFnCall& fn)
{
    fn.Result->SetUndefined();
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    // Note: this can also report buttons, etc.
    if (fn.NArgs >= 1)
    {
        GFxCharacter* pchar = psprite->GetCharacterAtDepth(((int)fn.Arg(0).ToNumber(fn.Env)) + 16384);
        if (pchar)
            fn.Result->SetAsCharacter(pchar->ToASCharacter());
    }
}


static void GFx_SpriteGetSWFVersion(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    
    fn.Result->SetInt(psprite->GetVersion());
}

static void GFx_SpriteStartDrag(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    GFxMovieRoot::DragState st;
    bool                    lockCenter = false;

    // If there are arguments, init appropriate values.
    if (fn.NArgs > 0)
    {
        lockCenter = fn.Arg(0).ToBool(fn.Env);
        if (fn.NArgs > 4)
        {
            st.Bound = true;
            st.BoundLT.x = GFC_PIXELS_TO_TWIPS((Float) fn.Arg(1).ToNumber(fn.Env));
            st.BoundLT.y = GFC_PIXELS_TO_TWIPS((Float) fn.Arg(2).ToNumber(fn.Env));
            st.BoundRB.x = GFC_PIXELS_TO_TWIPS((Float) fn.Arg(3).ToNumber(fn.Env));
            st.BoundRB.y = GFC_PIXELS_TO_TWIPS((Float) fn.Arg(4).ToNumber(fn.Env));
        }
    }

    st.pCharacter = psprite;
    // Init mouse offsets based on LockCenter flag.
    st.InitCenterDelta(lockCenter);

    // Begin dragging.
    psprite->GetMovieRoot()->SetDragState(st);  
}


static void GFx_SpriteStopDrag(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

     // Stop dragging.
    psprite->GetMovieRoot()->StopDrag();    
}

static void GFx_SpriteLoadMovie(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0)
    {
        GFxLoadQueueEntry::LoadMethod lm = GFxLoadQueueEntry::LM_None;

        // Decode load method argument.
        if (fn.NArgs > 1)
        {
            GASString str(fn.Arg(1).ToString(fn.Env).ToLower());
            if (str == "get")
                lm = GFxLoadQueueEntry::LM_Get;
            else if (str == "post")
                lm = GFxLoadQueueEntry::LM_Post;
        }

        // Post loadMovie into queue.
        GASString urlStr(fn.Arg(0).ToString(fn.Env));
        psprite->GetMovieRoot()->AddLoadQueueEntry(psprite, urlStr.ToCStr(), lm);
    }
}

static void GFx_SpriteUnloadMovie(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

     // Post unloadMovie into queue (empty url == unload).
    psprite->GetMovieRoot()->AddLoadQueueEntry(psprite, "");
}

static void GFx_SpriteLoadVariables(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0)
    {
        GFxLoadQueueEntry::LoadMethod lm = GFxLoadQueueEntry::LM_None;

        // Decode load method argument.
        if (fn.NArgs > 1)
        {
            GASString str(fn.Arg(1).ToString(fn.Env).ToLower());
            if (str == "get")
                lm = GFxLoadQueueEntry::LM_Get;
            else if (str == "post")
                lm = GFxLoadQueueEntry::LM_Post;
        }

        // Post loadMovie into queue.
        GASString urlStr(fn.Arg(0).ToString(fn.Env));
        psprite->GetMovieRoot()->AddVarLoadQueueEntry(psprite, urlStr.ToCStr(), lm);
    }
}

// Drawing API support
//=============================================================================
void GFxSprite::Clear()
{
    if(pDrawingAPI)
    {
        pDrawingAPI->Clear();
    }
    SetDirtyFlag();
}

void GFxSprite::SetNoLine()
{
    AcquirePath(false);
    pDrawingAPI->SetNoLine();
}

void GFxSprite::SetNoFill()
{
    AcquirePath(true);
    pDrawingAPI->SetNoFill();
}

void GFxSprite::SetLineStyle(Float lineWidth, 
                             UInt  rgba, 
                             bool  hinting, 
                             GFxLineStyle::LineStyle scaling, 
                             GFxLineStyle::LineStyle caps,
                             GFxLineStyle::LineStyle joins,
                             Float miterLimit)
{
    AcquirePath(false);
    pDrawingAPI->SetLineStyle(PixelsToTwips(lineWidth), rgba, hinting, scaling, caps, joins, miterLimit);
}

// The function begins new fill with an "empty" style. 
// It returns the pointer to just created fill style, so the caller
// can set any values. The function is used in Action Script, beginGradientFill
GFxFillStyle* GFxSprite::BeginFill()
{
    AcquirePath(true);
    return pDrawingAPI->SetNewFill();
}

GFxFillStyle* GFxSprite::CreateLineComplexFill()
{
    AcquirePath(true);
    return pDrawingAPI->CreateLineComplexFill();
}

void GFxSprite::BeginFill(UInt rgba)
{
    AcquirePath(true);
    pDrawingAPI->SetFill(rgba);
}


void GFxSprite::BeginBitmapFill(GFxFillType fillType,
                                GFxImageResource* pimageRes,
                                const Matrix& mtx)
{
    AcquirePath(true);
    pDrawingAPI->SetBitmapFill(fillType, pimageRes, mtx);
}

void GFxSprite::EndFill()
{
    AcquirePath(true);
    pDrawingAPI->ResetFill();
}

void GFxSprite::MoveTo(Float x, Float y)
{
    AcquirePath(false);
    pDrawingAPI->MoveTo(PixelsToTwips(x), 
                        PixelsToTwips(y));
}

void GFxSprite::LineTo(Float x, Float y)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *new GFxDrawingContext;
    pDrawingAPI->LineTo(PixelsToTwips(x), 
                        PixelsToTwips(y));
}

void GFxSprite::CurveTo(Float cx, Float cy, Float ax, Float ay)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *new GFxDrawingContext;
    pDrawingAPI->CurveTo(PixelsToTwips(cx), 
                         PixelsToTwips(cy),
                         PixelsToTwips(ax), 
                         PixelsToTwips(ay));
}

bool GFxSprite::AcquirePath(bool newShapeFlag)
{
    if (!pDrawingAPI) 
        pDrawingAPI = *new GFxDrawingContext;
    SetDirtyFlag();
    return pDrawingAPI->AcquirePath(newShapeFlag);
}

// Drawing API
//=============================================================================
static void GFx_SpriteClear(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->Clear();
}

static void GFx_SpriteBeginFill(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if(fn.NArgs > 0) // rgb:Number
    {
        UInt rgba = UInt(fn.Arg(0).ToNumber(fn.Env)) | 0xFF000000;
        if(fn.NArgs > 1) // alpha:Number
        {
            // Alpha is clamped to 0 to 100 range.
            Float alpha = ((Float)fn.Arg(1).ToNumber(fn.Env)) * 255.0f / 100.0f; 
            rgba &= 0xFFFFFF;            
            rgba |= UInt(GTL::gclamp(alpha, 0.0f, 255.0f)) << 24;
        }
        psprite->BeginFill(rgba);
    }
    else
    {
        psprite->SetNoFill();
    }
}

static void GFx_SpriteCreateGradient(const GASFnCall& fn, GFxFillStyle* fillStyle)
{
    // beginGradientFill(fillType:String, 
    //                   colors:Array, 
    //                   alphas:Array, 
    //                   ratios:Array, 
    //                   matrix:Object, 
    //                   [spreadMethod:String], 
    //                   [interpolationMethod:String], 
    //                   [focalPointRatio:Number]) : Void

    GASObject* arg = 0;
    if (fn.NArgs > 0)
    {
        GASString fillTypeStr(fn.Arg(0).ToString(fn.Env));

        if (fn.NArgs > 1 && 
           (arg = fn.Arg(1).ToObject())->GetObjectType() == GASObjectInterface::Object_Array)
        {
            const GASArrayObject* colors = (const GASArrayObject*)arg;
            if (fn.NArgs > 2 && 
               (arg = fn.Arg(2).ToObject())->GetObjectType() == GASObjectInterface::Object_Array)
            {
                const GASArrayObject* alphas = (const GASArrayObject*)arg;
                if (fn.NArgs > 3 && 
                   (arg = fn.Arg(3).ToObject())->GetObjectType() == GASObjectInterface::Object_Array)
                {
                    const GASArrayObject* ratios = (const GASArrayObject*)arg;
                    if (fn.NArgs > 4 && 
                        colors->GetSize() > 0 && 
                        colors->GetSize() == alphas->GetSize() &&
                        colors->GetSize() == ratios->GetSize())
                    {
                        GMatrix2D matrix;
                        GASValue v;
                        GASStringContext *psc = fn.Env->GetSC();

                        arg = fn.Arg(4).ToObject();

#ifndef GFC_NO_FXPLAYER_AS_MATRIX
                        if(arg->GetObjectType() == GASObjectInterface::Object_Matrix)
                        {
                            matrix = ((GASMatrixObject*)arg)->GetMatrix(fn.Env);
                        }
                        else
#endif
                        {
                            if (arg->GetConstMemberRaw(psc, "matrixType",  &v) && 
                                v.ToString(fn.Env) == "box")
                            {
                                Float x = 0;
                                Float y = 0;
                                Float w = 100;
                                Float h = 100;
                                Float r = 0;
                                if(arg->GetConstMemberRaw(psc, "x",  &v)) x = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "y",  &v)) y = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "w",  &v)) w = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "h",  &v)) h = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "r",  &v)) r = (Float)v.ToNumber(fn.Env);

                                x += w / 2;
                                y += h / 2;
                                w *= GASGradientBoxMagicNumber; 
                                h *= GASGradientBoxMagicNumber;

                                matrix.AppendRotation(r);
                                matrix.AppendScaling(w, h);
                                matrix.AppendTranslation(x, y);
                            }
                            else
                            {
                                // The matrix is transposed:
                                // a(0,0)  d(0,1)  g(0,2)
                                // b(1,0)  e(1,1)  h(1,2)
                                // c(2,0)  f(2,1)  i(2,2)
                                // Elements c,f,i are not used
                                if(arg->GetConstMemberRaw(psc, "a",  &v)) matrix.M_[0][0] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "d",  &v)) matrix.M_[0][1] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "g",  &v)) matrix.M_[0][2] = (Float)v.ToNumber(fn.Env);
                                if(arg->GetConstMemberRaw(psc, "b",  &v)) matrix.M_[1][0] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "e",  &v)) matrix.M_[1][1] = (Float)v.ToNumber(fn.Env) * GASGradientBoxMagicNumber;
                                if(arg->GetConstMemberRaw(psc, "h",  &v)) matrix.M_[1][2] = (Float)v.ToNumber(fn.Env);
                            }
                        }

                        int   spreadMethod    = 0; // pad
                        bool  linearRGB       = false;
                        Float focalPointRatio = 0;
                        if (fn.NArgs > 5)
                        {
                            GASString str(fn.Arg(5).ToString(fn.Env));

                                 if(str == "reflect") spreadMethod = 1;
                            else if(str == "repeat")  spreadMethod = 2;

                            if (fn.NArgs > 6)
                            {
                                linearRGB = fn.Arg(6).ToString(fn.Env) == "linearRGB";
                                if (fn.NArgs > 7)
                                {
                                    focalPointRatio = (Float)(fn.Arg(7).ToNumber(fn.Env));
                                    if (focalPointRatio < -1) focalPointRatio = -1;
                                    if (focalPointRatio >  1) focalPointRatio =  1;
                                }
                            }
                        }

                        // Create Gradient itself
                        GFxFillType fillType = GFxFill_LinearGradient;
                        if (fillTypeStr == "radial")
                        {
                            fillType = GFxFill_RadialGradient;
                            if (focalPointRatio != 0) 
                                fillType = GFxFill_FocalPointGradient;
                        }

                        GPtr<GFxGradientData> pgradientData = 
                            *new GFxGradientData(fillType, (UInt16)colors->GetSize(), linearRGB);

                        if (pgradientData)
                        {
                            pgradientData->SetFocalRatio(focalPointRatio);
                            
                            for (int i = 0; i < colors->GetSize(); i++)
                            {
                                UInt rgba = UInt(colors->GetElementPtr(i)->ToNumber(fn.Env)) | 0xFF000000;

                                // Alpha is clamped to 0 to 100 range.
                                Float alpha = ((Float)alphas->GetElementPtr(i)->ToNumber(fn.Env)) * 255.0f / 100.0f; 
                                rgba &= 0xFFFFFF;            
                                rgba |= UInt(GTL::gclamp(alpha, 0.0f, 255.0f)) << 24;

                                Float ratio = (Float)ratios->GetElementPtr(i)->ToNumber(fn.Env); 
                                ratio = GTL::gclamp(ratio, 0.0f, 255.0f);

                                GFxGradientRecord& record = (*pgradientData)[i];
                                record.Ratio = (UByte)ratio;
                                record.Color = rgba;                              
                            }

                            fillStyle->SetGradientFill(fillType, pgradientData, matrix);
                        }
                    }
                }
            }
        }
    }
}

static void GFx_SpriteBeginGradientFill(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    GFxFillStyle* fillStyle = psprite->BeginFill();
    if (fillStyle)
        GFx_SpriteCreateGradient(fn, fillStyle);
}

static void GFx_SpriteLineGradientStyle(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    GFxFillStyle* fillStyle = psprite->CreateLineComplexFill();
    if (fillStyle)
        GFx_SpriteCreateGradient(fn, fillStyle);
}

static void GFx_SpriteBeginBitmapFill(const GASFnCall& fn)
{
    //beginBitmapFill(bmp:BitmapData, 
    //               [matrix:Matrix], 
    //               [repeat:Boolean], 
    //               [smoothing:Boolean]) : Void

    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0) // bmp:BitmapData,
    {
        GPtr<GASObject> pobj = fn.Arg(0).ToObject();
        if (!pobj || pobj->GetObjectType() != GASObject::Object_BitmapData)
            return;

        GASBitmapData*    pbmpData = static_cast<GASBitmapData*>(pobj.GetPtr());
        GFxImageResource* pimageRes = pbmpData->GetImage();
        if (!pimageRes)
            return;

        GMatrix2D matrix;
        bool repeat = true;
        bool smoothing = false;

        if (fn.NArgs > 1) // [matrix:Matrix]
        {
#ifndef GFC_NO_FXPLAYER_AS_MATRIX
            GASObject* arg = fn.Arg(1).ToObject();
            if(arg->GetObjectType() == GASObjectInterface::Object_Matrix)
            {
                matrix = ((GASMatrixObject*)arg)->GetMatrix(fn.Env);
            }
#endif

            if (fn.NArgs > 2) // [repeat:Boolean]
            {
                repeat = fn.Arg(2).ToBool(fn.Env);
                if (fn.NArgs > 3) // [smoothing:Boolean]
                {
                    smoothing = fn.Arg(3).ToBool(fn.Env);
                }
            }
        }

        GFxFillType fillType;

        if (smoothing)
        {
            if (repeat) fillType = GFxFill_TiledSmoothImage;
            else        fillType = GFxFill_ClippedSmoothImage;
        }
        else
        {
            if (repeat) fillType = GFxFill_TiledImage;
            else        fillType = GFxFill_ClippedImage;
        }        

        psprite->BeginBitmapFill(fillType, pimageRes, matrix);
    }
}

static void GFx_SpriteEndFill(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;
    psprite->EndFill();
}

static void GFx_SpriteLineStyle(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs > 0) // thickness:Number
    {
        Float lineWidth = (Float)(fn.Arg(0).ToNumber(fn.Env));
        UInt  rgba = 0xFF000000;
        bool  hinting = false;
        GFxLineStyle::LineStyle scaling = GFxLineStyle::LineScaling_Normal;
        GFxLineStyle::LineStyle caps    = GFxLineStyle::LineCap_Round;
        GFxLineStyle::LineStyle joins   = GFxLineStyle::LineJoin_Round;
        Float miterLimit = 3.0f;

        if(fn.NArgs > 1) // rgb:Number
        {
            rgba = UInt(fn.Arg(1).ToNumber(fn.Env)) | 0xFF000000;

            if(fn.NArgs > 2) // alpha:Number
            {
                Float alpha = ((Float)fn.Arg(2).ToNumber(fn.Env)) * 255.0f / 100.0f; 

                rgba &= 0xFFFFFF;                
                rgba |= UInt(GTL::gclamp(alpha, 0.0f, 255.0f)) << 24;

                if(fn.NArgs > 3) // pixelHinting:Boolean
                {
                    hinting = fn.Arg(3).ToBool(fn.Env);

                    if(fn.NArgs > 4) // noScale:String
                    {
                        GASString str(fn.Arg(4).ToString(fn.Env));
                             if(str == "none")       scaling = GFxLineStyle::LineScaling_None;
                        else if(str == "vertical")   scaling = GFxLineStyle::LineScaling_Vertical;
                        else if(str == "horizontal") scaling = GFxLineStyle::LineScaling_Horizontal;

                        if(fn.NArgs > 5) // capsStyle:String
                        {
                            str = fn.Arg(5).ToString(fn.Env);
                                 if(str == "none")   caps = GFxLineStyle::LineCap_None;
                            else if(str == "square") caps = GFxLineStyle::LineCap_Square;

                            if(fn.NArgs > 6) // jointStyle:String
                            {
                                str = fn.Arg(6).ToString(fn.Env);
                                     if(str == "miter") joins = GFxLineStyle::LineJoin_Miter;
                                else if(str == "bevel") joins = GFxLineStyle::LineJoin_Bevel;

                                if(fn.NArgs > 7) // miterLimit:Number
                                {
                                    miterLimit = (Float)(fn.Arg(7).ToNumber(fn.Env));
                                    if(miterLimit < 1.0f)   miterLimit = 1.0f;
                                    if(miterLimit > 255.0f) miterLimit = 255.0f;
                                }
                            }
                        }
                    }
                }
            }
        }
        psprite->SetLineStyle(lineWidth, rgba, hinting, scaling, caps, joins, miterLimit);
    }
    else
    {
        psprite->SetNoLine();
    }
}

static void GFx_SpriteMoveTo(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 2) 
    {
        Float x = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float y = (Float)(fn.Arg(1).ToNumber(fn.Env));
        psprite->MoveTo(x, y);
    }
}

static void GFx_SpriteLineTo(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 2) 
    {
        Float x = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float y = (Float)(fn.Arg(1).ToNumber(fn.Env));
        psprite->LineTo(x, y);
    }
}

static void GFx_SpriteCurveTo(const GASFnCall& fn)
{
    GFxSprite* psprite = GFx_SpriteGetTarget(fn);
    if (psprite == NULL) return;

    if (fn.NArgs >= 4) 
    {
        Float cx = (Float)(fn.Arg(0).ToNumber(fn.Env));
        Float cy = (Float)(fn.Arg(1).ToNumber(fn.Env));
        Float ax = (Float)(fn.Arg(2).ToNumber(fn.Env));
        Float ay = (Float)(fn.Arg(3).ToNumber(fn.Env));
        psprite->CurveTo(cx, cy, ax, ay);
    }
}

//////////////////////////////////////////
void GASMovieClipObject::commonInit ()
{
    DynButtonHandlerCount = 0;
    HasButtonHandlers = 0;
}

static bool IsButtonEventName(GASStringContext* psc, const GASString& name)
{    
    return ((psc->GetBuiltin(GASBuiltin_onPress) == name) ||
            (psc->GetBuiltin(GASBuiltin_onRelease) == name) ||
            (psc->GetBuiltin(GASBuiltin_onReleaseOutside) == name) ||
            (psc->GetBuiltin(GASBuiltin_onRollOver) == name) ||
            (psc->GetBuiltin(GASBuiltin_onRollOut) == name) ||
            (psc->GetBuiltin(GASBuiltin_onDragOver) == name) ||
            (psc->GetBuiltin(GASBuiltin_onDragOut) == name)  );
}

// Updates DynButtonHandlerCount if name is begin added through SetMember or
// deleted through DeleteMember.
void    GASMovieClipObject::TrackMemberButtonHandler(GASStringContext* psc, const GASString& name, bool deleteFlag)
{
    // If we are one of button handlers, do the check.
    if ((name[0] == 'o') && (name.GetSize() > 1) && name[1] == 'n')
    {
        GASValue val;
        //!if (ASEnvironment.GetMember(name, &val))
        if (GetMemberRaw(psc, name, &val))
        {
            // Set and member already exists? No tracking increment.
            if (!deleteFlag)
                return;
        }
        else
        {
            // Delete and does not exist? No decrement.
            if (deleteFlag)
                return;
        }
        // Any button handler enables button behavior.
        if (IsButtonEventName(psc, name))
        {
            if (deleteFlag)
                DynButtonHandlerCount --;
            else
                DynButtonHandlerCount ++;
        }
    }
}

void GASMovieClipObject::Set__proto__(GASStringContext *psc, GASObject* protoObj) 
{   
    GASObject::Set__proto__(psc, protoObj);
    if (protoObj->GetObjectType() != Object_MovieClipObject)
    {
        //AB: if proto is being set to the sprite, and this proto is not 
        // an instance of MovieClipObject then we need to rescan it for button's
        // handlers to react on events correctly. This usually happen if 
        // component is extending MovieClip as a class and defines
        // buttons' handler as methods.
        struct MemberVisitor : GASObjectInterface::MemberVisitor
        {
            GPtr<GASMovieClipObject> obj;
            GASStringContext*        pStringContext;

            MemberVisitor(GASStringContext* psc, GASMovieClipObject* _obj)
                : obj(_obj), pStringContext(psc) { }

            virtual void Visit(const GASString& name, const GASValue& val, UByte flags)
            {
                GUNUSED2(val, flags);
                if (IsButtonEventName(pStringContext, name))
                {
                    obj->SetHasButtonHandlers(true);
                }
            }
        } visitor (psc, this);
        pProto->VisitMembers(psc, &visitor, GASObjectInterface::VisitMember_Prototype|GASObjectInterface::VisitMember_DontEnum);
    }
}

bool GASMovieClipObject::ActsAsButton() const   
{ 
    if ((DynButtonHandlerCount != 0) || HasButtonHandlers)
        return true;
    for(GASObject* pproto = pProto; pproto != 0; pproto = pproto->Get__proto__())
    {
        if (pProto->GetObjectType() == Object_MovieClipObject) 
        {
            // MA: No need to AddRef to 'pproto'.
            GASMovieClipProto* proto = static_cast<GASMovieClipProto*>(pproto);
            return proto->ActsAsButton();
        }
    }
    return false;
}

// Set the named member to the value.  Return true if we have
// that member; false otherwise.
bool    GASMovieClipObject::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{
    // _levelN can't function as a button
    // MovieClip.prototype has no sprite (GetSprite() returns 0) but
    // it should track button handlers.
    GPtr<GFxSprite> spr = GetSprite();
    if (!spr || spr->GetASRootMovie() != spr)
        TrackMemberButtonHandler(penv->GetSC(), name);

    return GASObject::SetMember(penv, name, val, flags);
}

// Set *val to the value of the named member and
// return true, if we have the named member.
// Otherwise leave *val alone and return false.
/*bool    GASMovieClipObject::GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)
{
    return GASObject::GetMember(penv, name, val);
}*/

// Delete a member field, if not read-only. Return true if deleted.
bool    GASMovieClipObject::DeleteMember(GASStringContext *psc, const GASString& name)
{
    TrackMemberButtonHandler(psc, name, 1);
    return GASObject::DeleteMember(psc, name);
}

void    GASMovieClipObject::ForceShutdown ()
{
    // traverse through all members, look for FUNCTION and release local frames.
    // Needed to prevent memory leak until we don't have garbage collection.

    for (MemberHash::iterator iter = Members.begin (); iter != Members.end (); ++iter)
    {        
        GASValue& value = iter->second.Value;
        if (value.IsFunction ())
        {
            value.V.FunctionValue.SetLocalFrame (0);
        }
    }
}

static const GASNameFunction MovieClipFunctionTable[] = 
{
    { "play",                   &GFx_SpritePlay },
    { "stop",                   &GFx_SpriteStop },
    { "gotoAndStop",            &GFx_SpriteGotoAndStop },
    { "gotoAndPlay",            &GFx_SpriteGotoAndPlay },
    { "nextFrame",              &GFx_SpriteNextFrame },
    { "prevFrame",              &GFx_SpritePrevFrame },
    { "getBytesLoaded",         &GFx_SpriteGetBytesLoaded },
    { "getBytesTotal",          &GFx_SpriteGetBytesTotal },
                                                     
    { "getBounds",              &GFx_SpriteGetBounds },
    { "localToGlobal",          &GFx_SpriteLocalToGlobal },
    { "globalToLocal",          &GFx_SpriteGlobalToLocal },
    { "hitTest",                &GFx_SpriteHitTest },
                                                     
    { "attachBitmap",           &GFx_SpriteAttachBitmap },
    { "attachMovie",            &GFx_SpriteAttachMovie },
    { "duplicateMovieClip",     &GFx_SpriteDuplicateMovieClip },
    { "removeMovieClip",        &GFx_SpriteRemoveMovieClip },
    { "createEmptyMovieClip",   &GFx_SpriteCreateEmptyMovieClip },
    { "createTextField",        &GFx_SpriteCreateTextField },
                                                     
    { "getDepth",               &GFxASCharacter::CharacterGetDepth },
    { "swapDepths",             &GFxSprite::SpriteSwapDepths },
    { "getNextHighestDepth",    &GFx_SpriteGetNextHighestDepth },
    { "getInstanceAtDepth",     &GFx_SpriteGetInstanceAtDepth },
                                                     
    { "getSWFVersion",          &GFx_SpriteGetSWFVersion },

    { "startDrag",              &GFx_SpriteStartDrag },
    { "stopDrag",               &GFx_SpriteStopDrag },

    { "setMask",                &GFx_SpriteSetMask },

    { "loadMovie",              &GFx_SpriteLoadMovie },
    { "unloadMovie",            &GFx_SpriteUnloadMovie },

    { "loadVariables",          &GFx_SpriteLoadVariables },

    // Drawing API

    { "clear",                  &GFx_SpriteClear     },
    { "beginFill",              &GFx_SpriteBeginFill },
    { "beginGradientFill",      &GFx_SpriteBeginGradientFill },
    { "beginBitmapFill",        &GFx_SpriteBeginBitmapFill },
    { "lineGradientStyle",      &GFx_SpriteLineGradientStyle },
    { "endFill",                &GFx_SpriteEndFill   },
    { "lineStyle",              &GFx_SpriteLineStyle },
    { "moveTo",                 &GFx_SpriteMoveTo    },
    { "lineTo",                 &GFx_SpriteLineTo    },
    { "curveTo",                &GFx_SpriteCurveTo   },

    { 0, 0 }
};

GASMovieClipProto::GASMovieClipProto(GASStringContext* psc, GASObject* prototype, const GASFunctionRef& constructor) :
    GASPrototype<GASMovieClipObject>(psc, prototype, constructor)
{
    InitFunctionMembers(psc, MovieClipFunctionTable, prototype);
    SetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_useHandCursor), GASValue(true), 
        GASPropFlags::PropFlag_DontDelete | GASPropFlags::PropFlag_DontEnum);
}

void GASMovieClipProto::GlobalCtor(const GASFnCall& fn) 
{
    fn.Result->SetAsObject(GPtr<GASObject>(*new GASMovieClipObject())); //!AB should create sprite too???
}
