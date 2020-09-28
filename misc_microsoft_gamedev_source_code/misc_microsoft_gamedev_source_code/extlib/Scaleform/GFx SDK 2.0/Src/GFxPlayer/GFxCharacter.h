/**********************************************************************

Filename    :   GFxCharacter.h
Content     :   Defines base functionality for characters, which are
                display list objects.
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

#ifndef INC_GFXCHARACTER_H
#define INC_GFXCHARACTER_H


#include "GTLTypes.h"
#include "GRefCount.h"
#include "GDebug.h"

#include "GRenderer.h"

#include "GFxLog.h"
#include "GFxPlayer.h"
//#include "GFxLoader.h"
//#include "GFxAction.h"

#include "GFxObject.h"

#include "GTypes2DF.h"
// For GFxPointTestCache
#include "GCompoundShape.h"

//#include <stdarg.h>

#include "GFxCharacterDef.h"


// ***** Declared Classes
class GFxCharacter;
class GFxASCharacter;
class GFxGenericCharacter;

class GFxPointTestCacheProvider;

class GFxSoundSample;


// ***** External Classes

class GFxMovieRoot;
class GFxMovieDataDef;
class GFxMovieDefImpl;
class GASEnvironment;
struct GFxTextFilter;

//class GFxRenderConfig;




// *****  GFxCharacter - Active object or shape on stage


// GFxCharacter is a live, active state instance of a GFxCharacterDef.
// It represents a single active element in a display list and thus includes
// the physical properties necessary for all objects on stage.
// However, it does not include the properties necessary for scripting
// or external access - that is the job of GFxASCharacter.

class GFxCharacter : public GRefCountBase<GFxCharacter>, public GFxLogBase<GFxCharacter>
{
public:

    // Public typedefs
    typedef GRenderer::Matrix       Matrix;
    typedef GRenderer::Cxform       Cxform;
    typedef GRenderer::BlendType    BlendType;

    typedef GASObjectInterface::ObjectType ObjectType;

protected:

    // These are physical properties of all display list entries.
    // Properties that belong only to scriptable objects, such as _name are in GFxASCharacter.
    GFxResourceId           Id;
    int                     Depth;
    UInt                    CreateFrame;
    Float                   Ratio;
    UInt16                  ClipDepth;
    union   {
        GFxASCharacter*         pParent;        // Parent is always GFxASCharacter.     
        GFxCharacter*           pParentChar;    // So that we can access it directly.
    };

    Cxform                  ColorTransform;
    Matrix                  Matrix_1;   

public:

    // Constructor
    GFxCharacter(GFxASCharacter* pparent, GFxResourceId id);
    ~GFxCharacter();
    

    // *** Accessors for physical display info

    GFxResourceId           GetId() const                       { return Id; }
    GFxASCharacter*         GetParent() const                   { return pParent; }
    void                    SetParent(GFxASCharacter* parent)   { pParent = parent; }  // for extern GFxMovieSub
    int                     GetDepth() const                    { return Depth; }
    void                    SetDepth(int d)                     { Depth = d; }  
    UInt                    GetCreateFrame() const              { return CreateFrame;}
    void                    SetCreateFrame(UInt frame)          { CreateFrame = frame; }
    const Matrix&           GetMatrix() const                   { return Matrix_1; }
    void                    SetMatrix(const Matrix& m)          { GASSERT(m.IsValid()); Matrix_1 = m;}
    const Cxform&           GetCxform() const                   { return ColorTransform; }
    void                    SetCxform(const Cxform& cx)         { ColorTransform = cx; }
    void                    ConcatenateCxform(const Cxform& cx) { ColorTransform.Concatenate(cx); }
    void                    ConcatenateMatrix(const Matrix& m)  { Matrix_1.Prepend(m); }
    Float                   GetRatio() const                    { return Ratio; }
    void                    SetRatio(Float f)                   { Ratio = f; }
    UInt16                  GetClipDepth() const                { return ClipDepth; }
    void                    SetClipDepth(UInt16 d)              { ClipDepth = d; }

    // Implemented only in GASCharacter. Return None/empty string by default on other characters.
    virtual const GASString& GetName() const;   //              { static GFxString Name; return Name; }     
    virtual bool            GetVisible() const                  { return true; }
    virtual BlendType       GetBlendMode() const                { return GRenderer::Blend_None; }   
    virtual bool            GetAcceptAnimMoves() const          { return true;  }
    // If any of below ASSERTs is hit, it is either because (1) their states are being set by tags
    // and therefore should be moved from GFxASCharacter here, or (2) because there is a bug
    // in external logic.
    virtual void            SetAcceptAnimMoves(bool accept)     { GUNUSED(accept); GASSERT(0); }
    virtual void            SetBlendMode(BlendType blend)       { GUNUSED(blend);  GASSERT(blend == GRenderer::Blend_None); }
    virtual void            SetFilters(const GFxTextFilter&)    { }
    // Name should not be necessary for text; however, it was seen for static text - needs research.
    virtual void            SetName(const GASString& name)
        { GFC_DEBUG_WARNING1(1, "GFxCharacter::SetName('%s') called on a non-scriptable character", name.ToCStr()); GUNUSED(name); }



    // *** Inherited transform access.

    // Get our concatenated matrix (all our ancestor transforms, times our matrix).
    // Maps from our local space into "world" space (i.e. root movie clip space).
    void                    GetWorldMatrix(Matrix *pmat) const;
    void                    GetWorldCxform(Cxform *pcxform) const;
    void                    GetLevelMatrix(Matrix *pmat) const;
    // Convenience versions.
    Matrix                  GetWorldMatrix() const              { Matrix m; GetWorldMatrix(&m); return m; }
    Cxform                  GetWorldCxform() const              { Cxform m; GetWorldCxform(&m); return m; }
    Matrix                  GetLevelMatrix() const              { Matrix m; GetLevelMatrix(&m); return m; }
    
    // Temporary - used until blending logic is improved.
    GRenderer::BlendType    GetActiveBlendMode() const;


    // *** Geometry query methods, applicable to all characters.

    // Return character bounds in specified coordinate space.
    virtual GRectF          GetBounds(const Matrix &t) const    { GUNUSED(t); return GRectF(0); }
    // Return transformed bounds of character in root movie space.
    GINLINE GRectF          GetWorldBounds() const              { return GetBounds(GetWorldMatrix()); }
    GINLINE GRectF          GetLevelBounds() const              { return GetBounds(GetLevelMatrix()); }

    // Hit-testing of shape/sprite.
    virtual bool            PointTestLocal(const GPointF &pt, bool testShape = 0) const 
        { GUNUSED2(pt, testShape); return false; }      
    
    // Will aways find action scriptable objects (parent is returned for regular shapes).
    virtual GFxASCharacter *GetTopMostMouseEntity(const GPointF &pt, bool testAll = false) { GUNUSED2(pt, testAll); return NULL; }



    // *** Resource/Asset Accessors
    
    // Returns the absolute root object, which corresponds to movie view and contains levels.
    virtual GFxMovieRoot*   GetMovieRoot() const                { return pParentChar->GetMovieRoot(); }

    // Obtains character definition relying on us. Must be overridden.
    virtual GFxCharacterDef* GetCharacterDef() const            = 0;

    // Returns SWF nested movie definition from which we access movie resources.
    // This definition may have been imported or loaded with 'loadMovie', so
    // it can be a nested movie (it does not need to correspond to root).
    virtual GFxMovieDefImpl* GetResourceMovieDef() const        { return pParentChar->GetResourceMovieDef(); }

    // Obtain the font manager. Font managers exist in sprites with their own GFxMovieDefImpl.
    virtual class GFxFontManager* GetFontManager() const        { return pParentChar->GetFontManager(); }

    // Returns a movie for a certain level.
    virtual GFxASCharacter* GetLevelMovie(SInt level) const     { return pParentChar->GetLevelMovie(level); }

    // Returns _root movie, which may or many not be the same as _level0.
    // The meaning of _root may change if _lockroot is set on a clip into which
    // a nested movie was loaded.
    virtual GFxASCharacter* GetASRootMovie() const              { return pParentChar->GetASRootMovie(); }

    // Helper that gets movie-resource relative versions.
    // Necessary because version of SWF execution depends on loadMovie nesting.
    UInt                    GetVersion() const;
    GINLINE bool            IsCaseSensitive() const             { return GetVersion() > 6; }

    // ASEnvironment - sometimes necessary for strings.
    virtual GASEnvironment* GetASEnvironment();
    virtual const GASEnvironment* GetASEnvironment() const;

    
    // *** GFxDisplayList required methods  

    // Displays the character onto the renderer, called from display list.
    virtual void            Display(GFxDisplayContext&, StackData stackData)  { }
    // Called from DisplayList on ReplaceDisplayObject.
    virtual void            Restart()                               { }
        
    // Advance Frame / "tick" functionality.
    // Set nextFrame = 1 if the frame is advanced by one step, 0 otherwise.
    // framePos must be in range [0,1) indicating where in a frame we are;
    // that could be useful for some animation optimizations.
    virtual void            AdvanceFrame(bool nextFrame, Float framePos = 0.0f) 
        { GUNUSED2(nextFrame, framePos); }

    // ActionScript event handler.  Returns true if a handler was called.
    virtual bool            OnEvent(const GFxEventId& id) { GUNUSED(id); return false; }
    // Special event handler; sprites also execute their frame1 actions on this event.
    virtual void            OnEventLoad()                           { OnEvent(GFxEventId::Event_Load); }
    // Special event handler; ensures that unload is called on child items in timely manner.
    virtual void            OnEventUnload()                         { OnEvent(GFxEventId::Event_Unload); }
    // Special event handler; key down and up
    // See also PropagateKeyEvent, KeyMask
    virtual bool            OnKeyEvent(const GFxEventId& id, int* pkeyMask)     
        { GUNUSED(pkeyMask); return OnEvent(id); }
    // Special event handler; char
    virtual bool            OnCharEvent(UInt32 wcharCode)     
        { GUNUSED(wcharCode); return false; }
    // Special event handler; mouse wheel support
    virtual bool            OnMouseWheelEvent(int mwDelta)
        { GUNUSED(mwDelta); return false; }

    // Propagates mouse event to all of the eligible clips.
    // Called to notify handlers of onClipEvent(mouseMove, mouseDown, mouseUp)
    virtual void            PropagateMouseEvent(const GFxEventId& id)   
        { OnEvent(id); }

    // Propagates key event to all of the eligible clips.
    // Called to notify handlers of onClipEvent(keyDown, keyUp), onKeyDown/onKeyUp, on(keyPress)
    enum KeyMask {
        KeyMask_KeyPress = 1,
        KeyMask_onKeyDown = 2,
        KeyMask_onKeyUp = 4,
        KeyMask_onClipEvent_keyDown = 8,
        KeyMask_onClipEvent_keyUp = 16,
        KeyMask_FocusedItemHandled = 0x20 // should be set, if focused item already handled the keyevent
    };
    virtual void            PropagateKeyEvent(const GFxEventId& id, int* pkeyMask)
    { 
        OnKeyEvent(id, pkeyMask); 
    }


    // *** Log support.

    // Implement logging through the root delegation.
    // (needs to be implemented in .cpp, so that GFxMovieRoot is visible)
    virtual GFxLog*         GetLog() const;
    virtual bool            IsVerboseAction() const;
    virtual bool            IsVerboseActionErrors() const;

    // Dynamic casting support - necessary to support ToASCharacter.
    virtual ObjectType      GetObjectType() const   { return GASObjectInterface::Object_BaseCharacter; }
    GINLINE bool            IsASCharacter() const   { return GASObjectInterface::IsTypeASCharacter(GetObjectType()); }
    GFxASCharacter*         ToASCharacter(); // In cpp file due to a static_cast.

    virtual bool            IsUsedAsMask() const { return false; }
};




// ***** Character Handle


/*  GFxCharacterHandle implementation - stores name and path

    Flash character handles are used to implement the "movieclip" type in ActionScript.
    This type has somewhat mysterious behavior in that it sometimes appears to behave
    as a pointer, while at other times working as a path.

    The most obvious part of this behavior is that it starts out acting as a pointer
    until the pointed-to character dies the first time. This means that during its
    'pointer-like' lifetime different movieclip types can disambiguate references
    to different sprite objects with the same name. Furthermore, it updates itself on
    name changes.
    
    After the original clip object dies, however, the movieclip type remembers its
    global path which it uses to resolve objects. If the name of the object changes,
    the change is not tracked and pointer reference is lost.

    TBD:
    This behavior is not entirely correct. Studio "instance names" seem to have more
    visibility then explicit name changes though AS (test_name_pathconflicts.swf).

    Also, this is not efficient in memory use and will be slow for character accesses
    after the original dies.
*/


