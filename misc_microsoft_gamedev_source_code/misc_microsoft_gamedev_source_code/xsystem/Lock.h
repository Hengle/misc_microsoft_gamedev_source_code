#ifndef _LOCK_H_
#define _LOCK_H_

class BLock
{
public:
   BLock();
   ~BLock();

   void lock(void);
   void unlock(void);

private:

   CRITICAL_SECTION m_CriticalSection;

};


#endif
