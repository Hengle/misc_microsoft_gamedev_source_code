/**********************************************************************

Filename    :   FxWin32App.cpp
Content     :   Shared base Win32 implementation for FxApp class.
Created     :   January 10, 2008
Authors     :   Michael Antonov, Maxim Didenko, Dmitry Polenur

Copyright   :   (c) 2008 Scaleform Corp. All Rights Reserved.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "FxWin32App.h"
#include "GStd.h"
#include <direct.h>
#include "FileFindWin32.h"
#define  WWND_CLASS_NAME         L"WIN32APP_Window_Class"

FxWin32App::FxWin32App() :FxApp()
{
    pFxDevice = NULL;

    hWND        = NULL;
    hInstance   = 0;
    hWNDNextViewer = NULL;
}
FxWin32App::~FxWin32App() 
{
    if (Created)
        KillWindow();
    delete pFxDevice;
}

// Global callback function to be called on window create
LRESULT CALLBACK Win32AppWindowProc(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam)
{
    FxWin32App  *papp;  

    // The first message to ever come in sets the long value to class pointer
    if (iMsg==WM_NCCREATE)
    {
        papp = (FxWin32App*) ((LPCREATESTRUCT)lParam)->lpCreateParams;

        if (!papp)
            return DefWindowProcW(hwnd,iMsg,wParam,lParam);
#ifdef GFC_64BIT_POINTERS
        SetWindowLongPtr(hwnd, 0, (LONG_PTR)papp);
#else
        SetWindowLong(hwnd, 0, (LONG)(size_t)papp);
#endif
        papp->hWND = hwnd;
    }

    // use size_t to quiet /Wp64 warning
    if ((papp=((FxWin32App*)(size_t)GetWindowLongPtr(hwnd,0)))==0)
        return DefWindowProcW(hwnd,iMsg,wParam,lParam);

    // Call member
    return papp->MemberWndProc(iMsg, wParam, lParam);
}



bool FxWin32App::SetupWindow(const char *pname, int width, int height,
                             const SetupWindowParams& extraParams)
{
    GUNUSED(extraParams);
    if (Created)
        return false;

    hInstance = GetModuleHandle(NULL);
    SetWidth(width);
    SetHeight(height);

    // Initialize the window class structure
    WNDCLASSEXW  wc;

    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc    = Win32AppWindowProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = sizeof(FxWin32App*);    // will need to store class pointer here
    wc.hInstance      = hInstance;
    wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);    // default icon
    wc.hIconSm        = LoadIcon(NULL, IDI_WINLOGO);        // windows logo small icon
    // set hCursor to NULL; otherwise cursor will be reverted back each time mouse moved.
    wc.hCursor        = NULL; //LoadCursor(NULL, IDC_ARROW);        // default arrow
    wc.hbrBackground  = NULL;     // no background needed
    wc.lpszMenuName   = NULL;     // no menu
    wc.lpszClassName  = WWND_CLASS_NAME;
    hCursor = LoadCursor(NULL, IDC_ARROW);

    // Register the windows class
    if (!RegisterClassExW(&wc))
    {
        MessageBoxA(NULL,"Unable to register the window class", "Win32 App Platform" , MB_OK | MB_ICONEXCLAMATION);  
        SetWidth(0);
        SetHeight(0);
        return 0;
    }

    DWORD dwExStyle;
    DWORD dwStyle;
    int   xpos = 0, ypos = 0;

    // Set the window style depending on FullScreen state
    if (FullScreen)
    {
        dwExStyle   = WS_EX_APPWINDOW;
        dwStyle     = WS_POPUP;   // FullScreen gets no borders or title bar
    }
    else
    {
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

        if (SizableWindow)
            dwStyle = WS_OVERLAPPEDWINDOW;
        else
        {
            // need WS_CAPTION for GL offsetting to work correctly.
            dwStyle = WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU;
        }

        xpos = ypos = CW_USEDEFAULT;
    }

    if (CursorDisabled)
        ShowCursorInstantly(false);

    // Initalize the rendering window to have width & height match the client area
    RECT  windowRect;
    windowRect.left     = 0;
    windowRect.right    = (LONG) GetWidth();
    windowRect.top      = 0;
    windowRect.bottom   = (LONG) GetHeight();
    // Account for borders and other style options
    AdjustWindowRectEx(&windowRect, dwStyle, 0, dwExStyle);

    wchar_t wpname[256];
    if (::MultiByteToWideChar(CP_ACP, 0, pname, (int)gfc_strlen(pname) + 1, wpname, sizeof(wpname)) == 0)
        wpname[0] = 0;

    // Create our window
    hWND = CreateWindowExW(
        dwExStyle,
        WWND_CLASS_NAME,
        wpname,              // Window name
        dwStyle |           // Window style required for OpenGL
        WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS,
        xpos, ypos,
        windowRect.right - windowRect.left, // width
        windowRect.bottom - windowRect.top, // height
        NULL,               // hParent
        NULL,               // hMenu
        hInstance,          // Application instance
        (LPVOID) this );    // Our pointer as an extra parameter

    if (!hWND)
    {
        MessageBoxA(NULL, "Unable to create window", "Win32 App Platform", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    // Check the client rect was changed by Windows (if, for example, too big window
    // was created).
    RECT clientRect;
    ::GetClientRect(hWND, &clientRect);
    if (clientRect.right - clientRect.left != width ||
        clientRect.bottom - clientRect.top != height)
    {
        // Calc new Width & Height
        SetWidth(clientRect.right - clientRect.left);
        SetHeight(clientRect.bottom - clientRect.top);
    }

    if (SupportDropFiles)
        ::DragAcceptFiles(hWND, 1);

    if (!SetupWindowDevice())
    {
        KillWindow();
        return false;
    }
    Created = true;

    // show the window in the foreground, and set the keyboard focus to it
    ShowWindow(hWND, SW_SHOW);
    SetForegroundWindow(hWND);
    SetFocus(hWND);

    if (!InitRenderer())
    {       
        KillWindow();
        return 0;
    }       

    return true;
}

void FxWin32App::KillWindow()
{
    KillWindowDevice();
    // Destroy the window
    if (hWND)       
    {
        DestroyWindow(hWND);
        hWND = NULL;
    }

    // Unregister our class to make name reusable
    UnregisterClassW(WWND_CLASS_NAME, hInstance);
    hInstance = NULL;

    Created = false;
}

bool FxWin32App::ProcessMessages()
{
    MSG msg;
    HWND hWndIME = 0;
    
     if(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))        
    {
        // This is necessary to hide the composition string pop up window on windows Vista. 
        hWndIME = ImmGetDefaultIMEWnd(msg.hwnd);
        ShowOwnedPopups(hWndIME, false);
		if((msg.message == WM_KEYDOWN) || (msg.message == WM_KEYUP) || ImmIsUIMessage(NULL, msg.message, msg.wParam, msg.lParam))
		{
           
			FxWin32IMEEvent event(true);
			event.message = msg.message;
			event.wParam = msg.wParam;
			event.lParam = msg.lParam;
            event.hWND   = hWND;
			OnIMEEvent(event);
		}
        if (msg.message == WM_QUIT)
        {
            // On WM_QUIT message, quit the application by setting Quit flag
            ExitCode = (SInt)msg.wParam;
            QuitFlag = 1;           
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (::IsIconic(hWND))
    	::Sleep(10);

    return !QuitFlag;       
}
// Sleeps for the specified number of milliseconds or till message.
void FxWin32App::SleepTillMessage(unsigned int ms)
{
    ::MsgWaitForMultipleObjects(0,0,0, ms, QS_ALLEVENTS);
}
void FxWin32App::SleepMilliSecs(unsigned int ms)
{
    ::Sleep(ms);
}
void FxWin32App::BringMainWindowOnTop()
{
	::BringWindowToTop(hWND);
	::SetForegroundWindow(hWND);
}

// Changes/sets window title
void FxWin32App::SetWindowTitle(const char *ptitle)
{
    wchar_t wptitle[256];
    if (::MultiByteToWideChar(CP_ACP, 0, ptitle, (int)gfc_strlen(ptitle) + 1, wptitle, sizeof(wptitle)) == 0)
        wptitle[0] = 0;
    ::SetWindowTextW(hWND, wptitle);
}

// *** Overrides

// This override is called to initialize OpenGL from setup window
bool    FxWin32App::InitRenderer()
{
    return pFxDevice->InitRenderer();
}
// Should/can be called every frame to prepare the render, user function
void    FxWin32App::PrepareRendererForFrame()
{   
    pFxDevice->PrepareRendererForFrame();
}

/*
// Message processing overrides
void    FxWin32App::OnChar(UInt32 wcharCode, UInt info)
{
    GUNUSED2(wcharCode,info);
}
void    FxWin32App::OnMouseButton(UInt button, bool downFlag, SInt x, SInt y, UInt mods)
{
    GUNUSED2(button,downFlag);
    GUNUSED3(x,y,mods);
}
void    FxWin32App::OnMouseWheel(SInt zdelta, SInt x, SInt y, UInt mods)
{
    GUNUSED4(zdelta,x,y,mods);
}
void    FxWin32App::OnMouseMove(SInt x, SInt y, UInt mods)
{
    GUNUSED3(x,y,mods);
}


// Called when sizing begins and ends.
void    FxWin32App::OnSizeEnter(bool enterSize)
{
    GUNUSED(enterSize);
}

// Handle dropped files
void    FxWin32App::OnDropFiles(char *path)
{   
    GUNUSED(path);
}
*/

