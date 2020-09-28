
//	VinceEvents.h : Auto-generated file that contains
//	the definition of all available event calls
//
//	New Template Created 2007/01/16 Rich Bonny <rbonny@microsoft.com>
//
//	MICROSOFT CONFIDENTIAL.  DO NOT DISTRIBUTE.
//	Copyright (c) Microsoft Corp.  All rights reserved.
        
//
// This file was automatically generated from EventSpecs.xml and GenerateEventHeader.xsl
// *** DO NOT HAND-EDIT ***

// The VINCE code can be largely excluded from project compilation by omitting the preprocessor
// definition _VINCE_. However, the #define's in this header file still need to be processed
// so that inline Vince macros are properly disabled. 

//	The following list of logging calls may be inserted into the application code.
//  Use of the macro format allows these calls to be conditionally excluded by the compiler.
//  See the #define section for compiler flags that can be used disable compilation
//  collectively or selectively.
        
// VINCE_EVENT_Message(MessageText);
// VINCE_EVENT_SmallEvent(ThreadNumber, TestInt, TestFloat, TestChar);
// VINCE_EVENT_MediumEvent(ThreadNumber, TestInt, TestFloat, TestBool, TestChar, TestWChar);
// VINCE_EVENT_LargeEvent(ThreadNumber, TestInt, TestFloat, TestBool, TestChar, TestInt1, TestInt2, TestInt3, TestInt4, TestInt5, TestInt6, TestInt7, TestInt8, TestWChar, TestChar2, TestWChar2);
// VINCE_EVENT_EnumTest(OrdinaryString, OrdinaryWstring, EnumerationString, EnumerationWstring);

#pragma once
#define VINCE_MAX_EVENTS 5

// Any files that need to be included to access object definitions:
        

// Declaration and initialization macros
#ifdef _VINCE_
  #define VINCE_INITIALIZE Vince::Initialize()
  #ifndef VINCE_EVENT
    #define VINCE_EVENT(event, params)  Vince::event params
  #endif
#else
    #define VINCE_INITIALIZE
    #define VINCE_EVENT(event, params)
#endif // _VINCE_

// Compiler Switches:

#ifndef VINCE_EVENT_Generic
#define VINCE_EVENT_Generic(event, params) VINCE_EVENT(event, params)
#endif
        
    #ifndef VINCE_EVENT_Message
    #define VINCE_EVENT_Message(MessageText)  VINCE_EVENT_Generic(LogEvent_Message, (MessageText))
    #endif
    
    #ifndef VINCE_EVENT_SmallEvent
    #define VINCE_EVENT_SmallEvent(ThreadNumber, TestInt, TestFloat, TestChar)  VINCE_EVENT_Generic(LogEvent_SmallEvent, (ThreadNumber, TestInt, TestFloat, TestChar))
    #endif
    
    #ifndef VINCE_EVENT_MediumEvent
    #define VINCE_EVENT_MediumEvent(ThreadNumber, TestInt, TestFloat, TestBool, TestChar, TestWChar)  VINCE_EVENT_Generic(LogEvent_MediumEvent, (ThreadNumber, TestInt, TestFloat, TestBool, TestChar, TestWChar))
    #endif
    
    #ifndef VINCE_EVENT_LargeEvent
    #define VINCE_EVENT_LargeEvent(ThreadNumber, TestInt, TestFloat, TestBool, TestChar, TestInt1, TestInt2, TestInt3, TestInt4, TestInt5, TestInt6, TestInt7, TestInt8, TestWChar, TestChar2, TestWChar2)  VINCE_EVENT_Generic(LogEvent_LargeEvent, (ThreadNumber, TestInt, TestFloat, TestBool, TestChar, TestInt1, TestInt2, TestInt3, TestInt4, TestInt5, TestInt6, TestInt7, TestInt8, TestWChar, TestChar2, TestWChar2))
    #endif
    
    #ifndef VINCE_EVENT_EnumTest
    #define VINCE_EVENT_EnumTest(OrdinaryString, OrdinaryWstring, EnumerationString, EnumerationWstring)  VINCE_EVENT_Generic(LogEvent_EnumTest, (OrdinaryString, OrdinaryWstring, EnumerationString, EnumerationWstring))
    #endif
    
