/**********************************************************************

Filename    :   GFxPlayerImpl.h
Content     :   SWF Player Implementation header file.
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   This file contains class declarations used in
                GFxPlayerImpl.cpp only. Declarations that need to be
                visible by other player files should be placed
                in GFxCharacter.h.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXPLAYERIMPL_H
#define INC_GFXPLAYERIMPL_H

#include "GFxCharacter.h"
#include "GMath.h"

#include "GFxButton.h"
#include "GFxDlist.h"
#include "GFxLoaderImpl.h"
#include "GFxFontResource.h"
#include "GFxShape.h"
#include "GFxMovieClipLoader.h"
#include "GFxLoadVars.h"

#include "GContainers.h"

// Font managers are allocated in sprite root nodes.
#include "GFxFontManager.h"

// For now
#include "GFxMovieDef.h"

#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
#include "GFxIMEImm32Dll.h"
#endif //defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)

// ***** Declared Classes
class GFxMovieDefImpl;
class GFxMovieRoot;
class GFxSpriteDef;
class GFxSprite;
// Helpers
class GFxImportInfo;
class GFxSwfEvent;
class GFxLoadQueueEntry;
class GFxDoublePrecisionGuard;
// Tag classes
class GFxPlaceObject2;
class GFxRemoveObject2;
class GFxSetBackgroundColor;
// class GASDoAction;           - in GFxAction.cpp
// class GFxStartSoundTag;      - in GFxSound.cpp


// ***** External Classes
class GFxLoader;

// From "GFxTimers.h"
class GASIntervalTimer;

class GFxIMECandidateListStyle;


// ***** Load Queue used by movie root

class GFxLoadQueueEntry : public GNewOverrideBase
{
    friend class GFxMovieRoot;
    // Next in load queue
    GFxLoadQueueEntry*          pNext;

public:

    // Load type, technically not necessary since URL & Level values are enough.
    // May become useful in the future, as we add other load options.
    enum LoadTypeFlags 
    {
        LTF_None        = 0,
        LTF_UnloadFlag  = 0x01, // Unload instead of load
        LTF_LevelFlag   = 0x02,  // Level instead of target character.
        LTF_VarsFlag    = 0x04
    };  
    enum LoadType
    {   
        LT_LoadMovie    = LTF_None,
        LT_UnloadMovie  = LTF_UnloadFlag,
        LT_LoadLevel    = LTF_LevelFlag,
        LT_UnloadLevel  = LTF_UnloadFlag | LTF_LevelFlag,
    };

    enum LoadMethod
    {
        LM_None,
        LM_Get,
        LM_Post
    };

    LoadType                    Type;
    LoadMethod                  Method;
    GFxString                   URL;
    SInt                        Level;
    GPtr<GFxCharacterHandle>    pCharacter;
    bool                        QueitOpen;
    // movie clip loader and variables loader should be held by GASValue because 
    // there is a cross-reference inside of their instances: loader._listener[0] = loader. 
    // GASValue can release such cross-referenced objects w/o memory leak, whereas GPtr can't.
    GASValue                    MovieClipLoaderHolder;
    GASValue                    LoadVarsHolder;

    // Constructor helper.      
    void    PConstruct(LoadType type, GFxCharacterHandle* pchar, SInt level, const GFxString &url, LoadMethod method, bool queitOpen)
    {
        Type        = type;
        pCharacter  = pchar;
        Method      = method;
        Level       = level;
        pNext       = 0;
        URL         = url;
        QueitOpen   = queitOpen;
    }

    // *** Constructors
    GFxLoadQueueEntry(const GFxString &url, LoadMethod method, bool loadingVars = false, bool queitOpen = false)
    {
        LoadTypeFlags typeFlag;
        if (loadingVars)
        {
            typeFlag = LTF_VarsFlag;
        }
        else
        {
            typeFlag = url ? LTF_None : LTF_UnloadFlag;
        }
        PConstruct((LoadType) (typeFlag), NULL, -1, url, method, queitOpen);
    }

    GFxLoadQueueEntry(GFxCharacterHandle* pchar, const GFxString &url, LoadMethod method, bool loadingVars = false, bool queitOpen = false)
    {
        LoadTypeFlags typeFlag;
        if (loadingVars)
        {
            typeFlag = LTF_VarsFlag;
        }
        else
        {
            typeFlag = url ? LTF_None : LTF_UnloadFlag;
        }
        PConstruct((LoadType) (typeFlag), pchar, -1, url, method, queitOpen);
    }   
    
    GFxLoadQueueEntry(SInt level, const GFxString &url, LoadMethod method, bool loadingVars = false, bool queitOpen = false)
    {
        LoadTypeFlags typeFlag;
        if (loadingVars)
        {
            typeFlag = LTF_VarsFlag;
        }
        else
        {
            typeFlag = url ? LTF_None : LTF_UnloadFlag;
        }
        PConstruct((LoadType) (LTF_LevelFlag | typeFlag), 0, level, url, method, queitOpen);
    }
};

class GFxLoadQueueEntryMT : public GNewOverrideBase
{
protected:
    friend class GFxMovieRoot;
    GFxLoadQueueEntryMT*      pNext;
    GFxLoadQueueEntryMT*      pPrev;

    GFxMovieRoot*             pMovieRoot;
    GFxLoadQueueEntry*        pQueueEntry;

public:
    GFxLoadQueueEntryMT(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    virtual ~GFxLoadQueueEntryMT();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    virtual bool LoadFinished() = 0;
};

// ***** GFxMoviePreloadTask
// The purpose of this task is to find a requested movie on the media, create a moviedefimp and 
// submit a task for the actual movie loading.
class GFxMoviePreloadTask : public GFxTask
{
public:
    GFxMoviePreloadTask(GFxMovieRoot* pmovieRoot, const GFxString& url, bool stripped, bool quietOpen);

    virtual void    Execute();

    GFxMovieDefImpl* GetMoiveDefImpl();

    bool IsDone() const { return Done; }

private:
    GPtr<GFxLoadStates>   pLoadStates;
    UInt                  LoadFlags;
    GFxString             Level0Path;

    GFxString             Url;
    GFxString             UrlStrGfx;
    GPtr<GFxMovieDefImpl> pDefImpl;
    GLock                 DefImplLock;
    volatile bool         Done;
};

//  ****** GFxLoadingMovieEntry
class GFxLoadQueueEntryMT_LoadMovie : public GFxLoadQueueEntryMT
{
    friend class GFxMovieRoot;

    GPtr<GFxMoviePreloadTask> pPreloadTask;
    GPtr<GFxSprite>           pNewChar;
    bool                      FirstCheck;
    GPtr<GFxASCharacter>      pOldChar;      
    GFxResourceId             NewCharId;
    bool                      CharSwitched;
    UInt                      BytesLoaded;
    bool                      FirstFrameLoaded;
public:
    GFxLoadQueueEntryMT_LoadMovie(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    ~GFxLoadQueueEntryMT_LoadMovie();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    bool LoadFinished();
};

// ****** GFxLoadVarsTask
// Reads a file with loadVars data on a separate thread
class GFxLoadVarsTask : public GFxTask
{
public:
    GFxLoadVarsTask(GFxLoadStates* pls, const GFxString& level0Path, const GFxString& url, GFxLoadQueueEntry* pqueueEntry);

    virtual void    Execute();

    // Retrieve loadVars data and file length. Returns false if data is not ready yet.
    // if returned fileLen is less then 0 then the requested url was not read successfully. 
    bool GetData(GFxString* data, SInt* fileLen) const;

private:
    GPtr<GFxLoadStates>  pLoadStates;
    GFxString       Level0Path;
    GFxString       Url;
    GFxLoadQueueEntry* pQueueEntry;

    GFxString       Data;
    SInt            FileLen;
    mutable GLock   DataLock;
    volatile bool   Done;
};

class GFxLoadQueueEntryMT_LoadVars : public GFxLoadQueueEntryMT
{
    GPtr<GFxLoadVarsTask> pTask;
    GPtr<GFxLoadStates>   pLoadStates;

public:
    GFxLoadQueueEntryMT_LoadVars(GFxLoadQueueEntry* pqueueEntry, GFxMovieRoot* pmovieRoot);
    ~GFxLoadQueueEntryMT_LoadVars();

    // Check if a movie is loaded. Returns false in the case if the movie is still being loaded.
    // Returns true if the movie is loaded completely or in the case of errors.
    bool LoadFinished();
};

//  ****** GFxASMouseListener

class GFxASMouseListener
{
public:
    virtual ~GFxASMouseListener() {}

    virtual void OnMouseMove(GASEnvironment *penv) const = 0;
    virtual void OnMouseDown(GASEnvironment *penv, UInt button, GFxASCharacter* ptarget) const = 0;
    virtual void OnMouseUp(GASEnvironment *penv, UInt button, GFxASCharacter* ptarget) const   = 0;
    virtual void OnMouseWheel(GASEnvironment *penv, int sdelta, GFxASCharacter* ptarget) const = 0;

    virtual bool IsEmpty() const = 0;
};


//  ***** GFxMovieDefRootNode

// GFxMovieDefRootNode is maintained in GFxMovieRoot for each GFxMovieDefImpl; this
// node is referenced by every GFxSprite which has its own GFxMovieDefImpl. We keep
// a distinction between imported and loadMovie based root sprites.
// 
// Purpose:
//  1) Allows us to cache temporary state of loading MovieDefs in the beginning of
//     Advance so that it is always consistent for all instances. If not done, it
//     would be possible to different instances of the same progressively loaded
//     file to be in different places in the same GfxMovieView frame.
//  2) Allows for GFxFontManager to be shared among same-def movies.

struct GFxMovieDefRootNode : public GPodDListNode<GFxMovieDefRootNode>, public GNewOverrideBase
{
    // The number of root sprites that are referencing this GFxMovieDef. If this
    // number goes down to 0, the node is deleted.
    UInt        SpriteRefCount;

    // GFxMovieDef these root sprites use. No need to AddRef because GFxSprite does.
    GFxMovieDefImpl* pDefImpl;

    // Cache the number of frames that was loaded.
    UInt        LoadingFrame;
    UInt32      BytesLoaded;
    // Imports don't get to rely on LoadingFrame because they are usually nested
    // and have independent frame count from import root. This LoadingFrame is
    // not necessary anyway because imports are guaranteed to be fully loaded.
    bool        ImportFlag;

    // The pointer to font manager used for a sprite. We need to keep font manager
    // here so that it can keep references to all GFxMovieDefImpl instances for
    // all fonts obtained through GFxFontLib.
    GPtr<GFxFontManager> pFontManager;

    GFxMovieDefRootNode(GFxMovieDefImpl *pdefImpl, bool importFlag = 0)
        : SpriteRefCount(1), pDefImpl(pdefImpl), ImportFlag(importFlag)
    {  }
};


//
// ***** GFxMovieRoot
//
// Global, shared root state for a GFxMovieSub and all its characters.
//

class GFxMovieRoot : public GFxMovieView, public GFxActionPriority
{
public:
    typedef GRenderer::Matrix Matrix;

    struct LevelInfo
    {
        // Level must be >= 0. -1 is used to indicate lack of a level
        // elsewhere, but that is not allowed here.
        SInt            Level;
        GPtr<GFxSprite> pSprite;
    };

    // Sorted array of currently loaded movie levels. Level 0 has
    // special significance, since it dictates viewport scale, etc. 
    GTL::garray<LevelInfo>  MovieLevels;
    // Pointer to sprite in level0 or null. Stored for convenient+efficient access.
    GFxSprite*              pLevel0Movie;
    // Convenience pointer to _level0's Def. Keep this around even if _level0 unloads,
    // to avoid external crashes.
    GPtr<GFxMovieDefImpl>   pLevel0Def;

    // A list of root MovieDefImpl objects used by all root sprites; these objects
    // can be loaded progressively in the background.
    GPodDList<GFxMovieDefRootNode> RootMovieDefNodes;

    // Set once the viewport has been specified explicitly.
    bool                    ViewportSet;
    GViewport               Viewport;
    Float                   PixelScale;
    // View scale values, used to adjust input mouse coordinates
    // map viewport -> movie coordinates.
    Float                   ViewScaleX, ViewScaleY;
    Float                   ViewOffsetX, ViewOffsetY; // in stage pixels (not viewport's ones)
    ScaleModeType           ViewScaleMode;
    AlignType               ViewAlignment;
    GRectF                  VisibleFrameRect; // rect, in swf coords (twips), visible in curr viewport
    Matrix                  ViewportMatrix;
   
        
    GPtr<GFxSharedStateImpl> pSharedState;
    

    // *** States cached in Advance()

    // States are cached then this flag is set.
    bool                        CachedLogFlag;
    mutable GPtr<GFxLog>        pCachedLog;    

    // Handler pointer cached during advance.
    GPtr<GFxUserEventHandler>   pUserEventHandler;
    GPtr<GFxFSCommandHandler>   pFSCommandHandler;
    GPtr<GFxExternalInterface>  pExtIntfHandler;
    
    // Obtains cached states. The mat is only accessed if CachedStatesFlag is not
    // set, which meas we are outside of Advance and Display.
    GFxLog*                 GetCachedLog() const
    {
         // Do not modify CachedLogFlag; that is only for Advance/Display.
        if (!CachedLogFlag)
            pCachedLog = GetLog();
        return pCachedLog;
    }
    

    // Verbosity - assigned from GFxActionControl.
    bool                    VerboseAction;
    bool                    LogRootFilenames;
    bool                    LogChildFilenames;
    bool                    LogLongFilenames;
    bool                    SuppressActionErrors;    

    // Amount of time that has elapsed since start of playback. Used for
    // reporting in action instruction.
    Float                   TimeElapsed;
    // Time remainder from previous advance, updated every frame.
    Float                   TimeRemainder;
    // Cached seconds per frame; i.e., 1.0f / FrameRate.
    Float                   FrameTime;


    GColor                  BackgroundColor;
    int                     MouseX, MouseY;
    enum MouseButtonMask
    {
        MouseButton_Left   = 1,
        MouseButton_Right  = 2,
        MouseButton_Middle = 4,

        MouseButton_AllMask = MouseButton_Left | MouseButton_Right | MouseButton_Middle
    };
    int                     MouseButtons; // use values from MouseButtonMask enum
    bool                    MouseMoved;
    int                     MouseWheelScroll;
    UInt                    MouseCursorType;
    void*                   pPrevTopMostEntityRawPtr; // use only for comparison
    void*                   UserData;
    bool                    MouseSupportEnabled;
    bool                    NeedMouseUpdate;
    GFxMouseButtonState     MouseButtonState;
   
    const GFxASMouseListener*     pASMouseListener; // a listener for AS Mouse class. Only one is necessary.
    bool                    LevelClipsChanged;
    // Set if Advance has not been called yet - generates warning on Display.
    bool                    AdvanceCalled;
    bool                    DirtyFlag;
    bool                    NoInvisibleAdvanceFlag;
    
    // Keyboard
    GPtr<GFxKeyboardState>  pKeyboardState;

    // Global Action Script state
    GPtr<GASGlobalContext>  pGlobalContext;

    // PointTest cache used to optimize shape hit-testing and mouse.
    GFxPointTestCache       PointTestCache;



    // Return value class - allocated after global context.
    // Used because GASString has no global or GNewOverrideBase.
    struct ReturnValueHolder : public GNewOverrideBase
    {
        char*     CharBuffer;
        UInt      CharBufferSize;
        GTL::garray_cc<GASString>   StringArray;
        UInt      StringArrayPos;

        ReturnValueHolder(GASGlobalContext* pmgr)
            : CharBuffer(0), CharBufferSize(0),
              StringArray(pmgr->GetBuiltin(GASBuiltin_empty_)),
              StringArrayPos(0) { }
        ~ReturnValueHolder() { if (CharBuffer) GFREE(CharBuffer); }

        inline char* PreAllocateBuffer(UInt size)
        {
            size = (size + 4095)&(~(4095));
            if (CharBufferSize < size || (CharBufferSize > size && (CharBufferSize - size) > 4096))
            {
                CharBuffer = (char*)GREALLOC(CharBuffer, size);
                CharBufferSize = size;
            }
            return CharBuffer;
        }
        inline void ResetPos() { StringArrayPos = 0; }
        inline void ResizeStringArray(UInt n)
        {
            StringArray.resize(GTL::gmax(1u,n));
        }
    };

    ReturnValueHolder*      pRetValHolder;

    // Return value storage for ExternalInterface.call.
    GASValue                ExternalIntfRetVal;


    // Instance name assignment counter.
    UInt32                  InstanceNameCount;

    
    class DragState
    {
    public:
        GFxASCharacter* pCharacter;
        bool            LockCenter;
        bool            Bound;
        // Bound coordinates
        GPointF         BoundLT;
        GPointF         BoundRB;
        // The difference between character origin and mouse location
        // at the time of dragStart, used and computed if LockCenter == 0.
        GPointF         CenterDelta;

        DragState()
            : pCharacter(0), LockCenter(0), Bound(0), 
              BoundLT(0.0), BoundRB(0.0), CenterDelta(0.0)
            { }

        // Initializes lockCenter and mouse centering delta
        // based on the character.
        void InitCenterDelta(bool lockCenter);
    };

    DragState               CurrentDragState;   // @@ fold this into GFxMouseButtonState?


    // Sticky variable hash link node.
    struct StickyVarNode : public GNewOverrideBase
    {
        GASString       Name;
        GASValue        Value;
        StickyVarNode*  pNext;
        bool            Permanent;

        StickyVarNode(const GASString& name, const GASValue &value, bool permanent)
            : Name(name), Value(value), pNext(0), Permanent(permanent) { }
        StickyVarNode(const StickyVarNode &node)
            : Name(node.Name), Value(node.Value), pNext(node.pNext), Permanent(node.Permanent) { }
        const StickyVarNode& operator = (const StickyVarNode &node)
            { pNext = node.pNext; Name = node.Name; Value = node.Value; Permanent = node.Permanent; return *this; }
    };

    // Sticky variable clip hash table.
    GASStringHash<StickyVarNode*>  StickyVariables;



    // *** Action Script execution

    // Action queue is stored as a singly linked list queue. List nodes must be traversed
    // in order for execution. New actions are inserted at the insert location, which is
    // commonly the end; however, in some cases insert location can be modified to allow
    // insertion of items before other items.


    // Action list to be executed 
    struct ActionEntry : public GNewOverrideBase
    {
        enum EntryType
        {
            Entry_None,
            Entry_Buffer,   // Execute pActionBuffer(pSprite env)
            Entry_Event,        // 
            Entry_Function,
            Entry_CFunction
        };

        ActionEntry*        pNextEntry;
        EntryType           Type;
        GPtr<GFxCharacterHandle> pCharacter;
        GPtr<GASActionBuffer>    pActionBuffer;
        GFxEventId          EventId;
        GASFunctionRef      Function;
        GASCFunctionPtr     CFunction;
        GASValueArray       FunctionParams;

        UInt                SessionId;

        GINLINE ActionEntry();
        GINLINE ActionEntry(const ActionEntry &src);
        GINLINE const ActionEntry& operator = (const ActionEntry &src);

        // Helper constructors
        GINLINE ActionEntry(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer);
        GINLINE ActionEntry(GFxASCharacter *pcharacter, const GFxEventId id);
        GINLINE ActionEntry(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params = 0);
        GINLINE ActionEntry(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params = 0);

        GINLINE void SetAction(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer);
        GINLINE void SetAction(GFxASCharacter *pcharacter, const GFxEventId id);
        GINLINE void SetAction(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params = 0);
        GINLINE void SetAction(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params = 0);
        GINLINE void ClearAction();

        // Executes actions in this entry
        void    Execute(GFxMovieRoot *proot) const;

        bool operator==(const ActionEntry&) const;
    };

  
    struct ActionQueueEntry
    {
        // This is a root of an action list. Root node action is 
        // always Entry_None and thus does not have to be executed.
        ActionEntry*                pActionRoot;

        // This is an action insert location. In a beginning, &ActionRoot, afterwards
        // points to the node after which new actions are added.
        ActionEntry*                pInsertEntry;

        // Pointer to a last entry
        ActionEntry*                pLastEntry;

        ActionQueueEntry() { Reset(); }
        inline void Reset() { pActionRoot = pLastEntry = pInsertEntry = NULL; }
    };
    struct ActionQueueType
    {
        ActionQueueEntry            Entries[AP_Count];
        // This is a modification id to track changes in queue during its execution
        int                         ModId;
        // This is a linked list of un-allocated entries.   
        ActionEntry*                pFreeEntry;
        UInt                        CurrentSessionId;
        UInt                        FreeEntriesCount;
        UInt                        LastSessionId;

        ActionQueueType();
        ~ActionQueueType();

        ActionQueueEntry& operator[](UInt i) { GASSERT(i < AP_Count); return Entries[i]; }
        const ActionQueueEntry& operator[](UInt i) const { GASSERT(i < AP_Count); return Entries[i]; }

        ActionEntry*                InsertEntry(Priority prio);
        void                        AddToFreeList(ActionEntry*);
        void                        Clear();
        ActionEntry*                GetInsertEntry(Priority prio)
        {
            return Entries[prio].pInsertEntry;
        }
        ActionEntry*                SetInsertEntry(Priority prio, ActionEntry* pinsertEntry)
        {
            ActionEntry* pie = Entries[prio].pInsertEntry;
            Entries[prio].pInsertEntry = pinsertEntry;
            return pie;
        }
        void                        FlushOnLoadQueues();
        ActionEntry*                FindEntry(Priority prio, const ActionEntry&);

        UInt                        StartNewSession(UInt* pprevSessionId);
        void                        RestoreSession(UInt sId)  { CurrentSessionId = sId; }
    };
    struct ActionQueueIterator
    {
        int                         ModId;
        ActionQueueType*            pActionQueue;
        ActionEntry*                pLastEntry;
        int                         CurrentPrio;

        ActionQueueIterator(ActionQueueType*);
        ~ActionQueueIterator();

        const ActionEntry*          getNext();
    };
    struct ActionQueueSessionIterator : public ActionQueueIterator
    {
        UInt                        SessionId;

        ActionQueueSessionIterator(ActionQueueType*, UInt sessionId);

        const ActionEntry*          getNext();
    };
    ActionQueueType             ActionQueue;

    GTL::garray<GPtr<GFxCharacter> > TopmostLevelCharacters;

    // Flags for event handlers
    bool                    OnEventXmlsocketOndataCalled;
    bool                    OnEventXmlsocketOnxmlCalled;
    bool                    OnEventLoadProgressCalled;

    // interval timer stuff
    GTL::garray<GASIntervalTimer *> IntervalTimers;
    int                             LastIntervalTimerId;

    // Focus management stuff
    bool                                IsShowingRect;
    mutable GWeakPtr<GFxASCharacter>    LastFocused;
    Bool3W DisableFocusAutoRelease;
    Bool3W AlwaysEnableFocusArrowKeys;
    Bool3W DisableFocusRolloverEvent;

    bool   BackgroundSetByTag;
    bool   MovieIsFocused;

    GFxIMECandidateListStyle*   pIMECandidateListStyle; // stored candidate list style
#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
    GFxIMEImm32Dll      Imm32Dll;
#endif

    // *** Constructor / Destructor

    GFxMovieRoot();
    ~GFxMovieRoot();
    
        
    // Sets a movie at level; used for initialization.
    bool                SetLevelMovie(SInt level, GFxSprite *psprite);
    // Returns a movie at level, or null if no such movie exists.
    bool                ReleaseLevelMovie(SInt level);
    // Returns a movie at level, or null if no such movie exists.
    GFxSprite*          GetLevelMovie(SInt level) const;
    
    // Finds a character given a global path. Path must 
    // include a _levelN entry as first component.
    GFxASCharacter*     FindTarget(const GASString& path) const;

    // Helper: parses _levelN tag and returns the end of it.
    // Returns level index, of -1 if there is no match.
    static SInt         ParseLevelName(const char* pname, const char **ptail, bool caseSensitive);

    
    // Dragging support.
    void                SetDragState(const DragState& st)   { CurrentDragState = st; }
    void                GetDragState(DragState* st)         { *st = CurrentDragState; }
    void                StopDrag()                          { CurrentDragState.pCharacter = NULL; }


    // Internal use in characters, etc.
    // Use this to retrieve the last state of the mouse, as set via
    // NotifyMouseState().  Coordinates are in PIXELS, NOT TWIPS.
    void                GetMouseState(int* x, int* y, int* buttons);

    
    // Return the size of a logical GFxMovieSub pixel as
    // displayed on-screen, with the current device
    // coordinates.
    Float               GetPixelScale() const               { return PixelScale; }

    Float               GetTimer() const                    { return TimeElapsed; }
    Float               GetFrameTime() const                { return FrameTime; }

    
    // Create new instance names for unnamed objects.
    GASString           CreateNewInstanceName();

    // *** Load/Unload movie support
    
    // Head of load queue.
    GFxLoadQueueEntry*  pLoadQueueHead;

    // Adds load queue entry and takes ownership of it.
    void                AddLoadQueueEntry(GFxLoadQueueEntry *pentry);

    // Adds load queue entry based on parsed url and target path.
    void                AddLoadQueueEntry(const char* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None,
                                          GASMovieClipLoader* pmovieClipLoader = NULL);
    void                AddLoadQueueEntry(GFxASCharacter* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None,
                                          GASMovieClipLoader* pmovieClipLoader = NULL);
    // Load queue entries for loading variables
    void                AddVarLoadQueueEntry(const char* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
    void                AddVarLoadQueueEntry(GFxASCharacter* ptarget, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);
    void                AddVarLoadQueueEntry(GASLoadVarsObject* ploadVars, const char* purl,
                                          GFxLoadQueueEntry::LoadMethod method = GFxLoadQueueEntry::LM_None);

    // Processes the load queue handling load/unload instructions.  
    void                ProcessLoadQueue();
    void                ProcessLoadVars(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GFxString& level0Path);
    // called from ProcessLoadVars should be private
    void                LoadVars(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GFxString& data, SInt fileLen);
    // called from LoadVars should be private
    GFxSprite*          CreateEmptySprite(GFxLoadStates* pls, SInt level);

    void                ProcessLoadMovieClip(GFxLoadQueueEntry *pentry, GFxLoadStates* pls, const GFxString& level0Path);

    GFxLoadQueueEntryMT* pLoadQueueMTHead;
    void                AddLoadQueueEntryMT(GFxLoadQueueEntry *pqueueEntry);
    void                AddMovieLoadQueueEntry(GFxLoadQueueEntry* pentry);


    // *** Helpers for loading images.

    typedef GFxLoader::FileFormatType FileFormatType;
        
    // Create a loaded image MovieDef based on image resource.
    // If load states are specified, they are used for bind states. Otherwise,
    // new states are created based on pSharedState and our loader.
    GFxMovieDefImpl*  CreateImageMovieDef(GFxImageResource *pimageResource, bool bilinear,
                                          const char *purl, GFxLoadStates *pls = 0);

    // Fills in a file system path relative to _level0. The path
    // will contain a trailing '/' if not empty, so that relative
    // paths can be concatenated directly to it.
    // Returns 1 if the path is non-empty.
    bool                GetLevel0Path(GFxString *ppath) const;
    

    // *** Action List management

    // Take care of this frame's actions, by executing ActionList. 
    void                DoActions();
    // Execute only actions from the certain session; all other actions are staying in the queue
    void                DoActionsForSession(UInt sessionId); 

    // Inserts an empty action and returns a pointer to it. Should call SetAction afterwards.   
    ActionEntry*        InsertEmptyAction(Priority prio = AP_Frame) { return ActionQueue.InsertEntry(prio); }
    bool                HasActionEntry(const ActionEntry& entry,Priority prio = AP_Frame)       
        { return ActionQueue.FindEntry(prio, entry) != NULL; }

   
    // *** GFxMovieView implementation

    virtual void        SetViewport(const GViewport& viewDesc);
    virtual void        GetViewport(GViewport *pviewDesc) const;
    virtual void        SetViewScaleMode(ScaleModeType);
    virtual ScaleModeType   GetViewScaleMode() const                        { return ViewScaleMode; }
    virtual void        SetViewAlignment(AlignType);
    virtual AlignType   GetViewAlignment() const                            { return ViewAlignment; }
    virtual GRectF      GetVisibleFrameRect() const                         { return TwipsToPixels(VisibleFrameRect); }
    const GRectF&       GetVisibleFrameRectInTwips() const                  { return VisibleFrameRect; }

    void                UpdateViewport();


    virtual void        SetVerboseAction(bool verboseAction)                { VerboseAction = verboseAction;    }
    // Turn off/on log for ActionScript errors..
    virtual void        SetActionErrorsSuppress(bool suppressActionErrors)  { SuppressActionErrors = suppressActionErrors; }
    // Background color.
    virtual void        SetBackgroundColor(const GColor color)      { BackgroundColor = color; }
#ifdef GFC_OS_PS2
    virtual void        SetBackgroundAlpha(Float alpha)             { BackgroundColor.SetAlpha( GTL::gclamp<UInt>((UInt)(alpha*255.0f), 0, 255) ); }
#else
    virtual void        SetBackgroundAlpha(Float alpha)             { BackgroundColor.SetAlpha( GTL::gclamp<UByte>((UByte)(alpha*255.0f), 0, 255) ); }
#endif
    virtual Float       GetBackgroundAlpha() const                  { return BackgroundColor.GetAlpha() / 255.0f; }

    bool                IsBackgroundSetByTag() const                { return BackgroundSetByTag; }
    void                SetBackgroundColorByTag(const GColor color) { SetBackgroundColor(color); BackgroundSetByTag = true; }



    // Actual execution and timeline control.
    virtual Float       Advance(Float deltaT, UInt frameCatchUpCount);

    // Events.
    virtual UInt        HandleEvent(const GFxEvent &event);
    virtual void        NotifyMouseState(int x, int y, int buttons);
    virtual bool        HitTest(int x, int y, HitTestType testCond = HitTest_Shapes);
    virtual bool        EnableMouseSupport(bool mouseEnabled);

    /*
    virtual void        SetUserEventHandler(UserEventCallback phandler, void* puserData) 
    { 
        pUserEventHandler = phandler; 
        pUserEventHandlerData = puserData;
    }

    // FSCommand
    virtual void        SetFSCommandCallback (FSCommandCallback phandler)   { pFSCommandCallback = phandler; }
    */

    virtual void*       GetUserData() const                                 { return UserData; }
    virtual void        SetUserData(void* ud)                               { UserData = ud;  }

    virtual bool        AttachDisplayCallback(const char* pathToObject, void (*callback)(void* userPtr), void* userPtr);

    virtual void        SetExternalInterfaceRetVal(const GFxValue&);

    // returns timerId to use with ClearIntervalTimer
    int                 AddIntervalTimer(GASIntervalTimer *timer);  
    void                ClearIntervalTimer(int timerId);      
    void                ShutdownTimers();


    // *** GFxMovie implementation

    // Many of these methods delegate to pMovie methods of the same name; however,
    // they are hidden into the .cpp file because GFxSprite is not yet defined.
    
    virtual GFxMovieDef* GetMovieDef() const;
    
    virtual UInt        GetCurrentFrame() const;
    virtual bool        HasLooped() const;  
    virtual void        Restart();  
    virtual void        GotoFrame(UInt targetFrameNumber);
    virtual bool        GotoLabeledFrame(const char* label, SInt offset = 0);

    virtual void        Display();

    virtual void        SetPlayState(PlayState s);
    virtual PlayState   GetPlayState() const;
    virtual void        SetVisible(bool visible);
    virtual bool        GetVisible() const;
    // Action Script access
    virtual bool        SetVariable(const char* ppathToVar, const GFxValue& value, SetVarType setType = SV_Sticky);
    virtual bool        GetVariable(GFxValue *pval, const char* ppathToVar) const;
    virtual bool        SetVariableArray(SetArrayType type, const char* ppathToVar,
                                         UInt index, const void* pdata, UInt count, SetVarType setType = SV_Sticky);
    virtual bool        SetVariableArraySize(const char* ppathToVar, UInt count, SetVarType setType = SV_Sticky);
    virtual UInt        GetVariableArraySize(const char* ppathToVar);
    virtual bool        GetVariableArray(SetArrayType type, const char* ppathToVar,
                                         UInt index, void* pdata, UInt count);

    //virtual bool        SetVariable(const char* pathToVar, const char* newValue, SetVarType setType = SV_Sticky);
    //virtual bool        SetVariable(const char* pathToVar, const wchar_t* newValue, SetVarType setType = SV_Sticky);
    //virtual bool        SetVariableDouble(const char* ppathToVar, Double value, SetVarType setType = SV_Sticky);

    //virtual bool        SetVariableArrayDouble(const char* ppathToVar, const Double*, UInt count, SetVarType setType = SV_Sticky);
    //virtual const char* GetVariable(const char* pathToVar) const;
    //virtual Double      GetVariableDouble(const char* ppathToVar) const;

    virtual bool        IsAvailable(const char* ppathToVar) const;
    
    virtual bool        Invoke(const char* pmethodName, GFxValue *presult, const GFxValue* pargs, UInt numArgs);
    virtual bool        Invoke(const char* pmethodName, GFxValue *presult, const char* pargFmt, ...);
    virtual bool        InvokeArgs(const char* pmethodName, GFxValue *presult, const char* pargFmt, va_list args);