void FxWin32App::StartMouseCapture()
{
    ::SetCapture(hWND);
}
void FxWin32App::EndMouseCapture()
{
    ::ReleaseCapture();
}

void FxWin32App::SetCursor(HCURSOR cursor)
{
    hCursor = cursor;
    ::SetCursor(hCursor);
}

void FxWin32App::ShowCursorInstantly(bool show)
{
    if (show)
    {
        while(::ShowCursor(TRUE) < 0)
            ;
    }
    else
    {
        while(::ShowCursor(FALSE) >= 0)
            ;
    }
}


// *** Window procedure

// Mousewheel support
#include    <winuser.h>
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL                   0x020A
#define WHEEL_DELTA                     120
#endif

// MouseLeave support
#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE                   0x02A3
#endif

// XButton WM_ message support
#ifndef WM_XBUTTONDOWN
#define WM_XBUTTONDOWN                  0x020B
#define WM_XBUTTONUP                    0x020C
#define WM_XBUTTONDBLCLK                0x020D
#define WM_NCXBUTTONDOWN                0x00AB
#define WM_NCXBUTTONUP                  0x00AC
#define WM_NCXBUTTONDBLCLK              0x00AD
// XButton values are WORD flags
#define GET_XBUTTON_WPARAM(wp)          (HIWORD(wp))
#define XBUTTON1                        0x0001
#define XBUTTON2                        0x0002
#endif
// XButton VK_ codes
#ifndef VK_XBUTTON1
#define VK_XBUTTON1                     0x05
#define VK_XBUTTON2                     0x06
#endif

