//==============================================================================
// Win32registry,cpp
//
// Copyright (c) 1999 Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "win32registry.h"

//==============================================================================
BWin32Registry registry;

// rg [1/31/06] - Had to add "ES_" prefix to these constants.
static const HKEY ES_HKEY_CLASSES_ROOT=(HKEY)1;
static const HKEY ES_HKEY_CURRENT_USER=(HKEY)2;
static const HKEY ES_HKEY_LOCAL_MACHINE=(HKEY)3;

//==============================================================================
// example usage
//==============================================================================
#ifdef _NEVERDEF

   char buffer[1024];
   DWORD dwbytes;
   bool rc;


   // the topic is looked for in the key:
   // HKEY_LOCAL_USER\\Software\\EnsembleStudios\\
   // 

   // this accesses a full key of: 
   // HKEY_LOCAL_USER\\Software\\EnsembleStudios\\RTS3\\MaxViewport

   long dwtopic = registry.createTopic("RTS3\\MaxViewport");

   // this accesses a full key of: 
   // HKEY_LOCAL_USER\\Software\\EnsembleStudios\\RTS3\\
   //

   long dwRts3topic = registry.createTopic("RTS3");

   if (rc < 0)
      { /* handle error */ }

   // this reads the key value: 
   // HKEY_LOCAL_USER\\Software\\EnsembleStudios\\RTS3\\MaxViewport\\WorkingDirectory

   rc = registry.readKey(dwtopic, "WorkingDirectory", buffer, sizeof(buffer), &dwbytes);

   if (rc < 0)
      { /* handle error */ }

   // this writes the key value
   // HKEY_LOCAL_USER\\Software\\EnsembleStudios\\RTS3\\MaxViewport\\enable
   
   // as a char buffer

   rc = registry.writeKey(dwtopic, "Enable", buffer);

   if (rc < 0)
      { /* handle error */ }

   // as a dword (ie: 4 bytes of data)

   long dwenable = 0x0c;

   rc = registry.writeKey(dwtopic, "Enable", dwenable);

   if (rc < 0)
      { /* handle error */ }

   // as a binary field of somelength

   BVector vector;

   rc = registry.writeKey(dwtopic, "Vector", &vector, sizeof(vector));

   if (rc < 0)
      { /* handle error */ }

   // when all done, remove topic which closes registry key

   registry.removeTopic(dwtopic);

#endif // _NEVERDEF

//==============================================================================
// BWin32Registry::~BWin32Registry
//==============================================================================
BWin32Registry::~BWin32Registry(void)
{
   for (DWORD i = 0; i < cMaxRegTopics; i++)
   {
      if (mTopics[i].mbInUse)
         removeTopic(i);
   }

} // BWin32Registry::~BWin32Registry

//==============================================================================
// BWin32Registry::createTopic
//==============================================================================
long BWin32Registry::createTopic(const BCHAR_T *pRoot, bool bReadOnly, bool bUser, bool bClassesRoot)
{
   if (!pRoot)
   {
      BASSERT(0);
      return -1;
   }

   BString usRegEntry(pRoot);

   // any room?
   if ((mdwTopicsInUse + 1) >= cMaxRegTopics)
   {
      BASSERT(0);
      return -1;
   }

   HKEY hkey;

   if (bUser)
   {
      if(bClassesRoot)
         hkey = ES_HKEY_CLASSES_ROOT;
      else
         hkey = ES_HKEY_CURRENT_USER;
   }
   else
      hkey = ES_HKEY_LOCAL_MACHINE;

   // look for topic
   DWORD i;
   DWORD j;
   for (i = 0, j = 0; (i < cMaxRegTopics) && (j < mdwTopicsInUse); i++)
   {
      if (mTopics[i].mbInUse)
      {
         j++;
         if ((bUser == mTopics[i].mbUser) && (bReadOnly == mTopics[i].mbReadOnly) && (usRegEntry == mTopics[i].mTopic))
         {
            return i;
         }
      }
   }

   // look for empty topic
   for (i = 0; i < cMaxRegTopics; i++)
      if (!mTopics[i].mbInUse)
         break;
   if(i>=cMaxRegTopics)
      return(-1);

   // default to user key for the build process
   #ifdef _BUILD_PROCESS
      bUser=true;
   #endif

   // add new topic
   mTopics[i].mbInUse = true;
   mTopics[i].mTopic = usRegEntry;
   mTopics[i].mbUser = bUser;
   mTopics[i].mbReadOnly = bReadOnly;

   long rc;
   DWORD disposition;

   rc = RegCreateKeyEx(hkey, mTopics[i].mTopic, 0, 0,
                       REG_OPTION_NON_VOLATILE, 
                       bReadOnly ? KEY_READ | KEY_QUERY_VALUE : KEY_ALL_ACCESS,
                       0, &mTopics[i].mhKey, &disposition );

   if (rc != ERROR_SUCCESS)
   {
      BFAIL("Problem occurred creating/obtaining the registry key. Please make sure you have administrator access to this machine.");
      mTopics[i].mbInUse = false;
      return -1;
   }

   // incr # of topics
   mdwTopicsInUse++;

   return i;

} // BWin32Registry::createTopic

