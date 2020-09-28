/**********************************************************************

Filename    :   GFxSprite.h
Content     :   Sprite Implementation header file.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxSprite.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSPRITE_H
#define INC_GFXSPRITE_H

//#include "GFxPlayerImpl.h"

#include "GFxCharacter.h"
#include "GFxDlist.h"

// For now
#include "GFxMovieDef.h"

#include "GFxAction.h"
#include "GFxObjectProto.h"
// For drawing API container.
#include "GFxDrawingContext.h"


// ***** Declared Classes
class GFxSpriteDef;
class GFxSprite;
class GASMovieClipObject;
class GASMovieClipProto;

// Forward decl - from GFxPlayerImpl.h
class GFxMovieRoot;



//
// GFxSpriteDef
//

// A sprite is a mini movie-within-a-movie.  It doesn't define
// its own characters; it uses the characters from the parent
// GFxMovieDefImpl, but it has its own frame counter, display list, etc.
//
// The sprite implementation is divided into a
// GFxSpriteDef and a GFxSprite.  The Definition
// holds the immutable data for a sprite, while the Instance
// contains the state for a specific instance being updated
// and displayed in the parent GFxASCharacter's display list.

class GFxSpriteDef : public GFxTimelineDef
{
    // MovieDef to which this sprite belongs.
    GFxMovieDataDef*        pMovieDef;

	GFxStringHash<UInt>     NamedFrames;    // stores 0-based frame #'s
    int                     FrameCount;
    int                     LoadingFrame;

    // GFxSprite control events for each frame.
    GTL::garray<Frame>      Playlist;

public:
    GFxSpriteDef(GFxMovieDataDef* pmd);

    ~GFxSpriteDef();
    
    
    // overloads from GFxMovieDef
    virtual Float           GetWidth() const                            { return 1; }
    virtual Float           GetHeight() const                           { return 1; }
    virtual bool            PointTestLocal(const GPointF &pt, bool testShape = 0, const GFxPointTestCacheProvider *pcache = 0) const;
    //virtual GRectF        GetBoundsLocal() const                                      { return GRectF(0); }   
    virtual UInt            GetFrameCount() const                       { return FrameCount; }
    virtual Float           GetFrameRate() const                        { return pMovieDef->GetFrameRate(); }
    virtual GRectF          GetFrameRect() const                        { return GRectF(0,0,1,1); }
    virtual UInt            GetLoadingFrame() const                     { return LoadingFrame; }
    virtual UInt            GetVersion() const                          { return pMovieDef->GetVersion(); }
    virtual UInt            GetSWFFlags() const                         { return pMovieDef->GetSWFFlags(); }
    virtual UInt            GetTagCount() const                         { return 0; }
    virtual UInt32          GetFileBytes() const                        { return pMovieDef->GetFileBytes(); }
    virtual const char*     GetFileURL() const                          { return pMovieDef->GetFileURL(); }
    // Log access
 //   virtual void            SetLog(GFxLog *plog)                        { GUNUSED(plog); GASSERT(0); }
 //   virtual GFxLog*         GetLog() const                              { return pMovieDef->GetLog(); }
 //   virtual GFxFontLib*     GetFontLib() const                          { return pMovieDef->GetFontLib(); }
    virtual GFxResourceId   GetNextGradientId()                         { return pMovieDef->GetNextGradientId(); }


    // Obtain MovieDataDef to which this resource belongs (will belong).
    virtual GFxMovieDataDef*    GetMovieDataDef() const                 { return pMovieDef; }
        
    // Should only be called on root
    virtual GImageInfoBase* CreateImageInfo(const GFxImageCreateInfo*)  { GASSERT(0); return 0;}


    virtual void            AddCharacter(GFxResourceId id, GFxCharacterDef* ch)   { GFC_DEBUG_WARNING(1, "AddCharacter tag appears in sprite tags\n"); GUNUSED2(id, ch); }
    virtual void            AddFont(GFxResourceId id, GFxFontResource* ch)                { GFC_DEBUG_WARNING(1, "AddFont tag appears in sprite tags\n"); GUNUSED2(id, ch); }


    // @@ would be nicer to not inherit these...
    virtual UInt            GetLoadFlags() const                        { GASSERT(0); return GFxLoader::LoadAll; }  
    virtual UInt            GetImageInfoCount() const                   { GASSERT(0); return 0; }
    virtual GImageInfoBase* GetImageInfo(UInt i, const char **pexportName) const 
        { GUNUSED(i); GASSERT(0); if (pexportName) *pexportName = 0; return NULL; }
    virtual GImageInfoBase* GetImageInfo(const char *pexportName) const { GUNUSED(pexportName); GASSERT(0); return NULL; }
    virtual void            AddImageInfo(GImageInfoBase* pi)            { GUNUSED(pi); GASSERT(0); }

  

    virtual GFxMovieView*   CreateInstance(bool initFirstFrame = 0)
        { GUNUSED(initFirstFrame); return NULL; }

    // Overloads from GFxCharacterDef. 
    virtual GFxCharacter*   CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
                                                    GFxMovieDefImpl *pbindingImpl);

    
    virtual void    SetLoadingPlaylistFrame(const Frame& frame)
    {
        GASSERT(Playlist[LoadingFrame].TagCount == 0);
        Playlist[LoadingFrame] = frame;
    }
    
    virtual void    SetLoadingInitActionFrame(const Frame& frame)
    {
        GUNUSED(frame);
        // Sprite def's should not have DoInitAction tags in them!
        GFC_DEBUG_WARNING(1, "GFxSpriteDef::SetLoadingInitActionFrame called! Ignored.\n");
    }


    // Labels the frame currently being loaded with the
    // given name.  A copy of the name string is made and
    // kept in this object.
    virtual void    AddFrameName(const char* name, GFxLog *plog);
        
    // Public access functions for GFxMovieDef.
    virtual UInt    GetFileAttributes() const                       { GASSERT(0); return 0; }   
    virtual UInt    GetMetadata(char *pbuff, UInt buffSize) const   { GASSERT(0); GUNUSED2(pbuff,buffSize); return 0; }
    // Sprites cannot have their own metadata / attributes?
    virtual void    SetMetadata(UByte *pdata, UInt size)            { GASSERT(0); GUNUSED2(pdata,size); }
    virtual void    SetFileAttributes(UInt attrs)                   { GASSERT(0); GUNUSED(attrs); }

    virtual void    SetExporterInfo(const GFxExporterInfoImpl& src) { GASSERT(0); GUNUSED(src); }
    virtual const GFxExporterInfo*  GetExporterInfo() const         { GASSERT(0); return 0; }

    virtual bool    GetImageFileInfo(GFxImageFileInfo *pinfo, int characterId)  { GASSERT(0); GUNUSED2(pinfo, characterId); return 0; }
   
    
    // Returns 0-based frame #  
    bool                    GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return GFxMovieDataDef::TranslateFrameString(NamedFrames, label, frameNumber, translateNumbers);        
    }

    // FrameNumber is 0-based
    const Frame&  GetPlaylist(int frameNumber) const    
    {
        return Playlist[frameNumber];
    }
    
    virtual const Frame*  GetInitActions(int frameNumber) const
    {
        GUNUSED(frameNumber);
        // Sprites do not have init actions in their
        // playlists!  Only the root GFxSprite
        // (GFxMovieDefImpl) Does (@@ correct?)
        return NULL;
    }
    
    /*
    virtual const GTL::garray<GFxMovieDataDef*>* GetImportSourceMoviesForFrame(UInt frame) const
    {
        return pMovieDef->GetImportSourceMoviesForFrame(frame);
    }
    */

    void            Read(GFxLoadProcess* p, GFxResourceId charId);

    // Initialize an empty clip.
    void            InitEmptyClipDef();

    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt            GetResourceTypeCode() const     { return MakeTypeCode(RT_SpriteDef); }

};