#include <stdio.h>

static int GetModifiers()
{
    int new_mods = 0;
    if (::GetKeyState(VK_SHIFT) & 0x8000)
        new_mods |= FxApp::KM_Shift;
    if (::GetKeyState(VK_CONTROL) & 0x8000)
        new_mods |= FxApp::KM_Control;
    if (::GetKeyState(VK_MENU) & 0x8000)
        new_mods |= FxApp::KM_Alt;
    if (::GetKeyState(VK_NUMLOCK) & 0x1)
        new_mods |= FxApp::KM_Num;
    if (::GetKeyState(VK_CAPITAL) & 0x1)
        new_mods |= FxApp::KM_Caps;
    if (::GetKeyState(VK_SCROLL) & 0x1)
        new_mods |= FxApp::KM_Scroll;
    return new_mods;
}
static struct 
{
    WPARAM winKey;
    FxApp::KeyCode appKey;
} KeyCodeMap[] = 
{
    {VK_BACK,    FxApp::Backspace},
    {VK_TAB,     FxApp::Tab},
    {VK_CLEAR,   FxApp::ClearKey},
    {VK_RETURN,  FxApp::Return},
    {VK_SHIFT,   FxApp::Shift},
    {VK_CONTROL, FxApp::Control},
    {VK_MENU,    FxApp::Alt},
    {VK_PAUSE,   FxApp::Pause},
    {VK_CAPITAL, FxApp::CapsLock},
    {VK_ESCAPE,  FxApp::Escape},
    {VK_SPACE,   FxApp::Space},
    {VK_PRIOR,   FxApp::PageUp},
    {VK_NEXT,    FxApp::PageDown},
    {VK_END,     FxApp::End},
    {VK_HOME,    FxApp::Home},
    {VK_LEFT,    FxApp::Left},
    {VK_UP,      FxApp::Up},
    {VK_RIGHT,   FxApp::Right},
    {VK_DOWN,    FxApp::Down},
    {VK_INSERT,  FxApp::Insert},
    {VK_DELETE,  FxApp::Delete},
    {VK_HELP,    FxApp::Help},
    {VK_NUMLOCK, FxApp::NumLock},
    {VK_SCROLL,  FxApp::ScrollLock},

    {VK_OEM_1,     FxApp::Semicolon},
    {VK_OEM_PLUS,  FxApp::Equal},
    {VK_OEM_COMMA, FxApp::Comma},
    {VK_OEM_MINUS, FxApp::Minus},
    {VK_OEM_PERIOD,FxApp::Period},
    {VK_OEM_2,     FxApp::Slash},
    {VK_OEM_3,     FxApp::Bar},
    {VK_OEM_4,     FxApp::BracketLeft},
    {VK_OEM_5,     FxApp::Backslash},
    {VK_OEM_6,     FxApp::BracketRight},
    {VK_OEM_7,     FxApp::Quote}
};

