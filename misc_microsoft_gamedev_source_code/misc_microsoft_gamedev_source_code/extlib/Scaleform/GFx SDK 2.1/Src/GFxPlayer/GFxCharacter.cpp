/**********************************************************************

Filename    :   GFxCharacter.cpp
Content     :   Base functionality for characters, which are display
                list objects.
Created     :   May 25, 2006  (moved from GFxPlayerImpl)
Authors     :   Michael Antonov
Notes       :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.


Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxCharacter.h"
#include "GFxPlayerImpl.h"
#include "GFxSprite.h"
#include "GFxNumber.h"


// ***** GFxCharacter   

GFxCharacter::GFxCharacter(GFxASCharacter* parent, GFxResourceId id)
    :
    Id(id),
    Depth(-1),
    Ratio(0.0f),
    ClipDepth(0),
    pParent(parent),
    Flags(0)
{
    GASSERT((parent == NULL && Id == GFxResourceId::InvalidId) ||
            (parent != NULL && Id.GetIdIndex() >= 0));
}

GFxCharacter::~GFxCharacter()
{
}

void GFxCharacter::SetDirtyFlag()
{
    GetMovieRoot()->SetDirtyFlag();
}

const GASString& GFxCharacter::GetName() const
{   
    // Stage characters, such as shapes have no name, yet GFxASCharacters do.
    // So always return an empty name here.
    return GetASEnvironment()->GetBuiltin(GASBuiltin_empty_);
}

// Character transform implementation.
void    GFxCharacter::GetWorldMatrix(Matrix *pmat) const
{           
    if (pParent)
    {
        pParent->GetWorldMatrix(pmat);
        pmat->Prepend(GetMatrix());
    }
    else
    {
        *pmat = GetMatrix();
    }
}

// Character transform implementation.
void    GFxCharacter::GetLevelMatrix(Matrix *pmat) const
{           
    if (pParent)
    {
        pParent->GetLevelMatrix(pmat);
        pmat->Prepend(GetMatrix());
    }
    else
    {
        *pmat = Matrix();
    }
}

// Character color transform implementation.
void    GFxCharacter::GetWorldCxform(Cxform *pcxform) const
{
    if (pParent)
    {
        pParent->GetWorldCxform(pcxform);
        pcxform->Concatenate(GetCxform());
    }
    else
    {
        *pcxform = GetCxform();     
    }
}

// Used during rendering.

// Temporary - used until blending logic is improved.
GRenderer::BlendType    GFxCharacter::GetActiveBlendMode() const
{
    GRenderer::BlendType    blend = GRenderer::Blend_None;
    const GFxCharacter*     pchar = this;
    
    while (pchar)
    {
        blend = pchar->GetBlendMode();
        if (blend > GRenderer::Blend_Layer)
            return blend;
        pchar = pchar->GetParent();
    }
    // Return last blend mode.
    return blend;
}


// This is not because of GFxMovieDefImpl dependency not available in the header.
UInt GFxCharacter::GetVersion() const
{
    return GetResourceMovieDef()->GetVersion();
}

GASEnvironment*         GFxCharacter::GetASEnvironment()
{
    GFxASCharacter* pparent = GetParent();
    while(pparent && !pparent->IsSprite())
        pparent = pparent->GetParent();
    if (!pparent)
        return 0;
    return pparent->ToSprite()->GetASEnvironment();
}

const GASEnvironment*   GFxCharacter::GetASEnvironment() const
{
    // Call non-const version. Const-ness really only matters for GFxSprite.
    return const_cast<GFxCharacter*>(this)->GetASEnvironment();
}

// (needs to be implemented in .cpp, so that GFxMovieRoot is visible)
GFxLog*     GFxCharacter::GetLog() const
{
    // By default, GetMovieRoot will delegate to parent.
    // GFxSprite should override GetMovieRoot to return the correct object.
    return GetMovieRoot()->GetCachedLog();
}
bool        GFxCharacter::IsVerboseAction() const
{
    return GetMovieRoot()->VerboseAction;
}

bool        GFxCharacter::IsVerboseActionErrors() const
{
    return !GetMovieRoot()->SuppressActionErrors;
}

GFxASCharacter* GFxCharacter::ToASCharacter()
{
    return IsASCharacter() ? static_cast<GFxASCharacter*>(this) : 0;
}

GFxScale9GridInfo* GFxCharacter::CreateScale9Grid(Float pixelScale) const
{
    GFxASCharacter* parent   = GetParent();
    GMatrix2D       shapeMtx = GetMatrix();
    while (parent)
    {
        if (parent->GetScale9Grid())
        {
            GRectF bounds = parent->GetRectBounds(GMatrix2D());
            return new GFxScale9GridInfo(parent->GetScale9Grid(), 
                                         parent->GetMatrix(), 
                                         shapeMtx, 
                                         pixelScale, 
                                         bounds);
        }
        shapeMtx.Append(parent->GetMatrix());
        parent = parent->GetParent();
    }
    return 0;
}

void GFxCharacter::OnEventUnload()
{
    // should it be before Event_Unload or after? (AB)
    if (IsTopmostLevelFlagSet())
    {
        GetMovieRoot()->RemoveTopmostLevelCharacter(this);
    }
    OnEvent(GFxEventId::Event_Unload);
}

// ***** GFxCharacterHandle



GFxCharacterHandle::GFxCharacterHandle(const GASString& name, GFxASCharacter *pparent, GFxASCharacter* pcharacter)
    : Name(name),NamePath(name.GetManager()->CreateEmptyString())
{
    RefCount    = 1;
    pCharacter  = pcharacter;    
    
    // Compute path based on parent
    GFxString namePathBuff;
    if (pparent)
    {
        pparent->GetAbsolutePath(&namePathBuff);
        namePathBuff += ".";
    }
    namePathBuff += Name.ToCStr();
    NamePath = name.GetManager()->CreateString(namePathBuff);
}

GFxCharacterHandle::~GFxCharacterHandle()
{
}

// Release a character reference, used when character dies
void    GFxCharacterHandle::ReleaseCharacter()
{
    pCharacter = 0;
}

    
// Changes the name.
void    GFxCharacterHandle::ChangeName(const GASString& name, GFxASCharacter *pparent)
{
    Name = name;
    // Compute path based on parent
    GFxString namePathBuff;
    if (pparent)
    {
        pparent->GetAbsolutePath(&namePathBuff);
        namePathBuff += ".";
    }
    namePathBuff += Name.ToCStr();
    NamePath = name.GetManager()->CreateString(namePathBuff);

    // Do we need to update paths in all parents ??
}

// Resolve the character, considering path if necessary.
GFxASCharacter* GFxCharacterHandle::ResolveCharacter(GFxMovieRoot *proot) const
{
    if (pCharacter)
        return pCharacter;
    // Resolve a global path based on Root.
    return proot->FindTarget(NamePath);
}





// ***** GFxASCharacter

// Constructor.
GFxASCharacter::GFxASCharacter(GFxMovieDefImpl* pbindingDefImpl,
                               GFxASCharacter* pparent, GFxResourceId id)
    :
    GFxCharacter(pparent, id),
    pDefImpl(pbindingDefImpl),
    TabIndex (0),
    pGeomData(0),
    Flags(0),
    pDisplayCallback(NULL),
    DisplayCallbackUserPtr(NULL)
{
    BlendMode = GRenderer::Blend_None;
    CreateFrame = 0;

    SetVisibleFlag();
    SetAcceptAnimMovesFlag();
    SetEnabledFlag();
    SetInstanceBasedNameFlag();
}

GFxASCharacter::~GFxASCharacter()
{
    if (pNameHandle)
        pNameHandle->ReleaseCharacter();
    if (pGeomData)
        delete pGeomData;
}

void GFxASCharacter::UpdateAlphaFlag()
{
    if ((fabs(ColorTransform.M_[3][0]) < 0.001f) &&
        (fabs(ColorTransform.M_[3][1]) < 1.0f) )
        SetAlpha0Flag();
    else
        ClearAlpha0Flag();
}

void GFxASCharacter::CloneInternalData(const GFxASCharacter* src)
{
    GASSERT(src);
    if (src->pGeomData)
        SetGeomData(*src->pGeomData);
    EventHandlers = src->EventHandlers;
}

GASGlobalContext*   GFxASCharacter::GetGC() const
{
    return GetMovieRoot()->pGlobalContext;
}

void    GFxASCharacter::SetName(const GASString& name)
{
    if (!name.IsEmpty())
        ClearInstanceBasedNameFlag();

    if (pNameHandle)
    {
        pNameHandle->ChangeName(name, pParent);
        
        // TBD: Propagate update to all children ??
    }
    else
    {
        pNameHandle = *new GFxCharacterHandle(name, pParent, this);
    }
}

const GASString& GFxASCharacter::GetName() const
{
    GFxCharacterHandle* pnameHandle = GetCharacterHandle();
    if (pnameHandle)
        return pnameHandle->GetName();
    return GFxCharacter::GetName();
}

// Determines the absolute path of the character.
void    GFxASCharacter::GetAbsolutePath(GFxString *ppath) const
{
    if (pParent)
    {
        GASSERT(pParent != this);
        pParent->GetAbsolutePath(ppath);
        *ppath += ".";
        *ppath += GetName().ToCStr();
    }
    else
    {
        if (IsSprite())
        {
            char nameBuff[64] = "";
            gfc_sprintf(nameBuff, 64, "_level%d", ((const GFxSprite*)this)->GetLevel());
            *ppath = nameBuff;
        }
        else
        {
            // Non-sprite characters must have parents.
            GASSERT(0);
            ppath->Clear();
        }
    }
}

GFxCharacterHandle* GFxASCharacter::GetCharacterHandle() const
{
    if (!pNameHandle)
    {
        // Hacky, but this can happen.
        // Will clearing child handles recursively on parent release work better?
        if (IsSprite() && ((const GFxSprite*)this)->IsUnloaded())
        {
            GFC_DEBUG_WARNING(1, "GetCharacterHandle called on unloaded sprite");
            // Returns temp handle which is essentially useless.
            pNameHandle = *new GFxCharacterHandle(GetASEnvironment()->GetBuiltin(GASBuiltin_empty_), 0, 0);
            return pNameHandle;
        }

        // Create new instance names as necessary.
        GASString name(GetMovieRoot()->CreateNewInstanceName());
        // SetName logic duplicated to take advantage of 'mutable' pNameHandle.
        pNameHandle = *new GFxCharacterHandle(name, pParent, const_cast<GFxASCharacter*>(this));
    }

    return pNameHandle;
}


// Implement mouse-dragging for this pMovie.
void    GFxASCharacter::DoMouseDrag()
{
    GFxMovieRoot::DragState     st;
    GFxMovieRoot*   proot = GetMovieRoot();
    proot->GetDragState(&st);

    if (this == st.pCharacter)
    {
        // We're being dragged!
        int x, y, buttons;
        proot->GetMouseState(&x, &y, &buttons);
        
        GPointF worldMouse(GFC_PIXELS_TO_TWIPS(x), GFC_PIXELS_TO_TWIPS(y));
        GPointF parentMouse;
        Matrix  parentWorldMat;
        if (pParent)
            parentWorldMat = pParent->GetWorldMatrix();
        
        parentWorldMat.TransformByInverse(&parentMouse, worldMouse);
        // if (!st.LockCenter) is not necessary, because then st.CenterDelta == 0.
        parentMouse += st.CenterDelta;

        if (st.Bound)
        {           
            // Clamp mouse coords within a defined rectangle
            parentMouse.x = GTL::gclamp(parentMouse.x, st.BoundLT.x, st.BoundRB.x);
            parentMouse.y = GTL::gclamp(parentMouse.y, st.BoundLT.y, st.BoundRB.y);
        }

        // Once touched, object is no longer animated by the timeline
        SetAcceptAnimMoves(0);

        // Place our origin so that it coincides with the mouse coords
        // in our parent frame.
        SetStandardMember(M_x, TwipsToPixels(Double(parentMouse.x)), false);
        SetStandardMember(M_y, TwipsToPixels(Double(parentMouse.y)), false);
        
        //Matrix    local = GetMatrix();
        //local.M_[0][2] = parentMouse.x;
        //local.M_[1][2] = parentMouse.y;
        //SetMatrix(local);

    }
}


// *** Shared ActionScript methods.

// Depth implementation - same in MovieClip, Button, TextField.
void    GFxASCharacter::CharacterGetDepth(const GASFnCall& fn)
{
    GFxASCharacter* pcharacter = fn.ThisPtr->ToASCharacter();
    if (!pcharacter)    
         pcharacter = fn.Env->GetTarget();  
    GASSERT(pcharacter);

    // Depth always starts at -16384,
    // probably to allow user assigned depths to start at 0.
    fn.Result->SetInt(pcharacter->GetDepth() - 16384);
}

// *** Standard member support


struct GFxASCharacter_MemberTableType
{
    char *                          pName;
    GFxASCharacter::StandardMember  Id;
};

GFxASCharacter_MemberTableType GFxASCharacter_MemberTable[] =
{
    { "_x",             GFxASCharacter::M_x },  
    { "_y",             GFxASCharacter::M_y },
    { "_xscale",        GFxASCharacter::M_xscale },
    { "_yscale",        GFxASCharacter::M_yscale },
    { "_currentframe",  GFxASCharacter::M_currentframe },
    { "_totalframes",   GFxASCharacter::M_totalframes },
    { "_alpha",         GFxASCharacter::M_alpha },
    { "_visible",       GFxASCharacter::M_visible },
    { "_width",         GFxASCharacter::M_width },
    { "_height",        GFxASCharacter::M_height },
    { "_rotation",      GFxASCharacter::M_rotation },
    { "_target",        GFxASCharacter::M_target },
    { "_framesloaded",  GFxASCharacter::M_framesloaded },
    { "_name",          GFxASCharacter::M_name },
    { "_droptarget",    GFxASCharacter::M_droptarget },
    { "_url",           GFxASCharacter::M_url },
    { "_highquality",   GFxASCharacter::M_highquality },
    { "_focusrect",     GFxASCharacter::M_focusrect },
    { "_soundbuftime",  GFxASCharacter::M_soundbuftime },
    // SWF 5+.
    { "_quality",       GFxASCharacter::M_quality },
    { "_xmouse",        GFxASCharacter::M_xmouse },
    { "_ymouse",        GFxASCharacter::M_ymouse }, 

    // Extra shared properties which can have a default implementation.
    { "_parent",        GFxASCharacter::M_parent },     
    { "blendMode",      GFxASCharacter::M_blendMode },
    { "cacheAsBitmap",  GFxASCharacter::M_cacheAsBitmap },
    { "filters",        GFxASCharacter::M_filters },
    { "enabled",        GFxASCharacter::M_enabled },
    { "trackAsMenu",    GFxASCharacter::M_trackAsMenu },
    { "_lockroot",      GFxASCharacter::M_lockroot },
    { "tabEnabled",     GFxASCharacter::M_tabEnabled },
    { "tabIndex",       GFxASCharacter::M_tabIndex },
    { "useHandCursor",  GFxASCharacter::M_useHandCursor },  
    
    // Not shared.
    { "menu",           GFxASCharacter::M_menu },

    // MovieClip custom.
    { "focusEnabled",   GFxASCharacter::M_focusEnabled },
    { "tabChildren",    GFxASCharacter::M_tabChildren },
    { "transform",      GFxASCharacter::M_transform },
    { "scale9Grid",     GFxASCharacter::M_scale9Grid },

    // TextField custom.
    { "text",           GFxASCharacter::M_text },
    { "textWidth",      GFxASCharacter::M_textWidth },
    { "textHeight",     GFxASCharacter::M_textHeight },
    { "textColor",      GFxASCharacter::M_textColor },
    { "length",         GFxASCharacter::M_length },
    { "html",           GFxASCharacter::M_html },
    { "htmlText",       GFxASCharacter::M_htmlText },
    { "autoSize",       GFxASCharacter::M_autoSize },
    { "wordWrap",       GFxASCharacter::M_wordWrap },
    { "multiline",      GFxASCharacter::M_multiline },
    { "border",         GFxASCharacter::M_border },
    { "variable",       GFxASCharacter::M_variable },
    { "selectable",     GFxASCharacter::M_selectable },
    { "embedFonts",     GFxASCharacter::M_embedFonts },
    { "antiAliasType",  GFxASCharacter::M_antiAliasType },
    { "hscroll",        GFxASCharacter::M_hscroll },
    { "scroll",         GFxASCharacter::M_scroll },
    { "maxscroll",      GFxASCharacter::M_maxscroll },
    { "maxhscroll",     GFxASCharacter::M_maxhscroll },
    { "background",     GFxASCharacter::M_background },
    { "backgroundColor",GFxASCharacter::M_backgroundColor },
    { "borderColor",    GFxASCharacter::M_borderColor },
    { "bottomScroll",   GFxASCharacter::M_bottomScroll },
    { "type",           GFxASCharacter::M_type },
    { "maxChars",       GFxASCharacter::M_maxChars },
    { "condenseWhite",  GFxASCharacter::M_condenseWhite },
    { "mouseWheelEnabled", GFxASCharacter::M_mouseWheelEnabled },
    { "password",       GFxASCharacter::M_password },
    
    // GFx extensions
    { "shadowStyle",    GFxASCharacter::M_shadowStyle },
    { "shadowColor",    GFxASCharacter::M_shadowColor },
    { "hitTestDisable", GFxASCharacter::M_hitTestDisable },
    { "noTranslate",    GFxASCharacter::M_noTranslate },
    { "caretIndex",     GFxASCharacter::M_caretIndex },
    { "numLines",       GFxASCharacter::M_numLines   },
    { "verticalAutoSize",GFxASCharacter::M_verticalAutoSize },
    { "fontScaleFactor", GFxASCharacter::M_fontScaleFactor },
    { "verticalAlign",  GFxASCharacter::M_verticalAlign },
    { "textAutoSize",   GFxASCharacter::M_textAutoSize },
    { "useRichTextClipboard", GFxASCharacter::M_useRichTextClipboard },
    { "alwaysShowSelection",  GFxASCharacter::M_alwaysShowSelection },
    { "selectionBeginIndex",  GFxASCharacter::M_selectionBeginIndex },
    { "selectionEndIndex",    GFxASCharacter::M_selectionEndIndex },
    { "selectionBkgColor",    GFxASCharacter::M_selectionBkgColor },
    { "selectionTextColor",   GFxASCharacter::M_selectionTextColor },
    { "inactiveSelectionBkgColor",  GFxASCharacter::M_inactiveSelectionBkgColor },
    { "inactiveSelectionTextColor", GFxASCharacter::M_inactiveSelectionTextColor },
    { "noAutoSelection", GFxASCharacter::M_noAutoSelection },
    { "disableIME",      GFxASCharacter::M_disableIME },
    { "topmostLevel",    GFxASCharacter::M_topmostLevel },
    { "noAdvance",       GFxASCharacter::M_noAdvance },

    // Dynamic Text
    { "autoFit",        GFxASCharacter::M_autoFit },
    { "blurX",          GFxASCharacter::M_blurX },
    { "blurY",          GFxASCharacter::M_blurY },
    { "blurStrength",   GFxASCharacter::M_blurStrength },
    { "outline",        GFxASCharacter::M_outline },
    
    // Dynamic Text Shadow
    { "shadowAlpha",      GFxASCharacter::M_shadowAlpha },
    { "shadowAngle",      GFxASCharacter::M_shadowAngle },
    { "shadowBlurX",      GFxASCharacter::M_shadowBlurX },
    { "shadowBlurY",      GFxASCharacter::M_shadowBlurY },
    { "shadowDistance",   GFxASCharacter::M_shadowDistance },
    { "shadowHideObject", GFxASCharacter::M_shadowHideObject },
    { "shadowKnockOut",   GFxASCharacter::M_shadowKnockOut },
    { "shadowQuality",    GFxASCharacter::M_shadowQuality },
    { "shadowStrength",   GFxASCharacter::M_shadowStrength },
    { "shadowOutline",    GFxASCharacter::M_shadowOutline },

    // Done.
    { 0,  GFxASCharacter::M_InvalidMember }
};

void    GFxASCharacter::InitStandardMembers(GASGlobalContext *pcontext)
{
    GCOMPILER_ASSERT( (sizeof(GFxASCharacter_MemberTable)/sizeof(GFxASCharacter_MemberTable[0]))
                        == M_StandardMemberCount + 1);

    // Add all standard members.
    GFxASCharacter_MemberTableType* pentry;
    GASStringManager* pstrManager = pcontext->GetStringManager();

    pcontext->StandardMemberMap.set_capacity(M_StandardMemberCount);  

    for (pentry = GFxASCharacter_MemberTable; pentry->pName; pentry++)
    {
        GASString name(pstrManager->CreateConstString(pentry->pName, strlen(pentry->pName),
                                                      GASString::Flag_StandardMember));
        pcontext->StandardMemberMap.add(name, (SByte)pentry->Id);
    }
}

// Looks up a standard member and returns M_InvalidMember if it is not found.
GFxASCharacter::StandardMember  GFxASCharacter::GetStandardMemberConstant(const GASString& memberName) const
{
    SByte   memberConstant = M_InvalidMember; // Has to be signed or conversion is incorrect!
    if (memberName.IsStandardMember())
    {
        GASGlobalContext* pcontext = GetGC();
        pcontext->StandardMemberMap.get_CaseCheck(memberName, &memberConstant, IsCaseSensitive());
    }        
    
    GASSERT((memberConstant != M_InvalidMember) ? memberName.IsStandardMember() : 1);
    return (StandardMember) memberConstant;
}

GFxASCharacter::GeomDataType& GFxASCharacter::GetGeomData(GFxASCharacter::GeomDataType& geomData) const
{
    if (!pGeomData)
    {
        // fill GeomData using Matrix_1
        const Matrix& m = GetMatrix();
        geomData.X = int(m.GetX());
        geomData.Y = int(m.GetY());
        geomData.XScale = m.GetXScale()*(Double)100.;
        geomData.YScale = m.GetYScale()*(Double)100.;
        geomData.Rotation = (m.GetRotation()*(Double)180.)/GFC_MATH_PI;
        geomData.OrigMatrix = Matrix_1;
    }
    else
    {
        geomData = *pGeomData;
    }
    return geomData;
}

void    GFxASCharacter::SetGeomData(const GeomDataType& gd)
{
    if (pGeomData)
        *pGeomData = gd;
    else
        pGeomData = new GeomDataType(gd);
}

void    GFxASCharacter::SetAcceptAnimMoves(bool accept)
{ 
    if (!pGeomData)
    {
        GeomDataType geomData;
        SetGeomData(GetGeomData(geomData));
    }
    SetAcceptAnimMovesFlag(accept); 
    SetDirtyFlag();
}

// BlendMode lookup table.
// Should be moved elsewhere so that it can apply to buttons, etc.
static char * GFx_BlendModeNames[1+ 14] =
{
    "normal",   // 0?
    "normal",   
    "layer",
    "multiply",
    "screen",
    "lighten",
    "darken",
    "difference",
    "add",
    "subtract",
    "invert",
    "alpha",
    "erase",
    "overlay",
    "hardlight"
};

void GFxASCharacter_MatrixScaleAndRotate2x2(GMatrix2D& m, Float sx, Float sy, Float radians)
{
    Float   cosAngle = cosf(radians);
    Float   sinAngle = sinf(radians);
    Float   x00 = m.M_[0][0];
    Float   x01 = m.M_[0][1];
    Float   x10 = m.M_[1][0];
    Float   x11 = m.M_[1][1];

    m.M_[0][0] = (x00*cosAngle-x10*sinAngle)*sx;
    m.M_[0][1] = (x01*cosAngle-x11*sinAngle)*sy;
    m.M_[1][0] = (x00*sinAngle+x10*cosAngle)*sx;
    m.M_[1][1] = (x01*sinAngle+x11*cosAngle)*sy;
}

// Handles built-in members. Return 0 if member is not found or not supported.
bool    GFxASCharacter::SetStandardMember(StandardMember member, const GASValue& val, bool opcodeFlag)
{   
    if (opcodeFlag && ((member < M_BuiltInProperty_Begin) || (member > M_BuiltInProperty_End)))
    {
        LogScriptError("Invalid SetProperty request, property number %d\n", member);
        return 0;
    }
    // Make sure that this character class supports the constant.
    if (member == M_InvalidMember)
        return 0;
    if (!((member <= M_SharedPropertyEnd) && (GetStandardMemberBitMask() & (1<<member))))
        return 0;
    GASEnvironment* pEnv = GetASEnvironment ();

    switch(member)
    {
    case M_x:
        {
            GASNumber xval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(xval))
                return 1;
            if (GASNumberUtil::IsNEGATIVE_INFINITY(xval) || GASNumberUtil::IsPOSITIVE_INFINITY(xval))
                xval = 0;
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix  m = GetMatrix();
            pGeomData->X = int(floor(PixelsToTwips(xval)));
            m.M_[0][2] = (Float) pGeomData->X;
            SetMatrix(m);
            return 1;
        }

    case M_y:
        {
            GASNumber yval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(yval))
                return 1;
            if (GASNumberUtil::IsNEGATIVE_INFINITY(yval) || GASNumberUtil::IsPOSITIVE_INFINITY(yval))
                yval = 0;
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix  m = GetMatrix();
            pGeomData->Y = int(floor(PixelsToTwips(yval)));
            m.M_[1][2] = (Float) pGeomData->Y;
            SetMatrix(m);
            return 1;
        }

    case M_xscale:
        {
            Double newXScale = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(newXScale) ||
                GASNumberUtil::IsNEGATIVE_INFINITY(newXScale) || GASNumberUtil::IsPOSITIVE_INFINITY(newXScale))
            {
                return 1;
            }
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            const Matrix& chm = GetMatrix();
            Matrix m = pGeomData->OrigMatrix;
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Double origXScale = m.GetXScale();
            pGeomData->XScale = newXScale;
            if (origXScale == 0)
            {
                newXScale = 0;
                origXScale = 1;
            }

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(newXScale/(origXScale*100.)), 
                Float(pGeomData->YScale/(m.GetYScale()*100.)),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            SetMatrix(m);
            return 1;
        }

    case M_yscale:
        {
            Double newYScale = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(newYScale) ||
                GASNumberUtil::IsNEGATIVE_INFINITY(newYScale) || GASNumberUtil::IsPOSITIVE_INFINITY(newYScale))
            {
                return 1;
            }
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            const Matrix& chm = GetMatrix();
            Matrix m = pGeomData->OrigMatrix;
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Double origYScale = m.GetYScale();
            pGeomData->YScale = newYScale;
            if (origYScale == 0)
            {
                newYScale = 0;
                origYScale = 1;
            }

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(pGeomData->XScale/(m.GetXScale()*100.)), 
                Float(newYScale/(origYScale* 100.)),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            SetMatrix(m);
            return 1;
        }

    case M_rotation:
        {
            GASNumber rval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(rval))
                return 1;
            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Double r = fmod((Double)rval, (Double)360.);
            if (r > 180)
                r -= 360;
            else if (r < -180)
                r += 360;
            pGeomData->Rotation = r;

            const Matrix& chm = GetMatrix();
            Matrix m = pGeomData->OrigMatrix;
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Double origRotation = m.GetRotation();

            // remove old rotation by rotate back and add new one

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(pGeomData->XScale/(m.GetXScale()*100.)), 
                Float(pGeomData->YScale/(m.GetYScale()*100.)),
                Float(-origRotation + r * GFC_MATH_PI / 180.));

            SetMatrix(m);
            return 1;
        }

    case M_width:
        {
            // MA: Width/Height modification in Flash is unusual in that it performs
            // relative axis scaling in the x local axis (width scaling) and y local
            // axis (height scaling). The resulting width after scaling does not
            // actually equal the original, instead, it is related to rotation!
            // AB: I am second on that! Looks like a bug in Flash.

            // NOTE: Although it works for many cases, this is still not correct. Modification 
            // of width seems very strange (if not buggy) in Flash.
            GASNumber wval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(wval) || GASNumberUtil::IsNEGATIVE_INFINITY(wval))
                return 1;
            if (GASNumberUtil::IsPOSITIVE_INFINITY(wval))
                wval = 0;

            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix m = pGeomData->OrigMatrix;
            const Matrix& chm = GetMatrix();
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Matrix im = m;
            im.AppendRotation(Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            Float oldWidth      = GetBounds(im).Width(); // width should be in local coords!
            Float newWidth      = Float(PixelsToTwips(wval));
            Float multiplier    = (fabsf(oldWidth) > 1e-6f) ? (newWidth / oldWidth) : 0.0f;

            Double origXScale = m.GetXScale();
            Double newXScale = origXScale * multiplier * 100;
            pGeomData->XScale = newXScale;
            if (origXScale == 0)
            {
                newXScale = 0;
                origXScale = 1;
            }
        
            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(fabs(newXScale/(origXScale*100.))), 
                Float(fabs(pGeomData->YScale/(m.GetYScale()*100.))),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            pGeomData->XScale = fabs(pGeomData->XScale);
            pGeomData->YScale = fabs(pGeomData->YScale);

            SetMatrix(m);
            return 1;
        }

    case M_height:
        {
            GASNumber hval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(hval) || GASNumberUtil::IsNEGATIVE_INFINITY(hval))
                return 1;
            if (GASNumberUtil::IsPOSITIVE_INFINITY(hval))
                hval = 0;

            SetAcceptAnimMoves(0);
            GASSERT(pGeomData);

            Matrix m = pGeomData->OrigMatrix;
            const Matrix& chm = GetMatrix();
            m.M_[0][2] = chm.M_[0][2];
            m.M_[1][2] = chm.M_[1][2];

            Matrix im = m;
            im.AppendRotation(Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            Float oldHeight     = GetBounds(im).Height(); // height should be in local coords!
            Float newHeight     = Float(PixelsToTwips(hval));
            Float multiplier    = (fabsf(oldHeight) > 1e-6f) ? (newHeight / oldHeight) : 0.0f;

            Double origYScale = m.GetYScale();
            Double newYScale = origYScale * multiplier * 100;;
            pGeomData->YScale = newYScale;
            if (origYScale == 0)
            {
                newYScale = 0;
                origYScale = 1;
            }

            GFxASCharacter_MatrixScaleAndRotate2x2(m,
                Float(fabs(pGeomData->XScale/(m.GetXScale()*100.))), 
                Float(fabs(newYScale/(origYScale* 100.))),
                Float(-m.GetRotation() + pGeomData->Rotation * GFC_MATH_PI / 180.));

            pGeomData->XScale = fabs(pGeomData->XScale);
            pGeomData->YScale = fabs(pGeomData->YScale);

            SetMatrix(m);
            return 1;
        }
    

    case M_alpha:
        {
            GASNumber aval = val.ToNumber(pEnv);
            if (val.IsUndefined() || GASNumberUtil::IsNaN(aval))
                return 1;

            // Set alpha modulate, in percent.
            Cxform  cx = GetCxform();
            cx.M_[3][0] = Float(aval / 100.);
            SetCxform(cx);
            SetAcceptAnimMoves(0);
            return 1;
        }

    case M_visible:
        {
            SetVisible(val.ToBool(pEnv));           
            // Setting visibility does not affect AnimMoves.
            return 1;
        }

    case M_blendMode:
        {
            // NOTE: Setting "blendMode" in Flash does NOT disconnect object from time-line,
            // so just setting the value is the correct behavior.
            if (val.GetType() == GASValue::STRING)
            {
                GASString  asstr(val.ToString(pEnv));
                GFxString  str = asstr.ToCStr();
                for (UInt i=1; i<(sizeof(GFx_BlendModeNames)/sizeof(GFx_BlendModeNames[0])); i++)
                {
                    if (str == GFx_BlendModeNames[i])
                    {
                        SetBlendMode((BlendType) i);
                        return 1;
                    }
                }
            }
            else
            {
                SInt mode = (SInt)val.ToNumber(pEnv);
                mode = GTL::gmax<SInt>(GTL::gmin<SInt>(mode,14),1);
                SetBlendMode((BlendType) mode);
            }
            return 1;
        }


    case M_name:
        {
            SetName(val.ToString(pEnv));
            return 1;
        }
        
    case M_enabled:
        {
            SetEnabledFlag(val.ToBool(pEnv));
            return 1;
        }       

    case M_trackAsMenu:
        {
            SetTrackAsMenuFlag(val.ToBool(pEnv));
            return 1;
        }

    // Read-only properties.
    case M_droptarget:
    case M_target:
    case M_url:
    case M_xmouse:
    case M_ymouse:  
    case M_parent: // TBD: _parent is not documented as read only. Can it be changed ??

    // These are only implemented for MovieClip. Technically, they should not be here;
    // however, they are Flash built-ins. Since they are read-only, we can handle them here.
    case M_currentframe:
    case M_totalframes:
    case M_framesloaded:        

        pEnv->LogScriptWarning("Attempt to write read-only property %s.%s, ignored\n",
                         GetName().ToCStr(), GFxASCharacter_MemberTable[member].pName);
        return 1;

    case M_tabEnabled:
        SetTabEnabledFlag(val.ToBool(GetASEnvironment()));
        return 1;

    case M_tabIndex:
        TabIndex = int(val.ToNumber(GetASEnvironment()));
        return 1;

    case M_focusrect:
        SetFocusRectFlag(val.ToBool(GetASEnvironment()));
        SetDirtyFlag();
        return 1;

    // Un-implemented properties:
    case M_soundbuftime:
    case M_highquality:
    case M_quality:
        // We have a default get below, so return 1.
        return 1;

    case M_useHandCursor:
        SetUseHandCursorFlag(val.ToBool(GetASEnvironment()));
        return 1;

    case M_cacheAsBitmap:
    case M_filters:
    case M_lockroot:
        // Allow at least property assignment in map for now.
        return 0;

    default:
        // Property not handled, fall out.
        return 0;
    }
}

bool    GFxASCharacter::GetStandardMember(StandardMember member, GASValue* val, bool opcodeFlag) const
{
    if (opcodeFlag && ((member < M_BuiltInProperty_Begin) || (member > M_BuiltInProperty_End)))
    {
        GetASEnvironment()->LogScriptError("Invalid GetProperty query, property number %d\n", member);
        return 0;
    }

    // Make sure that this character class supports the constant.
    if (member == M_InvalidMember)
        return 0;
    if (!((member <= M_SharedPropertyEnd) && (GetStandardMemberBitMask() & (1<<member))))
        return 0;

    switch(member)
    {
    case M_x:
        {           
            GeomDataType geomData;
            val->SetNumber(TwipsToPixels(Double(GetGeomData(geomData).X)));
            return 1;
        }
    case M_y:       
        {       
            GeomDataType geomData;
            val->SetNumber(TwipsToPixels(Double(GetGeomData(geomData).Y)));
            return 1;
        }

    case M_xscale:      
        {           
            GeomDataType geomData;
            val->SetNumber(GetGeomData(geomData).XScale);   // result in percent
            return 1;
        }
    case M_yscale:      
        {           
            GeomDataType geomData;
            val->SetNumber(GetGeomData(geomData).YScale);   // result in percent
            return 1;
        }

    case M_width:   
        {
            //GRectF    boundRect = GetLevelBounds();
            //!AB: width and height of nested movieclips returned in the coordinate space of its parent!
            GRectF  boundRect = GetBounds(GetMatrix());
            val->SetNumber(TwipsToPixels(floor((Double)boundRect.Width())));
            return 1;
        }
    case M_height:      
        {
            //GRectF    boundRect = GetLevelBounds();
            //!AB: width and height of nested movieclips returned in the coordinate space of its parent!
            GRectF  boundRect = GetBounds(GetMatrix());
            val->SetNumber(TwipsToPixels(floor((Double)boundRect.Height())));
            return 1;
        }

    case M_rotation:
        {
            GeomDataType geomData;
            // Verified against Macromedia player using samples/TestRotation.Swf
            val->SetNumber(GetGeomData(geomData).Rotation);
            return 1;
        }

    case M_alpha:   
        {
            // Alpha units are in percent.
            val->SetNumber(GetCxform().M_[3][0] * 100.F);
            return 1;
        }

    case M_visible: 
        {
            val->SetBool(GetVisible());
            return 1;
        }

    case M_blendMode:
        {
            val->SetString(GetASEnvironment()->CreateString(GFx_BlendModeNames[GetBlendMode()]));
            // Note: SWF 8 can sometimes report "undefined". TBD.
            return 1;
        }

    case M_name:
        {
            val->SetString(GetName());
            return 1;
        }
        
    case M_enabled:
        {           
            val->SetBool(GetEnabled());
            return 1;
        }   

    case M_trackAsMenu:
        {
            val->SetBool(GetTrackAsMenu());
            return 1;
        }

    case M_target:
        {
            // Full path to this object; e.G. "/sprite1/sprite2/ourSprite"
            GFxString            name;
            GPtr<GFxASCharacter> root = GetASRootMovie();

            for (const GFxASCharacter* p = this; p != 0 && p != root; p = p->GetParent())
            {
                const GASString& cname = p->GetName();
                name.Insert (cname.ToCStr(), 0);
                name.Insert ("/", 0);
            }
            val->SetString(GetASEnvironment()->CreateString(name));
            return 1;
        }

    case M_parent:
        {
            if (pParent)
                val->SetAsCharacter(pParent);
            else
                val->SetUndefined();
            return 1;
        }

    case M_droptarget:
        {
            // Absolute path in slash syntax where we were last Dropped (?)
            // @@ TODO
            //val->SetString(GetASEnvironment()->GetBuiltin(GASBuiltin__root));
            //return 1;

            // Crude implementation..
            val->SetUndefined();
            GFxMovieRoot* proot = GetASEnvironment()->GetMovieRoot();
            GPointF mousePos;
            int x, y, buttons;          
            proot->GetMouseState(&x, &y, &buttons);
            mousePos.x = (Float)PixelsToTwips(x);
            mousePos.y = (Float)PixelsToTwips(y);
            GFxASCharacter* ptopCh = proot->GetTopMostEntity(mousePos, true, this);
            GFxString            name;
            GPtr<GFxASCharacter> root = GetASRootMovie();
            for (const GFxASCharacter* p = ptopCh; p != 0 && p != root; p = p->GetParent())
            {
                const GASString& cname = p->GetName();
                name.Insert (cname.ToCStr(), 0);
                name.Insert ("/", 0);
            }
            val->SetString(GetASEnvironment()->CreateString(name));
            return 1;
        }

    case M_url: 
        {
            // our URL.
            GFxString urlStr = GetResourceMovieDef()->GetFileURL();
            GFxString urlStr1;

            for (UPInt i = 0, n  = urlStr.GetSize(); i < n; ++i)
            {
                if (urlStr[i] == '\\')
                    urlStr[i] = '/';

            }
            GASGlobalContext::EscapePath(urlStr.ToCStr(), urlStr.GetSize(), &urlStr1);
            val->SetString(GetASEnvironment()->CreateString(urlStr1));
            return 1;
        }

    case M_highquality: 
        {
            // Whether we're in high quality mode or not.
            val->SetBool(true);
            return 1;
        }
    case M_quality:
        {
            val->SetString(GetASEnvironment()->CreateString("HIGH"));
            return 1;
        }

    case M_focusrect:   
        {
            // Is a yellow rectangle visible around a focused GFxASCharacter Clip (?)
            if (IsFocusRectFlagDefined())
                val->SetBool(IsFocusRectFlagTrue());
            else
                val->SetNull();
            return 1;
        }

    case M_soundbuftime:        
        {
            // Number of seconds before sound starts to GFxStream.
            val->SetNumber(0.0);
            return 1;
        }

    case M_xmouse:  
        {
            // Local coord of mouse IN PIXELS.
            int x, y, buttons;          
            GetMovieRoot()->GetMouseState(&x, &y, &buttons);

            Matrix  m = GetWorldMatrix();
            GPointF a(GFC_PIXELS_TO_TWIPS(x), GFC_PIXELS_TO_TWIPS(y));
            GPointF b;
            
            m.TransformByInverse(&b, a);
            val->SetNumber(TwipsToPixels(b.x));
            return 1;
        }

    case M_ymouse:  
        {
            // Local coord of mouse IN PIXELS.
            int x, y, buttons;          
            GetMovieRoot()->GetMouseState(&x, &y, &buttons);

            Matrix  m = GetWorldMatrix();
            GPointF a(GFC_PIXELS_TO_TWIPS(x), GFC_PIXELS_TO_TWIPS(y));
            GPointF b;
            
            m.TransformByInverse(&b, a);
            val->SetNumber(TwipsToPixels(b.y));
            return 1;
        }

    case M_tabEnabled:
        if (IsTabEnabledFlagDefined())
            val->SetBool(IsTabEnabledFlagTrue());
        else
            val->SetUndefined();
        return 1;

    case M_tabIndex:
        val->SetNumber((GASNumber)TabIndex);
        return 1;

    case M_useHandCursor:
        if (!IsUseHandCursorFlagDefined())
            return 0;
        val->SetBool(IsUseHandCursorFlagTrue());
        return 1;

    // Un-implemented properties:
    case M_cacheAsBitmap:
    case M_filters:
    case M_lockroot:
        // Allow at least property assignment in map for now.
        return 0;

    // These are only implemented for MovieClip. Technically, they should not be here;
    // however, they are Flash built-ins. Return 0 so that GFxSprite properly handles them.
    case M_currentframe:
    case M_totalframes:
    case M_framesloaded:
        return 0;   

    default:
        break;
    }

    return 0;
}



// Duplicate the object with the specified name and add it with a new name at a new depth.
GFxASCharacter* GFxASCharacter::CloneDisplayObject(const GASString& newname, SInt depth, const GASObjectInterface *psource)
{
    GFxSprite* pparent = GetParent()->ToSprite();
    if (!pparent)
        return 0;
    if ((depth < 0) || (depth > (2130690045 + 16384)))
        return 0;

    // Clone us.
    GTL::garray<GFxSwfEvent*>   DummyEventHandlers;
    GFxCharPosInfo pos( GetId(), depth,
                        1, GetCxform(), 1, GetMatrix(),
                        GetRatio(), GetClipDepth());

    // true: replace if depth is occupied
    return pparent->AddDisplayObject(pos, newname, DummyEventHandlers, psource, true, 
                        GFC_MAX_UINT, false, 0, this)->ToASCharacter();
}

// Remove the object with the specified name.
void    GFxASCharacter::RemoveDisplayObject()
{
    if (!GetParent())
        return;
    GFxSprite* pparent = GetParent()->ToSprite();
    if (!pparent)
        return; 
    pparent->RemoveDisplayObject(GetDepth(), GetId());
}


void    GFxASCharacter::CopyPhysicalProperties(GFxASCharacter *poldChar)
{
    // Copy physical properties, used by loadMovie().
    SetDepth(poldChar->GetDepth());
    SetCxform(poldChar->GetCxform());
    SetMatrix(poldChar->GetMatrix());
    EventHandlers = poldChar->EventHandlers;
    if (poldChar->pGeomData)
        SetGeomData(*poldChar->pGeomData);

    // Re-link all ActionScript references.
    pNameHandle = poldChar->pNameHandle;
    poldChar->pNameHandle = 0;
    pNameHandle->pCharacter = this;
}


static GINLINE GFxEventId GFx_TreatEventId(const GFxEventId& id)
{
    // for keyDown/keyUp search by id only (w/o KeyCode/AsciiCode)
    if (id.Id == GFxEventId::Event_KeyDown || id.Id == GFxEventId::Event_KeyUp)
        return GFxEventId(id.Id); 
    return id;
}

bool    GFxASCharacter::HasEventHandler(const GFxEventId& id) const
{ 
    const EventsArray* evts = EventHandlers.get(GFx_TreatEventId(id));
    return evts != 0; 
}

bool    GFxASCharacter::InvokeEventHandlers(GASEnvironment* penv, const GFxEventId& id)
{ 
    const EventsArray* evts = EventHandlers.get(GFx_TreatEventId(id));
    if (evts)
    {
        for (UPInt i = 0, n = evts->size(); i < n; ++i)
        {
            const GASValue& method = (*evts)[i];
            GAS_Invoke0(method, NULL, this, penv);
        }
        return true;
    }
    return false; 
}

void    GFxASCharacter::SetSingleEventHandler(const GFxEventId& id, const GASValue& method)
{ 
    GASSERT(id.GetEventsCount() == 1);
    EventsArray* evts = EventHandlers.get(GFx_TreatEventId(id));
    if (!evts)
    {
        EventsArray ea;
        ea.push_back(method);
        EventHandlers.set(id, ea); 
    }
    else
    {
        evts->push_back(method);
    }
}

void    GFxASCharacter::SetEventHandlers(const GFxEventId& id, const GASValue& method)
{ 
    UInt numOfEvents = id.GetEventsCount();
    GASSERT(numOfEvents > 0);
    if (numOfEvents == 1)
    {
        SetSingleEventHandler(id, method);
    }
    else
    {
        // need to create multiple event handlers with the same method. This is
        // necessary when "on(release, releaseOutside)" kind of handlers is used.
        for (UInt i = 0, mask = 0x1; i < numOfEvents; mask <<= 1)
        {
            if (id.Id & mask)
            {
                ++i;
                GFxEventId copied = id;
                copied.Id = mask;
                SetSingleEventHandler(copied, method);
            }
        }
    }
}


// Execute this even immediately (called for processing queued event actions).
bool    GFxASCharacter::ExecuteEvent(const GFxEventId& id)
{
    // Keep GASEnvironment alive during any method calls!
    GPtr<GFxASCharacter> thisPtr(this);
    GASEnvironment* env = GetASEnvironment();
    // Keep target of GASEnvironment alive during any method calls!
    // note, that env->GetTarget() is not always equal to this (for buttons, for example)
    GPtr<GFxASCharacter> targetPtr(env->GetTarget());

    bool        handlerFound = 0;
    GASValue    method;

    // First, check for built-in onClipEvent() handler.
    if (HasEventHandler(id))
    {
        // Dispatch.
        InvokeEventHandlers(env, id);
        //GAS_Invoke0(method, env, this);
        handlerFound = 1;
    }

    // Check for member function, it is called after onClipEvent(). 
    // In ActionScript 2.0, event method names are CASE SENSITIVE.
    // In ActionScript 1.0, event method names are CASE INSENSITIVE.    
    GASString    methodName(id.GetFunctionName(env->GetSC()));
    if (methodName.GetSize() > 0)
    {           
        if (GetMemberRaw(env->GetSC(), methodName, &method))
        {
            if (!method.IsNull())
            {
                if (env->IsVerboseAction())
                    env->LogAction("\n!!! ExecuteEvent started '%s' = %p\n", methodName.ToCStr(), 
                                    method.ToFunction().GetObjectPtr());

                GAS_Invoke0(method, NULL, this, env);
                
                if (env->IsVerboseAction())
                    env->LogAction("!!! ExecuteEvent finished '%s' = %p\n\n", methodName.ToCStr(), 
                                    method.ToFunction().GetObjectPtr());
            }
            handlerFound = 1;
        }
    }

    return handlerFound;
}

bool    GFxASCharacter::ExecuteFunction(const GASFunctionRef& function, const GASValueArray& params)
{
    if (function.GetObjectPtr() != 0)
    {
        GASValue result;
        GASEnvironment* penv = GetASEnvironment();
        GASSERT(penv);

        int nArgs = (int)params.size();
        if (nArgs > 0)
        {
            for (int i = nArgs - 1; i >= 0; --i)
                penv->Push(params[i]);
        }
        function(GASFnCall(&result, this, penv, nArgs, penv->GetTopIndex()));
        if (nArgs > 0)
        {
            penv->Drop(nArgs);
        }
        return true;
    }
    return false;
}

bool    GFxASCharacter::ExecuteCFunction(const GASCFunctionPtr function, const GASValueArray& params)
{
    if (function != 0)
    {
        GASValue result;
        GASEnvironment* penv = GetASEnvironment();
        GASSERT(penv);

        int nArgs = (int)params.size();
        if (nArgs > 0)
        {
            for (int i = nArgs - 1; i >= 0; --i)
                penv->Push(params[i]);
        }
        function(GASFnCall(&result, this, penv, nArgs, penv->GetTopIndex()));
        if (nArgs > 0)
        {
            penv->Drop(nArgs);
        }
        return true;
    }
    return false;
}

// set this.__proto__ = psrcObj->prototype
void    GFxASCharacter::SetProtoToPrototypeOf(GASObjectInterface* psrcObj)
{
    GASValue prototype;
    GASStringContext *psc = GetASEnvironment()->GetSC();
    if (psrcObj->GetMemberRaw(psc, psc->GetBuiltin(GASBuiltin_prototype), &prototype))
        Set__proto__(psc, prototype.ToObject());
}

void    GFxASCharacter::VisitMembers(GASStringContext *psc, MemberVisitor *pvisitor, UInt visitFlags) const
{
    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
        asObj->VisitMembers(psc, pvisitor, visitFlags); 
}


// Delete a member field, if not read-only. Return true if deleted.
bool    GFxASCharacter::DeleteMember(GASStringContext *psc, const GASString& name)
{
    if (name.IsStandardMember())
    {    
        StandardMember member = GetStandardMemberConstant(name);
        if (member != M_InvalidMember && (member <= M_SharedPropertyEnd))
        {
            if (GetStandardMemberBitMask() & (1<<member))
            {
                switch(member)
                {
                    case M_useHandCursor:
                        UndefineUseHandCursorFlag();
                        return true;

                    default:
                        return false;
                }
            }
        }
    }

    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
    {
        if (asObj->DeleteMember(psc, name))
            return true;
    }
    return false;
}

bool    GFxASCharacter::SetMemberFlags(GASStringContext *psc, const GASString& name, const UByte flags)
{
    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
        return asObj->SetMemberFlags(psc, name, flags); 
    //TODO: Standard members?... (AB)
    return false;
}

bool    GFxASCharacter::SetMember(GASEnvironment* penv, const GASString& name, const GASValue& val, const GASPropFlags& flags)
{    
    if (name.IsStandardMember())
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (SetStandardMember(member, val, 0))
            return true;

        switch(member)
        {
        case M_topmostLevel:
            if (GetASEnvironment()->CheckExtensions())
            {
                SetTopmostLevelFlag(val.ToBool(GetASEnvironment())); 
                if (IsTopmostLevelFlagSet())
                    GetMovieRoot()->AddTopmostLevelCharacter(this);
                else
                    GetMovieRoot()->RemoveTopmostLevelCharacter(this);
            }
            break; // do not return - need to save it to members too

        case M_noAdvance:
            if (GetASEnvironment()->CheckExtensions())
            {
                SetNoAdvanceLocalFlag(val.ToBool(GetASEnvironment()));
            }
            break; // do not return - need to save it to members too

        default:
            break;
        }
    }

    // a special case for setting __proto__ to a movieclip, button or textfield
    // need to set it into the character, not to the AS object, since
    // Get__proto__ is no more virtual.
    if (penv->IsCaseSensitive())
    {
        if (name == penv->GetBuiltin(GASBuiltin___proto__))
        {   
            if (val.IsSet())
                Set__proto__(penv->GetSC(), val.ToObject());
        }
    }
    else
    {
        name.ResolveLowercase();
        if (penv->GetBuiltin(GASBuiltin___proto__).CompareBuiltIn_CaseInsensitive_Unchecked(name))
        {   
            if (val.IsSet())
                Set__proto__(penv->GetSC(), val.ToObject());
        }
    }
    // Note that MovieClipObject will also track setting of button
    // handlers, i.e. 'onPress', etc.
    GASObject* asObj = GetASObject();
    if (asObj) 
        return asObj->SetMember(penv, name, val, flags);
    return false;
}

bool GFxASCharacter::HasMember(GASStringContext *psc, const GASString& name, bool inclPrototypes)
{
    if (name.IsStandardMember())
    {
        StandardMember member = GetStandardMemberConstant(name);
        if (member != M_InvalidMember && (member <= M_SharedPropertyEnd))
        {
            if (GetStandardMemberBitMask() & (1<<member))
                return true;
        }
    }    

    GPtr<GASObject> asObj = GetASObject();
    if (asObj) 
        return asObj->HasMember(psc, name, inclPrototypes); 
    return false;
}

bool GFxASCharacter::FindMember(GASStringContext *psc, const GASString& name, GASMember* pmember)
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->FindMember(psc, name, pmember) : false;
}

void            GFxASCharacter::Set__proto__(GASStringContext *psc, GASObject* protoObj)
{
    GPtr<GASObject> obj = GetASObject();
    if (obj)
    {
        obj->Set__proto__(psc, protoObj);
        pProto = protoObj;
    }
}

/*GASObject*      GFxASCharacter::Get__proto__()
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->Get__proto__() : NULL;
}*/