//
// GFxSprite
//

class GFxSprite : public GFxASCharacter
{
    typedef GFxMovie::PlayState PlayState;

    typedef GFxTimelineDef::Frame Frame;
    // Data source for playlist (either GFxSpriteDef of GFxMovieDataDef).
    GPtr<GFxTimelineDef>    pDef;
    GFxMovieRoot*           pRoot;

    GFxDisplayList      DisplayList;    

    PlayState           PlayStatePriv;
    UInt                CurrentFrame;
    GTL::garray<bool>   InitActionsExecuted;    // a bit-GTL::garray class would be ideal for this

    GASEnvironment      ASEnvironment;


    // Used only for root-level sprites; otherwise -1
    SInt                Level;

    // A node created for every root sprite in a tree so that they can be
    // scanned and initialized efficiently to a thread-synched Advance.
    // Null for all non-root sprites.
    struct GFxSpriteRootNode* pRootNode;
    
    // AS object
    GPtr<GASMovieClipObject> ASMovieClipObj;

    GPtr<GFxDrawingContext>  pDrawingAPI;

    GPtr<GFxSprite>          MaskCharacter;

    enum
    {
        Flags_UpdateFrame       = 0x1,
        Flags_HasLoopedPriv     = 0x2,
        Flags_Unloaded          = 0x4, // Set once Unload has been called.
        Flags_OnEventLoadCalled = 0x8,