LRESULT FxWin32App::MemberWndProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
        OnCreate();
        break;
    case WM_DESTROY:
        OnDestroy();
        break;

    case WM_ACTIVATE:
        if (!HIWORD(wParam))
            Active = 1; // Window was restored or maximized
        else
            Active = 0; // Window was minimized
        return 0;

    case WM_SYSCOMMAND:
        // Look for screen-saver and power-save modes
        switch (wParam)
        {
        case SC_SCREENSAVE:     // Screen-saver is starting
        case SC_MONITORPOWER:   // Monitor is going to power-save mode
            // Prevent either from happening by returning 0
            return 0;
        default:
            break;
        }
        break;

    case WM_DROPFILES:
        {
            UInt itemCount = ::DragQueryFile((HDROP)wParam, 0xFFFFFFFF,0,0);
            if (itemCount)
            {
                // Get name
                char    buffer[512];
                buffer[0] = 0;
                ::DragQueryFileA((HDROP)wParam, 0, buffer, 512);
                ::DragFinish((HDROP)wParam);

                // Inform user about the drop
                OnDropFiles(buffer);
            }
        }
        return 0;

    case WM_CLOSE:
        // Window is being closed. // Send WM_QUIT to a message queue.
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        if (!LockOnSize)
            OnSize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_ENTERSIZEMOVE:
        OnSizeEnter(1);
        return 0;
    case WM_EXITSIZEMOVE:
        OnSizeEnter(0);
        return 0;

    case WM_KEYDOWN:
    case WM_KEYUP:
        {
        KeyCode kc = VoidSymbol;
        if (wParam >= 'A' && wParam <= 'Z')
        {
            kc = (KeyCode) ((wParam - 'A') + A);
        }
        else if (wParam >= VK_F1 && wParam <= VK_F15)
        {
            kc = (KeyCode) ((wParam - VK_F1) + F1);
        }
        else if (wParam >= '0' && wParam <= '9')
        {
            kc = (KeyCode) ((wParam - '0') + Num0);
        }
        else if (wParam >= VK_NUMPAD0 && wParam <= VK_DIVIDE)
        {
            kc = (KeyCode) ((wParam - VK_NUMPAD0) + KP_0);
        }
        else 
        {
            for (int i = 0; KeyCodeMap[i].winKey != 0; i++)
            {
                if (wParam == (UInt)KeyCodeMap[i].winKey)
                {
                    kc = KeyCodeMap[i].appKey;
                    break;
                }
            }
        }
        unsigned char asciiCode = 0;
        if (kc != VoidSymbol) 
        {
            // get the ASCII code, if possible.
            UINT uScanCode = ((UINT)lParam >> 16) & 0xFF; // fetch the scancode
            BYTE ks[256];
            WORD charCode;

            // Get the current keyboard state
            ::GetKeyboardState(ks);

            if (::ToAscii((UINT)wParam, uScanCode, ks, &charCode, 0) > 0)
            {
                //!AB, what to do if ToAscii returns > 1?
                asciiCode = LOBYTE (charCode);
            }
        }
        OnKey(kc, asciiCode, 0, GetModifiers(), message == WM_KEYDOWN ? 1 : 0);   
        return 0;
        }
    case WM_CHAR:
        {
            UInt32 wcharCode = (UInt32)wParam;
            OnChar(wcharCode, (UInt)lParam);
        }
        break;

    case WM_IME_SETCONTEXT:
        // This is an attempt to hide Windows IME UI Windows.
        lParam = 0;
        break;

    case WM_IME_STARTCOMPOSITION:
    case WM_IME_KEYDOWN:
    case WM_IME_COMPOSITION:        
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_NOTIFY:
    case WM_IME_CHAR:        // This message should not be passed down to DefWinProc otherwise it will
                             // generate WM_CHAR messages which will cause text duplication.
        {
            FxWin32IMEEvent event;
            event.message = message;
            event.wParam = wParam;
            event.lParam = lParam;
            event.hWND   = hWND;
            if (OnIMEEvent(event))
                return 0;
        }
	
        break;

    case WM_MOUSEMOVE:      
        OnMouseMove((SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers()); 
        return 0;
        // Mouse wheel support
    case WM_MOUSEWHEEL:
        {
            // Nonclient position must be adjusted to be inside the window
            POINT   wcl = {0,0};
            ::ClientToScreen(hWND, &wcl);
            SInt x = SInt(SInt16(LOWORD(lParam))) - wcl.x;
            SInt y = SInt(SInt16(HIWORD(lParam))) - wcl.y;
            OnMouseWheel((SInt(SInt16(HIWORD(wParam)))*128)/WHEEL_DELTA, x, y, GetModifiers());
            return 0;
        }
    case WM_SETCURSOR:
        if ((HWND)wParam == hWND)
        {
            if (CursorDisabled)
                break;

            if(LOWORD(lParam) == HTCLIENT)
            {
                if (CursorIsOutOfClientArea)
                {
                    bool cursorWasHidden = CursorHiddenSaved;
                    CursorIsOutOfClientArea = false;
                    CursorHiddenSaved = false;
                    if (cursorWasHidden && CursorHidden)
                        ShowCursorInstantly(false);
                }
                ::SetCursor(hCursor);
                return 1;
            }
            else if (!CursorIsOutOfClientArea)
            {
                CursorIsOutOfClientArea = true;
                CursorHiddenSaved = CursorHidden;
                if (CursorHidden)
                    ShowCursorInstantly(true);
            }
        }
        break;
        // Mouse button support
    case WM_LBUTTONDOWN:    
        OnMouseButton(0, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers());  
        return 0;
    case WM_LBUTTONUP:      
        OnMouseButton(0, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers()); 
        return 0;
    case WM_RBUTTONDOWN:    
        OnMouseButton(1, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers());  
        return 0;
    case WM_RBUTTONUP:      
        OnMouseButton(1, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers()); 
        return 0;
    case WM_MBUTTONDOWN:   
        OnMouseButton(2, 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers());  
        return 0;
    case WM_MBUTTONUP:   
        OnMouseButton(2, 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers());  
        return 0;
        // XButton support
    case WM_XBUTTONDOWN:    
        OnMouseButton(2+GET_XBUTTON_WPARAM(wParam), 1, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers());
        return 0;
    case WM_XBUTTONUP:
        OnMouseButton(2+GET_XBUTTON_WPARAM(wParam), 0, (SInt16) LOWORD(lParam), (SInt16) HIWORD(lParam), GetModifiers()); 
        return 0;

        /*
        case WM_CHAR:
        switch (toupper(wParam))
        {
        case VK_ESCAPE:
        {
        PostQuitMessage(0);
        return 0;
        }
        default:
        break;
        };
        break;
        */

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hWND, &ps);
            EndPaint(hWND, &ps);
        }
        break;

        // clipboard related messages
    case WM_DRAWCLIPBOARD:
        OnDrawClipboard(wParam, lParam);
        break;
    case WM_CHANGECBCHAIN:
        OnChangeCBChain(wParam, lParam);
        break;

    default:
        break;
    }

    return DefWindowProcW(hWND, message, wParam, lParam);
}