/*
    virtual const char* Invoke(const char* methodName, const char* methodArgFmt, ...);
    virtual const char* InvokeArgs(const char* methodName, const char* methodArgFmt, va_list args);
    virtual const char* InvokeArgs(const char* methodName, const char* methodArgFmt, 
                                   const void* const* methodArgs, int numArgs);*/

    // Implementation used by SetVariable/GetVariable.
    //bool                SetVariable(const char* pathToVar, const GASValue &val, SetVarType setType);
    void                GFxValue2ASValue(const GFxValue& gfxVal, GASValue* pdestVal) const;
    void                ASValue2GFxValue(GASEnvironment* penv, const GASValue& value, GFxValue* pdestVal) const;

    void                AddStickyVariable(const GASString& fullPath, const GASValue &val, SetVarType setType);
    void                ResolveStickyVariables(GFxASCharacter *pcharacter);
    void                ClearStickyVariables();

    GFxASCharacter*     GetTopMostEntity(const GPointF& mousePos, bool testAll, const GFxASCharacter* ignoreMC = NULL);

    // ***** GFxSharedState implementation
    
    virtual GFxSharedState* GetSharedImpl() const   { return pSharedState.GetPtr(); }

    // Mark/unmark the movie as one that has "focus". This is user responsibility to set or clear
    // the movie focus. This is important for stuff like IME functions correctly.
    void                OnMovieFocus(bool set);
    virtual bool        IsMovieFocused() const { return MovieIsFocused; }

    // Returns and optionally resets the "dirty flag" that indicates 
    // whether anything was changed on the stage (and need to be 
    // re-rendered) or not.
    virtual bool        GetDirtyFlag(bool doReset = true);
    void                SetDirtyFlag() { DirtyFlag = true; }

    void                SetNoInvisibleAdvanceFlag(bool f) { NoInvisibleAdvanceFlag = f; }
    void                ClearNoInvisibleAdvanceFlag()     { SetNoInvisibleAdvanceFlag(false); }
    bool                IsNoInvisibleAdvanceFlagSet() const { return NoInvisibleAdvanceFlag; }

    // Focus related functionality
