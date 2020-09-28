/**********************************************************************

Filename    :   GFxExportFsCommands.cpp
Content     :   SWF to GFX resource extraction and conversion tool
Created     :   October, 2006
Authors     :   Artyom Bolgar
Copyright   :   (c) 2006 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#define GFX_EXPORT_MAJOR_VERSION    1
#define GFX_EXPORT_MINOR_VERSION    11
#define GFX_EXPORT_VERSION (((GFX_EXPORT_MAJOR_VERSION)<<8)|(GFX_EXPORT_MINOR_VERSION))

#include "GFxExport.h"
#include "GFxPlayerImpl.h"

// Standard includes
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <zlib.h>

struct GFxFsCommand
{
    int                         EventId;  // for placeObject
    GFxString                   EventStr; // for placeObject
    int                         Conditions; // button condition mask
    GTL::garray<int>            CommandIndices;
    GTL::garray<int>            ParameterIndices;

    GFxFsCommand() : EventId(0), Conditions(0) {}
};

struct GFxFsCommandOrigin
{
    enum 
    {
        None,
        Action,
        InitAction,
        Button,
        MovieClip,
        PlaceObject
    }                         Type;
    int                       Id;
    GFxString                 Name;
    GTL::garray<GFxFsCommand> Commands;
    GTL::garray<GFxFsCommandOrigin> NestedCommands;

    GFxFsCommandOrigin() : Type(None), Id(0) {}
};

static const UByte* memstr (const UByte * str1, size_t strSize,
                            const char * str2)
{
    if ( !*str2 )
        return(str1);

    size_t i;
    for(i = 0; i < strSize; i++)
    {
        size_t j = i, k = 0;
        while ( j < strSize && *(str2 + k) && !(*(str1 + j)-*(str2 + k)) )
            j++, k++;

        if (!*(str2 + k))
            return(str1 + i);
    }

    return(NULL);

}

template <class T>
T* GetBufferPtr(UInt reqSize, T** pdynBuf, UInt* pdynBufSize, T* statBuf, UInt statBufSize)
{
    if (statBuf && reqSize <= statBufSize)
        return statBuf;
    if (pdynBufSize && pdynBuf && *pdynBuf && reqSize <= *pdynBufSize)
        return *pdynBuf;
    T* pbuf = (T*) GALLOC(reqSize);
    if (pdynBuf)
        *pdynBuf = pbuf;
    if (pdynBufSize)
        *pdynBufSize = reqSize;
    return pbuf;
}

template <class T>
void FreeBuffer(T** pdynBuf, UInt* pdynBufSize)
{
    if (pdynBuf && *pdynBuf)
    {
        GFREE(*pdynBuf);
        *pdynBuf = 0;
    }
    if (pdynBufSize)
        *pdynBufSize = 0;
}

static void WriteIndents(FILE* fp, UInt indent)
{
    for (UInt i = 0; i < indent; i++)
        fputs("   ", fp);
}

static int SearchForCmds(const UByte* pbuf, UPInt bufSize, GFxFsCommand& cmd, GTL::garray<GFxString>& stringHolderArray, bool exportParams)
{
    if (!pbuf)
        return 0;
    const UByte* fsCmd = pbuf;
    UPInt remainingSize = bufSize;
    const UByte* newfsCmd;
    do 
    {
        newfsCmd = memstr(fsCmd, remainingSize, "FSCommand:");
        if (newfsCmd)
        {
            size_t len = strlen((const char*)newfsCmd);
            stringHolderArray.push_back(GFxString((const char*)newfsCmd + sizeof("FSCommand:")-1));
            cmd.CommandIndices.push_back((int)stringHolderArray.size()-1);

            remainingSize -= (len + (newfsCmd - fsCmd));
            fsCmd = newfsCmd;
            fsCmd += len;

            if (exportParams)
            {
                const unsigned char* pend = (const unsigned char*)fsCmd;
                const unsigned char* pparam = pend + 1;
                for (UInt i = 1; i < remainingSize; ++i)
                {
                    ++pend;
                    if (*pend == 0)
                        break;
                    if (*pend < 32 || *pend > 127)
                    {
                        pparam = NULL;
                        break;
                    }
                }
                if (pparam && *pend == 0)
                {
                    stringHolderArray.push_back(GFxString((const char*)pparam));
                    cmd.ParameterIndices.push_back((int)stringHolderArray.size()-1);
                }
                else
                    cmd.ParameterIndices.push_back(-1);
            }
            else
                cmd.ParameterIndices.push_back(-1);

        }
    } while(newfsCmd != 0 && remainingSize > 0);
    return (int)cmd.CommandIndices.size();
}

static GFxString KeyCode2Str(int kc)
{
    GFxString keyStr = "";
    switch(kc)
    {
    case 1: keyStr = "<Left>"; break;
    case 2: keyStr = "<Right>"; break;
    case 3: keyStr = "<Home>"; break;
    case 4: keyStr = "<End>"; break;
    case 5: keyStr = "<Insert>"; break;
    case 6: keyStr = "<Delete>"; break;
    case 8: keyStr = "<Backspace>"; break;
    case 13: keyStr = "<Enter>"; break;
    case 14: keyStr = "<Up>"; break;
    case 15: keyStr = "<Down>"; break;
    case 16: keyStr = "<PageUp>"; break;
    case 17: keyStr = "<PageDown>"; break;
    case 18: keyStr = "<Tab>"; break;
    case 19: keyStr = "<Escape>"; break;
    case 32: keyStr = "<Space>"; break;
    default: 
        {
            if (kc >= 32 && kc <= 126)
            {
                char buf[2];
                buf[0] = char(kc);
                buf[1] = '\0';
                keyStr = buf;
            }
        }
    }
    return keyStr;
}

static void WriteFsCommand(FILE* fp, const GFxFsCommand& cmd, GTL::garray<GFxString>& stringHolderArray, bool writeParams, int indent)
{
    for (UPInt j = 0, k = cmd.CommandIndices.size(); j < k; j++)
    {
        WriteIndents(fp, indent);
        fprintf(fp, "%s", stringHolderArray[cmd.CommandIndices[j]].ToCStr());
        if (writeParams && cmd.ParameterIndices[j] >= 0 && stringHolderArray[cmd.ParameterIndices[j]].GetSize() > 0)
        {
            fprintf(fp, "(\"%s\")", stringHolderArray[cmd.ParameterIndices[j]].ToCStr());
        }
        fprintf(fp, "\n");
    }
}

void GFxDataExporter::LookForFsCommandsInTags
    (GFxMovieDef* pmovieDef, GFile* pin, UInt finalOffset, GTL::garray<GFxFsCommandOrigin>& fscommands, 
     GTL::garray<GFxString>& stringHolderArray)
{
    GUNUSED(pmovieDef);
    GUNUSED4(pin, finalOffset, fscommands, stringHolderArray);

    GFxStream sin(pin, NULL, NULL);
    GFxTagInfo tag;

    UByte readStatBuf[4096];
    UByte* preadDynBuf = 0;
    UInt dynReadBufSize = 0;
    while (sin.Tell() < (SInt)finalOffset)
    {
        int tagType = sin.OpenTag(&tag);
        GASSERT(tag.TagOffset + tag.TagLength <= (SInt)finalOffset);

        sin.SyncFileStream();

        // skip tag header
        pin->Seek(tag.TagDataOffset);

        switch(tagType)
        {
        case 12:
        case 59:
            {
                UByte* preadBuf = GetBufferPtr(tag.TagLength, &preadDynBuf, &dynReadBufSize, readStatBuf, sizeof(readStatBuf));
                if (pin->Read(preadBuf, SInt(tag.TagLength)) != SInt(tag.TagLength))
                {
                    fprintf(stderr, "\nError: Can't read from '%s'\n", pin->GetFilePath());
                    break;
                }
                GFxFsCommand cmd;
                if (SearchForCmds(preadBuf, tag.TagLength, cmd, stringHolderArray, FsCommandsParams) > 0)
                {
                    GFxFsCommandOrigin cmdOrig;
                    if (tagType == 12)
                        cmdOrig.Type = GFxFsCommandOrigin::Action;
                    else
                        cmdOrig.Type = GFxFsCommandOrigin::InitAction;
                    cmdOrig.Commands.push_back(cmd);
                    fscommands.push_back(cmdOrig);
                }
            }
            break;
        case 7:  // DefineButton
        case 34: // DefineButton2
            {
                UInt16 id = pin->ReadUInt16(); // button id

                GFxMovieDefImpl* pmovieImpl = static_cast<GFxMovieDefImpl*>(pmovieDef);
                GFxString exportName;
                const GFxString* pname = pmovieImpl->GetNameOfExportedResource(GFxResourceId(id));
                if (pname)
                    exportName = *pname;

                GFxFsCommandOrigin cmdOrig;
                cmdOrig.Type = GFxFsCommandOrigin::Button;
                cmdOrig.Id = id;
                cmdOrig.Name = exportName;

                GTL::garray<GFxFsCommand>& fscmds = cmdOrig.Commands;

                if (tag.TagType == 34)
                {
                    pin->ReadUByte(); // skip octet
                    int curoff = pin->Tell();
                    UInt16 actionOff = pin->ReadUInt16(); // actions offset
                    curoff += actionOff;
                    pin->Seek(curoff); // move to condactions

                    // read cond actions
                    while(curoff < tag.TagDataOffset + tag.TagLength)
                    {
                        UInt16 nextActionOff = pin->ReadUInt16();
                        UInt16 conditions = pin->ReadUInt16();

                        UInt actionSize;
                        if (nextActionOff == 0)
                            actionSize = (tag.TagDataOffset + tag.TagLength) - pin->Tell();
                        else
                            actionSize = nextActionOff;
                        UByte* preadBuf = GetBufferPtr(actionSize, &preadDynBuf, &dynReadBufSize, readStatBuf, sizeof(readStatBuf));
                        if (pin->Read(preadBuf, SInt(actionSize)) != SInt(actionSize))
                        {
                            fprintf(stderr, "\nError: Can't read from '%s'\n", pin->GetFilePath());
                            break;
                        }
                        GFxFsCommand cmd;
                        cmd.Conditions = conditions;

                        if (SearchForCmds(preadBuf, actionSize, cmd, stringHolderArray, FsCommandsParams))
                            fscmds.push_back(cmd);

                        if (nextActionOff == 0)
                            break;
                        curoff += actionSize;
                        pin->Seek(curoff);
                    }
                }
                else
                {
                    UInt actionSize = (tag.TagDataOffset + tag.TagLength) - pin->Tell();
                    UByte* preadBuf = GetBufferPtr(actionSize, &preadDynBuf, &dynReadBufSize, readStatBuf, sizeof(readStatBuf));
                    if (pin->Read(preadBuf, SInt(actionSize)) != SInt(actionSize))
                    {
                        fprintf(stderr, "\nError: Can't read from '%s'\n", pin->GetFilePath());
                        break;
                    }
                    GFxFsCommand cmd;

                    if (SearchForCmds(preadBuf, actionSize, cmd, stringHolderArray, FsCommandsParams))
                        fscmds.push_back(cmd);

                }

                if (fscmds.size() > 0)
                {
                    fscommands.push_back(cmdOrig);
                }
            }
            break;
        case 39: // Define Sprite
            {
                UInt16 id = pin->ReadUInt16(); // sprite id
                pin->ReadUInt16(); // skip numFrames
                GFxMovieDefImpl* pmovieImpl = static_cast<GFxMovieDefImpl*>(pmovieDef);
                GFxString exportName;
                const GFxString* pname = pmovieImpl->GetNameOfExportedResource(GFxResourceId(id));
                if (pname)
                    exportName = *pname;

                GFxFsCommandOrigin cmdOrig;
                cmdOrig.Type = GFxFsCommandOrigin::MovieClip;
                cmdOrig.Id = id;
                cmdOrig.Name = exportName;

                SInt curOff = pin->Tell();
                UInt size = tag.TagLength - (curOff - tag.TagDataOffset);
                UByte* preadBuf = GetBufferPtr(size, &preadDynBuf, &dynReadBufSize, readStatBuf, sizeof(readStatBuf));
                if (pin->Read(preadBuf, SInt(size)) != SInt(size))
                {
                    fprintf(stderr, "\nError: Can't read from '%s'\n", pin->GetFilePath());
                    break;
                }
                pin->Seek(curOff);

                // do a preliminary search
                GFxFsCommand cmd;
                if (SearchForCmds(preadBuf, tag.TagLength, cmd, stringHolderArray, FsCommandsParams) > 0)
                {
                    LookForFsCommandsInTags(pmovieDef, pin, tag.TagDataOffset + tag.TagLength, cmdOrig.NestedCommands, stringHolderArray);
                    fscommands.push_back(cmdOrig);
                }
            }
            break;
        case 26: // PlaceObj2
        case 70: // PlaceObj3
            {
                GFxPlaceObject2 placeObj2;
                placeObj2.Read(&sin, (GFxTagType)tagType, MovieInfo.Version);
                if (placeObj2.EventHandlers.size() > 0)
                {
                    GFxFsCommandOrigin cmdOrig;
                    cmdOrig.Type = GFxFsCommandOrigin::PlaceObject;
                    cmdOrig.Id = placeObj2.Pos.CharacterId.GetIdIndex();
                    cmdOrig.Name = placeObj2.Name;

                    // check event handlers for fscommands
                    for(UPInt i = 0, n = placeObj2.EventHandlers.size(); i < n; ++i)
                    {
                        GFxSwfEvent* pev = placeObj2.EventHandlers[i];
                        GASSERT(pev);
                        if (!pev->pActionOpData)
                            continue;
                        const UByte* pactions = pev->pActionOpData->GetBufferPtr();
                        UInt actionSize = pev->pActionOpData->GetLength();

                        GFxFsCommand cmd;
                        if (SearchForCmds(pactions, actionSize, cmd, stringHolderArray, FsCommandsParams) > 0)
                        {
                            cmd.EventId = pev->Event.Id;
                            GFxString eventStr;
                            switch(pev->Event.Id)
                            {
                            case GFxEventId::Event_Press: eventStr = "on("; eventStr += "press"; break;
                            case GFxEventId::Event_Release: eventStr = "on("; eventStr += "release"; break;
                            case GFxEventId::Event_ReleaseOutside: eventStr = "on("; eventStr += "releaseOutside"; break;
                            case GFxEventId::Event_RollOver: eventStr = "on("; eventStr += "rollOver"; break;
                            case GFxEventId::Event_RollOut: eventStr = "on("; eventStr += "rollOut"; break;
                            case GFxEventId::Event_DragOver: eventStr = "on("; eventStr += "dragOver"; break;
                            case GFxEventId::Event_DragOut: eventStr = "on("; eventStr += "dragOut"; break;
                            case GFxEventId::Event_KeyPress: 
                                {
                                    eventStr = "on("; eventStr += "keyPress \""; 
                                    eventStr += KeyCode2Str(pev->Event.KeyCode);
                                    eventStr += "\"";
                                    break;
                                }

                            case GFxEventId::Event_Initialize: eventStr = "onClipEvent("; eventStr += "initialize"; break;
                            case GFxEventId::Event_Load: eventStr = "on("; eventStr += "load"; break;
                            case GFxEventId::Event_Unload: eventStr = "on("; eventStr += "unload"; break;
                            case GFxEventId::Event_EnterFrame: eventStr = "onClipEvent("; eventStr += "enterFrame"; break;
                            case GFxEventId::Event_MouseDown: eventStr = "onClipEvent("; eventStr += "mouseDown"; break;
                            case GFxEventId::Event_MouseUp: eventStr = "onClipEvent("; eventStr += "mouseUp"; break;
                            case GFxEventId::Event_MouseMove: eventStr = "onClipEvent("; eventStr += "mouseMove"; break;
                            case GFxEventId::Event_KeyDown: eventStr = "onClipEvent("; eventStr += "keyDown"; break;
                            case GFxEventId::Event_KeyUp: eventStr = "onClipEvent("; eventStr += "keyUp"; break;
                            case GFxEventId::Event_Data: eventStr = "on("; eventStr += "data"; break;
                            case GFxEventId::Event_Construct: eventStr = "onClipEvent("; eventStr += "construct"; break;
                            default: eventStr = "(unknown";
                            }
                            eventStr += ")";
                            cmd.EventStr = eventStr;
                            cmdOrig.Commands.push_back(cmd);
                        }
                    }
                    if (cmdOrig.Commands.size() > 0)
                        fscommands.push_back(cmdOrig);
                }
            }
            break;
        }
        sin.CloseTag();
        sin.SyncFileStream();
    }
    FreeBuffer(&preadDynBuf, &dynReadBufSize);
}

void GFxDataExporter::DumpFsCommandsAsTree(FILE* fout, GTL::garray<GFxFsCommandOrigin>& fscommands, 
                                           GTL::garray<GFxString>& stringHolderArray, int indent)
{
    for (UPInt i = 0, n = fscommands.size(); i < n; ++i)
    {
        GFxFsCommandOrigin& cmdOrig = fscommands[i];
        switch(cmdOrig.Type)
        {
        case GFxFsCommandOrigin::None:
            break;

        case GFxFsCommandOrigin::Action:
        case GFxFsCommandOrigin::InitAction:
            WriteIndents(fout, indent);
            if (cmdOrig.Type == GFxFsCommandOrigin::Action)
                fprintf(fout, "Action:\n");
            else
                fprintf(fout, "InitAction:\n");
            WriteFsCommand(fout, cmdOrig.Commands[0], stringHolderArray, FsCommandsParams, indent + 1);
            fputs("\n", fout);
            break;
        case GFxFsCommandOrigin::Button:
            {
                WriteIndents(fout, indent);
                fprintf(fout, "Button, id = %d", cmdOrig.Id);
                if (cmdOrig.Name.GetSize() > 0)
                    fprintf(fout, ", export name = %s", cmdOrig.Name.ToCStr());
                fprintf(fout, ":\n");
                for(UPInt i = 0, n = cmdOrig.Commands.size(); i < n; i++)
                {
                    GFxFsCommand& cmd = cmdOrig.Commands[i];
                    int ind = indent;
                    if (cmd.Conditions != 0)
                    {
                        ind++;
                        WriteIndents(fout, ind);
                        fprintf(fout, "on(");
                        UInt nn = 0;
                        for(UInt j = 0, mask = 1; j < 9; j++, mask <<= 1)
                        {
                            const char* event = NULL;
                            if (cmd.Conditions & mask)
                            {
                                if (mask == GFxButtonAction::IDLE_TO_OVER_UP)
                                    event = "rollOver";
                                else if (mask == GFxButtonAction::OVER_UP_TO_IDLE)
                                    event = "rollOut";
                                else if (mask == GFxButtonAction::OVER_UP_TO_OVER_DOWN)
                                    event = "press";
                                else if (mask == GFxButtonAction::OVER_DOWN_TO_OVER_UP)
                                    event = "release";
                                else if (mask == GFxButtonAction::OVER_DOWN_TO_OUT_DOWN)
                                    event = "dragOut";
                                else if (mask == GFxButtonAction::OUT_DOWN_TO_OVER_DOWN)
                                    event = "dragOver";
                                else if (mask == GFxButtonAction::OUT_DOWN_TO_IDLE)
                                    event = "releaseOutside";
                                else if (mask == GFxButtonAction::IDLE_TO_OVER_DOWN)
                                    event = "mouseDown";
                                else if (mask == GFxButtonAction::OVER_DOWN_TO_IDLE)
                                    event = "mouseUp";
                                if (event)
                                {
                                    if (nn != 0)
                                        fprintf(fout, ", ");
                                    fprintf(fout, "%s", event);
                                    nn++;
                                }
                            }
                        }
                        if (cmd.Conditions & 0xFE00)
                        {
                            // keyPress
                            int kc = (cmd.Conditions >> 9) & 0x7F;
                            GFxString keyStr = KeyCode2Str(kc);
                            if (nn != 0)
                                fprintf(fout, ", ");
                            fprintf(fout, "keyPress \"%s\"", keyStr.ToCStr());
                        }
                        fprintf (fout, "):\n");
                        ind++;
                    }
                    WriteFsCommand(fout, cmd, stringHolderArray, FsCommandsParams, ind);
                    fputs("\n", fout);
                }
                fputs("\n", fout);
            }
            break;
        case GFxFsCommandOrigin::MovieClip:
            WriteIndents(fout, indent);
            fprintf(fout, "MovieClip, id = %d", cmdOrig.Id);
            if (cmdOrig.Name.GetSize() > 0)
                fprintf(fout, ", export name = %s", cmdOrig.Name.ToCStr());
            fprintf(fout, ":\n");
            if (cmdOrig.NestedCommands.size() > 0)
                DumpFsCommandsAsTree(fout, cmdOrig.NestedCommands, stringHolderArray, indent + 1);
            fputs("\n", fout);
            break;
        case GFxFsCommandOrigin::PlaceObject:
            WriteIndents(fout, indent);
            fprintf(fout, "Object instance");
            if (cmdOrig.Id != 0)
                fprintf(fout, ", id = %d", cmdOrig.Id);
            if (cmdOrig.Name.GetSize() > 0)
                fprintf(fout, ", instance name = %s", cmdOrig.Name.ToCStr());
            fprintf(fout, ":\n");
            for(UPInt i = 0, n = cmdOrig.Commands.size(); i < n; i++)
            {
                GFxFsCommand& cmd = cmdOrig.Commands[i];
                int ind = indent + 1;
                          
                WriteIndents(fout, ind++);
                fprintf(fout, "%s:\n", cmd.EventStr.ToCStr());
                WriteFsCommand(fout, cmd, stringHolderArray, FsCommandsParams, ind);
                fputs("\n", fout);
            }
            fputs("\n", fout);
            break;
        }
    }
}

static void AddFsCommandsToList(GTL::garray<GFxString>& cmdList, GTL::garray<int>& cmdSortedIdx, const GFxFsCommand& cmd, GTL::garray<GFxString>& stringHolderArray, bool addParams)
{
    for (UPInt j = 0, k = cmd.CommandIndices.size(); j < k; j++)
    {
        GFxString str = stringHolderArray[cmd.CommandIndices[j]].ToCStr();
        if (addParams && cmd.ParameterIndices[j] >= 0 && stringHolderArray[cmd.ParameterIndices[j]].GetSize() > 0)
        {
            str += "(\"";
            str += stringHolderArray[cmd.ParameterIndices[j]];
            str += "\")";
        }
        // find a location. Should be a sorted array. 
        UPInt i,n;
        for (i = 0, n = cmdSortedIdx.size(); i < n; ++i)
        {
            if (str.CompareNoCase(cmdList[cmdSortedIdx[i]]) <= 0)
                break;
        }
        if (i < n)
        {
            if (str.CompareNoCase(cmdList[cmdSortedIdx[i]]) != 0)
            {
                cmdList.push_back(str);
                cmdSortedIdx.insert(i, (int)cmdList.size()-1);
            }
        }
        else
        {
            cmdList.push_back(str);
            cmdSortedIdx.push_back((int)cmdList.size()-1);
        }
    }

}

void GFxDataExporter::MakeFsCommandsAsList(GTL::garray<GFxFsCommandOrigin>& fscommands, 
                                           GTL::garray<GFxString>& stringHolderArray, GTL::garray<GFxString>& cmdList, 
                                           GTL::garray<int>& cmdSortedIdx)
{
    for (UPInt i = 0, n = fscommands.size(); i < n; ++i)
    {
        GFxFsCommandOrigin& cmdOrig = fscommands[i];
        switch(cmdOrig.Type)
        {
        case GFxFsCommandOrigin::None:
            break;

        case GFxFsCommandOrigin::Action:
        case GFxFsCommandOrigin::InitAction:
            AddFsCommandsToList(cmdList, cmdSortedIdx, cmdOrig.Commands[0], stringHolderArray, FsCommandsParams);
            break;
        case GFxFsCommandOrigin::Button:
            {
                for(UPInt i = 0, n = cmdOrig.Commands.size(); i < n; i++)
                {
                    GFxFsCommand& cmd = cmdOrig.Commands[i];
                    AddFsCommandsToList(cmdList, cmdSortedIdx, cmd, stringHolderArray, FsCommandsParams);
                }
            }
        case GFxFsCommandOrigin::MovieClip:
            if (cmdOrig.NestedCommands.size() > 0)
                MakeFsCommandsAsList(cmdOrig.NestedCommands, stringHolderArray, cmdList, cmdSortedIdx);
            break;
        case GFxFsCommandOrigin::PlaceObject:
            for(UPInt i = 0, n = cmdOrig.Commands.size(); i < n; i++)
            {
                GFxFsCommand& cmd = cmdOrig.Commands[i];
                AddFsCommandsToList(cmdList, cmdSortedIdx, cmd, stringHolderArray, FsCommandsParams);
            }
            break;
        }
    }
}

void GFxDataExporter::WriteFsCommands(GFxMovieDef* pmovieDef, const char* swfFileName, const GFxString& path, const GFxString& name, UInt mask)
{
    if (!Quiet) printf("Looking for fscommands in '%s'", swfFileName);

    GPtr<GFile> pin = *new GSysFile(swfFileName, GFile::Open_Read);
    if (!pin || !pin->IsValid())
    {
        fprintf(stderr, "\nError: Can't open source file '%s' to read from\n", swfFileName);
        return;
    }

    // load header
    UInt32 header          = pin->ReadUInt32();
    pin->ReadUInt32(); // fileLength
    bool   compressed      = (header & 255) == 'C';
    if (compressed)
    {
        pin = *new GZLibFile(pin);
    }

    if (!pin || !pin->IsValid())
    {
        fprintf(stderr, "\nError: Can't read from source file '%s'\n", pin->GetFilePath());
        return;
    }

    GFxString names;

    GTL::garray<GFxFsCommandOrigin> fscommands;
    GTL::garray<GFxString> stringHolderArray;

    for (UPInt i = 0, n = TagsWithActions.size(); i < n; ++i)
    {
        const GFxTagInfo& tag = TagsWithActions[i];
        pin->Seek(tag.TagOffset);
        LookForFsCommandsInTags(pmovieDef, pin, tag.TagDataOffset + tag.TagLength, fscommands, stringHolderArray);
    }
    if (!Quiet) printf("\n");

    for (UInt curMask = 1; curMask <= mask; curMask <<= 1)
    {
        if (curMask & mask)
        {
            GFxString fname = path;
            fname += name;
            if (curMask == FSList)
            {
                fname += ".fsl";
                if (!Quiet) printf("Saving list of fscommands to '%s'", fname.ToCStr());
            }
            else
            {
                fname += ".fst";
                if (!Quiet) printf("Saving tree of fscommands to '%s'", fname.ToCStr());
            }

            FILE* fout;
#if defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
            fout = NULL;
            fopen_s(&fout, fname, "w");
#else
            fout = fopen(fname, "w");
#endif // defined(GFC_CC_MSVC) && (GFC_CC_MSVC >= 1400)
            if (!fout)
            {
                fprintf(stderr, "\nError: Can't open destination file '%s' to write to\n", fname.ToCStr());
                return;
            }
            
            if (curMask == FSTree)
            {
                DumpFsCommandsAsTree(fout, fscommands, stringHolderArray, 0);
            }
            if (curMask == FSList)
            {
                GTL::garray<GFxString>  cmdList;
                GTL::garray<int>        cmdSortedIdx;
                MakeFsCommandsAsList(fscommands, stringHolderArray, cmdList, cmdSortedIdx);
                for (UPInt i = 0, n = cmdSortedIdx.size(); i < n; ++i)
                {
                    fprintf(fout, "%s\n", cmdList[cmdSortedIdx[i]].ToCStr());
                }
            }

            fclose(fout);
            if (!Quiet) printf("\n");
        }
    }
}

