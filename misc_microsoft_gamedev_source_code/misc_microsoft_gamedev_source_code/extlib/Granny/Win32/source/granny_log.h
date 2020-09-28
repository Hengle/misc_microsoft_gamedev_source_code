#if !defined(GRANNY_LOG_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_log.h $
// $DateTime: 2007/04/23 12:49:29 $
// $Change: 14825 $
// $Revision: #17 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

/* ========================================================================
   Explicit Dependencies
   ======================================================================== */
#if !defined(GRANNY_NAMESPACE_H)
#include "granny_namespace.h"
#endif

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE EXPGROUP(LogGroup);

EXPTYPE enum log_message_type
{
    // An ignored log entry is just that - ignored.  It is also
    // the value returned as the "most serious" message type if
    // no messages have been entered.
    IgnoredLogMessage,

    // A note is simply information about something that occurred.  It
    // does not indicate that something bad is happening.
    NoteLogMessage,

    // A warning is an alert that something potentially dangerous may
    // be occurring, although it is not clear from the current context
    // whether or not it will result in undesirable behavior.
    WarningLogMessage,

    // An error is an alert that undesirable behavior will occur, or
    // more to the point, _would be_ occuring if the error checking
    // code did not catch it.
    ErrorLogMessage,

    OnePastLastMessageType
};

EXPTYPE enum log_message_origin
{
    // Debugging
    NotImplementedLogMessage,

    // Application codes
    ApplicationLogMessage,

    // Platform dependent code
    Win32SubsystemLogMessage,
    Win64SubsystemLogMessage,
    MacOSSubsystemLogMessage,
    ANSISubsystemLogMessage,
    GamecubeSubsystemLogMessage,
    PS2SubsystemLogMessage,
    PSPSubsystemLogMessage,
    PS3SubsystemLogMessage,
    XboxSubsystemLogMessage,
    XenonSubsystemLogMessage,

    MAXSubsystemLogMessage,
    MayaSubsystemLogMessage,
    XSISubsystemLogMessage,
    LightwaveSubsystemLogMessage,

    // Platform independent code
    FileWritingLogMessage,
    FileReadingLogMessage,
    ExporterLogMessage,
    CompressorLogMessage,
    StringLogMessage,
    StringTableLogMessage,
    VertexLayoutLogMessage,
    MeshLogMessage,
    PropertyLogMessage,
    SkeletonLogMessage,
    AnimationLogMessage,
    SetupGraphLogMessage,
    TextureLogMessage,
    BSplineLogMessage,
    HashLogMessage,
    LinkerLogMessage,
    InstantiatorLogMessage,
    DataTypeLogMessage,
    NameMapLogMessage,
    MaterialLogMessage,
    ModelLogMessage,
    StackAllocatorLogMessage,
    FixedAllocatorLogMessage,
    SceneLogMessage,
    TrackMaskLogMessage,
    LocalPoseLogMessage,
    WorldPoseLogMessage,
    NameLibraryLogMessage,
    ControlLogMessage,
    MeshBindingLogMessage,
    MathLogMessage,
    VersionLogMessage,
    MemoryLogMessage,
    DeformerLogMessage,
    VoxelLogMessage,
    BitmapLogMessage,
    IKLogMessage,
    CurveLogMessage,
    TrackGroupLogMessage,
    ThreadSafetyLogMessage,
    QuantizeLogMessage,
    BlendDagLogMessage,

    // ...add new message sources here.  Make sure to update GetMessageOriginString...

    // Terminator
    OnePastLastMessageOrigin
};

// Logs will not begin until SetLogFileName is called with a non-0 pointer.
// Logging will terminate once SetLogFileName is called with a 0 pointer.
// The optional "clear" parameter, if set to true, will clear out
// the log file of any current contents.  If false, log entries are
// appended to the file.
// If SetLogFileName discovers that a specified log file cannot be opened
// for writing, it will return false.  Otherwise, it will return true.
EXPAPI GS_MODIFY bool SetLogFileName(char const *FileName, bool Clear);

