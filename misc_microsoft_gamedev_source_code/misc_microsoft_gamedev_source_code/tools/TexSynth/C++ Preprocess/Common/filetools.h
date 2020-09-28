#ifndef __FILE_TOOLS__
#define __FILE_TOOLS__

#include <fstream>

inline void write_int(ostream& o,int i)
{
  o.write((char *)&i,sizeof(int));
}

inline void write_float(ostream& o,float f)
{
  o.write((char *)&f,sizeof(float));
}

inline int read_int(istream& is)
{
  int n;
  is.read((char *)&n,sizeof(int));
  return (n);
}

inline float read_float(istream& is)
{
  float f;
  is.read((char *)&f,sizeof(float));
  return (f);
}

#endif
