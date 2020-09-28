/**********************************************************************

Filename    :   GFxDlist.cpp
Content     :   
Created     :   
Authors     :   Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxLog.h"
#include "GFxPlayer.h"

#include "GFxDlist.h"
#include "GFxDisplayContext.h"
#include "GFxAction.h"
#include "GFxFilterDesc.h"


// ***** GFxDisplayList::DisplayEntry implementation


// Comparison used for sorting by depth.
int GFxDisplayList::DisplayEntry::Compare(const void* A, const void* B)
{
    DisplayEntry*   a = (DisplayEntry*) A;
    DisplayEntry*   b = (DisplayEntry*) B;

    if (a->GetDepth() < b->GetDepth())
    {
        return -1;
    }
    else if (a->GetDepth() == b->GetDepth())
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


// ***** GFxDisplayList - Character entry access.


// Find the index in the display list of the first object
// matching the given depth.  Failing that, return the index
// of the first object with a larger depth.
int GFxDisplayList::FindDisplayIndex(int depth)
{
    int size = (int)DisplayObjectArray.size();
    if (size == 0)  
        return 0;   
    
    // Binary search.
    int jump = size >> 1;
    int index = jump;
    for (;;)
    {
        jump >>= 1;
        if (jump < 1) jump = 1;
        
        if (depth > DisplayObjectArray[index].GetDepth())
        {
            if (index == size - 1)
            {
                index = size;
                break;
            }
            index += jump;
        }
        else if (depth < DisplayObjectArray[index].GetDepth())
        {
            if (index == 0
                || depth > DisplayObjectArray[index - 1].GetDepth())
            {
                break;
            }
            index -= jump;
        }
        else
        {
            // match -- scan backward to make sure this is the first entry with this depth.
            //
            // Linear.  But multiple objects with the same depth are only allowed for
            // very old SWF's, so we don't really care.
            for (;;)
            {
                if (index == 0
                    || DisplayObjectArray[index - 1].GetDepth() < depth)
                {
                    break;
                }
                index--;
            }
            GASSERT(DisplayObjectArray[index].GetDepth() == depth);
            GASSERT(index == 0 || DisplayObjectArray[index - 1].GetDepth() < depth);
            break;
        }
    }
    
    GASSERT(index >= 0 && index <= size);
    
    return index;
}


// Like the above, but looks for an exact match, and returns -1 if failed.
int GFxDisplayList::GetDisplayIndex(int depth)
{
    int index = FindDisplayIndex(depth);
    if (index >= (SInt)DisplayObjectArray.size()
        || GetDisplayObject(index).GetDepth() != depth)
    {
        // No object at that depth.
        return -1;
    }
    return index;
}


GFxCharacter*   GFxDisplayList::GetCharacterAtDepth(int depth)
{
    int index = GetDisplayIndex(depth);
    if (index != -1)
    {
        GFxCharacter*   ch = DisplayObjectArray[index].pCharacter.GetPtr();
        if (ch->GetDepth() == depth)
        {
            return ch;
        }
    }

    return NULL;
}


// Swaps two objects at depths.
bool    GFxDisplayList::SwapDepths(int depth1, int depth2)
{
    if (depth1 == depth2)
        return 1;

    int index1 = GetDisplayIndex(depth1);
    if (index1 == -1)
        return 0;
    int index2 = FindDisplayIndex(depth2);

    if (index2 >= (SInt)DisplayObjectArray.size() ||
        GetDisplayObject(index2).GetDepth() != depth2)
    {
        // Index 2 must be used for insertion.
        // Move the object and set depth.
        DisplayEntry de(DisplayObjectArray[index1]);
        DisplayObjectArray.remove(index1);
        if (index1 < index2)
            index2--;
        DisplayObjectArray.insert(index2, de);
    }
    else
    {
        // Otherwise, we are swapping depths with both existing entries.
        GTL::gswap(DisplayObjectArray[index1], DisplayObjectArray[index2]);
        if (DisplayObjectArray[index1].pCharacter)
            DisplayObjectArray[index1].pCharacter->SetDepth(depth1);
    }

    if (DisplayObjectArray[index2].pCharacter)
        DisplayObjectArray[index2].pCharacter->SetDepth(depth2);
    return 1;
}

// Returns maximum used depth.
int     GFxDisplayList::GetLargestDepthInUse() const
{
    // Display array is sorted by size, so get last value
    UPInt size = DisplayObjectArray.size();
    return size ? DisplayObjectArray[size-1].GetDepth() : -1;
}


GFxASCharacter* GFxDisplayList::GetCharacterByName(GASStringContext* psc, const GASString& name)
{
    if (name.IsEmpty())
        return NULL;
    GFxCharacter*  pch = NULL;
    int            i, n = GetCharacterCount();

    // See if we have a match on the display list.
    if (psc->IsCaseSensitive())
    {
        for (i = 0; i < n; i++)
        {
            GFxCharacter* p = GetCharacter(i);
            if (p->GetName() == name)
            {
                pch = p;
                break;
            }
        }
    }
    else
    {   // Case-insensitive version.
        name.ResolveLowercase();
        for (i = 0; i < n; i++)
        {
            GFxCharacter* p = GetCharacter(i);
            if (name.Compare_CaseInsensitive_Resolved(p->GetName()))
            {
                pch = p;
                break;
            }
        }
    }    
    // Only GFxASCharacter's have names.
    GASSERT((pch == 0) || pch->IsASCharacter());
    return (GFxASCharacter*)pch;
}



// ***** GFxDisplayList - Display list management


void    GFxDisplayList::AddDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, bool replaceIfDepthIsOccupied)    
{
    GASSERT(ch);
    
    int depth   = pos.Depth;
    int size    = (int)DisplayObjectArray.size();
    int index   = FindDisplayIndex(depth);

    if (replaceIfDepthIsOccupied)
    {
        // Eliminate an existing object if it's in the way.
        if (index >= 0 && index < size)
        {
            DisplayEntry & dobj = DisplayObjectArray[index];

            if (dobj.GetDepth() == depth)
            {               
                // Remove immediately.
                RemoveEntryAtIndex(index);              
            }
        }
    }
    else
    {
        // Caller wants us to allow multiple objects
        // with the same depth.  FindDisplayIndex()
        // returns the first matching depth, if there
        // are any, so the new GFxCharacter will get
        // inserted before all the others with the
        // same depth.  This matches the semantics
        // described by Alexi's SWF ref.  (This is all
        // for legacy SWF compatibility anyway.)
    }

    ch->SetDepth(depth);

    DisplayEntry di;    

    di.SetCharacter(ch);
    di.pCharacter->SetDepth(depth);
    di.pCharacter->SetCxform(pos.ColorTransform);
    di.pCharacter->SetMatrix(pos.Matrix_1);
    di.pCharacter->SetRatio(pos.Ratio);
    di.pCharacter->SetClipDepth(pos.ClipDepth);
    di.pCharacter->SetBlendMode(pos.BlendMode);

    if (pos.TextFilter)
        di.pCharacter->SetFilters(*pos.TextFilter);

    // Insert into the display list...
    GASSERT(index == FindDisplayIndex(depth));
    
    DisplayObjectArray.insert(index, di);

    // Do the frame1 Actions (if applicable) and the "OnClipEvent (load)" event.
    ch->OnEventLoad();
}


// Updates the transform properties of the object at the specified depth.
void    GFxDisplayList::MoveDisplayObject(const GFxCharPosInfo &pos)
{
    int     depth   = pos.Depth;
    UInt    size    = (UInt)DisplayObjectArray.size();
    int     index   = FindDisplayIndex(depth);

    // Bounds check.
    if (((UInt)index) >= size)
    {       
        if (size <= 0)
            GFC_DEBUG_WARNING(1, "MoveDisplayObject() - no objects on display list");
        else
            GFC_DEBUG_WARNING1(1, "MoveDisplayObject() - can't find object at depth %d", depth);      
        return;
    }
    
    DisplayEntry&   di = DisplayObjectArray[index];
    GFxCharacter*   ch = di.pCharacter.GetPtr();
    if (ch->GetDepth() != depth)
    {       
        GFC_DEBUG_WARNING1(1, "MoveDisplayObject() - no object at depth %d", depth);  
        return;
    }

    // Object re-added, unmark for remove.
    di.MarkForRemove(0);

    if (!ch->GetAcceptAnimMoves())
    {
        // This GFxCharacter is rejecting anim moves.  This happens after it
        // has been manipulated by ActionScript.
        return;
    }

    if (pos.HasCxform)  
        ch->SetCxform(pos.ColorTransform);
    if (pos.HasMatrix)
        ch->SetMatrix(pos.Matrix_1);    
    if (pos.BlendMode != GRenderer::Blend_None)
        ch->SetBlendMode(pos.BlendMode);
    if (pos.TextFilter)
        ch->SetFilters(*pos.TextFilter);

    ch->SetRatio(pos.Ratio);

    // MoveDisplayObject apparently does not change clip depth!  Thanks to Alexeev Vitaly.
    // ch->SetClipDepth(ClipDepth);
}


// Puts a new GFxCharacter at the specified depth, replacing any
// existing GFxCharacter.  If HasCxform or HasMatrix are false,
// then keep those respective properties from the existing character.
void    GFxDisplayList::ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch)   
{   
    UInt    size    = (UInt)DisplayObjectArray.size();
    int     depth   = pos.Depth;
    int     index   = FindDisplayIndex(depth);
    
    // Bounds check.
    if (((UInt)index) >= size)
    {
        // Error, no existing object found at depth.
        // Fallback -- add the object.
        AddDisplayObject(pos, ch, true);
        return;
    }
    
    DisplayEntry&   di = DisplayObjectArray[index];
    if (di.GetDepth() != depth)
    {   
        // This may occur if we are seeking *back* in a movie and find a ReplaceObject tag.
        // In that case, a previous object will not exist, since it would have been removed,
        // if it did.
        AddDisplayObject(pos, ch, true);
        return;
    }
    
    GPtr<GFxCharacter>  OldCh = di.pCharacter;

    // Put the new GFxCharacter in its place.
    GASSERT(ch);
    ch->SetDepth(depth);
    ch->Restart();
    
    // Set the display properties.
    di.MarkForRemove(0);
    di.SetCharacter(ch);

    // Use old characters matrices if new ones not specified.
    ch->SetCxform(pos.HasCxform ? pos.ColorTransform : OldCh->GetCxform());
    ch->SetMatrix(pos.HasMatrix ? pos.Matrix_1 : OldCh->GetMatrix());
    ch->SetBlendMode((pos.BlendMode != GRenderer::Blend_None) ? pos.BlendMode : OldCh->GetBlendMode());
    ch->SetRatio(pos.Ratio);
    ch->SetClipDepth(pos.ClipDepth);    
    if (pos.TextFilter)
        ch->SetFilters(*pos.TextFilter);
}


// Removes the object at the specified depth.
void    GFxDisplayList::RemoveDisplayObject(SInt depth, GFxResourceId id)
{
    UInt    size    = (UInt)DisplayObjectArray.size();    
    int     index   = FindDisplayIndex(depth);
    
    // Validity checks.
    if ( (((UInt)index) >= size) || (GetCharacter(index)->GetDepth() != depth) )
    {
        // Error -- no GFxCharacter at the given depth.
        GFC_DEBUG_WARNING1(1, "RemoveDisplayObject - no GFxCharacter at depth %d", depth);
        return;
    }

    GASSERT(GetCharacter(index)->GetDepth() == depth);
    
    if (id != GFxResourceId::InvalidId)
    {
        // Caller specifies a specific id; scan forward til we find it.
        for (;;)
        {
            if (GetCharacter(index)->GetId() == id)
            {
                break;
            }
            if (index + 1 >= (int)size
                || GetCharacter(index + 1)->GetDepth() != depth)
            {
                // Didn't find a match!
                GFC_DEBUG_WARNING2(1, "RemoveDisplayObject - no GFxCharacter at depth %d with id %d", depth, id.GetIdValue());
                return;
            }
            index++;
        }
        GASSERT((UInt)index < size);
        GASSERT(GetCharacter(index)->GetDepth() == depth);
        GASSERT(GetCharacter(index)->GetId() == id);
    }

    // Removing the GFxCharacter at GetDisplayObject(index).
    RemoveEntryAtIndex(index);
}


// Clear the display list.
void    GFxDisplayList::Clear() 
{
    UPInt i, n = DisplayObjectArray.size();
    for (i = 0; i < n; i++)
    {
        DisplayEntry&   di = DisplayObjectArray[i];     
        if (di.pCharacter)
            di.pCharacter->OnEventUnload();
        di.pCharacter->SetParent(NULL); //!AB: clear the parent for children in the display list 
                                        // being cleared, since their parent used to be removed, 
                                        // but children might stay alive for sometime.
    }   
    DisplayObjectArray.clear();
}


// Mark all entries for removal.
void    GFxDisplayList::MarkAllEntriesForRemoval()  
{   
    UPInt i, n = DisplayObjectArray.size();
    for (i = 0; i < n; i++) 
    {
        // Objects manipulated by action script survive end of frame.
        if (!DisplayObjectArray[i].pCharacter || DisplayObjectArray[i].pCharacter->GetAcceptAnimMoves())
            DisplayObjectArray[i].MarkForRemove(1);
    }
}

// Removes all objects that are marked for removal.
void    GFxDisplayList::RemoveMarkedObjects()   
{   
    // Must be in first-to-last order.
    for (UInt i = 0; i < DisplayObjectArray.size(); i++)
    {
        if (DisplayObjectArray[i].IsMarkedForRemove())
        {
            RemoveEntryAtIndex(i);
            i--;
        }
    }
}

// Helper to remove object at index.
void    GFxDisplayList::RemoveEntryAtIndex(UInt index)
{
    DisplayEntry & dobj = DisplayObjectArray[index];
    if (dobj.pCharacter)    
        dobj.pCharacter->OnEventUnload();
    DisplayObjectArray.remove(index);
}


// *** Frame processing.

// Advance referenced characters.
void    GFxDisplayList::AdvanceFrame(bool nextFrame, Float framePos)
{
    // Objects further on display list get processed first in Flash.
    UPInt    n = DisplayObjectArray.size();      
    
    for (SInt i = (SInt)(n - 1); i >= 0; i--)       
    {
        // @@@@ TODO FIX: If GTL::garray changes size due to
        // GFxCharacter actions, the iteration may not be
        // correct!
        //
        // What's the correct thing to do here?  Options:
        //
        // * copy the display list at the beginning,
        // iterate through the copy
        //
        // * Use (or emulate) a linked list instead of
        // an GTL::garray (still has problems; e.G. what
        // happens if the next or current object gets
        // removed from the dlist?)
        //
        // * iterate through current GTL::garray in depth
        // order.  Always find the next object using a
        // search of current GTL::garray (but optimize the
        // common case where nothing has changed).
        //
        // * ???
        //
        // Need to test to see what Flash does.
        if (n != DisplayObjectArray.size())
        {
            GFC_DEBUG_WARNING(1, "GFxPlayer bug - dlist size changed due to GFxCharacter actions, bailing on update!");
            break;
        }

        DisplayEntry & dobj = DisplayObjectArray[i];


        if (dobj.CanAdvanceChar())
        {
            GFxCharacter*   ch = dobj.pCharacter.GetPtr();
            GASSERT(ch);
            ch->AdvanceFrame(nextFrame, framePos);          
        }
        else
        {
            // Don't advance clips that were just loaded by this frame's tags; must do so
            // to prevent them from immediately ending up in frame 2.
            dobj.ClearJustLoaded();
        }
    }
}


// Display the referenced characters. Lower depths
// are obscured by higher depths.
void    GFxDisplayList::Display(GFxDisplayContext &context, StackData stackData)
{
    UInt        maskCount          = 0;
    int         highestMaskedLayer = 0;
    GRenderer*  prenderer          = context.GetRenderer();
    
  
    for (UPInt i = 0, n = DisplayObjectArray.size(); i < n; i++)
    {
        DisplayEntry&   dobj = DisplayObjectArray[i];
        GFxCharacter*   ch   = dobj.pCharacter.GetPtr();
        GASSERT(ch);

        // Don't display non-visible items, or items, used as a mask (MovieClip.setMask()).
        if (!ch->GetVisible() || ch->IsUsedAsMask() || ch->IsTopmostLevelFlagSet())
            continue;

        // If mask ended before this layer, remove it.
        if (maskCount && (ch->GetDepth() > highestMaskedLayer))
        {
            maskCount--;
            context.PopMask();
        }

        // Check to ensure that mask state is correct.
        if ((context.MaskStackIndexMax > context.MaskStackIndex) && !context.MaskRenderCount)
        {            
            // A different mask was rendered before this one; hence we must restore the mask state.
            // Draw mask with Decrement op.
            context.MaskRenderCount++;   
            while(context.MaskStackIndexMax > context.MaskStackIndex)
            {
                if (context.MaskStackIndexMax <= GFxDisplayContext::Mask_MaxStackSize)
                {
                    prenderer->BeginSubmitMask(GRenderer::Mask_Decrement);
                    context.MaskStack[context.MaskStackIndexMax-1]->Display(context,stackData);
                    prenderer->EndSubmitMask();
                    context.MaskStack[context.MaskStackIndexMax-1] = 0;
                }                
                context.MaskStackIndexMax--;
            }
            context.MaskRenderCount--;
        }

        // Check whether this object should become mask; if so - apply.
        if ((ch->GetClipDepth() > 0) && !context.MaskRenderCount)
        {
            context.PushAndDrawMask(ch,stackData);
           
            highestMaskedLayer = ch->GetClipDepth();
            maskCount++;
        }
        else
        {
            // Rendering of regular shapes (not masks).
            prenderer->PushBlendMode(ch->GetBlendMode());
            ch->Display(context,stackData);
            prenderer->PopBlendMode();
        }
    }
    
    // If a mask masks the scene all the way up to the highest layer, it will not be
    // disabled at the end of drawing the display list, so disable it manually.        
    while (maskCount > 0)
    {        
        context.PopMask();
        maskCount--;
    }
}



// Calls all onClipEvent handlers due to a mouse event. 
void    GFxDisplayList::PropagateMouseEvent(const GFxEventId& id)
{       
    // Objects further on display list get mouse messages first.
    
    for (SInt i = (SInt)(DisplayObjectArray.size() - 1); i >= 0; i--)
    {
        DisplayEntry&   dobj = DisplayObjectArray[i];
        GFxCharacter*   ch   = dobj.pCharacter.GetPtr();
        GASSERT(ch);

        if (!ch->GetVisible())
            continue;

        ch->PropagateMouseEvent(id);

        // Can display list change during an event ? check for bounds.
        // This probably isn't the right behavior, since some of characters might miss
        // the mouse event.
        // Copy pointers first, and process them later ? @TODO
        if (i >= (SInt)DisplayObjectArray.size())
            i = (SInt)DisplayObjectArray.size(); //? reset to the end?
    }
}

// Calls all onClipEvent handlers due to a key event.   
void    GFxDisplayList::PropagateKeyEvent(const GFxEventId& id, int* pkeyMask)
{       
    // Objects at the beginning of display list get key messages first.
    
    for (UPInt i = 0, n = DisplayObjectArray.size(); i < n; ++i)
    {
        DisplayEntry&   dobj = DisplayObjectArray[i];
        GFxCharacter*   ch   = dobj.pCharacter.GetPtr();
        GASSERT(ch);

        if (!ch->GetVisible())  
            continue;

        ch->PropagateKeyEvent(id, pkeyMask);

        // Can display list change during an event ? (answer: yes!) check for recent bounds.
        // This probably isn't the right behavior, since some of characters might miss
        // the key event.
        // Copy pointers first, and process them later ? @TODO.
        if (i + 1 >= DisplayObjectArray.size())
            break;
    }
}

// Visit all display list members that have a name.
void    GFxDisplayList::VisitMembers(GASObjectInterface::MemberVisitor *pvisitor, UInt visitFlags) const
{
    GUNUSED(visitFlags);

    // Enumeration Op lists children in-order.
    for (UInt i = 0; i<DisplayObjectArray.size(); i++)
    {
        const DisplayEntry& dobj = DisplayObjectArray[i];
        GFxCharacter*       ch   = dobj.pCharacter.GetPtr();
        GASSERT(ch);

        // This is used in enumeration op, which returns values by name only.
        if (ch->IsASCharacter() && !ch->GetName().IsEmpty())
            pvisitor->Visit(ch->GetName(), GASValue(ch->ToASCharacter()), 0);
    }
}
