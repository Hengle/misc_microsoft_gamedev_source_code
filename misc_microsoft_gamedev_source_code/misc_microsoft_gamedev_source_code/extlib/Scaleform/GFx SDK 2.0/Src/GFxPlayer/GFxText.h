/**********************************************************************

Filename    :   GFxText.h
Content     :   Text character implementation
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXTEXT_H
#define INC_GFXTEXT_H

#include "GRefCount.h"
#include "GFxCharacter.h"
#include "GFxObject.h"

#include "GFxAction.h"
#include "GFxStyledText.h"
#include "GFxObjectProto.h"

// A definition for a text display GFxCharacter, whose text can
// be changed at Runtime (by script or host).
struct GFxEditTextCharacterDef : public GFxCharacterDef
{
    GFxMovieDataDef*        RootDef;
    GFxResourcePtr<GFxFontResource> pFont;
    GFxResourceId           FontId;

    GRectF                  TextRect;
    Float                   TextHeight;

    GColor                  Color;
    int                     MaxLength;

    Float                   LeftMargin; // extra space between box border and text
    Float                   RightMargin;
    Float                   Indent;     // how much to indent the first line of multiline text
    Float                   Leading;    // extra space between Lines (in addition to default font line spacing)
    GFxString               DefaultText;
    GFxString               VariableName;

    enum
    {
        Flags_WordWrap      = 0x1,
        Flags_Multiline     = 0x2,
        Flags_Password      = 0x4,  // show asterisks instead of actual characters
        Flags_ReadOnly      = 0x8,
        Flags_AutoSize      = 0x10, // resize our bound to fit the text
        Flags_Selectable    = 0x20,
        Flags_Border        = 0x40, // forces background and border to be drawn
        Flags_Html          = 0x80,
        Flags_UseDeviceFont = 0x100, // when false, use specified SWF internal font.Otherwise, device font will be used
        Flags_HasLayout     = 0x200, // Alignment, LeftMargin, RightMargin, Indent, Leading are explicitly loaded
        Flags_AAReadability = 0x400  // Anti-aliased for readability
    };
    UInt16                  Flags;
    enum alignment
    {
        ALIGN_LEFT = 0,
        ALIGN_RIGHT,
        ALIGN_CENTER,
        ALIGN_JUSTIFY,
    };
    alignment   Alignment;

    GFxEditTextCharacterDef(GFxMovieDataDef* RootDef);
    ~GFxEditTextCharacterDef();

    void SetWordWrap(bool v = true)     { (v) ? Flags |= Flags_WordWrap : Flags &= (~Flags_WordWrap); }
    void ClearWordWrap()                { SetWordWrap(false); }
    bool IsWordWrap() const             { return (Flags & Flags_WordWrap) != 0; }

    void SetMultiline(bool v = true)    { (v) ? Flags |= Flags_Multiline : Flags &= (~Flags_Multiline); }
    void ClearMultiline()               { SetMultiline(false); }
    bool IsMultiline() const            { return (Flags & Flags_Multiline) != 0; }

    void SetPassword(bool v = true)     { (v) ? Flags |= Flags_Password : Flags &= (~Flags_Password); }
    void ClearPassword()                { SetPassword(false); }
    bool IsPassword() const             { return (Flags & Flags_Password) != 0; }

    void SetReadOnly(bool v = true)     { (v) ? Flags |= Flags_ReadOnly : Flags &= (~Flags_ReadOnly); }
    void ClearReadOnly()                { SetReadOnly(false); }
    bool IsReadOnly() const             { return (Flags & Flags_ReadOnly) != 0; }

    void SetAutoSize(bool v = true)     { (v) ? Flags |= Flags_AutoSize : Flags &= (~Flags_AutoSize); }
    void ClearAutoSize()                { SetAutoSize(false); }
    bool IsAutoSize() const             { return (Flags & Flags_AutoSize) != 0; }

    void SetSelectable(bool v = true)   { (v) ? Flags |= Flags_Selectable : Flags &= (~Flags_Selectable); }
    void ClearSelectable()              { SetSelectable(false); }
    bool IsSelectable() const           { return (Flags & Flags_Selectable) != 0; }

    void SetBorder(bool v = true)       { (v) ? Flags |= Flags_Border : Flags &= (~Flags_Border); }
    void ClearBorder()                  { SetBorder(false); }
    bool IsBorder() const               { return (Flags & Flags_Border) != 0; }

    void SetHtml(bool v = true)         { (v) ? Flags |= Flags_Html : Flags &= (~Flags_Html); }
    void ClearHtml()                    { SetHtml(false); }
    bool IsHtml() const                 { return (Flags & Flags_Html) != 0; }

    void SetUseDeviceFont(bool v = true){ (v) ? Flags |= Flags_UseDeviceFont : Flags &= (~Flags_UseDeviceFont); }
    void ClearUseDeviceFont()           { SetUseDeviceFont(false); }
    bool DoesUseDeviceFont() const      { return (Flags & Flags_UseDeviceFont) != 0; }

    void SetHasLayout(bool v = true)    { (v) ? Flags |= Flags_HasLayout : Flags &= (~Flags_HasLayout); }
    void ClearHasLayout()               { SetHasLayout(false); }
    bool HasLayout() const              { return (Flags & Flags_HasLayout) != 0; }

    void SetAAForReadability(bool v = true)  { (v) ? Flags |= Flags_AAReadability : Flags &= (~Flags_AAReadability); }
    void ClearAAForReadability()             { SetAAForReadability(false); }
    bool IsAAForReadability() const          { return (Flags & Flags_AAReadability) != 0; }

    GFxCharacter*   CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id, GFxMovieDefImpl *pbindingImpl);

    void InitEmptyTextDef();

    void Read(GFxLoadProcess* p, GFxTagType tagType);
    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    UInt GetResourceTypeCode() const     { return MakeTypeCode(RT_EditTextDef); }
};

// Helper struct.
class GFxStaticTextRecord : public GNewOverrideBase
{
public:
    class GlyphEntry
    {
    public:
        UInt    GlyphIndex;         // Index of the glyph in font
        Float   GlyphAdvance;       // Advance value from previous glyph

        GlyphEntry(UInt index = (~0u), Float advance = 0.0f)
        {
            GlyphIndex          = index;
            GlyphAdvance        = advance;
        }   
    };

    GTL::garray<GlyphEntry> Glyphs;
    GFxResourcePtr<GFxFontResource> pFont;

    GColor                  Color;
    GPointF                 Offset; 
    Float                   TextHeight; 
    UInt16                  FontId;

    GFxStaticTextRecord();

    void    Read(GFxStream* in, int glyphCount, int glyphBits, int advanceBits);

    // Obtains cumulative advance value.
    Float   GetCumulativeAdvance() const;
};


// An array of text glyph records, used for convenience.
class GFxStaticTextRecordList
{
public:

    GTL::garray<GFxStaticTextRecord*> Records;

    GFxStaticTextRecordList() {}

    ~GFxStaticTextRecordList()
    {
        Clear();
    }

    // Removes all records.
    void Clear();

    UInt GetSize() const
    {
        return (UInt)Records.size();
    }

    // Add a new record to the list.
    GFxStaticTextRecord* AddRecord();
};

class GFxStaticTextCharacterDef : public GFxCharacterDef
{
public:
    GFxMovieDataDef*        RootDef;
    GRectF                  TextRect;
    GRenderer::Matrix       MatrixPriv;
    GFxStaticTextRecordList TextRecords;
    enum
    {
        Flags_AAReadability = 0x01  // Anti-aliased for readability
    };
    UByte                   Flags;

    GFxStaticTextCharacterDef(GFxMovieDataDef* prootDef);
    GFxCharacter*   CreateCharacterInstance(GFxASCharacter* parent, GFxResourceId id,
        GFxMovieDefImpl *pbindingImpl);

    void SetAAForReadability(bool v = true)  { (v) ? Flags |= Flags_AAReadability : Flags &= (~Flags_AAReadability); }
    void ClearAAForReadability()             { SetAAForReadability(false); }
    bool IsAAForReadability() const          { return (Flags & Flags_AAReadability) != 0; }

    void    Read(GFxLoadProcess* p, GFxTagType tagType);

    virtual GRectF  GetBoundsLocal() const { return TextRect; }

    virtual bool    PointTestLocal(const GPointF &pt, bool testShape = 0, const GFxPointTestCacheProvider *pcache = 0) const;

    // *** GFxResource implementation

    // Query Resource type code, which is a combination of ResourceType and ResourceUse.
    virtual UInt    GetResourceTypeCode() const     { return MakeTypeCode(RT_TextDef); }
};

// external interface
class GASTextField : public GFxASCharacter
{
public:
    GASTextField(GFxMovieDefImpl *pbindingDefImpl, GFxASCharacter* parent, GFxResourceId id)
        : GFxASCharacter(pbindingDefImpl, parent, id) {}

    virtual void SetSelection(int beginIndex, int endIndex) = 0;
    virtual int GetCaretIndex() = 0;
};

// ActionScript objects.

class GASTextFieldObject : public GASObject
{
    friend class GASTextFieldProto;

    GWeakPtr<GFxASCharacter> pTextField;    // weak ref on textfield obj
protected:

    GASTextFieldObject(GASStringContext *psc = 0) : GASObject() { GUNUSED(psc); }
public:
    GASTextFieldObject(GASGlobalContext* gCtxt, GFxASCharacter* ptextfield) : 
        pTextField(ptextfield)
    {
        GASStringContext* psc = ptextfield->GetASEnvironment()->GetSC();
        Set__proto__(psc, gCtxt->GetPrototype(GASBuiltin_TextField));
    }

    virtual ObjectType      GetObjectType() const   { return Object_TextFieldASObject; }

    GFxASCharacter*         GetASCharacter() { return GPtr<GFxASCharacter>(pTextField); }
};

class GASTextFieldProto : public GASPrototype<GASTextFieldObject>
{
public:
    GASTextFieldProto(GASStringContext *psc, GASObject* prototype, const GASFunctionRef& constructor);

    static void GlobalCtor(const GASFnCall& fn);
};


#endif // INC_GFXTEXT_H

