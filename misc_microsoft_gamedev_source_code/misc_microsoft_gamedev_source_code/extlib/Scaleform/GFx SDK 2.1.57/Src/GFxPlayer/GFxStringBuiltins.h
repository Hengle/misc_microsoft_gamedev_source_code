/**********************************************************************

Filename    :   GFxStringBuiltins.h
Content     :   ActionScript string bultins and manager objects
Created     :   November 15, 2006
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#ifndef INC_GFXSTRINGBUILTINS_H
#define INC_GFXSTRINGBUILTINS_H

#include "GFxASString.h"


// **** Classes Declared

class GASStringBuiltinManager;
class GASStringContext;

// Forward declarations
class GASGlobalContext;


/* ***** String Builtin Notes

    Built-ins are strings that can be accessed very efficiently based on
    a constant. They are usually obtained from the built-in manager through
    an inline function such as

        // Returns a string for "undefined"
        pManager->GetBuiltin(GASBuiltin_undefined)
    
    Builtins are pre-created ahead of time, so that GetBuiltin() can be
    inlined to a single array access. Built-in strings are used heavily
    throughout AS so performance of their references and compares is of
    vital importance.
    
    This file defines a container GASStringBuiltinManager class that holds
    all of the allocated built-ins and AS string manager. One instance of
    this class is created as a base of GASGlobalContext and shared by all
    sprites within a movie views. For thread efficiency reasons, different
    movie views use different string managers.

*/


// ***** Global built-in types

enum GASBuiltinType
{
    GASBuiltin_empty_, // ""
    GASBuiltin_Object,
    GASBuiltin_Array,
    GASBuiltin_String,
    GASBuiltin_Number,
    GASBuiltin_Boolean,
    GASBuiltin_MovieClip,
    GASBuiltin_Function,
    GASBuiltin_Sound,
    GASBuiltin_Button,
    GASBuiltin_TextField,
    GASBuiltin_Color,
    GASBuiltin_Transform,
    GASBuiltin_Matrix,
    GASBuiltin_Point,
    GASBuiltin_Rectangle,
    GASBuiltin_ColorTransform,
    GASBuiltin_Capabilities,
    GASBuiltin_Stage,
    GASBuiltin_AsBroadcaster,
    GASBuiltin_Date,
    GASBuiltin_Selection,
    GASBuiltin_XML,
    GASBuiltin_XMLNode,
    GASBuiltin_Math,
    GASBuiltin_Key,
    GASBuiltin_Mouse,
    GASBuiltin_ExternalInterface,
    GASBuiltin_MovieClipLoader,
    GASBuiltin_BitmapData,
    GASBuiltin_LoadVars,
    GASBuiltin_TextFormat,

    // Necessary runtime keys
    GASBuiltin_unknown_, // "<unknown>"
    GASBuiltin_badtype_, // "<bad type>"
    GASBuiltin_undefined_, // "<undefined>"

    GASBuiltin_dot_,    // "."
    GASBuiltin_dotdot_, // ".."

    GASBuiltin_undefined,
    GASBuiltin_null,
    GASBuiltin_true,
    GASBuiltin_false,
    // Lowercase types for typeof reporting
    GASBuiltin_string,
    GASBuiltin_number,
    GASBuiltin_boolean,
    GASBuiltin_movieclip,
    GASBuiltin_object,
    GASBuiltin_function,

    // Path components
    GASBuiltin_this,    // "this"
    GASBuiltin_super,    // "super"
    GASBuiltin__global, // "_global"
    GASBuiltin__root,   // "_root"
    GASBuiltin__parent,   // "_parent"

    GASBuiltin__level0,     // "_level0"
    GASBuiltin__level0dot_, // "_level0."

    GASBuiltin_arguments,
    GASBuiltin_callee,
    GASBuiltin_caller,
    
    GASBuiltin_gfxExtensions,
    GASBuiltin_noInvisibleAdvance,

    GASBuiltin_NaN,
    GASBuiltin_Infinity,
    GASBuiltin_minusInfinity_, // "-Infinity"

    GASBuiltin_prototype,
    GASBuiltin___proto__, // "__proto__"
    GASBuiltin_constructor,
    GASBuiltin___constructor__, // "__constructor__"

    GASBuiltin__listeners,     // "_listeners"
    GASBuiltin___resolve,      // "__resolve"

    GASBuiltin_typeFunction_, // "[type Function]"
    GASBuiltin_typeObject_,   // "[type Object]"
    GASBuiltin_objectObject_, // "[object Object]"

    // Common methods
    GASBuiltin_toString,
    GASBuiltin_valueOf,

    GASBuiltin_onSetFocus,
    GASBuiltin_onKillFocus,

