
#pragma once

class BSemaphore
{
public:
   BSemaphore();
   ~BSemaphore();

   HRESULT initialize(long lInitialCount, long lMaxCount);
   HRESULT wait(DWORD dwTimeOut);
   HRESULT release(void);

private:

   HANDLE m_hSemaphore;
};