//==============================================================================
// BWin32Registry::removeTopic
//==============================================================================
bool BWin32Registry::removeTopic(long dwtopic)
{
   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   mdwTopicsInUse--;

   mTopics[dwtopic].mTopic.setNULL();
   mTopics[dwtopic].mbInUse = false;

   long rc = RegCloseKey(mTopics[dwtopic].mhKey);

   if (rc != ERROR_SUCCESS)
   {
      BASSERT(0);
      return false;
   }

   return true;

} // BWin32Registry::removeTopic

//==============================================================================
// BWin32Registry::writeKey
//==============================================================================
bool BWin32Registry::writeKey(long dwtopic, const BCHAR_T *pkey, long dwdata)
{
   // valid?

   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   // write key

   long rc = RegSetValueEx(mTopics[dwtopic].mhKey, pkey, 0, REG_DWORD, (const unsigned char *) &dwdata, 4);

   if (rc != ERROR_SUCCESS)
   {
      BASSERT(0);
      return false;
   }

   // all done

   return true;

} // BWin32Registry::writeKey

//==============================================================================
// BWin32Registry::writeKey
//==============================================================================
bool BWin32Registry::writeKey(long dwtopic, const BCHAR_T *pkey, const unsigned char *pbytes, long dwbytes)
{
   // valid?

   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   // write data

   long rc = RegSetValueEx(mTopics[dwtopic].mhKey, pkey, 0, REG_BINARY, pbytes, dwbytes);

   if (rc != ERROR_SUCCESS)
   {
      BASSERT(0);
      return false;
   }

   // all done

   return true;

} // BWin32Registry::writeKey

//==============================================================================
// BWin32Registry::writeKey
//==============================================================================
bool BWin32Registry::writeKey(long dwtopic, const BCHAR_T *pkey, const BCHAR_T *pbuffer)
{
   // valid?

   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   // write data

   long rc = RegSetValueEx(mTopics[dwtopic].mhKey, pkey, 0, REG_SZ, (const unsigned char *) pbuffer, (bcslen(pbuffer) + 1)*sizeof(BCHAR_T));

   if (rc != ERROR_SUCCESS)
   {
      BASSERT(0);
      return false;
   }

   // all done

   return true;

} // BWin32Registry::writeKey

//==============================================================================
// BWin32Registry::readKey
//==============================================================================
bool BWin32Registry::readKey(long dwtopic, const BCHAR_T *pkey, long *pdwdata)
{
   long rc;
   DWORD dwbytes;
   DWORD dwtype;

   // valid?

   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   // read data
   dwbytes = 4;
   rc = RegQueryValueEx(mTopics[dwtopic].mhKey, pkey, 0, &dwtype, (unsigned char *) pdwdata, &dwbytes);

   if (rc != ERROR_SUCCESS)
   {
      return false;
   }

   BASSERT(dwtype == REG_DWORD_LITTLE_ENDIAN);

   BASSERT(dwbytes == 4);

   // all done

   return true;
} // BWin32Registry::readKey

//==============================================================================
// BWin32Registry::readKey
//==============================================================================
bool BWin32Registry::readKey(long dwtopic, const BCHAR_T *pkey, unsigned char *pdwdata, DWORD dwmaxbytes, DWORD *pdwReadBytes)
{
   long rc;
   DWORD dwtype;

   // valid?

   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   // read data
   *pdwReadBytes = dwmaxbytes;   // must pass in the valid size   
   rc = RegQueryValueEx(mTopics[dwtopic].mhKey, pkey, 0, &dwtype, (unsigned char *) pdwdata, pdwReadBytes);

   if (rc != ERROR_SUCCESS)
   {
      return false;
   }

   BASSERT(dwtype == REG_BINARY);

   //BASSERT(dwmaxbytes > *pdwReadBytes);
   BASSERT(dwmaxbytes >= *pdwReadBytes);

   // all done

   return true;
} // BWin32Registry::readKey

//==============================================================================
// BWin32Registry::readKey
//==============================================================================
bool BWin32Registry::readKey(long dwtopic, const BCHAR_T *pkey, BCHAR_T *pbuffer, DWORD dwmaxbytes, DWORD *pdwReadBytes)
{
   long rc;
   DWORD dwtype;
   TCHAR cerrmsg[1024];

   // valid?

   if (!mTopics[dwtopic].mbInUse)
   {
      BASSERT(0);
      return false;
   }

   // read data
   *pdwReadBytes = dwmaxbytes;   // must pass in the valid size   
   rc = RegQueryValueEx(mTopics[dwtopic].mhKey, pkey, 0, &dwtype, (unsigned char *) pbuffer, pdwReadBytes);

   if (rc != ERROR_SUCCESS)
   {
      FormatMessage( 
          FORMAT_MESSAGE_FROM_SYSTEM | 
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          GetLastError(),
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
          cerrmsg,
          sizeof(cerrmsg) / sizeof(cerrmsg[0]),
          NULL 
      );

      return false;
   }

   BASSERT(dwtype == REG_SZ);

   //BASSERT(dwmaxbytes > *pdwReadBytes);
   BASSERT(dwmaxbytes >= *pdwReadBytes);

   // all done

   return true;
} // BWin32Registry::readKey

//==============================================================================
// eof: Win32registry,cpp
//==============================================================================
