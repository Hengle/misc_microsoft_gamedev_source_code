//============================================================================
//  Exception.h
//
//  Copyright (c) 2001-2002 Ensemble Studios
//
//============================================================================

#pragma once

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__


//============================================================================
//  Callback ProtoTypes
//============================================================================
typedef void        (CALLBACK TRACE_FUNC              )(const char* pFormat, ...);
typedef void        (CALLBACK MESSAGE_FUNC            )(const WCHAR* pFormat, ...);
typedef const WCHAR*(CALLBACK GET_STRING_FUNC         )();
typedef void        (CALLBACK SHUTDOWN_FULLSCREEN_FUNC)(bool bEmergency);
typedef void        (CALLBACK DISPLAY_WAIT_DLG_FUNC   )(bool bDisplay, const WCHAR* pCaption, const WCHAR* pText);
typedef const WCHAR*(CALLBACK PROMPT_USER_INFO_FUNC   )();
typedef bool        (CALLBACK ENTER_WINDOWED_MODE_FUNC)(bool bWindow);
typedef void        (CALLBACK PRE_POST_FUNC)(void);

//============================================================================
//  Public Interface
//============================================================================
void initExceptionHandler    ();
void shutdownExceptionHandler();
void setExceptionDirectories(const WCHAR *logDir, const WCHAR* exeDir);
void uploadWatsonData(const WCHAR* pFileList, const WCHAR* wzHeader, const WCHAR* wzItsOkay, const CONTEXT *optionalContext);


//============================================================================
//  Callback Registration
//============================================================================
void registerTraceFunc              (TRACE_FUNC *pFunc);
void registerMessageFunc            (MESSAGE_FUNC *pFunc);
void registerAppNameFunc            (GET_STRING_FUNC *pFunc);
void registerFullscreenShutdown     (SHUTDOWN_FULLSCREEN_FUNC *pFunc, HWND hWnd);
void registerExceptionWaitDlg       (DISPLAY_WAIT_DLG_FUNC *pFunc);
void registerExceptionDescriptionDlg(PROMPT_USER_INFO_FUNC *pFunc);
void registerEnterWindowedModeFunc  (ENTER_WINDOWED_MODE_FUNC *pFunc);
void registerPreFunc                (PRE_POST_FUNC *pFunc);
void registerPostFunc               (PRE_POST_FUNC *pFunc);

//============================================================================
// 
//============================================================================
LPCTSTR __stdcall GetFirstStackTraceString(DWORD dwOpts, EXCEPTION_POINTERS *pExPtrs);
LPCTSTR __stdcall GetNextStackTraceString(DWORD dwOpts, EXCEPTION_POINTERS *pExPtrs);

//============================================================================
// 
//============================================================================

#endif


