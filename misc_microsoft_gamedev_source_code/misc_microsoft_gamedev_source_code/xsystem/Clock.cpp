
#include "xsystem.h"
#include "Clock.h"

BClock::BClock() : 
   m_blIsRunning(false)
{
   memset(&m_liFreq, 0, sizeof(m_liFreq));
   memset(&m_liStartTime, 0, sizeof(m_liStartTime));
   memset(&m_liStopTime, 0, sizeof(m_liStopTime));
}

HRESULT BClock::initialize(void)
{
   memset(&m_liFreq, 0, sizeof(m_liFreq));
   memset(&m_liStartTime, 0, sizeof(m_liStartTime));
   memset(&m_liStopTime, 0, sizeof(m_liStopTime));

   if (!::QueryPerformanceFrequency(&m_liFreq))
      return S_FALSE;

   return S_OK;
}


HRESULT BClock::start(void)
{
   if (!QueryPerformanceCounter(&m_liStartTime))
      return S_FALSE;

   m_blIsRunning = true;
   return S_OK;
}

HRESULT BClock::stop(void)
{
   if (!QueryPerformanceCounter(&m_liStopTime))
      return S_FALSE;

   m_blIsRunning = false;
   return S_OK;
}

HRESULT BClock::restart(void)
{
   HRESULT hr = S_OK;

   if (FAILED(hr = stop()))
      return hr;

   if (FAILED(hr = start()))
      return hr;

   return hr;
}

_int64 BClock::getStartTimeMilliseconds(void) const
{
   return (m_liFreq.QuadPart == 0) ? 0 : m_liStartTime.QuadPart / m_liFreq.QuadPart * 1000;
}

_int64 BClock::getStopTimeMilliseconds(void) const
{
   return m_liStopTime.QuadPart / m_liFreq.QuadPart * 1000;
}

_int64 BClock::getElapsedTimeMilliseconds(void) const
{
   return (m_liFreq.QuadPart == 0) ? 0 : (m_liStopTime.QuadPart - m_liStartTime.QuadPart) /  m_liFreq.QuadPart * 1000;
}

_int64 BClock::getTimeMilliseconds(void) const
{
   LARGE_INTEGER li;
   BOOL ok = QueryPerformanceCounter(&li);
   ok;
   BASSERT(ok);
   return (li.QuadPart - m_liStartTime.QuadPart) /  m_liFreq.QuadPart * 1000;
}