        // This flag is set if sprite was loaded with loadMovie.
        // If so, _lockroot applies.
        Flags_LoadedSeparately  = 0x10,
        Flags_LockRoot          = 0x20,

        // Is the movie used as a mask for setMask method?
        Flags_UsedAsMask        = 0x40
    };
    UByte               Flags;

    // focus related stuff
    Bool3W              FocusEnabled;
    Bool3W              TabChildren;

    void SetUpdateFrame(bool v = true)      { Flags = (UByte)((v) ? (Flags | Flags_UpdateFrame)     : (Flags & (~Flags_UpdateFrame))); }
    void SetHasLoopedPriv(bool v = true)    { Flags = (UByte)((v) ? (Flags | Flags_HasLoopedPriv)   : (Flags & (~Flags_HasLoopedPriv))); }
    void SetUnloaded(bool v = true)         { Flags = (UByte)((v) ? (Flags | Flags_Unloaded)        : (Flags & (~Flags_Unloaded))); }
    void SetOnEventLoadCalled(bool v = true){ Flags = (UByte)((v) ? (Flags | Flags_OnEventLoadCalled)   : (Flags & (~Flags_OnEventLoadCalled))); }
    void SetLockRoot(bool v = true)         { Flags = (UByte)((v) ? (Flags | Flags_LockRoot)        : (Flags & (~Flags_LockRoot))); }
    void SetLoadedSeparately(bool v = true) { Flags = (UByte)((v) ? (Flags | Flags_LoadedSeparately): (Flags & (~Flags_LoadedSeparately))); }
    void SetUsedAsMask(bool v = true)       { Flags = (UByte)((v) ? (Flags | Flags_UsedAsMask)      : (Flags & (~Flags_UsedAsMask))); }

    bool IsUpdateFrame() const              { return (Flags & Flags_UpdateFrame) != 0; }
    bool IsOnEventLoadCalled() const        { return (Flags & Flags_OnEventLoadCalled) != 0; }
    bool IsLockRoot() const                 { return (Flags & Flags_LockRoot) != 0; }
    bool IsLoadedSeparately() const         { return (Flags & Flags_LoadedSeparately) != 0; }

    void                CloneInternalData(const GFxASCharacter* src);

    // used by both GetMember & GetMemberRaw to share common part of code
    bool                GetMember(GASEnvironment* penv, GASStringContext *psc, const GASString& name, GASValue* pval);
public: 