class GFxCharacterHandle : public GNewOverrideBase
{
    friend class GFxASCharacter;

    SInt                        RefCount;       // Custom NTS ref-count implementation to save space.   
    GFxASCharacter*             pCharacter;     // Character, can be null if it was destroyed!
        
    // This name is the same as object reference for all levels except _levelN.
    GASString                   Name;
    // Full Path from root, including _levelN. Used to resolve dead characters.
    GASString                   NamePath;

    // Release a character reference, used when character dies
    void                        ReleaseCharacter();

public:
    
    GFxCharacterHandle(const GASString& pname, GFxASCharacter *pparent, GFxASCharacter* pcharacter = 0);
    // Must remove from parent hash and clean parent up.
    ~GFxCharacterHandle();

    GINLINE const GASString&    GetName() const         { return Name; }
    GINLINE const GASString&    GetNamePath() const     { return NamePath; }
    GINLINE GFxASCharacter*     GetCharacter() const    { return pCharacter; }

    // Resolve the character, considering path if necessary.
    GFxASCharacter*             ResolveCharacter(GFxMovieRoot *poot) const; 

    // Ref-count implementation.
    GINLINE void                AddRef()                { RefCount++; }
    GINLINE void                Release(UInt flags=0)   
    { 
        GUNUSED(flags); 
        RefCount--; 
        if (RefCount<=0) 
            delete this; 
    }

