
#include "xsystem.h"
#include "Event.h"

BCoreEvent::BCoreEvent()
{
   m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

BCoreEvent::~BCoreEvent()
{
   if (m_hEvent)
      CloseHandle(m_hEvent);
}

HRESULT BCoreEvent::set(void)
{
   if (SetEvent(m_hEvent))
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }
   return S_OK;
}

HRESULT BCoreEvent::reset(void)
{
   if (ResetEvent(m_hEvent))
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }
   return S_OK;
}

HANDLE BCoreEvent::getEventHandle(void) const
{
   return m_hEvent;
}

HRESULT BCoreEvent::wait(DWORD dwTimeOut /* = INFINITE */)
{
   DWORD dwResult = WaitForSingleObject(m_hEvent, dwTimeOut);

   if (dwResult == WAIT_OBJECT_0)
      return S_OK;
   else if (dwResult == WAIT_TIMEOUT)
      return S_FALSE;

   return HRESULT_FROM_WIN32(GetLastError());
}