void FxWin32App::OnCreate()
{
    hWNDNextViewer = SetClipboardViewer(hWND); 
}

void FxWin32App::OnDestroy()
{
    if (!hWNDNextViewer)
        ChangeClipboardChain(hWND, hWNDNextViewer); 
}

void FxWin32App::OnDrawClipboard(WPARAM wParam, LPARAM lParam)
{
    GUNUSED2(wParam,lParam);
//    GPtr<GFxTextClipboard> pclipBoard = Loader.GetTextClipboard();
//    if (pclipBoard)
//    {
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) 
        return; 
    if (!OpenClipboard(hWND)) 
        return; 

    HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT); 
    if (hglb != NULL) 
    { 
        const wchar_t* pwstr = (const wchar_t*)GlobalLock(hglb); 
        if (pwstr != NULL) 
        {
            OnUpdateSystemCliboard(pwstr);
            GlobalUnlock(hglb); 
        } 
    } 
    CloseClipboard(); 
//    }
//    SendMessage(hWNDNextViewer, WM_DRAWCLIPBOARD, wParam, lParam); 
}

void FxWin32App::OnChangeCBChain(WPARAM wParam, LPARAM lParam)
{
    // If the next window is closing, repair the chain. 

    if ((HWND) wParam == hWNDNextViewer) 
        hWNDNextViewer = (HWND) lParam; 

    // Otherwise, pass the message to the next link. 

    else if (hWNDNextViewer != NULL) 
        SendMessage(hWNDNextViewer, WM_CHANGECBCHAIN, wParam, lParam); 
}

bool FxWin32App::UpdateFiles(char* filename, bool prev)
{
    WIN32_FIND_DATAA f;
    char    path[256];
    char*   pfilename=filename;
    SPInt len = strlen(pfilename);
	for (SPInt i=len; i>0; i--) 
    {
		if (pfilename[i]=='/' || pfilename[i]=='\\') {
			pfilename = pfilename+i+1;
				break;
			}
    }
    gfc_strcpy(path,sizeof(path),filename);
    unsigned int path_length =(unsigned int) (strlen(filename)-strlen(pfilename));
    path[path_length]=0; //current directory

    //  if( _getcwd( &path[0], 256 ))
    {
        gfc_strcat(path,256,"*.*");
        if (FindNextFileInList(&f, path, pfilename, prev))
        {
            gfc_strcpy(&filename[path_length], 256-path_length, f.cFileName);
            return true;
        }
    }
    return false;
}
