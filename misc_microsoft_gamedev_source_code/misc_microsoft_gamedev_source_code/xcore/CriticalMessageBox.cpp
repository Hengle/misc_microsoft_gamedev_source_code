//============================================================================
//
//  CriticalMessageBox.cpp
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xcore.h"
#include "CriticalMessageBox.h"

#ifdef XBOX
#include <xbdm.h>
long criticalMessageBox(const WCHAR* pMessage, const WCHAR* pTitle, HINSTANCE hInstance, HWND hParentWnd, long numButtons, const WCHAR** pButtonText)
{
   // rg [12/31/05] - FIXME, make this more complete.
   OutputDebugStringW(pMessage);

#ifndef BUILD_FINAL
#ifdef XBOX   
   if (DmIsDebuggerPresent())
#else
   if (IsDebuggerPresent())
#endif
   {   
      DebugBreak();
   }
#endif   
         
   return CRITICAL_MESSAGE_BOX_FAILED;
}

#else // XBOX

static BCriticalSection gCriticalSection;

//============================================================================
//  PRIVATE CONSTANTS
//============================================================================
static const long DIALOG_CONTROL_BASE         = 200;
static const long DIALOG_TEMPLATE_BUFFER_SIZE = 4096;
static const long BUFFER_SIZE                 = 1024;

//-- Control size and spacing
static const long CONTROL_HORIZONTAL_SPACE = 10;
static const long CONTROL_VERTICAL_SPACE   = 10;
static const long BUTTON_TOP_PAD           = 4;
static const long BUTTON_BOTTOM_PAD        = 4;
static const long BUTTON_LEFT_PAD          = 4;
static const long BUTTON_RIGHT_PAD         = 4;
static const long MIN_BUTTON_WIDTH         = 60;
static const long MIN_BUTTON_HEIGHT        = 20;
static const long MIN_DIALOG_WIDTH         = 200;
static const long MIN_DIALOG_HEIGHT        = 100;