// We skip the rest if _VINCE_ is not defined

#ifdef _VINCE_

namespace Vince
{
    enum ActiveFlag
    {
         Active_Message,
         Active_SmallEvent,
         Active_MediumEvent,
         Active_LargeEvent,
         Active_EnumTest,
    };

    void Initialize()
    {
        InitializeEvents( VINCE_MAX_EVENTS );
        InitializeEvent( Active_Message, "Message", "Generic.", true );
        InitializeEvent( Active_SmallEvent, "SmallEvent", "Generic.", true );
        InitializeEvent( Active_MediumEvent, "MediumEvent", "Generic.", true );
        InitializeEvent( Active_LargeEvent, "LargeEvent", "Generic.", true );
        InitializeEvent( Active_EnumTest, "EnumTest", "Generic.", true );
        InitializeCore();
    };

    void LogEvent_Message(const char* MessageText)
    {
        if ( StartEvent( Active_Message ) )
		{
            SendParameter( "MessageText", MessageText );
            SendEvent();
        }
    };
    
    void LogEvent_SmallEvent(int ThreadNumber, int TestInt, float TestFloat, const char* TestChar)
    {
        if ( StartEvent( Active_SmallEvent ) )
		{
            SendParameter( "ThreadNumber", ThreadNumber );
            SendParameter( "TestInt", TestInt );
            SendParameter( "TestFloat", TestFloat );
            SendParameter( "TestChar", TestChar );
            SendEvent();
        }
    };
    
    void LogEvent_MediumEvent(int ThreadNumber, int TestInt, float TestFloat, bool TestBool, const char* TestChar, const wchar_t* TestWChar)
    {
        if ( StartEvent( Active_MediumEvent ) )
		{
            SendParameter( "ThreadNumber", ThreadNumber );
            SendParameter( "TestInt", TestInt );
            SendParameter( "TestFloat", TestFloat );
            SendParameter( "TestBool", TestBool );
            SendParameter( "TestChar", TestChar );
            SendParameter( "TestWChar", TestWChar );
            SendEvent();
        }
    };
    
    void LogEvent_LargeEvent(int ThreadNumber, int TestInt, float TestFloat, bool TestBool, const char* TestChar, int TestInt1, int TestInt2, int TestInt3, int TestInt4, int TestInt5, int TestInt6, int TestInt7, int TestInt8, const wchar_t* TestWChar, const char* TestChar2, const wchar_t* TestWChar2)
    {
        if ( StartEvent( Active_LargeEvent ) )
		{
            SendParameter( "ThreadNumber", ThreadNumber );
            SendParameter( "TestInt", TestInt );
            SendParameter( "TestFloat", TestFloat );
            SendParameter( "TestBool", TestBool );
            SendParameter( "TestChar", TestChar );
            SendParameter( "TestInt1", TestInt1 );
            SendParameter( "TestInt2", TestInt2 );
            SendParameter( "TestInt3", TestInt3 );
            SendParameter( "TestInt4", TestInt4 );
            SendParameter( "TestInt5", TestInt5 );
            SendParameter( "TestInt6", TestInt6 );
            SendParameter( "TestInt7", TestInt7 );
            SendParameter( "TestInt8", TestInt8 );
            SendParameter( "TestWChar", TestWChar );
            SendParameter( "TestChar2", TestChar2 );
            SendParameter( "TestWChar2", TestWChar2 );
            SendEvent();
        }
    };
    
    void LogEvent_EnumTest(const char* OrdinaryString, const wchar_t* OrdinaryWstring, const char* EnumerationString, const wchar_t* EnumerationWstring)
    {
        if ( StartEvent( Active_EnumTest ) )
		{
            SendParameter( "OrdinaryString", OrdinaryString );
            SendParameter( "OrdinaryWstring", OrdinaryWstring );
            SendParameter( "EnumerationString", EnumerationString, true );
            SendParameter( "EnumerationWstring", EnumerationWstring, true );
            SendEvent();
        }
    };
    
} // namespace Vince
        
#endif // _VINCE_

    