bool GFxASCharacter::InstanceOf(GASEnvironment* penv, const GASObject* prototype, bool inclInterfaces) const
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->InstanceOf(penv, prototype, inclInterfaces) : false;
}

bool GFxASCharacter::Watch(GASStringContext *psc, const GASString& prop, const GASFunctionRef& callback, const GASValue& userData)
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->Watch(psc, prop, callback, userData) : false;
}

bool GFxASCharacter::Unwatch(GASStringContext *psc, const GASString& prop)
{
    GPtr<GASObject> obj = GetASObject();
    return (obj) ? obj->Unwatch(psc, prop) : false;
}

bool GFxASCharacter::IsFocusRectEnabled() const
{
    if (IsFocusRectFlagDefined())
        return IsFocusRectFlagTrue();
    GFxASCharacter* prootMovie = this->GetASRootMovie();
    if (prootMovie != this)
        return prootMovie->IsFocusRectEnabled();
    return true;
}

void GFxASCharacter::OnFocus(FocusEventType event, GFxASCharacter* oldOrNewFocusCh, GFxFocusMovedType)
{
    GASValue focusMethodVal;
    
    GASEnvironment* penv = GetASEnvironment();
    if (!penv) // penv == NULL if object is removed from its parent's display list
        return;
    GASStringContext* psc = penv->GetSC();
    GASString   focusMethodName(psc->GetBuiltin((event == SetFocus) ?
                                GASBuiltin_onSetFocus : GASBuiltin_onKillFocus));   
    if (GetMemberRaw(psc, focusMethodName, &focusMethodVal))
    {
        GASFunctionRef focusMethod = focusMethodVal.ToFunction();
        if (!focusMethod.IsNull())
        {
            if (oldOrNewFocusCh)
                penv->Push(GASValue(oldOrNewFocusCh));
            else
                penv->Push(GASValue::NULLTYPE);
            GASValue result;
            focusMethod(GASFnCall(&result, GASValue(this), penv, 1, penv->GetTopIndex()));
            penv->Drop(1);
        }
    }
}

