// ========================================================================
// $File: //jeffr/granny/rt/granny_log.cpp $
// $DateTime: 2007/10/03 14:47:54 $
// $Change: 16131 $
// $Revision: #11 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if !defined(GRANNY_LOG_H)
#include "granny_log.h"
#endif

#if !defined(GRANNY_ASSERT_H)
#include "granny_assert.h"
#endif

#if !defined(GRANNY_STRING_H)
#include "granny_string.h"
#endif

#if !defined(GRANNY_MEMORY_H)
#include "granny_memory.h"
#endif

#if !defined(GRANNY_FILE_WRITER_H)
#include "granny_file_writer.h"
#endif

#if !defined(GRANNY_STRING_FORMATTING_H)
#include "granny_string_formatting.h"
#endif

#if !defined(GRANNY_LIMITS_H)
#include "granny_limits.h"
#endif

#if !defined(GRANNY_CPP_SETTINGS_H)
// This should always be the last header included
#include "granny_cpp_settings.h"
#endif

USING_GRANNY_NAMESPACE;

BEGIN_GRANNY_NAMESPACE;

log_callback LogCallback;

END_GRANNY_NAMESPACE;


#if GRANNY_NO_LOGGING

bool GRANNY
SetLogFileName(char const *FileName, bool Clear)
{
    return false;
}

void GRANNY
GetLogCallback(log_callback* Result)
{
    Result->Function = 0;
    Result->UserData = 0;
}

void GRANNY
SetLogCallback(log_callback const &LogCallbackInit)
{
}

bool GRANNY
Logging(void)
{
    return false;
}

void GRANNY
FilterMessage(log_message_origin Origin, bool Enabled)
{
}

void GRANNY
FilterAllMessages(bool Enabled)
{
}

log_message_type GRANNY GetMostSeriousMessageType(void)
{
    return ((log_message_type)0 );
}
char const *GRANNY GetMostSeriousMessage(void)
{
    return "";
}
void GRANNY ClearMostSeriousMessage(void)
{
}

char const* GRANNY
GetLogMessageTypeString(log_message_type Type)
{
    return "UnknownMessageType";
}

char const* GRANNY
GetLogMessageOriginString(log_message_origin Origin)
{
    return "UnknownMessageOrigin";
}


#else //#if GRANNY_NO_LOGGING



static bool LogIsDisabled[OnePastLastMessageOrigin];

static int32x TypeCount[OnePastLastMessageType];
static int32x OriginCount[OnePastLastMessageOrigin];

static log_message_type MostSeriousMessageType = IgnoredLogMessage;
static char LogFileName[MaximumLogFileNameSize] = "";
static char MostSeriousMessage[MaximumMessageBufferSize];
static char TempMessageBuffer[MaximumMessageBufferSize];

bool GRANNY
SetLogFileName(char const *FileName, bool Clear)
{
    bool Result = true;

    if(FileName)
    {
        StringEquals(LogFileName, SizeOf(LogFileName), FileName);
    }
    else
    {
        LogFileName[0] = '\0';
    }

    if(*LogFileName)
    {
        file_writer *Writer = NewFileWriter(FileName, Clear);
        if(Writer)
        {
            DeleteFileWriter(Writer);
        }
        else
        {
            Result = false;
        }
    }

    return(Result);
}


void GRANNY
GetLogCallback(log_callback* Result)
{
    Assert(Result);

    *Result = LogCallback;
}

void GRANNY
SetLogCallback(log_callback const &LogCallbackInit)
{
    LogCallback = LogCallbackInit;
}

bool GRANNY
Logging(void)
{
    return(*LogFileName || LogCallback.Function);
}

static void
WriteMessage(log_message_type Type, log_message_origin Origin,
             int32x Length, char const *Message)
{
    if(*LogFileName)
    {
        static bool AvoidInfiniteRecursion = true;

        if(AvoidInfiniteRecursion)
        {
            AvoidInfiniteRecursion = false;

            file_writer *Writer = NewFileWriter(LogFileName, false);
            if(Writer)
            {
                SeekWriterFromEnd(*Writer, 0);
                Write(*Writer, Length, Message);

                // TODO: Platform-dependent EOL?
                Write(*Writer, 1, "\n");

                DeleteFileWriter(Writer);
            }

            AvoidInfiniteRecursion = true;
        }
    }

    if(LogCallback.Function)
    {
        LogCallback.Function(Type, Origin, Message, LogCallback.UserData);
    }
}

void GRANNY
_Log(log_message_type Type, log_message_origin Origin,
     char const *FormatString, ...)
{
    Assert(Type < OnePastLastMessageType);
    Assert(Origin < OnePastLastMessageOrigin);

    ++TypeCount[Type];
    ++OriginCount[Origin];

    if(Type != IgnoredLogMessage)
    {
        if(Type > MostSeriousMessageType)
        {
            // This message is our new most serious message, so we
            // write it to the serious message buffer and then to the file.
            MostSeriousMessageType = Type;

            c_argument_list Args;
            OpenArgumentList(Args, FormatString);
            int32x Length =
                ConvertToStringList(SizeOf(MostSeriousMessage),
                                    MostSeriousMessage,
                                    FormatString, Args);
            CloseArgumentList(Args);

            if(!LogIsDisabled[Origin])
            {
                WriteMessage(Type, Origin, Length, MostSeriousMessage);
            }
        }
        else
        {
            // This message is a regular log message, so we write it
            // directly to the log file.
            if(!LogIsDisabled[Origin] && Logging())
            {
                c_argument_list Args;
                OpenArgumentList(Args, FormatString);
                int32x Length = ConvertToStringList(SizeOf(TempMessageBuffer),
                                                    TempMessageBuffer,
                                                    FormatString, Args);
                CloseArgumentList(Args);

                WriteMessage(Type, Origin, Length, TempMessageBuffer);
            }
        }
    }
}

