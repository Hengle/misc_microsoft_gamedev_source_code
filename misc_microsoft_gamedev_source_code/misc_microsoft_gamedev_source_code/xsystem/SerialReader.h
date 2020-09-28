
#pragma once

class BSerialReader
{
public:

   virtual HRESULT readData(void* pData, long lLength) = 0;
   virtual long size(void) const = 0;

   HRESULT read(char &);
   HRESULT read(unsigned char &);
   HRESULT read(short &);
   HRESULT read(unsigned short &);
   HRESULT read(int &);
   HRESULT read(unsigned int &);
   HRESULT read(long &);
   HRESULT read(unsigned long &);
   HRESULT read(float &);
   HRESULT read(double &);

};
