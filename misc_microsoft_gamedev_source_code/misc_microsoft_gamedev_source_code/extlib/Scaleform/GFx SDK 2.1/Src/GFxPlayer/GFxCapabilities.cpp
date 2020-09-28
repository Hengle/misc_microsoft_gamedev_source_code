/**********************************************************************

Filename    :   GFxCapabilities.cpp
Content     :   System.capabilities reference class for ActionScript 2.0
Created     :   3/27/2007
Authors     :   Prasad Silva
Copyright   :   (c) 2005-2007 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GRefCount.h"
#include "GFxLog.h"
#include "GFxAction.h"
#include "GFxNumber.h"
#include "GUTF8Util.h"
#include "GFxCapabilities.h"
#include "GFxPlayerImpl.h"

// ****************************************************************************
// Helper function to retrieve the manufacturer string
//
inline GASString GFxCapabilities_Manufacturer(GASStringContext* psc)
{
#if defined(GFC_OS_MAC)
    return GASString(psc->CreateConstString("Scaleform Macintosh"));
#elif defined(GFC_OS_WIN32)
    return GASString(psc->CreateConstString("Scaleform Windows"));
#elif defined(GFC_OS_LINUX)
    return GASString(psc->CreateConstString("Scaleform Linux"));
#else
    return GASString(psc->CreateConstString("Scaleform Other OS Name"));
#endif
}

// ****************************************************************************
// Helper function to retrieve the OS string
//
inline GASString GFxCapabilities_OS(GASStringContext* psc)
{
#if defined(GFC_OS_MAC)
    return GASString(psc->CreateConstString("Mac OS X"));
#elif defined(GFC_OS_XBOX360)
    return GASString(psc->CreateConstString("XBox 360"));
#elif defined(GFC_OS_XBOX)
    return GASString(psc->CreateConstString("XBox"));
#elif defined(GFC_OS_WINCE)
    return GASString(psc->CreateConstString("Windows CE"));
#elif defined(GFC_OS_WIN32)
    return GASString(psc->CreateConstString("Windows"));
#elif defined(GFC_OS_LINUX)
    return GASString(psc->CreateConstString("Linux"));
#elif defined(GFC_OS_PSP)
    return GASString(psc->CreateConstString("PSP"));
#elif defined(GFC_OS_PS2)
    return GASString(psc->CreateConstString("PS2"));
#elif defined(GFC_OS_PS3)
    return GASString(psc->CreateConstString("PS3"));
#elif defined(GFC_OS_WII)
    return GASString(psc->CreateConstString("WII"));
#elif defined(GFC_OS_GAMECUBE)
    return GASString(psc->CreateConstString("GameCube"));
#elif defined (GFC_OS_QNX)
    return GASString(psc->CreateConstString("QNX"));
#elif defined (GFC_OS_SYMBIAN)
    return GASString(psc->CreateConstString("Symbian"));
#else
    return GASString(psc->CreateConstString("Other"));
#endif
}

// ****************************************************************************
// Helper function to retrieve the version string
//
inline GASString GFxCapabilities_Version(GASStringContext* psc)
{
#if defined(GFC_OS_WIN32)
    return GASString(psc->CreateConstString("WIN 8,0,0,0"));
#elif defined(GFC_OS_MAC)
    return GASString(psc->CreateConstString("MAC 8,0,0,0"));
#elif defined(GFC_OS_LINUX)
    return GASString(psc->CreateConstString("LINUX 8,0,0,0"));
#elif defined(GFC_OS_XBOX360)
    return GASString(psc->CreateConstString("XBOX360 8,0,0,0"));
#elif defined(GFC_OS_XBOX)
    return GASString(psc->CreateConstString("XBOX 8,0,0,0"));
#elif defined(GFC_OS_PSP)
    return GASString(psc->CreateConstString("PSP 8,0,0,0"));
#elif defined(GFC_OS_PS2)
    return GASString(psc->CreateConstString("PS2 8,0,0,0"));
#elif defined(GFC_OS_PS3)
    return GASString(psc->CreateConstString("PS3 8,0,0,0"));
#else
    return GASString(psc->CreateConstString("GFX 8,0,0,0"));
#endif
}

// ****************************************************************************
// Helper function to retrieve the server string
//
GASString GFxCapabilities_ServerString(GASEnvironment *penv)
{
    GASStringContext* psc = penv->GetSC();
    GFxString temp;
    // Legend:

    // A=t                      hasAudio
    temp += "A=f";   

    // SA=t                     hasStreamingAudio
    temp += "&SA=f";

    // SV=t                     hasStreamingVideo
    temp += "&SV=f";

    // EV=t                     hasEmbeddedVideo
    temp += "&EV=f";

    // MP3=t                    hasMP3  
    temp += "&MP3=f";

    // AE=t                     hasAudioEncoder
    temp += "&AE=f";

    // VE=t                     hasVideoEncoder
    temp += "&VE=f";

    // ACC=f                    hasAccessibility 
    temp += "&ACC=f";

    // PR=t                     hasPrinting
    temp += "&PR=f";

    // SP=t                     hasScreenPlayback
    temp += "&SP=f";

    // SB=f                     hasScreenBroadcast
    temp += "&SB=f";

    // DEB=t                    isDebugger
    temp += "&DEB=f";

    // V=WIN%208%2C0%2C0%2C0    version
    temp += "&V=";
    GFxString etemp;
    GFxString vtemp(GFxCapabilities_Version(psc).ToCStr());
    GASGlobalContext::Escape(vtemp.ToCStr(), vtemp.GetSize(), &etemp);
    temp += etemp.ToCStr();

    // M=Macromedia%20Windows   manufacturer
    temp += "&M=";
    etemp.Clear();
    GFxString mtemp(GFxCapabilities_Manufacturer(psc).ToCStr());
    GASGlobalContext::Escape(mtemp.ToCStr(), mtemp.GetSize(), &etemp);
    temp += etemp.ToCStr();

    // R=1600x1200              screenResolutionX x screenResolutionY
    GViewport vp;
    penv->GetMovieRoot()->GetViewport(&vp);
    temp += "&R=";
    temp += GASValue(vp.BufferWidth).ToString(penv).ToCStr();
    temp += "x";
    temp += GASValue(vp.BufferHeight).ToString(penv).ToCStr();

    // DP=72                    screenDPI
    temp += "&DP=72";

    // COL=color                screenColor
    temp += "&COL=color";

    // AR=1.0                   pixelAspectRatio
    temp += "&AR=1.0";

    // OS=Windows%20XP          os
    temp += "&OS=";
    etemp.Clear();
    GFxString ostemp(GFxCapabilities_OS(psc).ToCStr());
    GASGlobalContext::Escape(ostemp.ToCStr(), ostemp.GetSize(), &etemp);
    temp += etemp.ToCStr();

    // L=en                     language
    temp += "&L=en";

    // PT=External              playerType
    temp += "&PT=External";

    // AVD=f                    avHardwareDisable
    temp += "&AVD=f";

    // LFD=f                    localFileReadDisable
    temp += "&LFD=f";

    // WD=f                     windowlessDisable
    temp += "&WD=f";

    return penv->CreateString(temp);
}

// ****************************************************************************
// GASCapabilities ctor function constructor
//
GASCapabilitiesCtorFunction::GASCapabilitiesCtorFunction(GASStringContext *psc)
{
    GASPropFlags flags = GASPropFlags::PropFlag_ReadOnly | GASPropFlags::PropFlag_DontDelete;
    
    // not supported; setting to false
    GASValue falseVal(false);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("avHardwareDisable")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasAccessibility")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasAudio")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasAudioEncoder")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasEmbeddedVideo")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasIME")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasMP3")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasPrinting")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasScreenBroadcast")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasScreenPlayback")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasStreamingAudio")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasStreamingVideo")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("hasVideoEncoder")), falseVal, flags); 
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("isDebugger")), falseVal, flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("localFileReadDisable")), falseVal, flags);

    // hard-code to 'en'. this is a lazy hack.
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("language")), GASString(psc->CreateConstString("en")), flags);

    // set to 'Scaleform OS_STRING'. this logic is correct.    
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("manufacturer")), GFxCapabilities_Manufacturer(psc), flags);
    
    // set to the OS. this logic is correct (with lower resolution. ie: not returning Windows XP; just Windows).
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("os")), GFxCapabilities_OS(psc), flags);

    // hard coded to 1. perhaps use MovieClip.GetViewport to get the correct value..?
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("pixelAspectRatio")), GASValue(1), flags);

    // hard coded to 'External'. ???
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("playerType")), GASString(psc->CreateConstString("External")), flags);
    
    // hard coded to 'color'. assumption is that the alternative is {'black and white'|'black & white'|'bw'|'b&w'}
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("screenColor")), GASString(psc->CreateConstString("color")), flags);
    
    // hard coded to '72'. ???
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("screenDPI")), GASValue(72), flags);

    // set to platform+version string (ie: 'WIN 8,0,0,0')
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("version")), GFxCapabilities_Version(psc), flags);
    
    // this is not documented in the api reference.. hard coded to 'false'
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("windowlessDisable")), falseVal, flags);

    // these are computed on the fly. intercepted in getmemberraw
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("screenResolutionX")), GASValue(GASValue::UNSET), flags);    
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("screenResolutionY")), GASValue(GASValue::UNSET), flags);
    GASFunctionObject::SetMemberRaw(psc, GASString(psc->CreateConstString("serverString")), GASValue(GASValue::UNSET), flags);    
}

// ****************************************************************************
// Handle get method for properties computed on the fly
//
bool GASCapabilitiesCtorFunction::GetMember(GASEnvironment *penv, const GASString& name, GASValue *val)
{
    if (penv->IsCaseSensitive())
    {
        if (name == "screenResolutionX")
        {
            GViewport vp;
            penv->GetMovieRoot()->GetViewport(&vp);
            *val = GASValue(vp.BufferWidth);
            return true;
        }
        else if (name == "screenResolutionY")
        {
            GViewport vp;
            penv->GetMovieRoot()->GetViewport(&vp);
            *val = GASValue(vp.BufferHeight);
            return true;
        }
        else if (name == "serverString")
        {
            *val = GASValue(GFxCapabilities_ServerString(penv));
            return true;
        }
    }
    else
    {
        GASStringContext* psc = penv->GetSC();
        if (psc->CompareConstString_CaseInsensitive(name, "screenResolutionX"))     
        { 
            GViewport vp;
            penv->GetMovieRoot()->GetViewport(&vp);
            *val = GASValue(vp.BufferWidth);
            return true;
        }
        else if (psc->CompareConstString_CaseInsensitive(name, "screenResolutionY"))
        { 
            GViewport vp;
            penv->GetMovieRoot()->GetViewport(&vp);
            *val = GASValue(vp.BufferHeight);
            return true;
        }
        else if (psc->CompareConstString_CaseInsensitive(name, "serverString")) 
        {
            *val = GASValue(GFxCapabilities_ServerString(penv));
            return true;
        }
    }
    return GASFunctionObject::GetMember(penv, name, val);
}