    // Changes the name.
    void                        ChangeName(const GASString& pname, GFxASCharacter* pparent);
};



// *****  GFxCharacter - ActionScript controllable object on stage

// GFxASCharacter is a regular character that adds ActionScript control capabilities
// and extra fields, such as _name, which are only available in those objects.

class GFxASCharacter : public GFxCharacter, public GASObjectInterface
{
protected:

    // Data binding root for character; store it since there is no binding 
    // information in the corresponding GFxSpriteDef, GFxButtonCharacterDef, etc.
    // Resources for locally loaded handles come from here.
    // Technically, some other GFxCharacter's might need this too, but in most
    // cases they are ok just relying on binding info within GFxDisplayContext.
    GPtr<GFxMovieDefImpl>   pDefImpl;


    mutable GPtr<GFxCharacterHandle> pNameHandle;
    BlendType               BlendMode;
    bool                    Visible;
    bool                    AcceptAnimMoves;    // Once moved by script, don't accept tag moves.

    // Technically TrackAsMenu and Enabled are only available in Sprite and Button,
    // but its convenient to put them here.
    bool                    TrackAsMenu;
    bool                    Enabled;

    // Set if the instance name was assigned dynamically.
    bool                    InstanceBasedName;
    
    // focus related members
    Bool3W                  TabEnabled;
    int                     TabIndex;
    Bool3W                  FocusRect;