// Alternatively (or additionally), you can get called back on log
// messages to dump them to your own logfile or display them in a console.
EXPAPI typedef void log_function(log_message_type Type,
                                 log_message_origin Origin,
                                 char const *Message,
                                 void *UserData);
EXPTYPE_EPHEMERAL struct log_callback
{
    log_function *Function;
    void *UserData;
};

EXPAPI GS_SPECIAL void GetLogCallback(log_callback* Result);
EXPAPI GS_SPECIAL void SetLogCallback(log_callback const &LogCallback);

EXPAPI GS_SAFE char const* GetLogMessageTypeString(log_message_type Type);
EXPAPI GS_SAFE char const* GetLogMessageOriginString(log_message_origin Origin);

// Logging() returns true if logging is active, false if it is not.
// This can be used to ignore parts of the code that are for logging
// purposes only, to avoid speed hits in the case where logging is
// not needed.
EXPAPI GS_READ bool Logging(void);

#if !GRANNY_NO_LOGGING

#define Log0(T, O, F) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__);
#define Log1(T, O, F, a) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a);
#define Log2(T, O, F, a, b) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b);
#define Log3(T, O, F, a, b, c) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b, c);
#define Log4(T, O, F, a, b, c, d) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b, c, d);
#define Log5(T, O, F, a, b, c, d, e) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b, c, d, e);
#define Log6(T, O, F, a, b, c, d, e, f) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b, c, d, e, f);
#define Log7(T, O, F, a, b, c, d, e, f, g) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b, c, d, e, f, g);
#define Log8(T, O, F, a, b, c, d, e, f, g, h) _Log(T, O, "%s(%d): " F, __FILE__, __LINE__, a, b, c, d, e, f, g, h);
void _Log(log_message_type Type, log_message_origin Origin,
          char const *FormatString, ...);

#define LogNotImplemented(Message) _Log(WarningLogMessage, NotImplementedLogMessage, Message)

// The message counters indicate the number of times a particular
// type of message or message origin showed up in the log since the
// last time it was cleared (or the beginning of the program, if
// it was never cleared).
// TODO: Replace these with new log listing scheme (described on
// to-do list)
int32x GetMessageTypeCount(log_message_type Type);
int32x GetMessageOriginCount(log_message_origin Origin);
void ClearMessageTypeCounts(void);
void ClearMessageOriginCounts(void);

#else

#define Log0(T, O, F)
#define Log1(T, O, F, a)
#define Log2(T, O, F, a, b)
#define Log3(T, O, F, a, b, c)
#define Log4(T, O, F, a, b, c, d)
#define Log5(T, O, F, a, b, c, d, e)
#define Log6(T, O, F, a, b, c, d, e, f)
#define Log7(T, O, F, a, b, c, d, e, f, g)
#define Log8(T, O, F, a, b, c, d, e, f, g, h)

#define LogNotImplemented(Message)


// The message counters indicate the number of times a particular
// type of message or message origin showed up in the log since the
// last time it was cleared (or the beginning of the program, if
// it was never cleared).
// TODO: Replace these with new log listing scheme (described on
// to-do list)
inline int32x GetMessageTypeCount(log_message_type Type){return 0;}
inline int32x GetMessageOriginCount(log_message_origin Origin){return 0;}
inline void ClearMessageTypeCounts(void){}
inline void ClearMessageOriginCounts(void){}


#endif


// Filtering is straightforward.
EXPAPI GS_MODIFY void FilterMessage(log_message_origin Origin, bool Enabled);
EXPAPI GS_MODIFY void FilterAllMessages(bool Enabled);

// The most serious (ie, Error is more serious than warning, etc.)
// message is always kept available, even when logging is disabled.
// Clearing it resets everything and looks for the new most serious
// message.
EXPAPI GS_READ log_message_type GetMostSeriousMessageType(void);
EXPAPI GS_READ char const *GetMostSeriousMessage(void);
EXPAPI GS_MODIFY void ClearMostSeriousMessage(void);



END_GRANNY_NAMESPACE;

#include "header_postfix.h"
#define GRANNY_LOG_H
#endif