    enum MouseState
    {
        UP = 0,
        DOWN,
        OVER
    };
    MouseState          MouseStatePriv;

public:
    // Both pdef (play-list) and pdefImpl (binding) pointers must be provided.
    GFxSprite(GFxTimelineDef* pdef, GFxMovieDefImpl* pdefImpl,
              GFxMovieRoot* pr, GFxASCharacter* parent,
              GFxResourceId id, bool loadedSeparately = false);

    virtual ~GFxSprite();

    void            ForceShutdown ();
    
    void            ClearDisplayList();


    virtual void    OnIntervalTimer(void *timer);


    
    // These accessor methods have custom implementation in GFxSprite.
    virtual GFxCharacterDef*    GetCharacterDef() const        { return pDef.GetPtr(); }
    virtual GFxMovieDefImpl*    GetResourceMovieDef() const;    
    virtual GFxFontManager*     GetFontManager() const;
    virtual GFxMovieRoot*       GetMovieRoot() const;
    virtual GFxASCharacter*     GetLevelMovie(SInt level) const;
    virtual GFxASCharacter*     GetASRootMovie() const;

    GASEnvironment*             GetASEnvironment()          { return &ASEnvironment; }
    const GASEnvironment*       GetASEnvironment() const    { return &ASEnvironment; }
    GASObject*                  GetASObject ();
    GASObject*                  GetASObject () const;
    GINLINE GASMovieClipObject* GetMovieClipObject ()   { return ASMovieClipObj; }

    bool            IsUnloaded() const      { return (Flags & Flags_Unloaded) != 0; }    

    // Bounds computation.
    // "transform" matrix describes the transform applied to parent and us,
    // including the object's matrix itself. This means that if transform is
    // identity, GetBoundsTransformed will return local bounds and NOT parent bounds.
    virtual GRectF  GetBounds(const Matrix &transform) const;       


    UInt            GetCurrentFrame() const { return CurrentFrame; }
    UInt            GetFrameCount() const   { return pDef->GetFrameCount(); } 
    UInt32          GetBytesLoaded() const;
    UInt            GetLoadingFrame() const;

    // Level access.
    inline SInt     GetLevel() const        { return Level; }
    void            SetLevel(SInt level);
    inline bool     IsLevelsMovie() const   { return Level >= 0; }

    // Checks that load was called and does so if necessary.
    // Used for root-level sprites.
    void            ExecuteFrame0Events();




    // Returns 0-based frame #
    bool            GetLabeledFrame(const char* label, UInt* frameNumber, bool translateNumbers = 1)
    {
        return pDef->GetLabeledFrame(label, frameNumber, translateNumbers);
    }


    // Stop or play the sprite.
    void            SetPlayState(PlayState s)   { PlayStatePriv = s; }
    PlayState       GetPlayState() const        { return PlayStatePriv; }


    void            Restart();
    


    virtual bool    HasLooped() const { return (Flags & Flags_HasLoopedPriv) != 0; }

    // Combine the flags to avoid a conditional. It would be faster with a macro.
    GINLINE int      Transition(int a, int b) const
    {
        return (a << 2) | b;
    }

    // Return the topmost entity that the given point covers.  NULL if none.
    // Coords are in parent's frame.    
    virtual GFxASCharacter* GetTopMostMouseEntity(const GPointF &pt, bool testAll = false);   
    
    // Increment CurrentFrame, and take care of looping.
    void            IncrementFrameAndCheckForLoop();
    

    virtual void    AdvanceFrame(bool nextFrame, Float framePos);
    
    
    // Execute the tags associated with the specified frame.
    // noRemove is specified if Removal tags are not to be executed (causing warning). Useful
    // for executing tags in the target frame after a backwards seek.
    // frame is 0-based
    void            ExecuteFrameTags(UInt frame, bool stateOnly = false, bool noRemove = false,
                                     GFxActionPriority::Priority prio = GFxActionPriority::AP_Frame);
    // Executes init action tags only for the frame.
    void            ExecuteInitActionFrameTags(UInt frame);