// ***** GFxPointTestCache

GFxPointTestCache::GFxPointTestCache()
{
    Root.pHead = &Root;
    Root.pTail = &Root;
    // Start frames at 0.
    Frame   = 0;
    pUnused = 0;
}

GFxPointTestCache::~GFxPointTestCache()
{
    // Release cached nodes.
    while(Root.pNext != &Root)
        ReleaseNode(Root.pNext);
    // Release unused nodes.
    while(pUnused)
    {
        CacheNode *p = pUnused;
        pUnused = pUnused->pNext;
        delete p;
    }
}

// Adds node to head and returns it.
GFxPointTestCache::CacheNode* GFxPointTestCache::AddNode(const GFxPointTestCacheProvider *pchar)
{
    CacheNode *pnode;

    if (pUnused)
    {
        pnode   = pUnused;
        pUnused = pUnused->pNext;
    }
    else
    {
        pnode = new CacheNode;
        if (!pnode)
            return 0;
    }

    GASSERT(pchar->pHitTestNode == 0);

    // Insert into linked list.
    pnode->pCache       = this;
    pnode->Frame        = Frame;
    pnode->pProvider    = pchar;
    pchar->pHitTestNode = pnode;
    pnode->ShapesValid  = 0;

    pnode->InsertAtHead(&Root);
    return pnode;
}

