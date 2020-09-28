//==============================================================================
// config.cpp
//
// Copyright (c) 1999, Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "config.h"
#include "econfigenum.h"
#include "fileUtils.h"
#include "string\bsnprintf.h"
#include "stream\byteStream.h"

// moved to wacky static file
//BConfig gConfig;

//==============================================================================
// Defines
BConfigFormal::BConfigFormal(const char* text, const char* helpText, long defaultFlags/*=0*/, BConfigCallback pCallback/*=NULL*/)
{
   StringCchCopyA(mText, countof(mText), text);
#if CONFIG_HELP_TEXT   
   StringCchCopyA(mHelpText, countof(mHelpText), helpText);
#endif   
   
   mDefaultFlags=defaultFlags;
   mCallback=pCallback;
   mIndex=-1;
}

//=============================================================================
// ltrimWhitespace
//=============================================================================
void ltrimWhitespace(char *str)
{
   // Find first non-whitespace char.
   long pos;
   for(pos=0; str[pos]!='\0' && (str[pos]==' ' || str[pos]=='\t' || str[pos]==13 || str[pos]==10); pos++)
   {
      // spin.
   }

   // Bail now if empty.
   if(pos==0)
      return;

   // Move string down.
   for(long i=0; str[i]!='\0'; i++)
      str[i]=str[i+pos];
}

//==============================================================================
// BConfig::BConfig
//==============================================================================
BConfig::BConfig(void) :
   mConfigs(),
   mTableInitted(false), 
   mPersistentFile(),
   mEnforceOrder(false),
   mAllowCallbacks(false),
   mBaseDirectoryID(-1)
{
   // make sure our data is all set to blank
   clearAllState();

   // We always add the config formals defined at the ecore level.
   //BConfigTable table;
   //table.mFormalArray = gEConfigFormals;
   //table.mNumFormals = cMaxEConfigEnums;
   //addConfigTable(table);
} 

//==============================================================================
// BConfig::~BConfig
//==============================================================================
BConfig::~BConfig(void)
{
   //writePersistent();
   long num = mConfigFormals.getNumber();
   for(long i=0; i<num; i++)
   {
      if(mConfigFormals[i])
      {
         delete mConfigFormals[i];
         mConfigFormals[i]=NULL;
      }
   }
} 

//==============================================================================
// BConfig::init
//==============================================================================
void BConfig::init(const char *cmdLine)
{
   mAllowCallbacks = false;
   // store off the cmd line
   mCmdLine.set(cmdLine);

   // Note that it is critical that all the config tables have been installed into 
   // the config system before this goes off!
   
   // now read in all the data
   clearAllState();
   mAllowCallbacks = true;
}

//==============================================================================
// BConfig::init
//==============================================================================
long BConfig::registerConfig(const char* text, const char* helpText, long defaultFlags, BConfigCallback pCallback)
{
   BASSERT(mTableInitted);
   BConfigFormal* pFormal = new BConfigFormal(text, helpText, defaultFlags, pCallback);
   // add the formal to our big array-o-tron
   pFormal->mEnum = mConfigFormals.add(pFormal);
   return(pFormal->mEnum);
}


//==============================================================================
// BConfig::writePersistent
//==============================================================================
const char *cPersistentStart = "\r\n// *** BEGIN PERSISTENT CONFIG VARS ***\r\n";
const char *cPersistentEnd = "// *** END PERSISTENT CONFIG VARS ***\r\n";

void BConfig::writePersistent(void)
{
   BFile            file; 
   BConfigData       *pData=0;
   long              i, j;
   BDynamicSimArray<BSimString> fileDirectories;
   BSimString         fileName, tempFileName;

  
   // allow suspension of persistent vars altogether
   if (isDefined("noPersistentConfigs"))
      return;
 
   //set up a dummy file name for those persistent configs which get generated during the game and
   //do not have a directory set
   
   tempFileName.format(B("persistent.cfg"));

   //get a list of the directory/files that are needed to be opened
   for (i=0; i < mConfigs.getNumber(); i++)
   {
      pData = &mConfigs[i];
      if (pData->mFlags.isSet(BConfigData::cFlagPersistent) && (!pData->mFlags.isSet(BConfigData::cFlagNotSet)))
      {
         if (pData->mName.length() == 0)
            continue;

         if ((pData->mSource == BConfigData::cSourcePersistentCfg) || (pData->mSource == BConfigData::cSourceProfileCfg))
         {
            //If the config does not have a directory then give it one
            if (pData->mDirectory.length() <= 0)
               pData->mDirectory.set( tempFileName );

            fileDirectories.uniqueAdd( pData->mDirectory );
         }
      }
   } //end for loop on getting directory names
   

   // there should only ever be two places we are writing out to: profile.cfg and persistent.cfg
   // if we have some left over, it is usually cruft that is going to cause us to obliterate someone's profile.cfg
   BASSERT(fileDirectories.getNumber() <= 2);

   //loop throught the file directory names and write out the appropriate configs vars to it
   for (j=0; j<fileDirectories.getNumber(); j++)
   {
      fileName.set( fileDirectories.get(j) );
      //the persistent config does not have a directory set
      if (fileName.length() <= 0)
      {
         BFAIL("WARNING: blank persistent config variable directory");
         continue;
      }

      bool ok = file.openWriteable(mBaseDirectoryID, fileName, BFILE_OPEN_ENABLE_BUFFERING);
      if (!ok)
      {
         BSimString temp;
         temp.format(B("WARNING: Could not open the persistent cfg file: '%s'!"), fileName.getPtr());
         BFAIL(temp.getPtr());
      }
      else
      {
         file.fprintf("// %s\r\n", fileName.getPtr());
         // now loop and write out the configs which correspond to this directory
         for (i=0; i < mConfigs.getNumber(); i++)
         {
            pData = &mConfigs[i];
            if (pData->mFlags.isSet(BConfigData::cFlagPersistent) && !pData->mFlags.isSet(BConfigData::cFlagNotSet))
            {
               if (pData->mName.length() == 0)
                  continue;

               if ((fileName != pData->mDirectory) && (pData->mSource != BConfigData::cSourceCode))
               {
                  continue;
               }

               if (pData->mSource >= BConfigData::cSourcePersistentCfg) 
               {
                  switch(pData->mType)
                  {
                     case BConfigData::cDataFloat:
                        file.fprintf("%s %f\r\n",pData->mName.getPtr(),pData->mFloatVal);
                        break;
                     case BConfigData::cDataLong:
                        file.fprintf("%s %d\r\n",pData->mName.getPtr(),pData->mVal);
                        break;
                     case BConfigData::cDataString:
                        file.fprintf("%s \"%s\"\r\n", pData->mName.getPtr(), pData->mText.getPtr());
                        break;
                     case BConfigData::cDataNone:
                        file.fprintf("+%s\r\n",pData->mName.getPtr());
                        break;
                  }
               }
            }
         } //end for loop on config vars
      } // end else could not open file   
   } //end loop on file directory names

   return; 
} //end BConfig::writePersistent