    // Execute the tags associated with the specified frame, IN REVERSE.
    // I.E. if it's an "add" tag, then we do a "remove" instead.
    // Only relevant to the display-list manipulation tags: add, move, remove, replace.
    //
    // frame is 0-based
    void            ExecuteFrameTagsReverse(UInt frame);

    // Find placement tag and compute placement info for an object at specified depth.
    // Takes current frame, and modifies it to be the frame in which the tag was found.
    GFxPlaceObject2* FindPreviousPlaceObjectInfo(GFxCharPosInfo *ppos, UInt *pframe, int depth, GFxResourceId id, UInt findPrevFlags);
    // Helper, finds prev tag and updates frame.
    GFxPlaceObject2* FindPreviousPlaceObject2(UInt *pframe, int depth, GFxResourceId id);
    
    // Execute any remove-object tags associated with the specified frame.
    // frame is 0-based
    void            ExecuteRemoveTags(UInt frame);
    

    // Set the sprite state at the specified frame number.
    // 0-based frame numbers!!  (in contrast to ActionScript and Flash MX)
    void            GotoFrame(UInt targetFrameNumber);



    // Look up the labeled frame, and jump to it.
    bool            GotoLabeledFrame(const char* label, SInt offset = 0);
            
    void            Display(GFxDisplayContext &context, StackData stackData);



    // *** Display List management.
    
    // Find previous flag options.  
    enum {
        FindPrev_AllowMove  = 1,    // Allow move tags to be found. If not specified, we will look for add/replace only.
        FindPrev_NeedMatrix = 2,    // Must find a valid prev matrix (because it was replaced in next tag).
        FindPrev_NeedCXForm = 4,    // Must find a valid cxform.
        FindPrev_NeedBlend  = 8,    // Must find a previous blend mode.
    };

    // Add an object to the display list.
    // CreateFrame specifies frame used as 'CreateFrame' for the new object (def: ~0 == CurrentFrame).
    GFxCharacter*   AddDisplayObject(
                        const GFxCharPosInfo &pos, const GASString& name,
                        const GTL::garray<GFxSwfEvent*>& eventHandlers,
                        const GASObjectInterface *pinitSource,
                        bool replaceIfDepthIsOccupied, UInt createFrame = GFC_MAX_UINT,
                        bool placeObject = false,
                        GFxCharacterCreateInfo* pcharCreateOverride = 0,
                        GFxASCharacter* origChar = 0);

    // Updates the transform properties of the object at the specified depth.   
    void            MoveDisplayObject(const GFxCharPosInfo &pos);
    void            ReplaceDisplayObject(const GFxCharPosInfo &pos, const GASString& name);
    void            ReplaceDisplayObject(const GFxCharPosInfo &pos, GFxCharacter* ch, const GASString& name);
        
    // Remove the object at the specified depth.
    // If id != -1, then only remove the object at depth with matching id.
    void            RemoveDisplayObject(SInt depth, GFxResourceId id);
    // Remove *this* object from its parent.
    void            RemoveDisplayObject() {GFxASCharacter::RemoveDisplayObject(); }
    

    // Movie Loading support
    virtual bool        ReplaceChildCharacterOnLoad(GFxASCharacter *poldChar, GFxASCharacter *pnewChar);
    
        
    // For debugging -- return the id of the GFxCharacter at the specified depth.
    // Return 0 if nobody's home.
    GFxCharacter*   GetCharacterAtDepth(int depth);
    // Return -1 if nobody's home.
    GFxResourceId   GetIdAtDepth(int depth);

    

    void            ExecuteImportedInitActions(GFxMovieDef* psourceMovie);

    // Add the given action buffer to the list of action
    // buffers to be processed at the end of the next frame advance in root.    
    void            AddActionBuffer(GASActionBuffer* a,
                                    GFxActionPriority::Priority prio = GFxActionPriority::AP_Frame);


    // Need to access depths, etc.
    static void         SpriteSwapDepths(const GASFnCall& fn);