protected:
    struct ProcessFocusKeyInfo
    {
        GPtr<GFxASCharacter>                CurFocused;
        int                                 CurFocusIdx;
        GTL::garray<GPtr<GFxASCharacter> >  TabableArray;
        bool                                TabIndexed;
        bool                                Initialized;

        ProcessFocusKeyInfo():Initialized(false) {}
    };
    // Focus-related methods
    // Process keyboard input for focus
    void                ProcessFocusKey(short keycode, GFxEvent::EventType event, 
                                        const GFxSpecialKeysState& specialKeysState, ProcessFocusKeyInfo* pfocusInfo);
    void                FinalizeProcessFocusKey(ProcessFocusKeyInfo* pfocusInfo);
public:
    // Displays yellow rectangle around focused item
    void                DisplayFocusRect(const GFxDisplayContext& context);
    // Hides yellow focus rectangle. This may happen if mouse moved.
    void                HideFocusRect();
    // Returns the character currently with focus
    GPtr<GFxASCharacter> GetFocusedCharacter() { return GPtr<GFxASCharacter>(LastFocused); }
    // Checks, is the specified item focused or not?
    bool                IsFocused(const GFxASCharacter* ch) const { GPtr<GFxASCharacter> lch = LastFocused; return lch.GetPtr() == ch; }
    // Checks, is the specified item focused or not?
    bool                IsKeyboardFocused(const GFxASCharacter* ch) const { return (IsFocused(ch) && IsFocusRectShown()); }
    // Transfers focus to specified item. For movie clips and buttons the keyboard focus will not be set
    // to specified character, as well as focus rect will not be drawn.
    void                SetFocusTo(GFxASCharacter*);
    
    // Queue up setting of focus. ptopMostCh is might be necessary for stuff like IME to 
    // determine its state; might be NULL, if not available.
    void                QueueSetFocusTo
        (GFxASCharacter* ch, GFxASCharacter* ptopMostCh, GFxFocusMovedType fmt = GFx_FocusMovedByKeyboard);
    // Instantly transfers focus to specified item. For movie clips and buttons the keyboard focus will not be set
    // Instantly invokes pOldFocus->OnKillFocus, pNewFocus->OnSetFocus and Selection.bradcastMessage("onSetFocus").
    void                TransferFocus(GFxASCharacter* pNewFocus, GFxFocusMovedType fmt = GFx_FocusMovedByKeyboard);
    // Transfers focus to specified item. The keyboard focus transfered as well, as well as focus rect 
    // will be drawn (unless it is disabled).
    void                SetKeyboardFocusTo(GFxASCharacter*);
    // Returns true, if yellow focus rect CAN be shown at the time of call. This method will
    // return true, even if _focusrect = false and it is a time to show the rectangle.
    inline bool         IsFocusRectShown() const { return IsShowingRect; }

    void                ActivateFocusCapture();

    void                AddTopmostLevelCharacter(GFxCharacter*);
    void                RemoveTopmostLevelCharacter(GFxCharacter*);
    void                DisplayTopmostLevelCharacters(GFxDisplayContext &context, StackData stackdata) const;

    // Sets style of candidate list. Invokes OnCandidateListStyleChanged callback.
    void                SetIMECandidateListStyle(const GFxIMECandidateListStyle& st);
    // Gets style of candidate list
    void                GetIMECandidateListStyle(GFxIMECandidateListStyle* pst) const;