    Bool3W                  UseHandCursor;

    // extension
    bool                    HitTestDisable;        

    typedef GTL::garray<GASValue> EventsArray;

    GTL::ghash<GFxEventId, EventsArray, GFxEventIdHashFunctor> EventHandlers;
    // Display callbacks
    void                    (*pDisplayCallback)(void*);
    void*                   DisplayCallbackUserPtr;

    struct GeomDataType : public GNewOverrideBase
    {
        int     X, Y; // stored in twips
        Double  XScale, YScale;
        Double  Rotation;
        Matrix  OrigMatrix;

        GeomDataType() { X=Y=0; Rotation = 0; XScale=YScale = 100; }
    }                       *pGeomData;

    GeomDataType&           GetGeomData(GeomDataType&) const;
    void                    SetGeomData(const GeomDataType&);
public:

    typedef GASObjectInterface::ObjectType  ObjectType;

    // Constructor.
    GFxASCharacter(GFxMovieDefImpl* pbindingDefImpl, GFxASCharacter* pparent, GFxResourceId id);
    ~GFxASCharacter();

    // Implementation for GFxCharacter optional data.
    virtual BlendType       GetBlendMode() const                { return BlendMode; }
    virtual void            SetBlendMode(BlendType blend)       { BlendMode = blend; }
    virtual void            SetName(const GASString& name);
    virtual const GASString& GetName() const;
    virtual void            SetVisible(bool visible)            { Visible = visible; }
    virtual bool            GetVisible() const                  { return Visible; }

