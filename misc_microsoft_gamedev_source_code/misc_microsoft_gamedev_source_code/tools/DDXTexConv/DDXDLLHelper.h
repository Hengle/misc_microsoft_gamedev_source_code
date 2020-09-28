// File: DDXDLLHelper.h
#pragma once

class BDDXDLLHelper
{
public:
   BDDXDLLHelper();
   ~BDDXDLLHelper();

   bool init(void);
   bool deinit(void);

   IDDXDLL7* getInterface(void) const { return mpIDDXDLL; }

private:
   HMODULE mDLLHandle;
   IDDXDLL7* mpIDDXDLL;
};