#if defined(GFC_OS_WIN32) && !defined(GFC_NO_BUILTIN_KOREAN_IME) && !defined(GFC_NO_IME_SUPPORT)
    // handle korean IME (core part)
    UInt                HandleKoreanIME(const GFxIMEEvent& imeEvent);
#endif // GFC_NO_BUILTIN_KOREAN_IME
};




// ** Inline Implementation


GINLINE GFxMovieRoot::ActionEntry::ActionEntry()
{ 
    pNextEntry = 0;
    Type = Entry_None; pActionBuffer = 0;
    SessionId = 0;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(const GFxMovieRoot::ActionEntry &src)
{
    pNextEntry      = src.pNextEntry;
    Type            = src.Type;
    pCharacter      = src.pCharacter;
    pActionBuffer   = src.pActionBuffer;
    EventId         = src.EventId;
    Function        = src.Function;
    CFunction       = src.CFunction;
    FunctionParams  = src.FunctionParams;
    SessionId       = src.SessionId;
}

GINLINE const GFxMovieRoot::ActionEntry& 
GFxMovieRoot::ActionEntry::operator = (const GFxMovieRoot::ActionEntry &src)
{           
    pNextEntry      = src.pNextEntry;
    Type            = src.Type;
    pCharacter      = src.pCharacter;
    pActionBuffer   = src.pActionBuffer;
    EventId         = src.EventId;
    Function        = src.Function;
    CFunction       = src.CFunction;
    FunctionParams  = src.FunctionParams;
    SessionId       = src.SessionId;
    return *this;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer)
{
    pNextEntry      = 0;
    Type            = Entry_Buffer;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = pbuffer;
    SessionId       = 0;
}
    
GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, const GFxEventId id)
{
    pNextEntry      = 0;
    Type            = Entry_Event;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = 0;
    EventId         = id;
    SessionId       = 0;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params)
{
    pNextEntry      = 0;
    Type            = Entry_Function;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = 0;
    Function        = function;
    if (params)
        FunctionParams = *params;
    SessionId       = 0;
}