    // Timeline animation support: when (AcceptAnimMoves == 0), timeline has no effect.
    virtual void            SetAcceptAnimMoves(bool accept);
    virtual bool            GetAcceptAnimMoves() const          { return AcceptAnimMoves; }

    // Button & Sprite shared vars access.
    GINLINE bool            GetTrackAsMenu() const              { return TrackAsMenu; } 
    GINLINE bool            GetEnabled() const                  { return Enabled;   }

    GINLINE bool            HasInstanceBasedName() const        { return InstanceBasedName; }

    // Determines the absolute path of the character.
    void                    GetAbsolutePath(GFxString *ppath) const;
    GFxCharacterHandle*     GetCharacterHandle() const;

    
    // Retrieves a global AS context - there is only one in root.
    // GFxCharacter will implement this as { return GetMovieRoot()->pGlobalContext; }
    // This is short because it is used frequently in expressions.
    virtual GASGlobalContext*   GetGC() const;

    // Focus related stuff
    virtual bool            IsTabable() const = 0;
    inline  bool            IsTabIndexed() const { return TabIndex > 0; }
    inline  int             GetTabIndex() const { return TabIndex; }
    virtual bool            IsFocusRectEnabled() const;
    virtual bool            IsFocusEnabled() const { return true; }
    virtual void            FillTabableArray(GTL::garray<GPtr<GFxASCharacter> >&, bool* ) { }
    enum FocusEventType
    {
        KillFocus,
        SetFocus
    };
    // invoked when lose/gain focus
    virtual void            OnFocus(FocusEventType event, GFxASCharacter* oldOrNewFocusCh);
    // invoked when focused item is about to lose mouse input (clicked on another item, for example)
    virtual void            OnLosingMouseInput() {}
    // invoked when item is going to get focus (Selection.setFocus is invoked, or TAB is pressed)
    virtual void            OnGettingKeyboardFocus() {}
    // invoked when focused item is about to lose keyboard focus input (mouse moved, for example)
    virtual void            OnLosingKeyboardFocus() {}
    // returns rectangle for focusrect, in local coords
    virtual GRectF          GetFocusRect() const 
    {
        return GetBounds(GRenderer::Matrix());
    }

    virtual void            CloneInternalData(const GFxASCharacter* src);

    // Event handler accessors.
    bool                    HasEventHandler(const GFxEventId& id) const;
    bool                    InvokeEventHandlers(GASEnvironment* penv, const GFxEventId& id);
    void                    SetEventHandler(const GFxEventId& id, const GASValue& method);

    // From MovieSub: necessary?
    virtual GASObject*      GetASObject ()          { return NULL; }
    virtual GASObject*      GetASObject () const    { return NULL; }    
    virtual bool            OnButtonEvent(const GFxEventId& id)     { return OnEvent(id); }

    virtual GFxASCharacter* GetRelativeTarget(const GASString& name)    
        { GUNUSED(name); return 0; } // for buttons and edit box should just return NULL

    virtual void            SetDisplayCallback(void (*callback)(void*), void* userPtr)
    {
        pDisplayCallback = callback;
        DisplayCallbackUserPtr = userPtr;
    }
    virtual void            DoDisplayCallback()
    {
        if (pDisplayCallback)
            (*pDisplayCallback)(DisplayCallbackUserPtr);
    }

    // Utility.
    void                    DoMouseDrag();

    
    // *** Shared ActionScript methods.

    // Depth implementation - same in MovieClip, Button, TextField.
    static void CharacterGetDepth(const GASFnCall& fn);


    // *** Standard ActionScript property support

    
    // StandardMember property constants are used to support built-in properties,
    // to provide common implementation for those properties for which it is
    // identical in multiple character types, and to enable the use of switch()
    // statement in GetMember/SetMember implementations instead of string compares.

    // Standard properties are divided into several categories based on range.
    //
    //  1). [0 - 21] - These are properties directly used by SetProperty and
    //                 GetProperty opcodes in ActionScript. Thus, they cannot 
    //                 be renumbered.
    //
    //  2). [0 - 31] - These properties have conditional flags indicating
    //                 whether default implementation of GFxASCharacter is
    //                 usable for the specific character type.
    //
    //  3). [31+]    - These properties can be in a list, however, they
    //                 cannot have a default implementation.

    enum StandardMember
    {
        M_InvalidMember = -1,
        
