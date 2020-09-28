/**********************************************************************

Filename    :   GFxEvent.cpp
Content     :   
Created     :   
Authors     :   

Copyright   :   (c) 2001-2006 Scaleform Corp. All Rights Reserved.

Notes       :   

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include <string.h>
#include "GFxEvent.h"

    
GFxKeyboardState::KeyQueue::KeyQueue ()
{
    ResetState();
}

void GFxKeyboardState::KeyQueue::ResetState()
{
    PutIdx = 0;
    GetIdx = 0;
    Count = 0;
    memset (&Buffer, 0, sizeof(Buffer));
}

void GFxKeyboardState::KeyQueue::Put (short code, UByte ascii, UInt32 wcharCode, GFxEvent::EventType event, GFxSpecialKeysState specialKeysState) 
{
    if (Count < KeyQueueSize)
    {
        Buffer[PutIdx].code             = code;
        Buffer[PutIdx].ascii            = ascii;
        Buffer[PutIdx].wcharCode        = wcharCode;
        Buffer[PutIdx].event            = event;
        Buffer[PutIdx].specialKeysState = specialKeysState;
        if (++PutIdx >= KeyQueueSize)
            PutIdx = 0;
        ++Count;
    }
}
bool GFxKeyboardState::KeyQueue::Get (short* code, UByte* ascii, UInt32* wcharCode, GFxEvent::EventType* event, GFxSpecialKeysState* specialKeysState)
{
    if (Count > 0) 
    {
        *code               = Buffer[GetIdx].code;
        *ascii              = Buffer[GetIdx].ascii;
        *wcharCode          = Buffer[GetIdx].wcharCode;
        *event              = Buffer[GetIdx].event;
        if (specialKeysState) // specialKeysState is optional
            *specialKeysState   = Buffer[GetIdx].specialKeysState;
        if (++GetIdx >= KeyQueueSize)
            GetIdx = 0;
        --Count;
        return true;
    }
    return false;
}

GFxKeyboardState::GFxKeyboardState()
{
    memset(Keymap, 0, sizeof(Keymap));
    memset(Toggled, 0, sizeof(Toggled));
}

void GFxKeyboardState::ResetState()
{
    KeyQueue.ResetState();
    memset(Keymap, 0, sizeof(Keymap));
    memset(Toggled, 0, sizeof(Toggled));
}

bool GFxKeyboardState::IsKeyDown(int code) const
{
    if (code < 0 || code >= GFxKey::KeyCount) return false;

    int ByteIndex = code >> 3;
    int BitIndex = code - (ByteIndex << 3);
    int mask = 1 << BitIndex;

    GASSERT(ByteIndex >= 0 && ByteIndex < int(sizeof(Keymap)/sizeof(Keymap[0])));

    if (Keymap[ByteIndex] & mask)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GFxKeyboardState::IsKeyToggled(int code) const
{
    switch (code)
    {
        case GFxKey::NumLock:
            return Toggled[0];
        case GFxKey::CapsLock:
            return Toggled[1];
        case GFxKey::ScrollLock:
            return Toggled[2];
    }
    return false;
}

void GFxKeyboardState::SetKeyToggled(int code, bool toggle)
{
    switch (code)
    {
        case GFxKey::NumLock:
            Toggled[0] = toggle;
            break;
        case GFxKey::CapsLock:
            Toggled[1] = toggle;
            break;
        case GFxKey::ScrollLock:
            Toggled[2] = toggle;
            break;
    }
}

void GFxKeyboardState::SetKeyDown(int code, UByte ascii, GFxSpecialKeysState specialKeysState)
{
    if (code < 0 || code >= GFxKey::KeyCount) return;

    int ByteIndex = code >> 3;
    int BitIndex = code - (ByteIndex << 3);
    int mask = 1 << BitIndex;

    GASSERT(ByteIndex >= 0 && ByteIndex < int(sizeof(Keymap)/sizeof(Keymap[0])));

    Keymap[ByteIndex] |= mask;

    KeyQueue.Put(short(code), ascii, 0, GFxEvent::KeyDown, specialKeysState);
}

void GFxKeyboardState::SetKeyUp(int code, UByte ascii, GFxSpecialKeysState specialKeysState)
{
    if (code < 0 || code >= GFxKey::KeyCount) return;

    int ByteIndex = code >> 3;
    int BitIndex = code - (ByteIndex << 3);
    int mask = 1 << BitIndex;

    GASSERT(ByteIndex >= 0 && ByteIndex < int(sizeof(Keymap)/sizeof(Keymap[0])));

    Keymap[ByteIndex] &= ~mask;

    KeyQueue.Put (short(code), ascii, 0, GFxEvent::KeyUp, specialKeysState);
}

void GFxKeyboardState::SetChar(UInt32 wcharCode)
{
    KeyQueue.Put(0, 0, wcharCode, GFxEvent::CharEvent, 0);
}

bool GFxKeyboardState::GetQueueEntry(short* code, UByte* ascii, UInt32* wcharCode, GFxEvent::EventType* event, GFxSpecialKeysState* specialKeysState)
{
    return KeyQueue.Get (code, ascii, wcharCode, event, specialKeysState);
}

void GFxKeyboardState::NotifyListeners(GASStringContext *psc, short code, UByte ascii, UInt32 wcharCode, GFxEvent::EventType event)
{
    // notify listeners
    // Do we need specialKeysState here? (AB)
    UPInt i, n = Listeners.size();
    for (i = 0; i < n; i++)
    {
        GPtr<IListener> listener = Listeners[i];
        if (listener)
        {
            if (event == GFxEvent::KeyDown) listener->OnKeyDown(psc, code, ascii, wcharCode);
            else if (event == GFxEvent::KeyUp) listener->OnKeyUp(psc, code, ascii, wcharCode);
        }
    }
}

void GFxKeyboardState::CleanupListeners()
// Remove dead entries in the listeners list.  (Since
// we use WeakPtr's, listeners can disappear without
// notice.)
{
    for (int i = (int)Listeners.size() - 1; i >= 0; i--)
    {
        if (Listeners[i] == NULL)
        {
            Listeners.remove(i);
        }
    }
}

void GFxKeyboardState::AddListener(IListener* listener)
{
    CleanupListeners();

    for (UPInt i = 0, n = Listeners.size(); i < n; i++)
    {
        if (Listeners[i] == listener)
        {
            // Already in the list.
            return;
        }
    }

    Listeners.push_back(listener);
}

void GFxKeyboardState::RemoveListener(IListener* listener)
{
    CleanupListeners();

    for (int i = (int)Listeners.size() - 1; i >= 0; i--)
    {
        if (Listeners[i] == listener)
        {
            Listeners.remove(i);
        }
    }
}