//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::getCore(const BConfigData *pCfg, long *pVal)
{
   BASSERT(pVal);
   BASSERT(GetCurrentThreadId() == gEventDispatcher.getThreadId(cThreadIndexSim));

   if (pCfg == NULL)
      return(false);
   
   switch (pCfg->mType)
   {
      case BConfigData::cDataLong:
         // okay, this is the thing... if it is a long-val we are all set
         *pVal = pCfg->mVal;
         break;
      case BConfigData::cDataFloat:
         *pVal = long(pCfg->mFloatVal);  // yay casting
         break;
      case BConfigData::cDataString:
         // okay, we have to do something wacky here to indicate a
         // kind of wrong return type... maybe we should be more drastic
         // in the future?
         *pVal = pCfg->mText.getPtr()[0];
         break;
   }
   return(true);
}

//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::get(const char *name, long *pVal)
{
   BConfigData *pCfg;
   pCfg = findConfig(name);
   return(getCore(pCfg,pVal));
}

//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::get(long id, long *pVal)
{
   BConfigData *pCfg = NULL;
   pCfg = findConfig(id);
   return(getCore(pCfg,pVal));
}

//==============================================================================
// BConfig::getCore
//==============================================================================
bool BConfig::getCore(const BConfigData *pCfg, float *pVal)
{
   BASSERT(pVal);
   BASSERT(GetCurrentThreadId() == gEventDispatcher.getThreadId(cThreadIndexSim));
      
   if (pCfg == NULL)
      return(false);

   switch (pCfg->mType)
   {
      case BConfigData::cDataLong:
         *pVal = float(pCfg->mVal); // yay casting
         break;
      case BConfigData::cDataFloat:
         // okay, this is the thing... if it is a float we are all set
         *pVal = pCfg->mFloatVal;  
         break;
      case BConfigData::cDataString:
         // okay, we have to do something wacky here to indicate a
         // kind of wrong return type... maybe we should be more drastic
         // in the future?
         *pVal = pCfg->mText.getPtr()[0];
         break;
   }
   return(true);
}

//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::get(const char *name, float *pVal)
{
   BConfigData *pCfg;
   pCfg = findConfig(name);
   return(getCore(pCfg,pVal));
}

//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::get(long id, float *pVal)
{
   BConfigData *pCfg;
   pCfg = findConfig(id);
   return(getCore(pCfg,pVal));
}

//==============================================================================
// BConfig::getCore
//==============================================================================
bool BConfig::getCore(const BConfigData *pCfg, BSimString &string)
{
   BASSERT(GetCurrentThreadId() == gEventDispatcher.getThreadId(cThreadIndexSim));
   
   if (pCfg == NULL)
   {
      return(false);
   }
   
   switch (pCfg->mType)
   {
      case BConfigData::cDataLong:
         string.format(B("%d"), pCfg->mVal);
         break;
      case BConfigData::cDataFloat:
         string.format(B("%f"), pCfg->mFloatVal);
         break;
      case BConfigData::cDataString:
         // okay, this is the thing... if it is a string-val we are all set
         string.set(pCfg->mText.getPtr());
         break;
   }
   return(true);
}


//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::get(const char *name, BSimString &string)
{
   BConfigData *pCfg;
   pCfg = findConfig(name);
   return(getCore(pCfg,string));
}


//==============================================================================
// BConfig::get
//==============================================================================
bool BConfig::get(long id, BSimString &string)
{
   BConfigData *pCfg;
   pCfg = findConfig(id);
   return(getCore(pCfg,string));
}

//==============================================================================
// BConfig::findConfig
//==============================================================================
BConfigData *BConfig::findConfig(const char *name, bool findDeleted)
{
   // look through all of our configs for a matching name
   for (long i=0; i < mConfigs.getNumber(); i++)
   {
      if (stricmp(mConfigs[i].mName.getPtr(), name)==0)
      {
         if (mConfigs[i].mFlags.isSet(BConfigData::cFlagNotSet))
         {
            if (!findDeleted)
               return(NULL);
         }
         return(&mConfigs[i]);
      }
   }
   return(NULL);
}

//==============================================================================
// BConfig::findConfig
//==============================================================================
BConfigData *BConfig::findConfig(long id, bool findDeleted)
{
   // make sure this is a valid access
   BASSERT(id >=0);
   BASSERT(id < getNumConfigEnums());
   BASSERT(mTableInitted);

   long index = getFormal(id)->mIndex;
   if (index == -1)
      return(NULL);

   if (mConfigs[index].mFlags.isSet(BConfigData::cFlagNotSet))
   {
      if (!findDeleted)
         return(NULL);
   }
   return(&mConfigs[index]);
}