        // Built-in property constants. These must come in specified order since
        // they are used by ActionScript opcodes.
        M_x             = 0,
        M_BuiltInProperty_Begin = M_x,
        M_y             = 1,
        M_xscale        = 2,
        M_yscale        = 3,
        M_currentframe  = 4,
        M_totalframes   = 5,
        M_alpha         = 6,
        M_visible       = 7,
        M_width         = 8,
        M_height        = 9,
        M_rotation      = 10,
        M_target        = 11,
        M_framesloaded  = 12,
        M_name          = 13,
        M_droptarget    = 14,
        M_url           = 15,
        M_highquality   = 16,
        M_focusrect     = 17,
        M_soundbuftime  = 18,
        // SWF 5+.
        M_quality       = 19,
        M_xmouse        = 20,
        M_ymouse        = 21,
        M_BuiltInProperty_End= M_ymouse,

        // Extra shared properties which can have default implementation
        M_parent        = 22,       
        M_blendMode     = 23,
        M_cacheAsBitmap = 24,   // B, M
        M_filters       = 25,
        M_enabled       = 26,   // B, M
        M_trackAsMenu   = 27,   // B, M
        M_lockroot      = 28,   // M, however, it can stick around
        M_tabEnabled    = 29,       
        M_tabIndex      = 30,       
        M_useHandCursor = 31,   // B, M
        M_SharedPropertyEnd = M_useHandCursor,
        
        // Properties used by some characters.
        M_menu, // should be shared, not enough bits.
        
        // Movie clip.
        M_focusEnabled,
        M_tabChildren,
        M_transform,
        // Dynamic text.
        M_text,
        M_textWidth,
        M_textHeight,
        M_textColor,
        M_length,
        M_html,
        M_htmlText,
        M_autoSize,
        M_wordWrap,
        M_multiline,
        M_border,
        M_variable,
        M_selectable,
        M_embedFonts,
        M_antiAliasType,
        M_hscroll,
        M_scroll,
        M_maxscroll,
        M_maxhscroll,
        M_background,
        M_backgroundColor,
        M_borderColor,
        M_bottomScroll,
        M_type,
        M_maxChars,
        M_condenseWhite,
        M_mouseWheelEnabled,
        // Input text
        M_password,

        // GFx extensions
        M_shadowStyle,
        M_shadowColor,
        M_hitTestDisable,
        M_noTranslate,
        M_caretIndex,

        // Dynamic Text
        M_autoFit,
        M_blurX,
        M_blurY,
        M_blurStrength,
        M_outline,

        // Dynamic Text Shadow
        M_shadowAlpha,
        M_shadowAngle,
        M_shadowBlurX,
        M_shadowBlurY,
        M_shadowDistance,
        M_shadowHideObject,
        M_shadowKnockOut,
        M_shadowQuality,
        M_shadowStrength,
        M_shadowOutline,

        M_StandardMemberCount,

        
        // Standard bit masks.

        // Physical transform related members.
        M_BitMask_PhysicalMembers   = (1 << M_x) | (1 << M_y) | (1 << M_xscale) | (1 << M_yscale) |
                                      (1 << M_rotation) | (1 << M_width) | (1 << M_height) | (1 << M_alpha),        
        // Members shared by Button, MovieClip, TextField and Video.
        M_BitMask_CommonMembers     = (1 << M_visible) | (1 << M_parent) | (1 << M_name)        
    };


    // Returns a bit mask of supported standard members withing this character type.
    // GetStandardMember/SetStandardMember methods will fail if the requested constant is not in the bit mask.
    virtual UInt32          GetStandardMemberBitMask() const { return 0; }

    // Looks up a standard member and returns M_InvalidMember if it is not found.
    StandardMember          GetStandardMemberConstant(const GASString& newname) const;
    // Initialization helper - called to initialize member hash in GASGlobalContext
    static void             InitStandardMembers(GASGlobalContext *pcontext);


