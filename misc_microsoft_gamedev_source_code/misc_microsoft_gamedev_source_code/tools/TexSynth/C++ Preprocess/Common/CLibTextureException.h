#ifndef H_CLibTextureException__
#define H_CLibTextureException__

class CLibTextureException
{
protected:
  char m_szMsg[512];
public:

  CLibTextureException(){m_szMsg[0]='\0';}
  CLibTextureException(char *msg,...);

  const char *getMsg() const {return (m_szMsg);}
};

#endif