GINLINE GFxMovieRoot::ActionEntry::ActionEntry(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params)
{
    pNextEntry      = 0;
    Type            = Entry_CFunction;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = 0;
    CFunction        = function;
    if (params)
        FunctionParams = *params;
    SessionId       = 0;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, GASActionBuffer* pbuffer)
{
    Type            = Entry_Buffer;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = pbuffer;
    EventId.Id      = GFxEventId::Event_Invalid;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, const GFxEventId id)
{
    Type            = Entry_Event;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = 0;
    EventId         = id;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, const GASFunctionRef& function, const GASValueArray* params)
{
    Type            = Entry_Function;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = 0;
    Function        = function;
    if (params)
        FunctionParams = *params;
}

GINLINE void GFxMovieRoot::ActionEntry::SetAction(GFxASCharacter *pcharacter, const GASCFunctionPtr function, const GASValueArray* params)
{
    Type            = Entry_CFunction;
    pCharacter      = pcharacter->GetCharacterHandle();
    pActionBuffer   = 0;
    CFunction       = function;
    if (params)
        FunctionParams = *params;
}

GINLINE void GFxMovieRoot::ActionEntry::ClearAction()
{
    Type            = Entry_None;
    pActionBuffer   = 0;
    pCharacter      = 0;
    Function.DropRefs();
    FunctionParams.resize(0);
}

