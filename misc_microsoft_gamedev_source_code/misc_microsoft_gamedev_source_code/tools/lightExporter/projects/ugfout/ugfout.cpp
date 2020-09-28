#include "ugfout.h"
using namespace gr;

LogFile gLog;

HINSTANCE hInstance;

#define LGTOUT_CLASS_ID Class_ID(0xBF9f36b2, 0x36AC41C9)

static int controlsInit = FALSE;

BOOL WINAPI DllMain(HINSTANCE hinstDLL,ULONG fdwReason,LPVOID lpvReserved)
{
   hInstance = hinstDLL;

   // Initialize the custom controls. This should be done only once.
   if (!controlsInit) 
   {
      controlsInit = TRUE;
      DisableThreadLibraryCalls(hInstance);
      InitCustomControls(hInstance);
      InitCommonControls();
   }

   return (TRUE);
}

__declspec( dllexport ) const TCHAR* LibDescription()
{
  return GetString(IDS_LIBDESCRIPTION);
}

__declspec( dllexport ) int LibNumberClasses()
{
  return 1;
}

__declspec( dllexport ) ClassDesc* LibClassDesc(int i)
{
   return GetLGTOutDesc();
}

__declspec( dllexport ) ULONG LibVersion()
{
  return VERSION_3DSMAX;
}

__declspec( dllexport ) ULONG CanAutoDefer()
{
  return 1;
}

TCHAR *GetString(int id)
{
  static TCHAR buf[256];

  if (hInstance)
    return LoadString(hInstance, id, buf, sizeof(buf)) ? buf : NULL;

  return NULL;
}

static BOOL CALLBACK AboutBoxDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg) 
   {
      case WM_INITDIALOG:
      {
         CenterWindow(hWnd, GetParent(hWnd));
         break;
      }         
      case WM_COMMAND:
      {
         switch (LOWORD(wParam)) 
         {
            case IDOK:
            {
               EndDialog(hWnd, 1);
               break;
            }
         }
         break;
      }         
      default:
      {
         return FALSE;
      }
   }
   
   return TRUE;
}

// Dummy function for progress bar
DWORD WINAPI fn(LPVOID arg)
{
  return(0);
}

//------------------------------------------------------------------------------------

class LGTOutClassDesc : public ClassDesc
{
public:
   int           IsPublic()                     { return 1; }
   void*         Create(BOOL loading = FALSE)   { return new LGTOut; }
   const TCHAR*  ClassName()                    { return _T("LGTOut"); }
   SClass_ID     SuperClassID()                 { return SCENE_EXPORT_CLASS_ID; }
   Class_ID      ClassID()                      { return LGTOUT_CLASS_ID; }
   const TCHAR*  Category()                     { return GetString(IDS_CATEGORY); }
};

static LGTOutClassDesc LGTOutDesc;

ClassDesc* GetLGTOutDesc()
{
   return &LGTOutDesc;
}

int LGTOut::ExtCount()
{
   return 2;
}

const TCHAR * LGTOut::Ext(int n)
{
   switch(n) 
   {
      case 0:
      {
         // This cause a static string buffer overwrite
         // return GetString(IDS_EXTENSION1);
         return _T("LGT");
      }
	  case 1:
	  {
		  return _T("CAM");
	  }
   }
   
   return _T("");
}

const TCHAR * LGTOut::LongDesc() { return _T("ES LGT/CAM Exporter"); }
const TCHAR * LGTOut::ShortDesc() { return _T("ES LGT/CAM Export"); }
const TCHAR * LGTOut::AuthorName() { return _T("Ensemble Studios (RG & VD)"); }
const TCHAR * LGTOut::CopyrightMessage() { return GetString(IDS_COPYRIGHT); }
const TCHAR * LGTOut::OtherMessage1() { return _T(""); }
const TCHAR * LGTOut::OtherMessage2() { return _T(""); }

unsigned int LGTOut::Version() { return 100; }

BOOL LGTOut::SupportsOptions(int ext, DWORD options)
{
   //assert(ext == 0); // We only support one extension
   return(options == SCENE_EXPORT_SELECTED) ? TRUE : FALSE;
}

void LGTOut::ResetExportOptions(void)
{
   SetStartFrame(0);
   SetEndFrame(0);
   SetSampleInterval(2);
}

LGTOut::LGTOut()
{
   canceled = false;
   ip = NULL;
   pStream = NULL;

   ResetExportOptions();

   gLog.setTraceEcho(false);
}

LGTOut::~LGTOut()
{
   if (pStream)
   {
      fclose(pStream);
      pStream = NULL;
   }

   gLog.close();
}

void LGTOut::ShowAbout(HWND hWnd)
{
   DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutBoxDlgProc, 0);
}