//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
void AlignToDWORD(WORD*& pBuffer)
{
   DWORD temp = (DWORD)pBuffer;
   temp  += 3;
   temp >>= 2;
   temp <<= 2;
   pBuffer = (WORD*)temp;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AlignToWORD(WORD*& pBuffer)
{
   DWORD temp = (DWORD)pBuffer;
   temp  += 1;
   temp >>= 1;
   temp <<= 1;
   pBuffer = (WORD*)temp;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool addDialogTemplate(WORD*& pWorkSpace, long& bytesLeft, const WCHAR* pTitle, long numButtons)
{
   //-- Validate parameters.
   if (!pWorkSpace)
      return false;
   if (!pTitle)
      return false;
   if (numButtons < 0)
      return false;

   //-- Check for space.
   if (bytesLeft < (sizeof(DWORD) + sizeof(DLGTEMPLATE)))
      return false;

   //-- Align the workspace to a DWORD boundary, and assume worst case memory offset.
   AlignToDWORD(pWorkSpace);
   bytesLeft -= sizeof(DWORD);

   //-- Add the dialog template.
   DLGTEMPLATE* pDialogTemplate = (DLGTEMPLATE*)pWorkSpace;
   pDialogTemplate->style = WS_VISIBLE | WS_POPUP | WS_BORDER | DS_MODALFRAME | WS_CAPTION | DS_SETFONT;
   pDialogTemplate->cdit  = (WORD)(1 + numButtons);
   pDialogTemplate->x     = 0;
   pDialogTemplate->y     = 0;
   pDialogTemplate->cx    = 0;
   pDialogTemplate->cy    = 0;
   bytesLeft -= sizeof(DLGTEMPLATE);
   pWorkSpace = (WORD*)(pDialogTemplate + 1);

   //-- Check for space.
   if (bytesLeft < (sizeof(WORD) + sizeof(WORD) + sizeof(WORD)))
      return false;

   //-- Align the workspace to a WORD boundary, and assume worst case memory offset.
   AlignToWORD(pWorkSpace);
   bytesLeft -= sizeof(WORD);

   //-- Add the menu.  0 means no menu.
   *pWorkSpace++ = 0;
   bytesLeft -= sizeof(WORD);
   
   //-- Add the dialog class.  0 means use default dialog class.
   *pWorkSpace++ = 0;
   bytesLeft -= sizeof(WORD);
   
   //-- Add the dialog title (in unicode).
//   long numChars = MultiByteToWideChar(CP_ACP, 0, pTitle, -1, (LPWSTR)pWorkSpace, bytesLeft - 1);
   long numChars  = wcslen(pTitle);
   if (!numChars)
      return false;
   long charsLeft = (bytesLeft - 1) / sizeof(WCHAR);
   StringCchCopyNExW(reinterpret_cast<wchar_t*>(pWorkSpace), charsLeft, pTitle, charsLeft, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   pWorkSpace[numChars] = 0;
   pWorkSpace += (numChars + 1);
   bytesLeft  -= (sizeof(WORD) * (numChars + 1));
   
   //-- Add the font point size.
   *pWorkSpace++ = 8;
   bytesLeft -= sizeof(WORD);

   //-- Add the font name.
   numChars = MultiByteToWideChar(CP_ACP, 0, "Microsoft Sans Serif", -1, (LPWSTR)pWorkSpace, bytesLeft - 1);
   if (!numChars)
      return false;
   pWorkSpace += numChars;
   bytesLeft  -= (sizeof(WORD) * numChars);
   
   //-- All done.
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool addStaticTextTemplate(WORD*& pWorkSpace, long& bytesLeft, const WCHAR* pMessage)
{
   //-- Validate parameters.
   if (!pWorkSpace)
      return false;
   if (!pMessage)
      return false;

   //-- Check for space.
   if (bytesLeft < (sizeof(DWORD) + sizeof(DLGITEMTEMPLATE)))
      return false;

   //-- Align the workspace to a DWORD boundary, and assume worst case memory offset.
   AlignToDWORD(pWorkSpace);
   bytesLeft -= sizeof(DWORD);

   //-- Add the dialog item template.
   DLGITEMTEMPLATE* pItemTemplate = (DLGITEMTEMPLATE*)pWorkSpace;
   pItemTemplate->style = WS_CHILD | WS_VISIBLE | SS_LEFT;
   pItemTemplate->x     = 0;
   pItemTemplate->y     = 0;
   pItemTemplate->cx    = 0;
   pItemTemplate->cy    = 0;
   pItemTemplate->id    = DIALOG_CONTROL_BASE;
   bytesLeft -= sizeof(DLGITEMTEMPLATE);
   pWorkSpace = (WORD*)(pItemTemplate + 1);

   //-- Check for space.
   if (bytesLeft < (sizeof(WORD) + sizeof(WORD) + sizeof(WORD)))
      return false;

   //-- Align the workspace to a WORD boundary, and assume worst case memory offset.
   AlignToWORD(pWorkSpace);
   bytesLeft -= sizeof(WORD);

   //-- Specify the class off the control.  0xFFFF indicates that we are using a
   //-- predefined class.  0x0082 is the predefined code for a static text box.
   *pWorkSpace++ = 0xFFFF;
   *pWorkSpace++ = 0x0082;
   bytesLeft -= sizeof(WORD);
   bytesLeft -= sizeof(WORD);
   
   //-- Add the text.
//   long numChars = MultiByteToWideChar(CP_ACP, 0, pMessage, -1, (LPWSTR)pWorkSpace, bytesLeft - 1);
   long numChars  = wcslen(pMessage);
   long charsLeft = (bytesLeft - 1) / sizeof(WCHAR);
   StringCchCopyNExW(reinterpret_cast<wchar_t*>(pWorkSpace), charsLeft, pMessage, charsLeft, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   pWorkSpace[numChars] = 0;
   pWorkSpace += (numChars + 1);
   bytesLeft -= (sizeof(WORD) * (numChars + 1));

   //-- Check for space.
   if (bytesLeft < sizeof(WORD))
      return false;

   //-- Add the creation data.  0 means there is no creation data.
   *pWorkSpace++ = 0;
   bytesLeft -= sizeof(WORD);

   //-- All done.
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool addButtonTemplate(WORD*& pWorkSpace, long& bytesLeft, const WCHAR* pText, long ID)
{
   //-- Validate parameters.
   if (!pWorkSpace)
      return false;
   if (!pText)
      return false;

   //-- Check for space.
   if (bytesLeft < (sizeof(DWORD) + sizeof(DLGITEMTEMPLATE)))
      return false;

   //-- Align the workspace to a DWORD boundary, and assume worst case memory offset.
   AlignToDWORD(pWorkSpace);
   bytesLeft -= sizeof(DWORD);

   //-- Add the dialog item template.
   DLGITEMTEMPLATE* pItemTemplate = (DLGITEMTEMPLATE*)pWorkSpace;
   pItemTemplate->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
   pItemTemplate->x     = 0;
   pItemTemplate->y     = 0;
   pItemTemplate->cx    = 0;
   pItemTemplate->cy    = 0;
   pItemTemplate->id    = (WORD)ID;
   bytesLeft -= sizeof(DLGITEMTEMPLATE);
   pWorkSpace = (WORD*)(pItemTemplate + 1);

   //-- Make the first button a default button.
   if (ID == (DIALOG_CONTROL_BASE + 1))
      pItemTemplate->style |= BS_DEFPUSHBUTTON;
   else
      pItemTemplate->style |= BS_PUSHBUTTON;

   //-- Check for space.
   if (bytesLeft < (sizeof(WORD) + sizeof(WORD) + sizeof(WORD)))
      return false;

   //-- Align the workspace to a WORD boundary, and assume worst case memory offset.
   AlignToWORD(pWorkSpace);
   bytesLeft -= sizeof(WORD);

   //-- Specify the class off the control.  0xFFFF indicates that we are using a
   //-- predefined class.  0x0080 is the predefined code for a button.
   *pWorkSpace++ = 0xFFFF;
   *pWorkSpace++ = 0x0080;
   bytesLeft -= sizeof(WORD);
   bytesLeft -= sizeof(WORD);
   
   //-- Add the text.
//   long numChars = MultiByteToWideChar(CP_ACP, 0, pText, -1, (LPWSTR)pWorkSpace, bytesLeft - 1);
   long numChars  = wcslen(pText);
   long charsLeft = (bytesLeft - 1) / sizeof(WCHAR);
   StringCchCopyNExW(reinterpret_cast<wchar_t*>(pWorkSpace), charsLeft, pText, charsLeft, NULL, NULL, STRSAFE_FILL_BEHIND_NULL);
   pWorkSpace[numChars] = 0;
   pWorkSpace += (numChars + 1);
   bytesLeft -= (sizeof(WORD) * (numChars + 1));

   //-- Check for space.
   if (bytesLeft < sizeof(WORD))
      return false;

   //-- Add the creation data.  0 means there is no creation data.
   *pWorkSpace++ = 0;
   bytesLeft -= sizeof(WORD);

   //-- All done.
   return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void resizeDialog(HWND hWnd)
{
   HDC   hDC;
   SIZE  size;
   long  len;
   long  dialogWidth  = MIN_DIALOG_WIDTH;
   long  dialogHeight = MIN_DIALOG_HEIGHT;
   WCHAR buffer[BUFFER_SIZE];

   //-- Count the buttons that we have and total up their widths.
   HWND hItem;
   long numButtons       = 0;
   long totalbuttonWidth = 0;
   long maxButtonHeight  = 0;
   for (;;)
   {
      //-- Get the next button.
      hItem = GetDlgItem(hWnd, DIALOG_CONTROL_BASE + numButtons + 1);
      if (hItem == NULL)
         break;

      //-- Get the dimensions of the button text.
      len = GetWindowTextW(hItem, buffer, BUFFER_SIZE);
      hDC = GetDC(hItem);
      GetTextExtentPoint32W(hDC, buffer, len, &size);
      ReleaseDC(hItem, hDC);

      //-- See how wide we should make this button.
      long buttonWidth = size.cx + BUTTON_LEFT_PAD + BUTTON_RIGHT_PAD;
      if (buttonWidth < MIN_BUTTON_WIDTH)
         buttonWidth = MIN_BUTTON_WIDTH;

      //-- See how tall we should make this button.
      long buttonHeight = size.cy + BUTTON_TOP_PAD + BUTTON_BOTTOM_PAD;
      if (buttonHeight < MIN_BUTTON_HEIGHT)
         buttonHeight = MIN_BUTTON_HEIGHT;

      //-- See if we need to adjust our max button height.
      if (maxButtonHeight < buttonHeight)
         maxButtonHeight = buttonHeight;

      //-- Increment counters.
      totalbuttonWidth += buttonWidth;
      numButtons++;
   }

   //-- Adjust total button width to account for spacing.  Then see if we need
   //-- to grow the dialog width.
   totalbuttonWidth += (CONTROL_HORIZONTAL_SPACE * (numButtons + 1)); 
   if (dialogWidth < totalbuttonWidth)
      dialogWidth = totalbuttonWidth;

   //-- Get the size of the caption.
   len = GetWindowTextW(hWnd, buffer, BUFFER_SIZE);
   hDC = GetDC(hWnd);
   GetTextExtentPoint32W(hDC, buffer, len, &size);
   ReleaseDC(hWnd, hDC);

   //-- See if we need to grow the dialog to accomodate the caption.
   if (dialogWidth < size.cx)
      dialogWidth = size.cx;

   //-- Get the size of the text box.
   hItem = GetDlgItem(hWnd, DIALOG_CONTROL_BASE);
   len   = GetWindowTextW(hItem, buffer, BUFFER_SIZE);
   hDC   = GetDC(hItem);
   GetTextExtentPoint32W(hDC, buffer, len, &size);
   ReleaseDC(hItem, hDC);

   //-- Compute a good size for the text box.
   long textArea   = size.cx * size.cy;
   long textWidth  = dialogWidth - (2 * CONTROL_HORIZONTAL_SPACE);
   long textHeight = dialogHeight - maxButtonHeight - (3 * CONTROL_VERTICAL_SPACE);
   if (textArea > 0)
   {
      //-- Compute a 4:3 ratio (using cy as a pad for a little safety).
      long goodWidth  = (long)(sqrt((float)textArea) * 1.25) + size.cy;
      if (textWidth < goodWidth)
         textWidth = goodWidth;
      long goodHeight = (textArea / (goodWidth - size.cy)) + size.cy;
      long numLines = (goodHeight / size.cy) + 1;
      goodHeight += (numLines * size.cy) / 6;
      if (textHeight < goodHeight)
         textHeight = goodHeight;
   }   

   //-- See if we need to resize the dialog.
   long totalTextWidth    = textWidth + (2 * CONTROL_HORIZONTAL_SPACE);
   long totalDialogHeight = textHeight + maxButtonHeight + (CONTROL_VERTICAL_SPACE * 3);
   if (dialogWidth < totalTextWidth)
      dialogWidth = totalTextWidth;
   if (dialogHeight < totalDialogHeight)
      dialogHeight = totalDialogHeight;

   //-- Account for extra width of dialog (title bar, frame, etc).
   RECT outerRect, innerRect;
   GetWindowRect(hWnd, &outerRect);
   GetClientRect(hWnd, &innerRect);
   long extraWidth  = (outerRect.right  - outerRect.left) - innerRect.right;
   long extraHeight = (outerRect.bottom - outerRect.top ) - innerRect.bottom;
   dialogWidth  += extraWidth;
   dialogHeight += extraHeight;

   //-- Resize the dialog.
   long screenWidth  = GetSystemMetrics(SM_CXSCREEN);
   long screenHeight = GetSystemMetrics(SM_CYSCREEN);
   long screenX      = (screenWidth  - dialogWidth ) / 2;
   long screenY      = (screenHeight - dialogHeight) / 2;
   MoveWindow(hWnd, screenX, screenY, dialogWidth, dialogHeight, true);

   //-- Resize the text box.
   long titleBarHeight = GetSystemMetrics(SM_CYCAPTION);
   MoveWindow(hItem, CONTROL_HORIZONTAL_SPACE, CONTROL_VERTICAL_SPACE, textWidth, textHeight + titleBarHeight, true);

   //-- Space out the buttons. (x -= 3 is pure cheeze).
   long x      = ((dialogWidth - totalbuttonWidth) / 2) + CONTROL_HORIZONTAL_SPACE;
   long y      = textHeight + (2 * CONTROL_VERTICAL_SPACE);
   long button = 0;
   x -= 3;
   for (;;)
   {
      //-- Get the next button.
      hItem = GetDlgItem(hWnd, DIALOG_CONTROL_BASE + button + 1);
      if (hItem == NULL)
         break;

      //-- Get the dimensions of the button text.
      len = GetWindowTextW(hItem, buffer, BUFFER_SIZE);
      hDC = GetDC(hItem);
      GetTextExtentPoint32W(hDC, buffer, len, &size);
      ReleaseDC(hItem, hDC);

      //-- See how wide we should make this button.
      long buttonWidth = size.cx + BUTTON_LEFT_PAD + BUTTON_RIGHT_PAD;
      if (buttonWidth < MIN_BUTTON_WIDTH)
         buttonWidth = MIN_BUTTON_WIDTH;

      //-- Set the window width.
      MoveWindow(hItem, x, y, buttonWidth, maxButtonHeight, true);

      //-- Set position of next button.
      x += buttonWidth;
      x += CONTROL_HORIZONTAL_SPACE;

      //-- Look for next button.
      button++;
   }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   //-- We resize the dialog on the initial update.
   if (message == WM_INITDIALOG)
   {
      //-- Resize it.
      resizeDialog(hWnd);
      
      //-- Make it the topmost window.
      SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

      //-- Set the focus to the first button.
      SetFocus(GetDlgItem(hWnd, DIALOG_CONTROL_BASE + 1));

      //-- Play a sound.
      MessageBeep(MB_ICONEXCLAMATION);
      return FALSE;
   }

   //-- After that, we only care about WM_COMMAND, cuz that's how button
   //-- clicks get communicated.
   if (message != WM_COMMAND)
      return FALSE;

   //-- Make sure this command came from a control.
   if (lParam == NULL)
      return FALSE;

   //-- Get the control's ID and notification code.
   WORD wControlID  = LOWORD(wParam);
   WORD wNotifyCode = HIWORD(wParam);

   //-- We only care about button clicks.
   if (wNotifyCode != BN_CLICKED)
      return FALSE;

   //-- Close the dialog and return the control ID of the button that was
   //-- clicked.
   int returnCode = wControlID;
   EndDialog(hWnd, returnCode);

   return TRUE;
}


//============================================================================
//  PUBLIC FUNCTIONS
//============================================================================
long criticalMessageBox(const WCHAR* pMessage, const WCHAR* pTitle, HINSTANCE hInstance, HWND hParentWnd, long numButtons, const WCHAR** pButtonText)
{
   BScopedCriticalSection lock(gCriticalSection);
   
   //-- Make sure we have a message and a title.
   if (!pMessage)
      return CRITICAL_MESSAGE_BOX_FAILED;
   if (!pTitle)
      return CRITICAL_MESSAGE_BOX_FAILED;

   //-- Default the buttons if none are provided.
   const WCHAR* pDefaultButtonText[1] = { L"OK" };
   if ((numButtons == 0) || (pButtonText == NULL))
   {
      numButtons  = 1;
      pButtonText = pDefaultButtonText;
   }

   //-- Dialog templates want to be in global memory.  So first we need to
   //-- allocate a global buffer to build the template in.
   HGLOBAL hBuffer = GlobalAlloc(GMEM_ZEROINIT, DIALOG_TEMPLATE_BUFFER_SIZE);
   if (!hBuffer)
      return CRITICAL_MESSAGE_BOX_FAILED;

   //-- Lock the buffer.
   void* pBuffer = GlobalLock(hBuffer);
   if (!pBuffer)
   {
      GlobalFree(hBuffer);
      return CRITICAL_MESSAGE_BOX_FAILED;
   }

   //-- We navigate the buffer with a WORD aligned pointer, becuase thats how
   //-- DialogBoxIndirect() wants it.
   WORD* pWorkSpace = (WORD*)pBuffer;
   long  bytesLeft  = DIALOG_TEMPLATE_BUFFER_SIZE;

   //-- Create the dialog.
   if (!addDialogTemplate(pWorkSpace, bytesLeft, pTitle, numButtons))
   {
      GlobalUnlock(hBuffer);
      GlobalFree(hBuffer);
      return CRITICAL_MESSAGE_BOX_FAILED;
   }

   //-- Create the text box.
   if (!addStaticTextTemplate(pWorkSpace, bytesLeft, pMessage))
   {
      GlobalUnlock(hBuffer);
      GlobalFree(hBuffer);
      return CRITICAL_MESSAGE_BOX_FAILED;
   }

   //-- Create the buttons.
   for (long button = 0; button < numButtons; ++button)
   {
      if (!addButtonTemplate(pWorkSpace, bytesLeft, pButtonText[button], DIALOG_CONTROL_BASE + button + 1))
      {
         GlobalUnlock(hBuffer);
         GlobalFree(hBuffer);
         return CRITICAL_MESSAGE_BOX_FAILED;
      }
   }

   //-- Unlock the buffer.
   GlobalUnlock(hBuffer);

   //-- Launch the dialog.
   WORD* pTemplate = (WORD*)pBuffer;
   AlignToDWORD(pTemplate);
   int returnCode = DialogBoxIndirect(hInstance, (DLGTEMPLATE*)pTemplate, hParentWnd, (DLGPROC)DialogProc);

   //-- Release the global memory.
   GlobalFree(hBuffer);

   //-- Check fora dialog error.
   if (returnCode == -1)
   {
//      DWORD error = GetLastError();
      return CRITICAL_MESSAGE_BOX_FAILED;
   }

   //-- Make sure the return code was a button ID.
   long buttonID = returnCode - DIALOG_CONTROL_BASE - 1;
   if ((buttonID < 0) || (buttonID >= numButtons))
      return CRITICAL_MESSAGE_BOX_FAILED;

   //-- Return the button ID.
   return buttonID;
}

#endif // XBOX