GINLINE bool GFxMovieRoot::ActionEntry::operator==(const GFxMovieRoot::ActionEntry& e) const
{
    return Type == e.Type && pActionBuffer == e.pActionBuffer && pCharacter == e.pCharacter &&
           CFunction == e.CFunction && Function == e.Function && EventId == e.EventId;
}

// ** End Inline Implementation


//!AB: This class restores high precision mode of FPU for X86 CPUs.
// Direct3D may set the Mantissa Precision Control Bits to 24-bit (by default 53-bits) and this 
// leads to bad precision of FP arithmetic. For example, the result of 0.0123456789 + 1.0 will 
// be 1.0123456789 with 53-bit mantissa mode and 1.012345671653 with 24-bit mode.
class GFxDoublePrecisionGuard
{
    unsigned    fpc;
    //short       fpc;
public:

    GFxDoublePrecisionGuard ()
    {
#if defined (GFC_CC_MSVC) && defined(GFC_CPU_X86)
  #if (GFC_CC_MSVC >= 1400)
        // save precision mode (control word)
        _controlfp_s(&fpc, 0, 0);
        // set 64 bit precision
        unsigned _fpc;
        _controlfp_s(&_fpc, _PC_64, _MCW_PC);
  #else
        // save precision mode (control word)
        fpc = _controlfp(0,0);
        // set 64 bit precision
        _controlfp(_PC_64, _MCW_PC);
  #endif
        /*
        // save precision mode (only for X86)
        GASM fstcw fpc;
        short _fpc = (fpc & ~0x300) | 0x300;  // 64-bit mantissa (REAL10)
        //short _fpc = (fpc & ~0x300) | 0x200;  // 53-bit mantissa (REAL8)
        // set 64 bit precision
        GASM fldcw _fpc;
        */
#endif
    }

    ~GFxDoublePrecisionGuard ()
    {
#if defined (GFC_CC_MSVC) && defined (GFC_CPU_X86)
  #if (GFC_CC_MSVC >= 1400)
        // restore precision mode
        unsigned _fpc;
        _controlfp_s(&_fpc, fpc, _MCW_PC);
  #else
        _controlfp(fpc, _MCW_PC);
  #endif
        /*
        // restore precision mode (only for X86)
        GASM fldcw fpc;
        */
#endif
    }
};

#endif // INC_GFXIMPL_H
