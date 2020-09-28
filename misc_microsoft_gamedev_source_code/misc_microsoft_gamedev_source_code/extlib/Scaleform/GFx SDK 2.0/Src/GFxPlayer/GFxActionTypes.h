/**********************************************************************

Filename    :   GFxActionTypes.h
Content     :   ActionScript implementation classes
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/


#ifndef INC_GFXACTIONTYPES_H
#define INC_GFXACTIONTYPES_H

#include "GRefCount.h"
#include "GTLTypes.h"
#include "GFxASString.h"
#include "GFxEvent.h"
//#include "GFxPlayerResource.h"
#include "GFxLog.h"
#include "GFxStringBuiltins.h"

#include <stdio.h>

#if !defined(GFC_OS_SYMBIAN) && !defined(GFC_CC_RENESAS)
#include <wchar.h>
#endif

// ***** Declared Classes
struct GASNameFunction;
struct GASNameNumber;
class GFxEventId;
class GASWithStackEntry;
class GASActionBuffer;

// ***** External Classes
class GASValue;
class GASObject;
class GASFnCall;
class GASActionBuffer;
class GASObjectInterface;
class GFxStream;
class GASEnvironment;

class GASString;
class GASStringContext;
enum  GASBuiltinType;

class GFxASCharacter;



typedef GTL::garray<GASValue> GASValueArray;

// Floating point type used by ActionScript.
// Generally, this should be Double - a 64-bit floating point value; however,
// on systems that do not have Double (PS2) this can be another type, such as a float.
// A significant problem with Float is that in AS it is used to substitute integers,
// but it does not have enough mantissa bits so do so correctly. To account for this,
// we could store both Int + Float in GASFloat and do some conditional logic...

#ifdef GFC_NO_DOUBLE
typedef Float       GASNumber;
// This is a version of float that can be stored in a union; thus GASFloat 
// must be convertible to GASFloatValue, though perhaps with some extra calls.
typedef GASNumber   GASNumberValue;

//#define   GFX_ASNUMBER_NAN
#define GFX_ASNUMBER_ZERO   0.0f

#else

typedef Double      GASNumber;
// This is a version of float that can be stored in a union; thus GASFloat 
// must be convertible to GASFloatValue, though perhaps with some extra calls.
typedef GASNumber   GASNumberValue;

//#define   GFX_ASNUMBER_NAN
#define GFX_ASNUMBER_ZERO   0.0

#endif

// C-code function type used by values.
typedef void (*GASCFunctionPtr)(const GASFnCall& fn);

// Name-Function pair for statically defined operations 
// (like String::concat, etc)
struct GASNameFunction
{
    const char*     Name;
    GASCFunctionPtr Function;
};

// Name-Number pair for statically defined constants 
struct GASNameNumber
{
    const char* Name;
    int         Number;
};

// 3-way bool type - Undefined, True, False states
class Bool3W
{
    enum
    {
        Undefined, True, False
    };
    UByte Value;
public:
    inline Bool3W() : Value(Undefined) {}
    inline Bool3W(bool v) : Value(UByte((v)?True:False)) {}
    inline Bool3W(const Bool3W& v): Value(v.Value) {}

    inline Bool3W& operator=(const Bool3W& o)
    {
        Value = o.Value;
        return *this;
    }
    inline Bool3W& operator=(bool o)
    {
        Value = UByte((o) ? True : False);
        return *this;
    }
    inline bool operator==(const Bool3W& o) const
    {
        return (Value == Undefined || o.Value == Undefined) ? false : Value == o.Value;
    }
    inline bool operator==(bool o) const
    {
        return (Value == Undefined) ? false : Value == ((o) ? True : False);
    }
    inline bool operator!=(const Bool3W& o) const
    {
        return (Value == Undefined || o.Value == Undefined) ? false : Value != o.Value;
    }
    inline bool operator!=(bool o) const
    {
        return (Value == Undefined) ? false : Value != ((o) ? True : False);
    }
    inline bool IsDefined() const { return Value != Undefined; }
    inline bool IsTrue() const    { return Value == True; }
    inline bool IsFalse() const   { return Value == False; }
};


// ***** GFxEventId

// Describes various ActionScript events that can be fired of and that
// users can install handlers for through on() and onClipEvent() handlers.
// Events include button and movie clip mouse actions, keyboard handling,
// load & unload notifications, etc. 

class GFxEventId
{
public:
    // These must match the function names in GFxEventId::GetFunctionName()
    enum IdCode
    {
        Event_Invalid,

        // These are for buttons & sprites.
        Event_Press,
        Event_Release,
        Event_ReleaseOutside,
        Event_RollOver,
        Event_RollOut,
        Event_DragOver,
        Event_DragOut,
        Event_KeyPress,

        // These are for sprites only.
        Event_Initialize,
        Event_Load,
        Event_Unload,
        Event_EnterFrame,
        Event_MouseDown,
        Event_MouseUp,
        Event_MouseMove,
        Event_KeyDown,
        Event_KeyUp,
        Event_Data,
        Event_Construct,
        
        // These are for the MoveClipLoader ActionScript only
        Event_LoadStart,
        Event_LoadError,
        Event_LoadProgress,
        Event_LoadInit,

        // These are for the XMLSocket ActionScript only
        Event_SockClose,
        Event_SockConnect,
        Event_SockData,
        Event_SockXML,

        // These are for the XML ActionScript only
        Event_XMLLoad,
        Event_XMLData,

        Event_COUNT
    };

    UInt32              WcharCode;
    short               KeyCode;
    UByte               Id;
    UByte               AsciiCode;
    GFxSpecialKeysState SpecialKeysState;

    GFxEventId() : WcharCode(0), KeyCode(GFxKey::VoidSymbol),Id(Event_Invalid), AsciiCode(0) {}

    GFxEventId(UByte id, short c = GFxKey::VoidSymbol, UByte ascii = 0, UInt32 wcharCode = 0)
        : WcharCode(wcharCode), KeyCode(c), Id(id), AsciiCode(ascii)
    {
        // For the button key events, you must supply a keycode.
        // Otherwise, don't.
        //!GASSERT((KeyCode == GFxKey::VoidSymbol && (Id != Event_KeyPress)) || 
        //!     (KeyCode != GFxKey::VoidSymbol && (Id == Event_KeyPress)));
    }

    GINLINE bool IsButtonEvent() const
        { return (Id >= Event_Press) && (Id <= Event_KeyPress); }

    GINLINE bool IsKeyEvent() const
        { return (Id >= Event_KeyDown) && (Id <= Event_KeyUp); }

    bool    operator==(const GFxEventId& id) const 
    { 
        return Id == id.Id && (!IsButtonEvent() || (KeyCode == id.KeyCode)); 
    }

    // Return the name of a method-handler function corresponding to this event.
    GASString           GetFunctionName(GASStringContext *psc) const;
    GASBuiltinType      GetFunctionNameBuiltinType() const;

    // converts keyCode/asciiCode from this event to the on(keyPress <>) format
    int                 ConvertToButtonKeyCode () const;

    size_t              HashCode() const
    {
        size_t hash = WcharCode;
        if (IsButtonEvent()) 
            hash ^= KeyCode;
        return hash;
    }
};

struct GFxEventIdHashFunctor
{
    size_t operator()(const GFxEventId& data) const 
    { 
        return data.HashCode();
    }
};

// ***** GASWithStackEntry

// The "with" stack is for Pascal-like 'with() { }' statement scoping. Passed
// as argument for GASActionBuffer execution and used by variable lookup methods.

class GASWithStackEntry
{
protected:
    GPtr<GRefCountBaseImpl>     pObjectOrChar;
    bool                        IsObject;

public: 
    int                         BlockEndPc;
    
    GASWithStackEntry()         
    {
        BlockEndPc = 0;
        IsObject   = 0;
    }

    GASWithStackEntry(GASObject* pobj, int end);
    GASWithStackEntry(GFxASCharacter* pcharacter, int end);

    // Conversions: non-inline because they need to know character types.
    GASObject*          GetObject() const;
    GFxASCharacter*     GetCharacter() const;
    GASObjectInterface* GetObjectInterface() const;
};


// ***** GASActionBuffer

// ActionScript buffer for action opcodes. The associated dictionary is stored in a
// separate GASActionBuffer class, which needs to be local for GFxMovieView instances.

class GASActionBufferData : public GRefCountBase<GASActionBufferData>
{
public:
    GASActionBufferData()
    {
    }
    ~GASActionBufferData()
    {
    }

    // Reads action instructions from the SWF file stream.
    void    Read(GFxStream* in);

    bool    IsNull() const      { return Buffer.size() < 1 || Buffer[0] == 0; }
    UInt    GetLength() const   { return (UInt)Buffer.size(); }
    const UByte* GetBufferPtr() const { return (IsNull()) ? NULL : (const UByte*)&Buffer[0]; }

private:    
    // ActionScript byte-code data.
    GTL::garray<UByte>  Buffer;
};


// GASActionBuffer - movie view instance specific ActionBuffer class. This class
// caches dictionary data that needs to be local to movie view instances.

class GASActionBuffer : public GRefCountBase<GASActionBuffer>
{    
    // Pointer to opcode byte data.
    GPtr<GASActionBufferData>   pBufferData;   
    // Cached dictionary.
    GTL::garray_cc<GASString>   Dictionary;    
    int                         DeclDictProcessedAt;        

public:
    GASActionBuffer(GASStringContext *psc, GASActionBufferData *pbufferData) :
      pBufferData(pbufferData), Dictionary(psc->GetBuiltin(GASBuiltin_empty_)),
      DeclDictProcessedAt(-1)
    { }
   
    // Type call for execution; a distinction is sometimes necessary 
    // to determine how certain ops, such as 'var' declarations are handled.
    enum ExecuteType
    {
        Exec_Unknown    = 0,
        Exec_Function   = 1,
        Exec_Function2  = 2,
        Exec_Event      = 3
        // Must fit in a byte.
    };

    void    Execute(GASEnvironment* env);
    void    Execute(GASEnvironment* env,
                    int startPc, int execBytes,
                    GASValue* retval,
                    const GTL::garray<GASWithStackEntry>& initialWithStack,
                    ExecuteType execType);

    void    ProcessDeclDict(GASStringContext *psc, UInt StartPc, UInt StopPc, class GFxActionLogger &logger);
    
    bool    IsNull() const      { return pBufferData->IsNull(); }
    UInt    GetLength() const   { return pBufferData->GetLength(); }
    const UByte* GetBufferPtr() const { return pBufferData->GetBufferPtr(); }
};


#endif //INC_GFXACTIONTYPES_H
