
#pragma once

class BSerialWriter 
{
public:
   virtual HRESULT writeData(void* pData, long lLength) = 0;
   virtual long capacity(void) const                    = 0;

   HRESULT write(char);
   HRESULT write(unsigned char);
   HRESULT write(short);
   HRESULT write(unsigned short);
   HRESULT write(int);
   HRESULT write(unsigned int);
   HRESULT write(long);
   HRESULT write(unsigned long);
   HRESULT write(float);
   HRESULT write(double);
};

