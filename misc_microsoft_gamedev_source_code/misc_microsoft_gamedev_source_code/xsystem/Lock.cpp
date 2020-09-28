
#include "xsystem.h"
#include "lock.h"

BLock::BLock()
{
   InitializeCriticalSection(&m_CriticalSection);
}

BLock::~BLock()
{
   DeleteCriticalSection(&m_CriticalSection);
}

void BLock::lock(void)
{
   EnterCriticalSection(&m_CriticalSection);
}

void BLock::unlock(void)
{
   LeaveCriticalSection(&m_CriticalSection);
}

