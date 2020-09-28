
#include "xsystem.h"
#include "semaphore.h"

BSemaphore::BSemaphore()
{
}

HRESULT BSemaphore::initialize(long lInitialCount, long lMaxCount)
{
   m_hSemaphore = CreateSemaphore(NULL, lInitialCount, lMaxCount, NULL);
   if (m_hSemaphore == NULL)
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }
   return S_OK;
}

HRESULT BSemaphore::wait(DWORD dwTimeOut)
{
   HRESULT hr = S_OK;
   DWORD dwRetCode;

   dwRetCode = WaitForSingleObject(m_hSemaphore, dwTimeOut);
   switch (dwRetCode)
   {
   case WAIT_OBJECT_0:
      hr = S_OK;
      break;
   case WAIT_TIMEOUT:
      hr = S_FALSE;
      break;
   default:
      hr = HRESULT_FROM_WIN32(GetLastError());
      break;
   }

   return hr;
}

HRESULT BSemaphore::release(void)
{
   if (!ReleaseSemaphore(m_hSemaphore, 1, NULL))
   {
      return HRESULT_FROM_WIN32(GetLastError());
   }
   return S_OK;
}

BSemaphore::~BSemaphore()
{
   CloseHandle(m_hSemaphore);
}