    // Returns maximum used depth, -1 if no depth is available.
    SInt        GetLargestDepthInUse() const { return DisplayList.GetLargestDepthInUse();}


    //
    // *** Button logc support
    // 

    // Returns 1 if sprite acts as a button due to handlers.
    bool            ActsAsButton() const;


    //
    // *** ActionScript support
    //
    
    // GFxASCharacter override to indicate which standard members are handled by us.
    virtual UInt32      GetStandardMemberBitMask() const;

    // Handles built-in members. Return 0 if member is not found or not supported.
    virtual bool            SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag);
    virtual bool            GetStandardMember(StandardMember member, GASValue* val, bool opcodeFlag) const;

    // GASObjectInterface implementation.
    virtual bool        SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool        GetMember(GASEnvironment* penv, const GASString& name, GASValue* pval)
    {
        return GetMember(penv, NULL, name, pval);
    }
    virtual bool        DeleteMember(GASStringContext *psc, const GASString& name);
    virtual void        VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags = 0) const;
    virtual ObjectType  GetObjectType() const { return Object_Sprite; }
    virtual bool        SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    virtual bool        GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* pval)
    {
        return GetMember(NULL, psc, name, pval);
    }
    
    virtual GFxASCharacter* GetRelativeTarget(const GASString& name);

    // Execute the actions for the specified frame.  The
    // FrameSpec could be an integer or a string.
    virtual void        CallFrameActions(const GASValue& FrameSpec);


    // Override to check handlers for an id.
    // Propagates an event to all children.
    virtual void    PropagateMouseEvent(const GFxEventId& id);
    virtual void    PropagateKeyEvent(const GFxEventId& id, int* pkeyMask);
    
    // Dispatch event Handler(s), if any.
    virtual bool    OnEvent(const GFxEventId& id);
    // Special event handler; ensures that unload is called on child items in timely manner.
    virtual void    OnEventUnload();
    // Dispatch key event
    virtual bool    OnKeyEvent(const GFxEventId& id, int* pkeyMask);

    // Execute action buffer
    virtual bool    ExecuteBuffer(GASActionBuffer* pactionbuffer);
    // Execute this even immediately (called for processing queued event actions).
    virtual bool    ExecuteEvent(const GFxEventId& id);


    // Handle a button event.
    virtual bool    OnButtonEvent(const GFxEventId& id);

    // Do the events That (appear to) happen as the GFxASCharacter
    // loads.  frame1 tags and actions are Executed (even
    // before Advance() is called).  Then the onLoad event
    // is triggered.
    virtual void    OnEventLoad();

    // Do the events that happen when there is XML data waiting
    // on the XML socket connection.
    virtual void    OnEventXmlsocketOnxml();    
    // Do the events That (appear to) happen on a specified interval.
    virtual void    OnEventIntervalTimer(); 
    // Do the events that happen as a MovieClip (swf 7 only) loads.
    virtual void    OnEventLoadProgress();

    bool            Invoke(const char* methodName, GASValue* presult, UInt numArgs);
    bool            InvokeArgs(const char* methodName, GASValue *presult, const char* methodArgFmt, va_list args);     
    //virtual const char* InvokeArgs(const char* methodName, const char* methodArgFmt, const void* const* methodArgs, int numArgs);

    void SetHasButtonHandlers (bool has);

    virtual bool    PointTestLocal(const GPointF &pt, bool testShape = 0) const;

    void CalcDisplayListHitTestMaskArray(GTL::garray<UByte> *phitTest, const GPointF &p, bool testShape) const;

    void NotifyMemberChanged (const GASString& name, const GASValue& val);

    // focus stuff
    virtual bool            IsTabable() const;
    virtual void            FillTabableArray(GTL::garray<GPtr<GFxASCharacter> >& array, bool* tabIndexed);
    // reports focusEnabled property state (used for Selection.setFocus())
    virtual bool            IsFocusEnabled() const;
    // invoked when item is going to get focus (Selection.setFocus is invoked, or TAB is pressed)
    virtual void            OnGettingKeyboardFocus();
    // invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
    virtual void            OnLosingKeyboardFocus();

    // notify the sprite to be inserted as a _levelN movie
    virtual void            OnInsertionAsLevel(int level);

    UInt                    GetCursorType() const;

    bool                    IsUsedAsMask() const { return (Flags & Flags_UsedAsMask) != 0; }
    void                    SetMask(GFxSprite* ch);
    GFxSprite*              GetMask() const { return MaskCharacter; }

    //
    // *** Drawing API support
    //
    void Clear();
    void SetNoFill();
    void SetNoLine();
    void SetLineStyle(Float lineWidth, 
                      UInt  rgba = 0, 
                      bool  hinting = false, 
                      GFxLineStyle::LineStyle scaling = GFxLineStyle::LineScaling_Normal, 
                      GFxLineStyle::LineStyle caps    = GFxLineStyle::LineCap_Round,
                      GFxLineStyle::LineStyle joins   = GFxLineStyle::LineJoin_Round,
                      Float miterLimit = 3.0f);
    GFxFillStyle* BeginFill();
    GFxFillStyle* CreateLineComplexFill();
    void BeginFill(UInt rgba);    
    void BeginBitmapFill(GFxFillType fillType,
                         GFxImageResource* pimageRes, 
                         const Matrix& mtx);
    void EndFill();
    void MoveTo(Float x, Float y);
    void LineTo(Float x, Float y);
    void CurveTo(Float cx, Float cy, Float ax, Float ay);
    bool AcquirePath(bool newShapeFlag);
};



