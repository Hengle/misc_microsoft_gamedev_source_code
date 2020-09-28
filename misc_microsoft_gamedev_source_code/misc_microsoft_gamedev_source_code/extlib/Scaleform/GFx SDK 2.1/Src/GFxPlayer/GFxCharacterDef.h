/**********************************************************************

Filename    :   GFxCharacter.h
Content     :   Defines abstract base character and playlist
                data classes.
Created     :   
Authors     :   Michael Antonov, TU

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   More implementation-specific classes are
                defined in the GFxPlayerImpl.h

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXCHARACTERDEF_H
#define INC_GFXCHARACTERDEF_H

#include "GRefCount.h"
#include "GTypes2DF.h"
#include "GMatrix2D.h"

#include "GFxResource.h"


#include "GRenderer.h"
// ***** Declared Classes

class GFxCharacterDef;
class GFxTimelineDef;


// ***** External Classes

class GFxCharacter;
class GFxASCharacter;
class GASExecuteTag;
class GFxDisplayContext;
class GFxPointTestCacheProvider;
class GFxMovieDataDef;
class GFxMovieDefImpl;
class GFxLog;


// ***** GFxCharacterDef

// A GFxCharacterDef is the immutable data representing the template of a movie element.
// This is not a public interface.  It's here so it can be mixed into MovieDefinition,
// MovieDefinitionSub, and GFxSpriteDef, without using multiple inheritance.
class GFxCharacterDef : public GFxResource
{
    GFxResourceId        Id;
public:

    // Identifiers for code-defined characters (ok since Flash chars are <= 0xFFFF).
    // These can be passed directly to GetCharDef.
    enum {
        CharId_EmptyMovieClip               = GFxResourceId::IdType_InternalConstant | 1,
        CharId_EmptyTextField               = GFxResourceId::IdType_InternalConstant | 2,

        // These IDs are defined for image file loaded MovieDefs.
        // The constant are with
        CharId_ImageMovieDef_ImageResource  = 0,
        CharId_ImageMovieDef_ShapeDef       = 1
    };

    GFxCharacterDef()
        : Id(GFxResourceId(GFxResourceId::InvalidId))
    {  }

    virtual ~GFxCharacterDef() { }

    virtual void        Display(GFxDisplayContext &context, GFxCharacter* pinstanceInfo, StackData stackData);

    // Determine if the specified point falls into a shape.
    virtual bool        DefPointTestLocal(const GPointF &pt, bool testShape = 0,
                                          const GFxCharacter *pinst = 0) const 
    { GUNUSED3(pt, testShape, pinst); return false; }  

    // Obtains character bounds in local coordinate space.
    virtual GRectF      GetBoundsLocal() const
    { return GRectF(0); }   

    virtual GRectF      GetRectBoundsLocal() const
    { return GRectF(0); }   
       
    virtual UInt        GetVersion() const
    { GASSERT(0); return 0; }

    // Creates a new instance of a character, ownership is passed to creator (character is
    // not automatically added to parent as a child; that is responsibility of the caller).
    // Default behavior is to create a GenericCharacter.
    //  - pbindingImpl argument provides the GFxMovieDefImpl object with binding tables
    //    necessary for the character instance to function. Since in the new implementation
    //    loaded data is separated from instances, GFxCharacterDef will not know what
    //    what GFxMovieDefImpl it is associated with, so this data must be passed.
    //    This value is significant when creating GFxSprite objects taken from an import,
    //    since their GFxMovieDefImpl is different from pparent->GetResourceMovieDef().
    virtual GFxCharacter*       CreateCharacterInstance(GFxASCharacter* pparent, GFxResourceId rid,
                                                        GFxMovieDefImpl *pbindingImpl);

    
    // Id access - necessary for attachMobie
    // MA TBD: attachMovie should obtain an Id in some other way, such as
    // export table traversal and lookup.
    GFxResourceId               GetId() const           { return Id; }
    void                        SetId(GFxResourceId id) { Id = id; }    
};



// *****  GFxTimelineDef - Character with frame Playlist support

class GFxTimelineDef : public GFxCharacterDef
{
public:

  
    struct Frame
    {
        // NOTE: Tag and tag array memory is responsible for
        //
        GASExecuteTag** pTagPtrList;
        UInt            TagCount;

        Frame()
            : pTagPtrList(0), TagCount(0) { }       
        Frame(const Frame &src)
            : pTagPtrList(src.pTagPtrList), TagCount(src.TagCount) { }

        // Frames are returned by value because playlist array
        // can be reallocated on a loading thread in GFxMovieDataDef.
        Frame& operator = (const Frame &src)
        { pTagPtrList = src.pTagPtrList; TagCount = src.TagCount; return *this; }

        // Calls destructors on all of the tag objects.
        void    DestroyTags();

        UInt           GetTagCount() const      { return TagCount; }
        GASExecuteTag* GetTag(UInt index) const { return pTagPtrList[index]; }
    };


    // *** Playlist Access
    
    virtual UInt                GetFrameCount() const                                   = 0;

    virtual const Frame         GetPlaylist(int frameNumber) const                      = 0;
    virtual bool                GetInitActions(Frame* pframe, int frameNumber) const    = 0;
    
    virtual bool                GetLabeledFrame(const char* label, UInt* frameNumber, 
                                                bool translateNumbers = 1)              = 0;
    
};


// TimelineDef with loading support. This is a base class for GFxSpriteDef; however,
// it is used to avoid the inclusion of GFxSprite class in GFxLoadProcess.h.
class GFxTimelineIODef : public GFxTimelineDef
{
public:

    // *** Playlist loading

    // Adds frames to list. Frame tag / tag pointer list ownership is passed to TimelineDef.
    // TimeDef is responsible for destructing tags, however, memory belongs to GFxMovieDataDef.
    virtual void                SetLoadingPlaylistFrame(const Frame& frame)             = 0;
    virtual void                AddFrameName(const char* name, GFxLog *plog)            = 0;

    // Not supported by GFxSpriteDef and thus not included here.
    // virtual void                SetLoadingInitActionFrame(const Frame& frame) = 0;
};



#endif // INC_GFXCHARACTERDEF_H