void GRANNY
FilterMessage(log_message_origin Origin, bool Enabled)
{
    Assert(Origin < OnePastLastMessageOrigin);
    LogIsDisabled[Origin] = !Enabled;
}

void GRANNY
FilterAllMessages(bool Enabled)
{
    {for(int32x OriginIndex = 0;
         OriginIndex < OnePastLastMessageOrigin;
         ++OriginIndex)
    {
        LogIsDisabled[OriginIndex] = !Enabled;
    }}
}

log_message_type GRANNY
GetMostSeriousMessageType(void)
{
    return(MostSeriousMessageType);
}

char const *GRANNY
GetMostSeriousMessage(void)
{
    return(MostSeriousMessage);
}

void GRANNY
ClearMostSeriousMessage(void)
{
    MostSeriousMessageType = IgnoredLogMessage;
    MostSeriousMessage[0] = '\0';
}

int32x GRANNY
GetMessageTypeCount(log_message_type Type)
{
    Assert(Type < OnePastLastMessageType);
    return(TypeCount[Type]);
}

int32x GRANNY
GetMessageOriginCount(log_message_origin Origin)
{
    Assert(Origin < OnePastLastMessageOrigin);
    return(OriginCount[Origin]);
}

void GRANNY
ClearMessageTypeCounts(void)
{
    {for(int32x OriginIndex = 0;
         OriginIndex < OnePastLastMessageOrigin;
         ++OriginIndex)
    {
        OriginCount[OriginIndex] = 0;
    }}
}

void GRANNY
ClearMessageOriginCounts(void)
{
    {for(int32x TypeIndex = 0;
         TypeIndex < OnePastLastMessageType;
         ++TypeIndex)
    {
        TypeCount[TypeIndex] = 0;
    }}
}


#define ENUM_STRING_CASE(x) case x ## LogMessage: return #x

char const* GRANNY
GetLogMessageTypeString(log_message_type Type)
{
    switch (Type)
    {
        ENUM_STRING_CASE(Ignored);
        ENUM_STRING_CASE(Note);
        ENUM_STRING_CASE(Warning);
        ENUM_STRING_CASE(Error);

        default:
            return "UnknownMessageType";
    }
}

char const* GRANNY
GetLogMessageOriginString(log_message_origin Origin)
{
    switch (Origin)
    {
        ENUM_STRING_CASE(NotImplemented);
        ENUM_STRING_CASE(Application);
        ENUM_STRING_CASE(Win32Subsystem);
        ENUM_STRING_CASE(Win64Subsystem);
        ENUM_STRING_CASE(MacOSSubsystem);
        ENUM_STRING_CASE(ANSISubsystem);
        ENUM_STRING_CASE(GamecubeSubsystem);
        ENUM_STRING_CASE(PS2Subsystem);
        ENUM_STRING_CASE(PSPSubsystem);
        ENUM_STRING_CASE(PS3Subsystem);
        ENUM_STRING_CASE(XboxSubsystem);
        ENUM_STRING_CASE(XenonSubsystem);
        ENUM_STRING_CASE(MAXSubsystem);
        ENUM_STRING_CASE(MayaSubsystem);
        ENUM_STRING_CASE(XSISubsystem);
        ENUM_STRING_CASE(LightwaveSubsystem);
        ENUM_STRING_CASE(FileWriting);
        ENUM_STRING_CASE(FileReading);
        ENUM_STRING_CASE(Exporter);
        ENUM_STRING_CASE(Compressor);
        ENUM_STRING_CASE(String);
        ENUM_STRING_CASE(StringTable);
        ENUM_STRING_CASE(VertexLayout);
        ENUM_STRING_CASE(Mesh);
        ENUM_STRING_CASE(Property);
        ENUM_STRING_CASE(Skeleton);
        ENUM_STRING_CASE(Animation);
        ENUM_STRING_CASE(SetupGraph);
        ENUM_STRING_CASE(Texture);
        ENUM_STRING_CASE(BSpline);
        ENUM_STRING_CASE(Hash);
        ENUM_STRING_CASE(Linker);
        ENUM_STRING_CASE(Instantiator);
        ENUM_STRING_CASE(DataType);
        ENUM_STRING_CASE(NameMap);
        ENUM_STRING_CASE(Material);
        ENUM_STRING_CASE(Model);
        ENUM_STRING_CASE(StackAllocator);
        ENUM_STRING_CASE(FixedAllocator);
        ENUM_STRING_CASE(Scene);
        ENUM_STRING_CASE(TrackMask);
        ENUM_STRING_CASE(LocalPose);
        ENUM_STRING_CASE(WorldPose);
        ENUM_STRING_CASE(NameLibrary);
        ENUM_STRING_CASE(Control);
        ENUM_STRING_CASE(MeshBinding);
        ENUM_STRING_CASE(Math);
        ENUM_STRING_CASE(Version);
        ENUM_STRING_CASE(Memory);
        ENUM_STRING_CASE(Deformer);
        ENUM_STRING_CASE(Voxel);
        ENUM_STRING_CASE(Bitmap);
        ENUM_STRING_CASE(IK);
        ENUM_STRING_CASE(Curve);
        ENUM_STRING_CASE(TrackGroup);
        ENUM_STRING_CASE(ThreadSafety);
        ENUM_STRING_CASE(Quantize);
        ENUM_STRING_CASE(BlendDag);

        default:
            return "UnknownMessageOrigin";
    }

}

#endif //#else //#if GRANNY_NO_LOGGING