void   GFxPointTestCache::ReleaseNode(GFxPointTestCache::CacheNode* pnode)
{
    pnode->pProvider->pHitTestNode = 0;
    pnode->pProvider = 0;
    pnode->Remove();

    // Clear shapes in node.
    if (pnode->Shapes.size() > 0)
    {
        pnode->Shapes.resize(1);
        pnode->Shapes[0].RemoveAll();
    }    

    // Add to unused pool.
    pnode->pNext = pUnused;
    pUnused = pnode;
}

// Increments frame and discards stale elements from tail.
void    GFxPointTestCache::NextFrame()
{
    Frame++;

    // Constant: controls after how many frames we discard HT data.
    enum { DiscardFrameCount = 5 };

    // Discard elements that were there many frames ago.
    CacheNode *p = (CacheNode*)Root.pTail;
    while(p != &Root)
    {
        // NOTE: This logic is UInt wrap-around safe.
        UInt delta = (Frame - p->Frame);
        if (delta > DiscardFrameCount)
        {
            ReleaseNode(p);
            p = (CacheNode*)Root.pTail;
        }
        else
            break;
    }
}


// ***** GFxGenericCharacter

GFxGenericCharacter::GFxGenericCharacter(GFxCharacterDef* pdef, GFxASCharacter* pparent, GFxResourceId id)
: GFxCharacter(pparent, id), GFxPointTestCacheProvider(&pparent->GetMovieRoot()->PointTestCache)
{
    pDef = pdef;
    GASSERT(pDef);
}

    
GFxASCharacter*  GFxGenericCharacter::GetTopMostMouseEntity(const GPointF &pt, bool testAll, const GFxASCharacter* ignoreMC)
{   
    if (!GetVisible())
        return 0;

    Matrix  m = GetMatrix();
    GPointF p;          
    m.TransformByInverse(&p, pt);

    if ((ClipDepth == 0) && pDef->DefPointTestLocal(p, 1, this))
    {
        // The mouse is inside the shape.
        GFxASCharacter* pParent = GetParent();
        while (pParent && pParent->IsSprite())
        {
            // Parent sprite would have to be in button mode to grab attention.
            GFxSprite * psprite = (GFxSprite*)pParent;
            if (testAll || psprite->ActsAsButton())
            {
                // Check if sprite should be ignored
                if (!ignoreMC || (ignoreMC != psprite))
                    return psprite;
            }
            pParent = pParent->GetParent ();
        }
    }
    return 0;
}

bool GFxGenericCharacter::PointTestLocal(const GPointF &pt, UInt8 hitTestMask) const
{     
    return pDef->DefPointTestLocal(pt, hitTestMask & HitTest_TestShape, this);
}

void  GFxGenericCharacter::Display(GFxDisplayContext &context, StackData stackData)
{
	// Pass matrix on the stack
	StackData newStackData;
	newStackData.stackMatrix = stackData.stackMatrix * this->Matrix_1;
	newStackData.stackColor = stackData.stackColor * this->ColorTransform;

	pDef->Display(context, this, newStackData);    // pass in transform info 
    //pDef->Display(context, this);    // pass in transform info       
}
