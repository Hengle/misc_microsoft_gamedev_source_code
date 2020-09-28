

#ifndef COREEVENT_H
#define COREEVENT_H

class BCoreEvent
{
public:
   BCoreEvent();
   ~BCoreEvent();
   
   HRESULT set(void);
   HRESULT reset(void);
   HRESULT wait(DWORD dwTimeOut = INFINITE);

   HANDLE getEventHandle(void) const;
   
protected:

   HANDLE m_hEvent;

};

#endif
