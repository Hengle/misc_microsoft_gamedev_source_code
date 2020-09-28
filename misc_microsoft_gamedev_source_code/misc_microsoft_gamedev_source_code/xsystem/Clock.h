
#pragma once

class BClock 
{
public:
   BClock();
   HRESULT initialize(void);
   HRESULT start(void);
   HRESULT stop(void);
   HRESULT restart(void);

   _int64  getStartTimeMilliseconds(void)   const;
   _int64  getStopTimeMilliseconds(void)    const;
   _int64  getTimeMilliseconds(void)        const;
   _int64  getElapsedTimeMilliseconds(void) const;

private:

  bool  m_blIsRunning;
  LARGE_INTEGER m_liFreq;
  LARGE_INTEGER m_liStartTime;
  LARGE_INTEGER m_liStopTime;
};

