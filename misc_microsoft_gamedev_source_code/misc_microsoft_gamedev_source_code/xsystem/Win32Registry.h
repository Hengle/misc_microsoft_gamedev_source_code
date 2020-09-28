//==============================================================================
// Win32registry,h
//
// Copyright (c) 1999 Ensemble Studios
//==============================================================================

#ifndef __WIN32REGISTRYHDR__

#define __WIN32REGISTRYHDR__

//==============================================================================
class BWin32Registry;

extern BWin32Registry registry;

//==============================================================================
class BWin32RegistryTopic
{
   public:
      BWin32RegistryTopic(void) :
                                 mhKey(0),
                                 mbInUse(false),
                                 mbUser(false),
                                 mbReadOnly(false)
      {
      }

      ~BWin32RegistryTopic(void) 
      {
      }


      BString mTopic;
      HKEY mhKey;
      bool mbInUse;
      bool mbUser;
      bool mbReadOnly;

}; // BWin32RegistryTopic

//==============================================================================
class BWin32Registry
{
   public:
      BWin32Registry(void) : mdwTopicsInUse(0)
      {
      }

      ~BWin32Registry(void);

      long createTopic(const BCHAR_T *pRoot, bool bReadOnly, bool bUser=false, bool bClassesRoot = false);
      bool removeTopic(long dwtopic);

      bool writeKey(long dwtopic, const BCHAR_T *pkey, long dwdata);
      bool writeKey(long dwtopic, const BCHAR_T *pkey, const unsigned char *pbytes, long dwbytes);
      bool writeKey(long dwtopic, const BCHAR_T *pkey, const BCHAR_T *pbuffer);

      bool readKey(long dwtopic, const BCHAR_T *pkey, long *pdwdata);
      bool readKey(long dwtopic, const BCHAR_T *pkey, unsigned char *dwdata, DWORD dwmaxbytes, DWORD *pdwReadBytes);
      bool readKey(long dwtopic, const BCHAR_T *pkey, BCHAR_T *pbuffer, DWORD dwmaxbytes, DWORD *pdwReadBytes);

   private:
      enum
      {
         cMaxRegTopics = 8
      };

      BWin32RegistryTopic mTopics[cMaxRegTopics];
      DWORD mdwTopicsInUse;

}; // BWin32Registry

//==============================================================================

#endif // __WIN32REGISTRYHDR__

//==============================================================================
// Win32registry,h
//==============================================================================