    // Handles built-in members. Return 0 if member is not found or not supported.
    virtual bool            SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag);
    virtual bool            GetStandardMember(StandardMember member, GASValue* val, bool opcodeFlag) const;



    // GASObjectInterface stuff - for now.
    virtual bool            SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())  
        { GUNUSED4(penv, name, val, flags); GASSERT(0); return false; }
    virtual bool            GetMember(GASEnvironment* penv, const GASString& name, GASValue* val)                    
        { GUNUSED3(penv, name, val); GASSERT(0); return false; }
    virtual void            VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags = 0) const;
    virtual bool            DeleteMember(GASStringContext *psc, const GASString& name);
    virtual bool            SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags);
    virtual bool            HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes);              

    virtual bool            SetMemberRaw(GASStringContext *psc, const GASString& name, const GASValue& val, const GASPropFlags& flags = GASPropFlags())
    {   GUNUSED(psc);
        return SetMember(GetASEnvironment(), name, val, flags);
    }
    virtual bool            GetMemberRaw(GASStringContext *psc, const GASString& name, GASValue* val)
    {   GUNUSED(psc);
        return GetMember(GetASEnvironment(), name, val);
    }

    virtual bool            FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember);
    virtual bool            InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces = true) const;
    virtual bool            Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData);
    virtual bool            Unwatch(GASStringContext *psc, const GASString& prop);

    virtual void            Set__proto__(GASStringContext *psc, GASObject* protoObj);
    //virtual GASObject*      Get__proto__();

    // set this.__proto__ = psrcObj->prototype
    void                    SetProtoToPrototypeOf(GASObjectInterface* psrcObj);

    // Override these so they don't cause conflicts due to being present in both of our bases.
    virtual ObjectType      GetObjectType() const   { return GFxCharacter::GetObjectType(); }
    GINLINE bool            IsASCharacter() const   { return GASObjectInterface::IsASCharacter(); }
    GINLINE GFxASCharacter* ToASCharacter()         { return GASObjectInterface::ToASCharacter(); }

    
    // *** Parent list manipulation functions (work on all types of characters)

    // Duplicate *this* object with the specified name and add it with a new name 
    // at a new depth in our parent (work only if parent is a sprite).
    GFxASCharacter*     CloneDisplayObject(const GASString& newname, SInt depth, const GASObjectInterface *psource);
    // Remove *this* object from its parent.
    void                RemoveDisplayObject();

    
    // *** Movie Loading support

    // Called to substitute poldChar child of this control from new child.
    // This is done to handle loadMovie only; physical properties of the character are copied.
    virtual bool        ReplaceChildCharacterOnLoad(GFxASCharacter *poldChar, GFxASCharacter *pnewChar) 
        { GUNUSED2(poldChar, pnewChar); return 0; }

    // Helper for ReplaceChildCharacterOnLoad.
    // Copies physical properties and reassigns character handle.
    void                CopyPhysicalProperties(GFxASCharacter *poldChar);
    

    // *** GFxSprite's virtual methods.

    // These timeline related methods are implemented only in GFxSprite; for other
    // ActionScript characters they do nothing. It is convenient to put them here
    // to avoid many type casts in ActionScript opcode implementation.
    virtual UInt            GetCurrentFrame() const             
        { return 0; }   
    virtual bool            GetLabeledFrame(const char* plabel, UInt* frameNumber, bool translateNumbers = 1) 
        { GUNUSED3(plabel, frameNumber, translateNumbers); return 0; }
    virtual void            GotoFrame(UInt targetFrameNumber)   
        { GUNUSED(targetFrameNumber); }
    virtual void            SetPlayState(GFxMovie::PlayState s) 
        { GUNUSED(s); }

    // Execute action buffer
    virtual bool            ExecuteBuffer(GASActionBuffer* pactionBuffer) 
        { GUNUSED(pactionBuffer); return false; }
    // Execute this even immediately (called for processing queued event actions).
    virtual bool            ExecuteEvent(const GFxEventId& id);
    // Execute function
    virtual bool            ExecuteFunction(const GASFunctionRef& function, const GASValueArray& params);
    // Execute ?function
    virtual bool            ExecuteCFunction(const GASCFunctionPtr function, const GASValueArray& params);

    virtual UInt            GetCursorType() const { return GFxMouseCursorEvent::ARROW; }
};



// ***** GFxPointTestCache - cache for efficient PointTest implementation

// PointTest calls on shapes can be quite expensive because they require compound
// shape objects that are used for hit testing. This class 
class GFxPointTestCache;
class GFxPointTestCacheProvider;

class GFxPointTestCache
{
public:
    struct CacheNode;

    struct CacheNodeBase : public GNewOverrideBase
    {            
        union {
            CacheNodeBase* pHead;
            CacheNode*     pNext;

        };
        union {
            CacheNodeBase* pTail;
            CacheNode*     pPrev;
        };
    };

