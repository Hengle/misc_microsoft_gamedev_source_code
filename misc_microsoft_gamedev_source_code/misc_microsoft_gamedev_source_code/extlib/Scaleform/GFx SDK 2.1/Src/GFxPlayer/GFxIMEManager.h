/**********************************************************************

Filename    :   GFxIMEManager.h
Content     :   IME Manager base functinality
Created     :   Dec 17, 2007
Authors     :   Artyom Bolgar

Notes       :   
History     :   

Copyright   :   (c) 1998-2007 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/
#ifndef INC_IMEMANAGER_H
#define INC_IMEMANAGER_H

#include "GConfig.h"
#include "GFxLoader.h"
#include "GTypes2DF.h"

#ifndef GFC_NO_IME_SUPPORT
class GFxASCharacter;
class GFxIMEEvent;

// A structure used to transfer IME candidate list styles
// from ActionScript function to IME implementation.

class GFxIMECandidateListStyle : public GNewOverrideBase
{
    UInt32 TextColor;
    UInt32 BackgroundColor;
    UInt32 IndexBackgroundColor;
    UInt32 SelectedTextColor;
    UInt32 SelectedBackgroundColor;
    UInt32 SelectedIndexBackgroundColor;
    UInt32 ReadingWindowTextColor;
    UInt32 ReadingWindowBackgroundColor;
    UInt   FontSize; // in points 
    UInt   ReadingWindowFontSize; 
    enum
    {
        Flag_TextColor                      = 0x01,
        Flag_BackgroundColor                = 0x02,
        Flag_IndexBackgroundColor           = 0x04,
        Flag_SelectedTextColor              = 0x08,
        Flag_SelectedBackgroundColor        = 0x10,
        Flag_SelectedIndexBackgroundColor   = 0x20,
        Flag_FontSize                       = 0x40,
        Flag_ReadingWindowTextColor         = 0x80,
        Flag_ReadingWindowBackgroundColor   = 0x100,
        Flag_ReadingWindowFontSize          = 0x200
    };
    UInt16  Flags;
public:
    GFxIMECandidateListStyle():Flags(0) {}

    void    SetTextColor(UInt32 color)         { Flags |= Flag_TextColor; TextColor = color; }
    UInt32  GetTextColor() const               { return (HasTextColor()) ? TextColor : 0; }
    void    ClearTextColor()                   { Flags &= (~Flag_TextColor); }
    bool    HasTextColor() const               { return (Flags & Flag_TextColor) != 0; }

    void    SetBackgroundColor(UInt32 backgr)  { Flags |= Flag_BackgroundColor; BackgroundColor = backgr; }
    UInt32  GetBackgroundColor() const         { return (HasBackgroundColor()) ? BackgroundColor : 0; }
    void    ClearBackgroundColor()             { Flags &= (~Flag_BackgroundColor); }
    bool    HasBackgroundColor() const         { return (Flags & Flag_BackgroundColor) != 0; }

    void    SetIndexBackgroundColor(UInt32 backgr) { Flags |= Flag_IndexBackgroundColor; IndexBackgroundColor = backgr; }
    UInt32  GetIndexBackgroundColor() const        { return (HasIndexBackgroundColor()) ? IndexBackgroundColor : 0; }
    void    ClearIndexBackgroundColor()            { Flags &= (~Flag_IndexBackgroundColor); }
    bool    HasIndexBackgroundColor() const        { return (Flags & Flag_IndexBackgroundColor) != 0; }

    void    SetSelectedTextColor(UInt32 color) { Flags |= Flag_SelectedTextColor; SelectedTextColor = color; }
    UInt32  GetSelectedTextColor() const       { return (HasSelectedTextColor()) ? SelectedTextColor : 0; }
    void    ClearSelectedTextColor()           { Flags &= (~Flag_SelectedTextColor); }
    bool    HasSelectedTextColor() const       { return (Flags & Flag_SelectedTextColor) != 0; }

    void    SetSelectedBackgroundColor(UInt32 backgr) { Flags |= Flag_SelectedBackgroundColor; SelectedBackgroundColor = backgr; }
    UInt32  GetSelectedBackgroundColor() const        { return (HasSelectedBackgroundColor()) ? SelectedBackgroundColor : 0; }
    void    ClearSelectedBackgroundColor()            { Flags &= (~Flag_SelectedBackgroundColor); }
    bool    HasSelectedBackgroundColor() const        { return (Flags & Flag_SelectedBackgroundColor) != 0; }

    void    SetSelectedIndexBackgroundColor(UInt32 backgr) { Flags |= Flag_SelectedIndexBackgroundColor; SelectedIndexBackgroundColor = backgr; }
    UInt32  GetSelectedIndexBackgroundColor() const        { return (HasSelectedIndexBackgroundColor()) ? SelectedIndexBackgroundColor : 0; }
    void    ClearSelectedIndexBackgroundColor()            { Flags &= (~Flag_SelectedIndexBackgroundColor); }
    bool    HasSelectedIndexBackgroundColor() const        { return (Flags & Flag_SelectedIndexBackgroundColor) != 0; }

    void    SetFontSize(UInt fs) { Flags |= Flag_FontSize; FontSize = fs; }
    UInt    GetFontSize() const  { return (HasFontSize()) ? FontSize : 0; }
    void    ClearFontSize()      { Flags &= (~Flag_FontSize); }
    bool    HasFontSize() const  { return (Flags & Flag_FontSize) != 0; }

    // Reading Window styles.
    void    SetReadingWindowTextColor(UInt fs) { Flags |= Flag_ReadingWindowTextColor; ReadingWindowTextColor = fs; }
    UInt    GetReadingWindowTextColor() const  { return (HasReadingWindowTextColor()) ? ReadingWindowTextColor : 0; }
    void    ClearReadingWindowTextColor()      { Flags &= (~Flag_ReadingWindowTextColor); }
    bool    HasReadingWindowTextColor() const  { return (Flags & Flag_ReadingWindowTextColor) != 0; }

    void    SetReadingWindowBackgroundColor(UInt fs) { Flags |= Flag_ReadingWindowBackgroundColor; ReadingWindowBackgroundColor = fs; }
    UInt    GetReadingWindowBackgroundColor() const  { return (HasReadingWindowBackgroundColor()) ? ReadingWindowBackgroundColor : 0; }
    void    ClearReadingWindowBackgroundColor()      { Flags &= (~Flag_ReadingWindowBackgroundColor); }
    bool    HasReadingWindowBackgroundColor() const  { return (Flags & Flag_ReadingWindowBackgroundColor) != 0; }

    void    SetReadingWindowFontSize(UInt fs) { Flags |= Flag_ReadingWindowFontSize; ReadingWindowFontSize = fs; }
    UInt    GetReadingWindowFontSize() const  { return (HasReadingWindowFontSize()) ? ReadingWindowFontSize : 0; }
    void    ClearReadingWindowFontSize()      { Flags &= (~Flag_ReadingWindowFontSize); }
    bool    HasReadingWindowFontSize() const  { return (Flags & Flag_ReadingWindowFontSize) != 0; }
};

// IME manager interface class. This class may be used as a base class for various
// IME implementations. It also provides utility functions to control composition, 
// candidate list and so on.

class GFxIMEManager : public GFxState
{
    friend class GFxMovieRoot;

    class GFxIMEManagerImpl *pImpl;

    // handles focus. 
    GFxASCharacter* HandleFocus(GFxMovieView* pmovie, 
                                GFxASCharacter* poldFocusedItem, 
                                GFxASCharacter* pnewFocusingItem, 
                                GFxASCharacter* ptopMostItem);

    // callback, invoked when mouse button is down. buttonsState is a mask:
    //   bit 0 - right button is pressed,
    //   bit 1 - left
    //   bit 2 - middle
    void OnMouseDown(GFxMovieView* pmovie, int buttonsState, GFxASCharacter* pitemUnderMousePtr);
    void OnMouseUp(GFxMovieView* pmovie, int buttonsState, GFxASCharacter* pitemUnderMousePtr);

public:
    GFxIMEManager();
    ~GFxIMEManager();

    //**** utility methods
    // creates the composition string, if not created yet
    void StartComposition();
    
    // finalizes the composition string by inserting the string into the
    // actual text. If pstr is not NULL then the content of pstr is being used;
    // otherwise, the current content of composition string will be used.
    void FinalizeComposition(const wchar_t* pstr, UPInt len = GFC_MAX_UPINT);

    // clears the composition string. FinalizeComposition with pstr != NULL
    // still may be used after ClearComposition is invoked.
    void ClearComposition();

    // release the composition string, so no further composition string related
    // functions may be used.
    void ReleaseComposition();

    // changes the text in composition string
    void SetCompositionText(const wchar_t* pstr, UPInt len = GFC_MAX_UPINT);

    // relocates the composition string to the current caret position
    void SetCompositionPosition();

    // sets cursor inside the composition string. "pos" is specified relative to 
    // composition string.
    void SetCursorInComposition(UPInt pos);

    // turns on/off wide cursor.
    void SetWideCursor(bool = true);

    // styles of text highlighting (used in HighlightText)
    enum TextHighlightStyle
    {
        THS_CompositionSegment   = 0,
        THS_ClauseSegment        = 1,
        THS_ConvertedSegment     = 2,
        THS_PhraseLengthAdj      = 3,
        THS_LowConfSegment       = 4
    };
    // highlights the clause in composition string.
    // Parameter "clause" should be true, if this method is called to highlight
    // the clause (for example, for Japanese IME). In this case, in addition to the
    // requested highlighting whole composition string will be underline by single
    // underline.
    void HighlightText(UPInt pos, UPInt len, TextHighlightStyle style, bool clause);

    // returns view rectangle of currently focused text field
    // and cursor rectangle, both in stage (root) coordinate space.
    // cursorOffset may be negative, specifies the offset from the 
    // actual cursor pos.
    void GetMetrics(GRectF* pviewRect, GRectF* pcursorRect, int cursorOffset = 0);

    // Returns true, if text field is currently focused.
    bool IsTextFieldFocused() const;

    // Returns true, if the specified text field is currently focused.
    bool IsTextFieldFocused(GFxASCharacter* ptextfield) const;

    // Checks if candidate list is loaded
    bool IsCandidateListLoaded() const;

    // Sets candidate list movie path
    void SetIMEMoviePath(const char* pcandidateSwfPath);

    // Sets style of candidate list. Invokes OnCandidateListStyleChanged callback.
    bool SetCandidateListStyle(const GFxIMECandidateListStyle& st);

    // Gets style of candidate list
    bool GetCandidateListStyle(GFxIMECandidateListStyle* pst) const;

    // Loads candidate list movie, if it wasn't loaded yet. It will invoke
    // OnCandidateListLoaded(path) virtual method once movie is loaded.
    bool AcquireCandidateList();

    // Fills pdest with error message, if candidate list failed to load.
    // Returns 'true' if error occurred.
    bool GetCandidateListErrorMsg(GFxString* pdest);

    // Finalize the composition and release the text field.
    void DoFinalize();

    // This function checks if the pTextField ptr is NULL. 
    bool IsTextfieldNull();

    // enables/disables IME
    void EnableIME(bool enable);

    // sets currently focused movie view to IME manager.
    virtual void SetActiveMovie(GFxMovieView*);
    virtual bool IsMovieActive(GFxMovieView*) const;
    virtual GFxMovieView* GetActiveMovie() const;
    // cleans out the movie from the IME. NO ACTIONSCRIPT should be used
    // with movie being cleaned up! As well as, do not save it in GPtr.
    virtual void ClearActiveMovie();

    // *** callbacks, invoked from core. These virtual methods
    // might be overloaded by the implementation.

    // invoked to check does the symbol belong to candidate list or not
    virtual bool IsCandidateList(const char* ppath) { GUNUSED(ppath); return false; }

    // Handles IME events, calling callbacks and switching states.
    virtual UInt HandleIMEEvent(GFxMovieView* pmovie, const GFxIMEEvent& imeEvent);

    // handle "_global.imecommand(cmd, param)"
    virtual void IMECommand(GFxMovieView* pmovie, const char* pcommand, const char* pparam) 
        { GUNUSED3(pmovie, pcommand, pparam); }

    // invoked when need to finalize the composition.
    virtual void OnFinalize() {}

    // invoked when need to cleanup and shutdown the IME. Do not invoke ActionScript from it!
    virtual void OnShutdown() {}

    // invoked when candidate list loading is completed.
    virtual void OnCandidateListLoaded(const char* pcandidateListPath);

    // invoked when candidate list's style has been changed by ActionScript
    virtual void OnCandidateListStyleChanged(const GFxIMECandidateListStyle& style) { GUNUSED(style); }

    // invoked when ActionScript is going to get candidate list style properties.
    // The default implementation just retrieves styles from movie view.
    virtual void OnCandidateListStyleRequest(GFxIMECandidateListStyle* pstyle) const;

    // handles enabling/disabling IME, invoked from EnableIME method
    virtual void OnEnableIME(bool enable) { GUNUSED(enable); }
};

inline void                 GFxSharedState::SetIMEManager(GFxIMEManager *ptr)       
{ 
    SetState(GFxState::State_IMEManager, ptr); 
}

inline GPtr<GFxIMEManager>  GFxSharedState::GetIMEManager() const                   
{ 
    return *(GFxIMEManager*) GetStateAddRef(GFxState::State_IMEManager); 
}

#else // #ifndef GFC_NO_IME_SUPPORT
class GFxIMECandidateListStyle : public GNewOverrideBase
{
};

// inline stub for GFxIMEManager
class GFxIMEManager : public GFxState
{
public:
    GFxIMEManager();
};
#endif // #ifndef GFC_NO_IME_SUPPORT

#endif //INC_IMEMANAGER_H
