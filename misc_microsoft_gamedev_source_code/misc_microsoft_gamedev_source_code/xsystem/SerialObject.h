
#pragma once

class BSerialReader;
class BSerialWriter;

class BSerialObject
{
public:
   virtual HRESULT serialize(BSerialWriter&)   = 0;
   virtual HRESULT deserialize(BSerialReader&) = 0;
   virtual long serializedSize(void)           = 0;
};