    struct CacheNode : public CacheNodeBase
    {
        // Character this cache is for.
        const GFxPointTestCacheProvider*  pProvider;
        // Cache manager that created this node.
        GFxPointTestCache*          pCache;
        // Frame in which this node was created/used.
        // This frame is arbitrary and has no relation to playback time.
        UInt                        Frame;
        bool                        ShapesValid;
        // Cached shapes for paths.
        GTL::garray<GCompoundShape> Shapes;

        void    InsertAtHead(CacheNodeBase* pr)
        {                 
            pNext = pr->pNext;
            pPrev = (CacheNode*)pr;
            pr->pNext->pPrev = this;
            pr->pNext = this;
        }
        void    Remove()
        {   
            pNext->pPrev = pPrev;
            pPrev->pNext = pNext;
        }

        void    MoveToHead(UInt frame)
        {
            Remove();
            InsertAtHead(&pCache->Root);
            Frame = frame;
        }
    };



    // Root node used to store links. Not used for data storage.
    CacheNodeBase  Root;
private:
    // Unused allocated node pool.
    CacheNode*     pUnused;
    // Frame counter; incremented but wrap-around safe.
    UInt           Frame;

public:

    GFxPointTestCache();
    ~GFxPointTestCache();

    // Adds node to head and returns it.
    CacheNode*  AddNode(const GFxPointTestCacheProvider *pchar);    
    // Remove node and update its provider.
    void        ReleaseNode(CacheNode* pnode);
    
    // Increments frame and discards stale elements from tail.
    void        NextFrame();

    // Updates node for use.
    void        UpdateNode(CacheNode* pnode)
    {
        pnode->MoveToHead(Frame);
    }

};

// Items that support caching derive from this.
class GFxPointTestCacheProvider
{
public:
    typedef GFxPointTestCache::CacheNode CacheNode;
    
    GFxPointTestCache*  pHitTestCache;
    mutable CacheNode*  pHitTestNode;
    mutable GPointF     LastHitTest;      
    mutable bool        LastHitTestResult;    

    GFxPointTestCacheProvider(GFxPointTestCache *pcache)
    {
        LastHitTest.SetPoint(-GFC_MAX_FLOAT, -GFC_MAX_FLOAT);
        LastHitTestResult = 0;
        pHitTestNode = 0;
        pHitTestCache = pcache;
    }
    ~GFxPointTestCacheProvider()
    {
        // Important: If character dies, remove its node from cache.
        if (pHitTestNode)
            pHitTestNode->pCache->ReleaseNode(pHitTestNode);
    }

    CacheNode* GetNode() const
    {
        return 0;
        // MA WIP: Enabling this will reduce PointTest cost,
        // at the expense of more cache memory. TBD.
        /*
        if (!pHitTestNode)
            pHitTestNode = pHitTestCache->AddNode(this);
        else
            pHitTestCache->UpdateNode(pHitTestNode);
        return pHitTestNode;
        */
    }
};



// ***** GFxGenericCharacter - character implementation with no extra data

// GFxGenericCharacter is used for non ActionScriptable characters that don't store
// unusual state in their instances. It is used for shapes, morph shapes and static
// text fields.

class GFxGenericCharacter : public GFxCharacter, public GFxPointTestCacheProvider
{
public:
    GFxCharacterDef*    pDef;
   
    GFxGenericCharacter(GFxCharacterDef* pdef, GFxASCharacter* pparent, GFxResourceId id);

    virtual GFxCharacterDef* GetCharacterDef() const
    {
        return pDef;
    }
  
    virtual void    Display(GFxDisplayContext &context,StackData stackData);

    // These are used for finding bounds, width and height.
    virtual GRectF  GetBounds(const Matrix &transform) const
    {       
        return transform.EncloseTransform(pDef->GetBoundsLocal());
    }   

    virtual bool    PointTestLocal(const GPointF &pt, bool testShape = 0) const;

    // Override this to hit-test shapes and make Button-Mode sprites work.
    GFxASCharacter*  GetTopMostMouseEntity(const GPointF &pt, bool testAll = false);
};



// ***** Resources 


class GFxSoundSample : public GFxResource
{
public:
    virtual GFxSoundSample* ToSoundSample() { return this; }
};





#endif // INC_GFXCHARACTER_H