    // Event name strings
    GASBuiltin_INVALID,             // Event_Invalid "INVALID"
    GASBuiltin_onPress,             // Event_Press
    GASBuiltin_onRelease,           // Event_Release
    GASBuiltin_onReleaseOutside,    // Event_ReleaseOutside
    GASBuiltin_onRollOver,          // Event_RollOver
    GASBuiltin_onRollOut,           // Event_RollOut
    GASBuiltin_onDragOver,          // Event_DragOver
    GASBuiltin_onDragOut,           // Event_DragOut
    GASBuiltin_akeyPressa_,          // Event_KeyPress  "@keyPress@"
    GASBuiltin_ainitializea_,        // Event_Initialize "@initialize@"
    GASBuiltin_onLoad,              // Event_Load
    GASBuiltin_onUnload,            // Event_Unload
    GASBuiltin_onEnterFrame,        // Event_EnterFrame
    GASBuiltin_onMouseDown,         // Event_MouseDown
    GASBuiltin_onMouseUp,           // Event_MouseUp
    GASBuiltin_onMouseMove,         // Event_MouseMove
    GASBuiltin_onMouseWheel,        // Mouse listener's onMouseWheel
    GASBuiltin_onKeyDown,           // Event_KeyDown
    GASBuiltin_onKeyUp,             // Event_KeyUp
    GASBuiltin_onData,              // Event_Data
    GASBuiltin_aconstructa_,         // Event_Construct "@construct@"
    // These are for the MoveClipLoader ActionScript only
    GASBuiltin_onLoadStart,         // Event_LoadStart
    GASBuiltin_onLoadError,         // Event_LoadError
    GASBuiltin_onLoadProgress,      // Event_LoadProgress
    GASBuiltin_onLoadInit,          // Event_LoadInit
    // These are for the XMLSocket ActionScript only
    GASBuiltin_onSockClose,         // Event_SockClose
    GASBuiltin_onSockConnect,       // Event_SockConnect
    GASBuiltin_onSockData,          // Event_SockData
    GASBuiltin_onSockXML,           // Event_SockXML
    // These are for the XML ActionScript only
    GASBuiltin_onXMLLoad,           // Event_XMLLoad
    GASBuiltin_onXMLData,            // Event_XMLData

    // Common members
    GASBuiltin_width,
    GASBuiltin_height,
    GASBuiltin_useHandCursor,

    // Sprite members - for efficiency    
    GASBuiltin_x,
    GASBuiltin_y,
    GASBuiltin_xMin,
    GASBuiltin_xMax,
    GASBuiltin_yMin,
    GASBuiltin_yMax,

    // Mouse class extensions
    GASBuiltin_setCursorType,
    GASBuiltin_LEFT,
    GASBuiltin_RIGHT,
    GASBuiltin_MIDDLE,
    GASBuiltin_ARROW,
    GASBuiltin_HAND,
    GASBuiltin_IBEAM,

    // Numbers are used heavily during argument pushing in SWF 6
    // and Array visits elsewhere. So pre-allocate low numbers
    // to prevent them ending up in allocator.
    // This range must be continuous.
    GASBuiltin_0,
    GASBuiltin_1,
    GASBuiltin_2,
    GASBuiltin_3,
    GASBuiltin_4,
    GASBuiltin_5,
    GASBuiltin_6,
    GASBuiltin_7,
    
    // Put length as last so that we can have checks for it.
    GASBuiltin_length,
    GASBuiltinConst_Count_,
    // Number of numeric built-ins
    GASBuiltinConst_DigitMax_ = GASBuiltin_7 - GASBuiltin_0
};


// ***** GASStringBuiltinManager class

class GASStringBuiltinManager
{
    // String manager holding all strings.
    GASStringNodeHolder         Builtins[GASBuiltinConst_Count_];
    GASStringManager            StringManager;    

    void    InitBuiltins();
    void    ReleaseBuiltins();    

public:
    
    GASStringBuiltinManager();
    ~GASStringBuiltinManager();

    // String access and use.   
    const GASStringNodeHolder&  GetBuiltinNodeHolder(GASBuiltinType btype) const { return Builtins[btype]; }
    const GASString& GetBuiltin(GASBuiltinType btype) const              { return (const GASString&)GetBuiltinNodeHolder(btype); }
    GASString        CreateConstString(const char *pstr)                 { return StringManager.CreateConstString(pstr); }
    GASString        CreateString(const char *pstr)                      { return StringManager.CreateString(pstr); }
    GASString        CreateString(const wchar_t *pwstr)                  { return StringManager.CreateString(pwstr); }
    GASString        CreateString(const char *pstr, size_t length)       { return StringManager.CreateString(pstr, length); }
    GASString        CreateString(const GFxString& str)                  { return StringManager.CreateString(str); }

    GASStringManager* GetStringManager() { return &StringManager;  }

    static const char* GetBuiltinCStr(GASBuiltinType btype);
};


// ***** GASStringContext

// String lookup context is passed to functions, such as GASObject::GetMemberRaw to that they
// can access builtin constants, create new strings, and check for case sensitivity correctly.
// In Flash, some files are case-insensitive while others are case-sensitive. Because of that,
// SWFVersion in string context allows us to pick correct behavior.

class GASStringContext
{
public:

    GASStringContext(GASGlobalContext* pcontext = 0, UInt version = 0)
    {
        pContext = pcontext;
        SWFVersion = (UByte)version;
    }

    // Provided GASStringBuiltinManager interface    
    GASGlobalContext*    pContext;
    UByte                SWFVersion;

    inline void         UpdateVersion(UInt version)  { SWFVersion = (UByte) version; }
    inline UInt         GetVersion() const           { return (UInt) SWFVersion; }
    inline bool         IsCaseSensitive() const      { return SWFVersion > 6; }

    // String access and use (implemented in GASAction.h).
    const GASString&    GetBuiltin(GASBuiltinType btype) const;
    GASString    CreateConstString(const char *pstr) const;
    GASString    CreateString(const char *pstr) const;    
    GASString    CreateString(const wchar_t *pwstr) const;
    GASString    CreateString(const char *pstr, size_t length) const;    
    GASString    CreateString(const GFxString& str) const;

    bool         CompareConstString_CaseCheck(const GASString& pstr1, const char* pstr2);
    bool         CompareConstString_CaseInsensitive(const GASString& pstr1, const char* pstr2);
};


#endif