//==============================================================================
// BConfig::setCore
//==============================================================================
BConfigData *BConfig::setCore(const char *name)
{
   // first look for an existing match
   BConfigData *pConfig;
   pConfig = findConfig(name, true);
   if (pConfig != NULL)
   {
#ifdef BUILD_FINAL
      if (pConfig->mFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
         return(NULL);
#endif

      if (mEnforceOrder)
      {
         // if we aren't cool enough to override this, then, um, don't
         if (mCurrentSource < pConfig->mSource)
            return(NULL);
      }
      // okay, we have a match, so just return that
      pConfig->mFlags.unset(BConfigData::cFlagNotSet);
      BASSERT(mCurrentSource != -1);
      pConfig->mSource = mCurrentSource;
      // restore the proper formal index
      BConfigFormal *pFormal = getFormal(name);
      if (pFormal != NULL)
      {
         pFormal->mIndex = pConfig->mIndex;
         if (pConfig->mIndex == -1)
            BASSERT(0);
         pConfig->mFormalIndex = pFormal->mEnum;
      }
      return(pConfig);
   }

   return createCore(name);
}

//==============================================================================
// BConfig::isFormal
//==============================================================================
bool BConfig::isFormal(long configIndex) const
{
   for (long i=0;i < getNumConfigEnums();i++)
   {
      if (getFormal(i)->mIndex == configIndex)
         return true;
   }

   return false;
}

//==============================================================================
// BConfig::createCore
//==============================================================================
BConfigData *BConfig::createCore(long id)
{
   // Check id's validity.
   if(id<0 || id>=getNumConfigEnums())
   {
      BASSERT(0);
      return(false);
   }
   
   BBitVector tempFlags;
   tempFlags.setAll(getFormal(id)->mDefaultFlags);
#ifdef BUILD_FINAL
   if (tempFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
      return(NULL);
#endif

   // lets "allocate" a new one
   BConfigData *pConfig=0;
   int index;

   index = mConfigs.getNumber();
   mConfigs.setNumber(index + 1);
   pConfig = &mConfigs[index];
   pConfig->mName.set(getFormal(id)->mText);
   pConfig->mType = BConfigData::cDataNone;
   pConfig->mFlags.setAll(getFormal(id)->mDefaultFlags);
   pConfig->mIndex = index;
   pConfig->mSource = mCurrentSource;
   pConfig->mFormalIndex = id;
   BASSERT(mCurrentSource != -1);

   // check whether we need to update the formal table
   if (mTableInitted)
   {
      getFormal(id)->mIndex = index;
   }

   if (getFormal(id)->mIndex == -1)
      BASSERT(0);
   return(pConfig);
}

//==============================================================================
// BConfig::createCore
//==============================================================================
BConfigData *BConfig::createCore(const char *name)
{
   // check whether we need to update the formal table
   long i;
   long formalIndex = -1;
   for (i=0; i < getNumConfigEnums(); i++)
   {
      if (stricmp(getFormal(i)->mText,name) == 0)
      {
         formalIndex = i;
         break;
      }
   }

   if (formalIndex != -1)
   {
      BBitVector tempFlags;
      tempFlags.setAll(getFormal(formalIndex)->mDefaultFlags);
#ifdef BUILD_FINAL
      if (tempFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
         return(NULL);
#endif
   }

   // lets "allocate" a new one
   BConfigData *pConfig=0;
   int index;

   index = mConfigs.getNumber();
   mConfigs.setNumber(index + 1);
   pConfig = &mConfigs[index];   
   pConfig->mName.set(name);
   pConfig->mType = BConfigData::cDataNone;
   pConfig->mIndex = index;
   pConfig->mSource = mCurrentSource;
   pConfig->mFormalIndex = formalIndex;

   if (mTableInitted && (formalIndex != -1))
   {
      getFormal(formalIndex)->mIndex = index;
      pConfig->mFlags.setAll(getFormal(formalIndex)->mDefaultFlags);
   }

   if (getFormal(name) && (getFormal(name)->mIndex == -1))
      BASSERT(0);

   return(pConfig);
}

//==============================================================================
// BConfig::setCore
//==============================================================================
BConfigData *BConfig::setCore(long id)
{
   // Check id's validity.
   if(id<0 || id>=getNumConfigEnums())
   {
      BASSERT(0);
      return(false);
   }

   // first look for an existing match
   BConfigData *pConfig;
   pConfig = findConfig(id, true);
   if (pConfig != NULL)
   {
#ifdef BUILD_FINAL
      if (pConfig->mFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
         return(NULL);
#endif

      if (mEnforceOrder)
      {
         // if we aren't cool enough to override this, then, um, don't
         if (mCurrentSource < pConfig->mSource)
            return(NULL);
      }
      // okay, we have a match, so just return that
      pConfig->mFlags.unset(BConfigData::cFlagNotSet);
      BASSERT(mCurrentSource != -1);
      pConfig->mSource = mCurrentSource;
      if (pConfig->mIndex == -1)
      {
         BASSERT(0);
      }
      BConfigFormal *pFormal = getFormal(id);
      if (pFormal == NULL)
      {
         BASSERT(0);
      }
      else
      {
         pFormal->mIndex = pConfig->mIndex;
         pConfig->mFormalIndex = id;
      }
      return(pConfig);
   }

   return createCore(id);
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(const char *name, long val, long flags)
{   
   // Note: If you change the way this works, also change the way the overloaded fn in titanconfig works
   BConfigData *pConfig = setCore(name);
   if (!pConfig)
      return;

   BConfig::set(pConfig, val, flags);
   const BConfigFormal *pFormal = getFormal(name);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(pFormal->mEnum, true);   
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(const char *name, const char *val, long flags)
{
   // Note: If you change the way this works, also change the way the overloaded fn in titanconfig works
   BConfigData *pConfig = setCore(name);

   if (!pConfig)
      return;
   BConfig::set(pConfig, val, flags);
   const BConfigFormal *pFormal = getFormal(name);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(pFormal->mEnum, true);
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(const char *name, float val, long flags)
{
   BConfigData *pConfig = setCore(name);

   // Note: If you change the way this works, also change the way the overloaded fn in titanconfig works
   if (!pConfig)
      return;
   BConfig::set(pConfig, val, flags);
   const BConfigFormal *pFormal = getFormal(name);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(pFormal->mEnum, true);   
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(long id, long val, long flags)
{
   BConfigData *pConfig = setCore(id);
      
   // Note: If you change the way this works, also change the way the overloaded fn in titanconfig works
   if (!pConfig)
      return;
   BConfig::set(pConfig, val, flags, id);
   const BConfigFormal *pFormal = getFormal(id);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(id, true);   
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(long id, const char *val, long flags)
{
   BConfigData *pConfig = setCore(id);

   // Note: If you change the way this works, also change the way the overloaded fn in titanconfig works
   if (!pConfig)
      return;
   BConfig::set(pConfig, val, flags, id);
   const BConfigFormal *pFormal = getFormal(id);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(id, true);   
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(long id, float val, long flags)
{
   BConfigData *pConfig = setCore(id);

   // Note: If you change the way this works, also change the way the overloaded fn in titanconfig works      
   if (!pConfig)
      return;
   BConfig::set(pConfig, val, flags, id);
   const BConfigFormal *pFormal = getFormal(id);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(id, true);   
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(BConfigData *pConfig, long val, long flags, long id)
{  
   id; flags;

#ifdef BUILD_FINAL
   if (pConfig->mFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
      return;
#endif

   pConfig->mVal = val;
   pConfig->mFloatVal = 0;
   pConfig->mText.empty();
   pConfig->mType = BConfigData::cDataLong;      
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(BConfigData *pConfig, const char *val, long flags, long id)
{
   id; flags;

#ifdef BUILD_FINAL
   if (pConfig->mFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
      return;
#endif

   pConfig->mVal = 0;
   pConfig->mFloatVal = 0;
   pConfig->mText.set(val);
   pConfig->mType = BConfigData::cDataString;   
}

//==============================================================================
// BConfig::set
//==============================================================================
void BConfig::set(BConfigData *pConfig, float val, long flags, long id)
{
   id; flags;

#ifdef BUILD_FINAL
   if (pConfig->mFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
      return;
#endif

   pConfig->mFloatVal = val;
   pConfig->mVal = 0;
   pConfig->mText.empty();
   pConfig->mType = BConfigData::cDataFloat;   
}


//==============================================================================
// BConfig::getName
//==============================================================================
bool BConfig::getName(long id, BSimString *pVal)
{
   BConfigData *pCfg;
   pCfg = findConfig(id);
   if (pCfg)
   {
      pVal->set(pCfg->mName);
      return(true);
   }

   return(false);
}

//==============================================================================
// BConfig::getFormalName
//==============================================================================
bool BConfig::getFormalName(long id, BSimString *pVal)
{
   if ( (id<0) || (id>=getNumConfigEnums()) )
      return(false);

   pVal->set(getFormal(id)->mText);

   return(true);
}


//==============================================================================
// BConfig::getDirectory
//==============================================================================
bool BConfig::getDirectory(const char *name, BSimString *pVal)
{
   bool                 retVal=true;
   BConfigData          *pConfig=0;

   pConfig = findConfig(name);
   if (pConfig)
      pVal->set( pConfig->mDirectory );
   else
      retVal = false;

   return(retVal);
}

//==============================================================================
// BConfig::getDirectory
//==============================================================================
bool BConfig::getDirectory(long id, BSimString *pVal)
{
   bool                 retVal=true;
   BConfigData          *pConfig=0;

   pConfig = findConfig(id);
   if (pConfig)
     pVal->set( pConfig->mDirectory );
   else
      retVal = false;

   return(retVal);
}

//==============================================================================
// BConfig::setDirectory
//==============================================================================
bool BConfig::setDirectory(const char *name, const char *val)
{
   bool                 retVal=true;
   BConfigData          *pConfig=0;

   pConfig = findConfig(name);
   if (pConfig)
      pConfig->mDirectory.set(val);
   else
      retVal = false;

   return(retVal);
}

//==============================================================================
// BConfig::setDirectory
//==============================================================================
bool BConfig::setDirectory(long id, const char *val)
{
   bool                 retVal=true;
   BConfigData          *pConfig=0;

   pConfig = findConfig(id);
   if (pConfig)
      pConfig->mDirectory.set(val);
   else
      retVal = false;

   return(retVal);
}

//==============================================================================
// BConfig::define
//==============================================================================
void BConfig::define(const char *name)
{
   // the act of doing the core set does it all for us...
   if (!setCore(name))
      return;

   const BConfigFormal *pFormal = getFormal(name);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(pFormal->mEnum, true);
}

//==============================================================================
// BConfig::define
//==============================================================================
void BConfig::define(long id)
{
   // the act of doing the core set does it all for us...
   if (!setCore(id))
      return;

   const BConfigFormal *pFormal = getFormal(id);
   if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
      pFormal->mCallback(id, true);
}

//==============================================================================
// BConfig::toggleDefine
//==============================================================================
void BConfig::toggleDefine(const char *name)
{
   if (isDefined(name))
   {
      remove(name);
   }
   else
   {
      define(name);
   }
}

//==============================================================================
// BConfig::toggleDefine
//==============================================================================
void BConfig::toggleDefine(long id)
{
   if (isDefined(id))
   {
      remove(id);
   }
   else
   {
      define(id);
   }
}

//==============================================================================
// BConfig::removeInternal
//==============================================================================
bool BConfig::removeInternal(BConfigData *pConfig, const char *name)
{
   // update the formals table
   /*
   if (mTableInitted)
   {
      long i;
      for (i=0; i < getNumConfigEnums(); i++)
      {
         if (fastStricmp(getFormal(i)->mText, name) == 0)
         {
            getFormal(i)->mIndex = -1;
            break;
         }
      }
   }
   */
   
#ifdef BUILD_FINAL
   if (pConfig->mFlags.isSet(BConfigData::cFlagRestricted) && (mCurrentSource != BConfigData::cSourceGameCfg))
      return(NULL);
#endif

   if (mEnforceOrder)
   {
      // if we aren't cool enough to override this, then, um, don't
      if (mCurrentSource < pConfig->mSource)
         return false;
   }

   name;
   // a match?  blow it away!
   //pConfig->erase();
   pConfig->mFlags.set(BConfigData::cFlagNotSet);
   pConfig->mSource = mCurrentSource;
   BASSERT(mCurrentSource != -1);
   pConfig->mDirectory.empty();

   // MS 9/24/2003: bracket this in a check against -1 so we don't assert
   // on configs that aren't formalized (on JCE's suggestion)
   if(pConfig->mFormalIndex != -1)
   {
      BConfigFormal *pFormal = getFormal(pConfig->mFormalIndex);
      if (mAllowCallbacks && (pFormal != NULL) && (pFormal->mCallback != NULL))
         pFormal->mCallback(pFormal->mEnum, false);   
   }

   // we did find something to erase
   return(true);
}

//==============================================================================
// BConfig::remove
//==============================================================================
bool BConfig::remove(const char *name)
{
   BConfigData *pConfig;
   pConfig = findConfig(name);
   
   // no match so we have to create an "unset" config to preserve ordering
   if (pConfig == NULL)
   {
      //  [7/18/2002] that functionality is no longer needed, so we should just return
      return(true);

      //define(name);
      //return(remove(name));
   }

   return removeInternal(pConfig, name);
}

//==============================================================================
// BConfig::remove
//==============================================================================
bool BConfig::remove(long id)
{
   BConfigData *pConfig;
   pConfig = findConfig(id);

   // no match so we have to create an "unset" config to preserve ordering
   if (pConfig == NULL)
   {
      //  [7/18/2002] that functionality is no longer needed, so we should just return
      return(true);

      //define(id);
      //return(remove(id));
   }
   
   return removeInternal(pConfig, pConfig->mName.getPtr());
}

//==============================================================================
// BConfig::removeIfFlag
//==============================================================================
void BConfig::removeIfFlag(long flagPos)
{
   int i;

   for (i=0; i < mConfigs.getNumber(); i++)
   {
      if ( mConfigs[i].mFlags.isSet(flagPos) )
      {
         remove(mConfigs[i].mName.getPtr());
      }
   } //end outer loop
}

//==============================================================================
// BConfig::removeExactByFlags
// remove all configs variable which have a particular set of flags
//==============================================================================
void BConfig::removeExactByFlags(BBitVector flags)
{
   int i;

   for (i=0; i < mConfigs.getNumber(); i++)
   {
      if ( mConfigs[i].mFlags.getValue() == flags.getValue() )
      {
         remove(mConfigs[i].mName.getPtr());
      }
   } //end outer loop
}

//==============================================================================
// BConfig::clearAllState
//==============================================================================
void BConfig::clearAllState(long exceptSource)
{
   int i;
   for (i=0; i < mConfigs.getNumber(); i++)
   {
      if (mConfigs[i].mSource != exceptSource)
         remove(mConfigs[i].mName.getPtr());
   }   

   mCurrentSource = BConfigData::cSourceUnknown;

   // set up for formals, now that we've done all that
   // required, since we cleared previously
   //createEnumTable();
   mTableInitted = true;
}

//==============================================================================
// BConfig::clearSource
//==============================================================================
void BConfig::clearSource(long source)
{
   int i;
   for (i=0; i < mConfigs.getNumber(); i++)
   {
      if (mConfigs[i].mSource != source)
         continue;

      if (mConfigs[i].mFlags.isSet(BConfigData::cFlagNotSet))
         continue;

      remove(mConfigs[i].mName.getPtr());      
   }   
}

//==============================================================================
// BConfig::processToken
//==============================================================================
void BConfig::processToken(BSimString &token)
{
   // this is a semi-gruesome hack to work around the fact that we don't want to
   // change allocation when specifying memory manager controls on the cmd line
   if (token.compare(B("mm"), false, 2) == 0)
      return;

   // if we are starting with a + or -, that means to define or undefine a var
   if (token.getPtr()[0] == B('-'))
   {
      // remove the leading - as we go in
      remove(token.getPtr() + 1);
      return;
   }
   if (token.getPtr()[0] == B('+'))
   {
      // remove the leading + as we go in
      define(token.getPtr() + 1);
   }

   // make sure we have an equals sign, otherwise someone else probably thinks
   // they are handling this, so we should punt and let them
   if (token.findLeft(B("=")) == -1)
      return;

/*
   const char *arg;
   arg = strchr(token,'=');
   if (arg == NULL)
      return;
*/
   // poke the = to a space so we can pass it off as a normal .cfg line, like from a file
   token.findAndReplace(B('='), B(' '));
   //token.replace('=', ' ');

   process(token);
}


//==============================================================================
// BConfig::process
//
// Process a config string to get the config var's name and value if any.
// Returns a true if a valid config name was found and puts that name in the BSimString *
// if it is not NULL.  This is intended to allow putting the extra processing on config
// var files out to the 'read type' methods. That will be were persistentence is set.
//==============================================================================
bool BConfig::process(BSimString &command, BSimString *pConfigName)
{
   // ignore "comment" lines
   if (strncmp(command.getPtr(),"//",2) == 0)
      return(false);

   // separate out the name from the data
   const char *name;
   const char *remainder;
   const char *data;
   char buffer[BConfigData::cLineLength];
   const char separators[] = " \n\r=";
   const char numbers[] = "0123456789+-";
   int len;

   // Get length.
   len = command.length();

   // strip off the trailing newline, if there is one
   if (len > 0 && command[len-1] == B('\n'))
      command.remove(len-1, 1);                 // remove it.
/*
   if (len>0 && command.right() == '\n')
      command.trimRight();

   if (len>0 && command.right() == '\r')
      command.trimRight();
*/
   len = command.length();
   if (len > 0 && command[len-1] == B('\r'))
      command.remove(len-1, 1);


   // make sure there is some kind of actual content
   if (command.length() == 0)
      return(false);

   // make a copy of the passed in command string so we can do whatever we want with it
   bsnprintf(buffer,sizeof(buffer), "%s", (const char *) command.getPtr());

   // if the line starts with a - it is an undefine
   if (buffer[0] == '-')
   {
      remove(buffer + 1);
      if (pConfigName)
         pConfigName->set( (char *)(buffer+1) );
      return(true);
   }
   // similarly, if it starts with + it is a define (and should remove the +)
   if (buffer[0] == '+')
   {
      define(buffer + 1);
      if (pConfigName)
         pConfigName->set( (char *)(buffer+1));
      return(true);
   }

   // the name is the first token, always
   char *pStrTokContext = NULL;
   name = strtok_s(buffer,separators, &pStrTokContext);
   if (name == NULL)
      return(false);

   // the data part is the rest of the string
   remainder = strtok_s(NULL,separators, &pStrTokContext);
   if (remainder == NULL)
   {
      // okay, we didn't have any more tokens, so lets just define this puppy
      define(name);
      if (pConfigName)
         pConfigName->set(name);
      return(true);
   }
   // note that data isn't really just the next token, since there could be embedded spaces in string data
   int nameLen = strlen(name);
   data = command.getPtr() + nameLen + 1;

   // okay, now remainder is a string, but is representing a numeric value or a string value?

   // if it has a quotation mark up front, definitely a string.
   if (data[0] == '\"')
   {
      char temp[BConfigData::cStringLength];
      bsnprintf(temp, sizeof(temp), "%s", data + 1);

      // strip out the final "
      for (len = strlen(temp); len > 0; len--)
      {
         if (temp[len - 1] == '\"')
         {
            temp[len - 1] = '\0';
            break;
         }
      }
      set(name,temp);

   }
   // alternatively, if it starts with a number, it's either long or float
   else if (strchr(numbers,data[0]) != NULL)
   {
      // if there is a . inside, it's a float, otherwise an int
      if (strchr(data,'.') != NULL)
      {
         float val;
         val = float(atof(data));
         set(name,val);
      }
      else
      {
         long val;
         char* pEnd = const_cast<char*>(data + (command.length() - nameLen - 1));
         //val = atoi(data);
         val = strtol(data, &pEnd, 0);
         set(name,val);
      }
   }
   else
   {
      // failing that, just call it a string variable and be done with it
      set(name,data);
   }

   if (pConfigName)
      pConfigName->set(name);
   return(true);
}


//==============================================================================
// BConfig::read
//==============================================================================
long BConfig::read(long dirID, const BSimString& filename, long source, bool persistent, bool saveFilename, bool tryHardDriveFirst)
{
#ifdef XBOX  
   BSimString path;

   BSimString buf;
   
   BFixedString<1024> line;

   uchar* pFileData = NULL;
   unsigned long fileSize = 0;
   bool readHDFile = false;
   
   tryHardDriveFirst;
#ifndef LTCG
   if (tryHardDriveFirst)
   {
      BSimString qualifiedPath;
      if (cFME_SUCCESS == gFileManager.constructQualifiedPath(dirID, filename, qualifiedPath))
      {
         BSimString fullPath("d:\\");
         fullPath += qualifiedPath;
         FILE* pFile = NULL;
         if (0 == fopen_s(&pFile, fullPath, "rb"))
         {
            fseek(pFile, 0, SEEK_END);
            fileSize = ftell(pFile);
            rewind(pFile);
            
            pFileData = new uchar[Math::Max<unsigned long>(1U, fileSize)];
            
            if (fread(pFileData, 1, fileSize, pFile) != fileSize)
            {
               BFATAL_FAIL("BConfig::read: Failed reading config file");
            }
            
            readHDFile = true;
            
            fclose(pFile);
            
            gConsoleOutput.resource("BConfig::read: Loaded %s from hard drive!", qualifiedPath.getPtr());
            trace("BConfig::read: Loaded %s from hard drive!", qualifiedPath.getPtr());
         }
      }
   }
#endif   

   if (!readHDFile)
   {
      if (!BFileUtils::loadFile(dirID, filename, (void**)&pFileData, &fileSize))
         return(cErrFileNotFound);
      
      gConsoleOutput.resource("Loaded %s using file system", filename.getPtr());
   }
   
   BByteStream byteStream(pFileData, fileSize);
   
   long oldSource = mCurrentSource;
   setCurrentSource(source);

   BSimString configName;
   char temp[256];
   long slashStarCount=0;

   for ( ; ; )
   {
      if (byteStream.bytesLeft() == 0)
         break;
         
      line.readLine(byteStream);
      
      // checking this makes it stop processing the last line of the cfg file twice

      buf.set(line);

      // handle bad data in file
      long index = buf.findLeft(B('\r'));
      if (index != -1)
         buf.remove(index, (buf.length() - index));

      // Slash star comments
      // Copy so we can munge.
      bsnprintf(temp, 256, "%s", buf.getPtr());
      ltrimWhitespace(temp);
      if(temp[0]=='/' && temp[1]=='*')
      {
         slashStarCount++;
         continue;
      }
      else if(temp[0]=='*' && temp[1]=='/')
      {
         slashStarCount--;
         continue;
      }
      if(slashStarCount>0)
         continue;

      if (process( buf, &configName))
      {
         if (persistent)
            setPersistent(configName.getPtr());

         //test if saving filename and do so
         if (saveFilename)
            if (filename.length() > 0)
              setDirectory(configName.getPtr(),filename.getPtr());
      }
   }
   
#ifndef BUILD_FINAL   
   if (readHDFile)
      delete[] pFileData;
   else
#endif   
      BFileUtils::unloadFile(pFileData);

   setCurrentSource(oldSource);
#endif // XBOX
   return(cErrOK);
}

//==============================================================================
// BConfig::processString
//==============================================================================
void BConfig::processString(const BSimString &text)
{
   // TBD: we should fix all this ANSI stuff.
   // first we make a copy so that we don't screw up the original with strtok
   char buf[2048];
   bsnprintf(buf, sizeof(buf), "%s", text.getPtr());

   BSimString temp[BConfig::cMaxCommandLineTokens];
   int count = 0;

   const char separators[] = " ";
   const char *token;

   // first we strtok to break out all the unique chunks
   // then we process them all.  This is broken up like this
   // so that it is legal to use strtok (which maintains internal non
   // re-entrant state) within processToken.
   char *pStrTokContext = NULL;
   token = strtok_s(buf,separators,&pStrTokContext);
   while (token != NULL)
   {
      temp[count].set(token);
      count++;
      token = strtok_s(NULL,separators,&pStrTokContext);
   }

   int i;
   for (i = 0; i < count; i++)
   {
      processToken(temp[i]);
   }
}

//==============================================================================
// BConfig::dumpAll
//==============================================================================
void BConfig::dumpAll(OUTPUT_PROC outputFn)
{
   // look through all of our configs for a matching name
   int i;
   outputFn("Config Variables");
   outputFn("----------------");
   for (i=0; i < mConfigs.getNumber(); i++)
   {
      if (mConfigs[i].mName.length() == 0)
         continue;

      if (mConfigs[i].mFlags.isSet(BConfigData::cFlagNotSet))
         continue;

      dump(&mConfigs[i], outputFn);
   }
}

//==============================================================================
// BConfig::dumpSubstring
//==============================================================================
void BConfig::dumpSubstring(const char *search, OUTPUT_PROC outputFn)
{
   // look through all of our configs for a matching name
   int i;
   BSimString temp;

   temp.format(B("Config Variables matching \"%s\""),search);

   outputFn(temp.getPtr());
   outputFn("----------------");
   for (i=0; i < mConfigs.getNumber(); i++)
   {
      if (mConfigs[i].mName.length() == 0)
         continue;

      if (mConfigs[i].mFlags.isSet(BConfigData::cFlagNotSet))
         continue;

      if (mConfigs[i].mName.findLeft(BSimString(search)) == -1)
         continue;

      dump(&mConfigs[i], outputFn);
   }

}

//==============================================================================
// BConfig::dump
//==============================================================================
void BConfig::dump(const BConfigData *pCfg, OUTPUT_PROC outputFn)
{
   if (pCfg == NULL)
   {
      outputFn("No match.");
      return;
   }

   BSimString temp;
   switch(pCfg->mType)
   {
      case BConfigData::cDataFloat:
         temp.format(B("%s = %f"),pCfg->mName.getPtr(),pCfg->mFloatVal);
         break;
      case BConfigData::cDataLong:
         temp.format(B("%s = %d"),pCfg->mName.getPtr(),pCfg->mVal);
         break;
      case BConfigData::cDataString:
         temp.format(B("%s = %s"),pCfg->mName.getPtr(),pCfg->mText.getPtr());
         break;
      case BConfigData::cDataNone:
         temp.format(B("+%s"),pCfg->mName.getPtr());
         break;
   }
   outputFn(temp.getPtr());
}

//==============================================================================
// BConfig::setPersistent
//==============================================================================
void BConfig::setPersistent(const char *name, bool persist)
{   
   if (persist)
   {
      setFlag(name, BConfigData::cFlagPersistent);
      BConfigData *pConfig = findConfig(name);
//      if (pConfig)
      if (pConfig && (pConfig->mSource != BConfigData::cSourceProfileCfg))
         pConfig->mSource = BConfigData::cSourcePersistentCfg;
   }
   else
      unsetFlag(name, BConfigData::cFlagPersistent);
}

//==============================================================================
// BConfig::setPersistent
//==============================================================================
void BConfig::setPersistent(long id, bool persist)
{   
   if (persist)
   {
      setFlag(id, BConfigData::cFlagPersistent);
      BConfigData *pConfig = findConfig(id);
//      if (pConfig)
      if (pConfig && (pConfig->mSource != BConfigData::cSourceProfileCfg))
         pConfig->mSource = BConfigData::cSourcePersistentCfg;
   }
   else
      unsetFlag(id, BConfigData::cFlagPersistent);
}


//==============================================================================
// BConfig::setFlag
//==============================================================================
void BConfig::setFlag(const char *name, long index)
{
   BConfigData *pData;
   pData = findConfig(name);
   if (pData == NULL)
      return;

   pData->mFlags.set(index);
}

//==============================================================================
// BConfig::setFlag
//==============================================================================
void BConfig::setFlag(long id, long index)
{
   BConfigData *pData;
   pData = findConfig(id);
   if (pData == NULL)
      return;

   pData->mFlags.set(index);
}


//==============================================================================
// BConfig::setAllFlags
//==============================================================================
void BConfig::setAllFlags(const char *name, BBitVector flags)
{
   BConfigData *pData;
   pData = findConfig(name);
   if (pData == NULL)
      return;

   pData->mFlags.setAll( flags.getValue() );
}

//==============================================================================
// BConfig::setAllFlags
//==============================================================================
void BConfig::setAllFlags(long id, BBitVector flags)
{
   BConfigData *pData;
   pData = findConfig(id);
   if (pData == NULL)
      return;

   pData->mFlags = flags;
}

//==============================================================================
// BConfig::unsetFlag
//==============================================================================
void BConfig::unsetFlag(const char *name, long index)
{
   BConfigData *pData;
   pData = findConfig(name);
   if (pData == NULL)
      return;
  
   pData->mFlags.unset(index);
}

//==============================================================================
// BConfig::unsetFlag
//==============================================================================
void BConfig::unsetFlag(long id, long index)
{
   BConfigData *pData;
   pData = findConfig(id);
   if (pData == NULL)
      return;

   pData->mFlags.unset(index);
}

//==============================================================================
// BConfig::isSetFlag
//==============================================================================
bool BConfig::isSetFlag(const char *name, long index, bool &flagValue)
{
   BConfigData *pData;
   pData = findConfig(name);
   if (pData == NULL)
      return(false);   
  
   flagValue = pData->mFlags.isSet(index);
   return(true);
}

//==============================================================================
// BConfig::isSetFlag
//==============================================================================
bool BConfig::isSetFlag(long id, long index, bool &flagValue)
{
   BConfigData *pData;
   pData = findConfig(id);
   if (pData == NULL)
      return(false);

   flagValue = pData->mFlags.isSet(index);
   return(true);
}

//==============================================================================
// BConfig::setPersistentFile
//==============================================================================
void BConfig::setPersistentFile(long dirID, const BSimString& filename)
{
   BString path;
	if(gFileManager.doesFileExist(dirID, BString(filename.getPtr()), &path))
   {
      setPersistentFile(path.getPtr());
   }
}

//==============================================================================
// BConfig::createEnumTable
//==============================================================================
/*void BConfig::createEnumTable(void)
{
   long i,n;
   BConfigFormal *pFormal;
   long formalBaseIndex = 0; // where each table starts counting
   long t;

   mConfigFormals.clear();
   
   // go through all the contributory config tables (from core, engine (titan or vista), and application)
   for (t = 0; t < mConfigTables.getNumber(); t++)
   {
      BConfigFormal *formalArray = mConfigTables[t].mFormalArray;
      long numFormals = mConfigTables[t].mNumFormals;

      for (n=0; n < numFormals; n++)
      {
         pFormal = &(formalArray[n]);
         // Verify that our table is in order
         // If you hit this assert, someone put something in configenum.cpp 
         // that does not match the right order in configenum.h.
         BASSERT(pFormal->mEnum == n + formalBaseIndex);
         if (pFormal->mEnum != n + formalBaseIndex)
         {
            trace("formal enum %d (%s) does not match index %d (%d + %d)",pFormal->mEnum, pFormal->mText, n, n,formalBaseIndex);
            bool suck;
            suck = true;
         }

         // add the formal to our big array-o-tron
         mConfigFormals.add(pFormal);

         // poke in our location in the real array
         pFormal->mIndex = -1;
         for (i=0; i < mConfigs.getNumber(); i++)
         {
            if (stricmp(mConfigs[i].mName, pFormal->mText)==0)
            {
               pFormal->mIndex = i;
               break;
            }
         }
      }
      formalBaseIndex = formalBaseIndex + numFormals;
   }

   mTableInitted = true;
}
*/
//==============================================================================
// BConfig::configHelp
//==============================================================================
int __cdecl configAlphaSort(const void *a, const void *b)
{
   BConfigFormal *pA = gConfig.getFormal(*(long *)a);
   BConfigFormal *pB = gConfig.getFormal(*(long *)b);
   //long *pB = (long *)b;
   return(strcmp(pA->mText,pB->mText));
}

void BConfig::configHelp(const char *filter, bool substring, OUTPUT_PROC outputFn)
{
   long n, m, len;
   BSimString helpText;
   BConfigData *pCfgData;

   // first, make an alphabetically sorted list
   static BDynamicSimLongArray indices;
   indices.setNumber(0);
   for (n=0; n < getNumConfigEnums(); n++)
   {
      indices.add(n);
   }
   indices.sort(configAlphaSort);
   
   BSimString filterBStr(filter);
   filterBStr.toLower();

   len = strlen(filter);
   for (m=0; m < getNumConfigEnums(); m++)
   {
      // find the right alpha-sorted config
      n = indices[m];
      // filter out anything that doesn't match the prefix
      if (len > 0) 
      {
         if (substring)
         {
            BSimString temp;
            temp.set(getFormal(n)->mText);
            temp.toLower();
            if (temp.findLeft(filterBStr) == -1)
               continue;
         }
         else
         {
            if (strnicmp(filter,getFormal(n)->mText,len) != 0)
               continue;
         }
      }

      helpText.set(getFormal(n)->mText);
      pCfgData = findConfig(n);
      if (pCfgData != NULL)
      {
         if (pCfgData->mType == BConfigData::cDataNone)
         {
            helpText.append(B(" (+)"));
         }
         else
         {
            BSimString temp;
            if (getCore(pCfgData, temp))
            {
               helpText.append(B(" ("));
               helpText.append(temp);
               helpText.append(B(")"));
            }
         }
      }

#if CONFIG_HELP_TEXT      
      if (strlen(getFormal(n)->mHelpText) > 0)
      {
         helpText.append(B(" : "));
         helpText.append(BSimString(getFormal(n)->mHelpText));
      }
#endif
      
      outputFn(helpText.getPtr());
   }
}


//==============================================================================
// BConfig::getFormal
//==============================================================================
BConfigFormal *BConfig::getFormal(const char *name)
{
   for (long i=0; i < getNumConfigEnums(); i++)
   {
      BConfigFormal *pFormal = getFormal(i);
      if (stricmp(pFormal->mText, name) == 0)
         return(pFormal);
   }
   // pass back dummy formal to avoid returning NULL which would then be promptly dereferenced
   return(NULL);
}

//==============================================================================
// BConfigData::BConfigData
//==============================================================================
BConfigData::BConfigData() :
   mName(),
   mDirectory(),
   mType(cDataNone),   
   mText(),
   mVal(0),       // 27apr01 - ham - added remaining vars into ctor and erase so we stop copying bad data
   mFloatVal(0)
{
   erase();
}

//==============================================================================
// BConfigData::erase
//==============================================================================
void BConfigData::erase(void)
{
   mName.empty();
   mDirectory.empty();
   mText.empty();
   mType = cDataNone;
   mVal = 0;
   mFloatVal = 0;
   mSource = -1;
   mIndex = -1;
   mFormalIndex = -1;
}

//==============================================================================
// eof: config.cpp
//==============================================================================
