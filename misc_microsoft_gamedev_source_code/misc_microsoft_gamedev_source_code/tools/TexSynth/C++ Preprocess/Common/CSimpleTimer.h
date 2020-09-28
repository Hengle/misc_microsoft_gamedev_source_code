#ifndef __SIMPLE_TIMER__
#define __SIMPLE_TIMER__

#include <stdio.h>
#include <windows.h>

class SimpleTimer
{
private:
  int         m_iTmStart;
  const char *m_szMsg;
  bool        m_bMsgBox;
public:
  SimpleTimer(const char *msg,bool msgbox=false)
  {
    m_iTmStart = timeGetTime();
    m_szMsg    = msg;
    m_bMsgBox  = msgbox;
  }
  
  ~SimpleTimer()
  {
    int elapsed=timeGetTime() - m_iTmStart;
    printf(m_szMsg,elapsed/60000,(elapsed/1000)%60,elapsed%1000);

    static char msg[128];
    StringCchPrintfA(msg,128,m_szMsg,elapsed/60000,(elapsed/1000)%60,elapsed%1000);
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
    if (m_bMsgBox)
      MessageBoxA(NULL,msg,"Timer",MB_OK | MB_ICONINFORMATION);
  }
};

#endif