//!AB should be separated later.
class GASMovieClipObject : public GASObject
{
    friend class GASMovieClipProto;

    GWeakPtr<GFxSprite> pSprite;    // weak ref on sprite obj

    void commonInit();
protected:
    // If any of these variables are non-zero, sprite acts as a button,
    // catching all of buttonAction logic and disabling nested buttons
    // or similar clip actions.

    // Set if the sprite has any button event handlers.
    bool                HasButtonHandlers;
    // Count of dynamic handlers installed by SetMember: onPress, onRollOver, etc.
    UByte               DynButtonHandlerCount;  

    // Updates DynButtonHandlerCount if name is begin added through SetMember,
    // or deleted 
    void            TrackMemberButtonHandler(GASStringContext* psc, const GASString& name, bool deleteFlag = 0);

    GASMovieClipObject(GASStringContext* psc = 0) : GASObject() { GUNUSED(psc); commonInit(); }
public:
    GASMovieClipObject(GASGlobalContext* gCtxt, GFxSprite* psprite) : pSprite(psprite)
    {
        commonInit ();
        GASStringContext* psc = psprite->GetASEnvironment()->GetSC();
        Set__proto__ (psc, gCtxt->GetPrototype(GASBuiltin_MovieClip));
    }

    // Set the named member to the value.  Return true if we have
    // that member; false otherwise.
    virtual bool        SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags());
    //virtual bool      GetMember(GASEnvironment* penv, const GASString& name, GASValue* val);
    virtual bool        DeleteMember(GASStringContext *psc, const GASString& name);

    virtual void        Set__proto__(GASStringContext *psc, GASObject* protoObj);

    // Returns 1 if sprite acts as a button due to handlers.
    bool                ActsAsButton() const;

    GINLINE void        SetHasButtonHandlers(bool has)      { HasButtonHandlers=has; }

    virtual ObjectType  GetObjectType() const   { return Object_MovieClipObject; }

    GPtr<GFxSprite>     GetSprite() { return pSprite; }

    void                ForceShutdown();

    virtual GFxASCharacter*         GetASCharacter() { return GPtr<GFxSprite>(pSprite).GetPtr(); }
};

class GASMovieClipProto : public GASPrototype<GASMovieClipObject>
{
public:
    GASMovieClipProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);

};

#endif // INC_GFXSPRITE_H
