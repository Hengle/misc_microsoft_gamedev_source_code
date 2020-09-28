/**********************************************************************

Filename    :   GFxDlist.h
Content     :   Character display list for root and nested clips
Created     :   
Authors     :   Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXDLIST_H
#define INC_GFXDLIST_H

#include "GTLTypes.h"
#include "GFxCharacter.h"
#include "GRenderer.h"

#include "GFxMovieDef.h"


// ***** Declared Classes
class GFxCharPosInfo;
class GFxDisplayList;


// A list of active characters.
class GFxDisplayList : public GNewOverrideBase
{

private:

    friend class GFxSprite;

    // A describing an entry in the display list. A depth-sorted array of these
    // structures is maintained.
    class DisplayEntry
    {
    public:

        enum  FrameFlags
        {           
            // Set if the object was just loaded in this frame. When objects are loaded
            // they are marked here, to prevent them from being immediately advanced.
            // AdvanceFrame will skip JustLoaded children, and clear the flag.
            Flag_JustLoaded         = 0x00000001,
            // An object is marked for removal. Objects that are marked for remove in
            // the last frame of the movie before wrap-around to first frame.
            // are not considered a part of display list, unless 
            Flag_MarkedForRemove    = 0x00000002,
        };


        // Frame flags.
        UInt32              Flags;
        // Pointer to the character.
        GPtr<GFxCharacter>  pCharacter;


        DisplayEntry()          
        {
            Flags = Flag_JustLoaded;
        }

        DisplayEntry(const DisplayEntry& di)            
        {           
            *this= di;
        }

        ~DisplayEntry()
        {
        }

        void    operator=(const DisplayEntry& di)
        {
            Flags       = di.Flags;         
            pCharacter  = di.pCharacter;
        }

        void    SetCharacter(GFxCharacter* pch)
        {
            pCharacter = pch;
        }
        

        // Returns depth of a character, no null checking for speed.
        GINLINE int     GetDepth() const            { return pCharacter->GetDepth(); }

        // Flag manipulation.
        GINLINE bool    IsJustLoaded() const        { return (Flags & Flag_JustLoaded) != 0; }
        GINLINE bool    IsMarkedForRemove() const   { return (Flags & Flag_MarkedForRemove) != 0; }
        // Only advance characters if they are not just loaded and not marked for removal.
        GINLINE bool    CanAdvanceChar() const      { return !(Flags & (Flag_JustLoaded|Flag_MarkedForRemove)); }

        GINLINE void    MarkForRemove(bool remove)
        {
            if (remove) Flags |= Flag_MarkedForRemove;
            else        Flags &= ~Flag_MarkedForRemove;
        }
        GINLINE void    ClearJustLoaded()
        {           
            Flags &= ~Flag_JustLoaded;
        }

        // Comparison function used for insertion and sorting by depth.
        static int      Compare(const void* A, const void* B);
    };

    
    // Display array, sorted by depth.
    GTL::garray<DisplayEntry> DisplayObjectArray;


public:

    // *** Display list management.

    
    // Adds a new character to the display list.
    void        AddDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, bool replaceIfDepthIsOccupied);
    // Moves an existing character in display list at given depth.
    void        MoveDisplayObject(const GFxCharPosInfo &pos);
    // Replaces a character at depth with a new one.
    void        ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch);
    // Remove a character at depth.
    void        RemoveDisplayObject(SInt depth, GFxResourceId id);


    // Clear the display list, unloading all objects.
    void        Clear();

    // Mark all entries for removal, usually done when wrapping from
    // last frame to the next one. Objects that get re-created will
    // survive, other ones will die.
    void        MarkAllEntriesForRemoval();
    // Removes all objects that are marked for removal. Removed objects
    // receiveUnload event.
    void        RemoveMarkedObjects();

    // Helper to remove object at index.
    void        RemoveEntryAtIndex(UInt index);



    // *** Frame processing.


    // Advance referenced characters.
    void        AdvanceFrame(bool nextFrame, Float framePos);

    // Display the referenced characters.
    void        Display(GFxDisplayContext &context, StackData stackData);

    // Calls all onClipEvent handlers due to a mouse event. 
    void        PropagateMouseEvent(const GFxEventId& id);

    // Calls all onClipEvent handlers due to a key event.   
    void        PropagateKeyEvent(const GFxEventId& id, int* pkeyMask);

    
    // *** Character entry access.

    // Finds display array index at depth, or next depth.
    int         FindDisplayIndex(int depth);
    // Like the above, but looks for an exact match, and returns -1 if failed.
    int         GetDisplayIndex(int depth);

    // Swaps two objects at depths.
    // Depth1 must always have a valid object. Depth2 will be inserted, if not existent.
    bool        SwapDepths(int depth1, int depth2);
    // Returns maximum used depth, -1 if display list is empty.
    int         GetLargestDepthInUse() const;


    int             GetCharacterCount() const           { return (UInt)DisplayObjectArray.size(); }
    GFxCharacter*   GetCharacter(int index) const       { return DisplayObjectArray[index].pCharacter.GetPtr(); }
    // Get the display object at the given position.
    DisplayEntry&   GetDisplayObject(UInt idx)          { return DisplayObjectArray[idx]; }

    // May return NULL.
    GFxCharacter*   GetCharacterAtDepth(int depth);

    // Note that only GFxASCharacters have names, so it's ok to return GFxASCharacter.
    // Case sensitivity depends on string context SWF version. May return NULL.
    // If there are multiples, returns the *first* match only!
    GFxASCharacter* GetCharacterByName(GASStringContext* psc, const GASString& name);    

    // Visit all display list members that have a name
    void            VisitMembers(GASObjectInterface::MemberVisitor *pvisitor, UInt visitFlags) const;
};


#endif // INC_GFXDLIST_H