static BOOL CALLBACK LGTExportDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   Interval animRange;
   ISpinnerControl  *spin;

   LGTOut *exp = (LGTOut*)GetWindowLong(hWnd, GWL_USERDATA);
   
   switch (msg)
   {
      case WM_INITDIALOG:
      {
         exp = (LGTOut*)lParam;
         SetWindowLong(hWnd,GWL_USERDATA,lParam);
         CenterWindow(hWnd, GetParent(hWnd));

         // Setup the spinner control for the static frame#
         // We take the frame 0 as the default value
         animRange = exp->GetInterface()->GetAnimRange();
         spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN));
         spin->LinkToEdit(GetDlgItem(hWnd,IDC_STATIC_FRAME), EDITTYPE_INT );
         spin->SetLimits(animRange.Start() / GetTicksPerFrame(), animRange.End() / GetTicksPerFrame(), TRUE);
         spin->SetScale(1.0f);
         spin->SetValue(0, FALSE);
         ReleaseISpinner(spin);

         spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN2));
         spin->LinkToEdit(GetDlgItem(hWnd,IDC_STATIC_FRAME2), EDITTYPE_INT );
         spin->SetLimits(animRange.Start() / GetTicksPerFrame(), animRange.End() / GetTicksPerFrame(), TRUE);
         spin->SetScale(1.0f);
         spin->SetValue(animRange.End() / GetTicksPerFrame(), FALSE);
         ReleaseISpinner(spin);

         spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN3));
         spin->LinkToEdit(GetDlgItem(hWnd,IDC_STATIC_FRAME3), EDITTYPE_INT );
         spin->SetLimits(1, 30, TRUE);
         spin->SetScale(1.0f);
         spin->SetValue(exp->GetSampleInterval(), FALSE);
         ReleaseISpinner(spin);

         break;
      }         
      case CC_SPINNER_CHANGE:
      {
         spin = (ISpinnerControl*)lParam;
         break;
      }
      case WM_COMMAND:
      {
         switch (LOWORD(wParam)) 
         {
            case IDC_BUTTON1:
            {
               animRange = exp->GetInterface()->GetAnimRange();
               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN));
               spin->SetValue(0, FALSE);
               ReleaseISpinner(spin);

               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN2));
               spin->SetValue(animRange.End() / GetTicksPerFrame(), FALSE);
               ReleaseISpinner(spin);

               break;
            }
            case IDC_BUTTON2:
            {
               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN));
               spin->SetValue(0, FALSE);
               ReleaseISpinner(spin);

               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN2));
               spin->SetValue(0, FALSE);
               ReleaseISpinner(spin);

               break;
            }
            case IDOK:
            {
               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN));
               exp->SetStartFrame(spin->GetIVal());
               ReleaseISpinner(spin);

               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN2));
               exp->SetEndFrame(spin->GetIVal());
               ReleaseISpinner(spin);

               spin = GetISpinner(GetDlgItem(hWnd, IDC_STATIC_FRAME_SPIN3));
               exp->SetSampleInterval(spin->GetIVal());
               ReleaseISpinner(spin);
               
               EndDialog(hWnd, 1);
               break;
            }
            case IDCANCEL:
            {
               EndDialog(hWnd, 0);
               break;
            }
         } // switch (LOWORD(wParam)) 
         break;
      }         
      default:
      {
         return FALSE;
      }
   } // switch (msg)       
   
   return TRUE;
}

// Start the exporter!
// This is the real entrypoint to the exporter. After the user has selected
// the filename (and he's prompted for overwrite etc.) this method is called.
int LGTOut::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options)
{
#if CATCH_EXCEPTIONS
   try
#endif
   {
      // Set a global prompt display switch
      BOOL showPrompts = suppressPrompts ? FALSE : TRUE;
      onlySelected = (options & SCENE_EXPORT_SELECTED) ? TRUE : FALSE;

      // Grab the interface pointer.
      ip = i;

      ResetExportOptions();

      SetEndFrame(i->GetAnimRange().End() / GetTicksPerFrame());

      if(showPrompts) 
      {
         // Prompt the user with our dialogbox, and get all the options.
         if (!DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_LGTOUT_DLG),
            ip->GetMAXHWnd(), LGTExportDlgProc, (LPARAM)this)) 
         {
            return 1;
         }
      }

      Trace("start frame: %i\n", GetStartFrame());
      Trace("end frame: %i\n", GetEndFrame());
      Trace("sample interval: %i\n", GetSampleInterval());

      if (nStartFrame > nEndFrame)
         std::swap(nStartFrame, nEndFrame);

      Interval animRange = GetInterface()->GetAnimRange();

      // Open the stream
      pStream = _tfopen(name,_T("wb"));
      if (!pStream) 
      {
         return 0;
      }

      gLog.open(BigString(eVarArg, "%s.log", name));

      ip->ProgressStart(GetString(IDS_PROGRESS_MSG), TRUE, fn, NULL);

      mUGFExporter.exportLights(
         FILEStream(pStream, false, true), 
         ip, 
         GetStartFrame(), 
         GetEndFrame(), 
         GetSampleInterval(), 
         onlySelected);

      ip->ProgressEnd();

      // Close the stream
      fclose(pStream);
      pStream = NULL;

      gLog.close();
   }
#if CATCH_EXCEPTIONS
   catch (const char* pMsg)
   {
      MessageBox(ip ? ip->GetMAXHWnd() : NULL, pMsg, "Exception", MB_OK | MB_ICONEXCLAMATION);

      if (pStream)
      {
         fclose(pStream);
         pStream = NULL;
      }

      ip->ProgressEnd();

      return (0);
   }
   catch (...)
   {
      MessageBox(ip ? ip->GetMAXHWnd() : NULL, "The exporter crashed, sorry.", "Exception", MB_OK | MB_ICONEXCLAMATION);

      if (pStream)
      {
         fclose(pStream);
         pStream = NULL;
      }

      gLog.close();

      ip->ProgressEnd();

      return (0);
   }
#endif

   return 1;
}
