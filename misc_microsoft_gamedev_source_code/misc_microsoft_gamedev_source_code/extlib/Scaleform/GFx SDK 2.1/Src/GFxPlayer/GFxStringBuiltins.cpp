/**********************************************************************

Filename    :   GFxStringBuiltins.cpp
Content     :   ActionScript string bultin manager implementation
Created     :   November 15, 2006
Authors     :   Michael Antonov

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxStringBuiltins.h"



// ***** GASStringBuiltinManager

GASStringBuiltinManager::GASStringBuiltinManager()
{
    InitBuiltins();
}
GASStringBuiltinManager::~GASStringBuiltinManager()
{
    ReleaseBuiltins();
}


// Build in string table
static const char* GFx_pASBuiltinTable[] = 
{
    "",
    "Object",
    "Array",
    "String",
    "Number",
    "Boolean",
    "MovieClip",
    "Function",
    "Sound",
    "Button",
    "TextField",
    "Color",
    "Transform",
    "Matrix",
    "Point",
    "Rectangle",
    "ColorTransform",
    "capabilities",
    "Stage",
    "AsBroadcaster",
    "Date",
    "Selection",
    "XML",
    "XMLNode",
    "Math",
    "Key",
    "Mouse",
    "ExternalInterface",
    "MovieClipLoader",
    "BitmapData",
    "LoadVars",
    "TextFormat",

    // Necessary runtime keys
    "<unknown>",
    "<bad type>",
    "<undefined>",
    ".",
    "..",
    "undefined",
    "null",
    "true",
    "false",    
    // Lowercase types for typeof reporting
    "string",
    "number",
    "boolean",
    "movieclip",
    "object",
    "function",
    // Path components
    "this",
    "super",
    "_global",
    "_root",
    "_parent",
    "_level0",
    "_level0.",
    "arguments",
    "callee",
    "caller",

    "gfxExtensions",
    "noInvisibleAdvance",
     // Numbers
    "NaN",
    "Infinity",
    "-Infinity", // "-Infinity"

    "prototype",
    "__proto__", // "__proto__"
    "constructor",
    "__constructor__", // "__constructor__"
    "_listeners",     // "_listeners"
    "__resolve",      // "__resolve"
    "[type Function]",
    "[type Object]",
    "[object Object]",
    // Common methods
    "toString",
    "valueOf",
    "onSetFocus",
    "onKillFocus",

    // Event name strings
    "INVALID",             // Event_Invalid "INVALID"
    "onPress",             // Event_Press
    "onRelease",           // Event_Release
    "onReleaseOutside",    // Event_ReleaseOutside
    "onRollOver",          // Event_RollOver
    "onRollOut",           // Event_RollOut
    "onDragOver",          // Event_DragOver
    "onDragOut",           // Event_DragOut
    "@keyPress@",          // Event_KeyPress  "@keyPress@"
    "@initialize@",        // Event_Initialize "@initialize@"
    "onLoad",              // Event_Load
    "onUnload",            // Event_Unload
    "onEnterFrame",        // Event_EnterFrame
    "onMouseDown",         // Event_MouseDown
    "onMouseUp",           // Event_MouseUp
    "onMouseMove",         // Event_MouseMove
    "onMouseWheel",
    "onKeyDown",           // Event_KeyDown
    "onKeyUp",             // Event_KeyUp
    "onData",              // Event_Data
    "@construct@",         // Event_Construct "@construct@"
    // These are for the MoveClipLoader ActionScript only
    "onLoadStart",         // Event_LoadStart
    "onLoadError",         // Event_LoadError
    "onLoadProgress",      // Event_LoadProgress
    "onLoadInit",          // Event_LoadInit
    // These are for the XMLSocket ActionScript only
    "onSockClose",         // Event_SockClose
    "onSockConnect",       // Event_SockConnect
    "onSockData",          // Event_SockData
    "onSockXML",           // Event_SockXML
    // These are for the XML ActionScript only
    "onXMLLoad",           // Event_XMLLoad
    "onXMLData",           // Event_XMLData

    // Common members
    "width",
    "height",
    "useHandCursor",

    // Sprite members - for efficiency    
    "x",
    "y",
    "xMin",
    "xMax",
    "yMin",
    "yMax",

    // Mouse class extensions
    "setCursorType",
    "LEFT",
    "RIGHT",
    "MIDDLE",
    "ARROW",
    "HAND",
    "IBEAM",

    // Numbers
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",

    // Put length as last so that we can have checks for it.
    "length"
};


const char* GASStringBuiltinManager::GetBuiltinCStr(GASBuiltinType btype)
{
    GCOMPILER_ASSERT((sizeof(GFx_pASBuiltinTable)/sizeof(const char*)) == GASBuiltinConst_Count_);
    GCOMPILER_ASSERT(GASBuiltin_7 == (GASBuiltinType)(GASBuiltin_0 + 7));

    GASSERT(btype < GASBuiltinConst_Count_);
    return GFx_pASBuiltinTable[btype];
}

void       GASStringBuiltinManager::InitBuiltins()
{    
    for (UInt i = 0; i<GASBuiltinConst_Count_; i++)
    {
        GASString str(StringManager.CreateConstString(GFx_pASBuiltinTable[i],
                                                      strlen(GFx_pASBuiltinTable[i]),
                                                      (UInt32)GASString::Flag_Builtin) );
        Builtins[i].pNode = str.pNode;
        str.pNode->AddRef();
        str.pNode->ResolveLowercase_Impl();
    }

    // If this hits, there is some kind of ordering issue.
    GASSERT(GetBuiltin(GASBuiltin_unknown_) == "<unknown>");
    GASSERT(GetBuiltin(GASBuiltin_prototype) == "prototype");
    GASSERT(GetBuiltin(GASBuiltin_onXMLData) == "onXMLData");    
    GASSERT(GetBuiltin(GASBuiltin_length) == "length");
}

void      GASStringBuiltinManager::ReleaseBuiltins()
{    
    for (UInt i = 0; i<GASBuiltinConst_Count_; i++)
    {  
        Builtins[i].pNode->Release();
        // For safety - these should never be accessed after release.
        Builtins[i].pNode = 0; 
    }
}



