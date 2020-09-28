//==============================================================================
// config.h
//
// Copyright (c) 1999 - 2001 Ensemble Studios
//==============================================================================

#ifndef _CONFIG_H_
#define _CONFIG_H_

//#include "logmanager.h"

// valid config syntax for .cfg files:
// <name>                     defines the config var "name"
// +<name>                    (same)
// -<name>                    undefines the config var "name", even if it has been previously defined
// <name> <num>               defines and sets the config var "name" to a number
// <name> <string>            defines and sets the config var "name" to a string
// <name> "<string>"          (same)

// each config operation must be separated by a newline

// so as an example:

// wireframe 1
// useNewMovement
// +useblackmap
// -usefog
// username Xemu

// valid config syntax for command line processing
// +<name>                    defines the config var "name"
// -<name>                    undefines the config var "name", even if it is set in a .cfg file
// <name>=<num>               defines and sets the config var "name" to a number
// <name>=<string>            defines and sets the config var "name" to a string
// <name>="<string>"          (same)

// on the command line, config operations are separated by spaces

// an example:

// rts3.exe +useNewMovement -moveTracking username=Xemu wireframe=1

// <string> arguments cannot begin with a number.  If you want a string that begins
// with a number, you can use the "<string>" format.  There is currently no support
// for setting a string that begins with the " character.

// Note:  One big limitation of the command line entry is that you cannot use it to
// set string values that have embedded spaces.  Sadness.

// Note that config variables are case-insensitive.  If enough people complain, I can
// easily change that.

//==============================================================================
// Includes

#include "bitvector.h"
#include "threading\eventDispatcher.h"

//==============================================================================
class BConfig;
class BConfigFormal;

//==============================================================================
// 
//==============================================================================
typedef void (*BConfigCallback)(long configEnum, bool beingDefined);

//==============================================================================
// 
//==============================================================================

#define CONFIG_HELP_TEXT 0

class BConfigFormal
{
   public:
      enum
      {
         cTextSize=62,
#if CONFIG_HELP_TEXT               
         cHelpTextSize=256
#endif         
      };

   public:
      BConfigFormal(){}
      ~BConfigFormal(){}
      BConfigFormal(const char* text, const char* helpText, long defaultFlags=0, BConfigCallback pCallback=NULL);

      long              mEnum;
      
      long              mDefaultFlags;
      // this function is called when the associated config changes
      BConfigCallback   mCallback;

      // set automatically, so should be last thing
      long              mIndex;
      
      char              mText[cTextSize];

#if CONFIG_HELP_TEXT      
      // associated help text
      char              mHelpText[cHelpTextSize];
#endif      
};

//==============================================================================
// 
//==============================================================================
// An atomic "config unit" can store either a long or a string
class BConfigData
{
   public:

      enum
      {
         cNameLength = 32,
         cStringLength = 512,
         cLineLength = 1000,
      };

      enum
      {
         cDataNone,
         cDataFloat,
         cDataLong,
         cDataString,
      };

      //used for the BitVector flags
      enum
      {
         cFlagPersistent, // tells if the config variable is consistent or not.  Defaults off
         cFlagMultiplayer, // tells if the config variable is passed around in multiplayer
         cFlagImmediate, // for multiplayer, tells if the config var is updated immediately rather than synchronously
         cFlagNotSet, // if a config exists, but has this, it should be treated as if it is not defined.
         cFlagRestricted, // only can be set from privileged source (game.cfg)
         cFlagConfigEnd
      };

      // these are in order of priority (load order)
      enum
      {
         cSourceInvalid = -1,
         cSourceUnknown = 0,
         cSourceGameCfg,
         cSourceLocaleCfg,
         cSourcePersistentCfg,
         cSourceProfileCfg,
         cSourceUserCfg,
         cSourceFinalCfg,
         cSourceCommandLine,
         cSourceCode,
      };

      // constructor
      BConfigData();

      // clear out the data
      void erase(void);

      // our name
      BSimString   mName;
      // our type
      long        mType;

      // our directory
      BSimString   mDirectory;

      // our actual value.  Only one of these should
      // ever be set, which is used is determined by type
      long        mVal;
      float       mFloatVal;
      BSimString     mText;

      // flags
      BBitVector  mFlags;

      // where was this last touched from?
      long        mSource;

      // Where are we in the mConfigs array, for reverse lookup
      long        mIndex;

      // The formal index do we match with
      long        mFormalIndex;
};

//==============================================================================

typedef void (*OUTPUT_PROC)(const char *str);
typedef BDynamicSimArray<BConfigData> BConfigArray;
//==============================================================================

class BConfigTable
{
   public:
      BConfigFormal     *mFormalArray;
      long              mNumFormals;
};


//==============================================================================
class BConfig
{
   public:

      // constants
      enum
      {
         cErrOK,
         cErrFileNotFound,
      };

      enum
      {
         cMaxCommandLineTokens = 64,
         cMaxConfigFileSize = 4096,
      };
      // Constructors
      BConfig(void);

      // Destructors
      ~BConfig(void);

      // Functions

      // ready to begin!
      void                 init(const char *cmdLine);
      
      long                 registerConfig(const char* text, const char* helpText, long defaultFlags, BConfigCallback pCallback);

      // returns whether or not that config var has been defined at all
      inline bool          isDefined(long id) const
                           {
                              BASSERT(GetCurrentThreadId() == gEventDispatcher.getThreadId(cThreadIndexSim));
                              
                              // make sure this is a valid access
                              BASSERT(id >=0);
                              BASSERT(id < getNumConfigEnums());
                              BASSERT(mTableInitted);
                              if (id < 0 || id >= getNumConfigEnums() || !mTableInitted)
                                 return(false);

                              long index = getFormal(id)->mIndex;
                              if (index == -1)
                                 return(false);

                              const BConfigData *pConfigData = &mConfigs[index];
                              if (!pConfigData)
                                 return(false);

                              return !pConfigData->mFlags.isSet(BConfigData::cFlagNotSet);
                           }

      // returns whether or not that config var has been set, and fills in 
      // pVal with an dword config value.  For text config variables, pVal
      // will be set to the char value of the first letter.
      bool                 get(long ID, long *pVal);
      bool                 getCore(const BConfigData *pCfg, long *pVal);

      // returns whether or not that config var has been set, and fills in 
      // pVal with an dword config value.  For text config variables, pVal
      // will be set to the char value of the first letter.
      bool                 get(long id, float *pVal);
      bool                 getCore(const BConfigData *pCfg, float *pVal);

      // returns whether or not a config var has been set, and fills in
      // pVal with the text config value.  For numeric config variables,
      // pVal will contain the value as a text string.
      bool                 get(long id, BSimString &string);
      bool                 getCore(const BConfigData *pCfg, BSimString &string);


      // sets a config variable and stuffs the value, setting what "kind"
      // of config var it is
      virtual void         set(long id, long val, long flags=0);
      virtual void         set(long id, float val, long flags=0);
      virtual void         set(long id, const char *val, long flags=0);

      // gets the name of a config variable from the id;
      bool                 getName(long id, BSimString *pVal);
      // gets the name of a Formal config variable from the id;
      bool                 getFormalName(long id, BSimString *pVal);


      // gets the directory of a config variable
      bool                 getDirectory(const char *name, BSimString *pVal);
      bool                 getDirectory(long id, BSimString *pVal);
      // set the directory of a config variable
      bool                 setDirectory(const char *name, const char *val);
      bool                 setDirectory(long id, const char *val);

      // causes a config var to be defined or un-defined
      virtual void         define(long id);

      // if it is defined, it is undefined, and if undefined, becomes defined
      void                 toggleDefine(const char *name);
      void                 toggleDefine(long id);

      // erases a thing from the config sys altogether (causing it to be undefined)
      // returns whether or not it actually did anything
      virtual bool         remove(long id);

      // remove all configs which have the exact same group of flags set  
      void                 removeExactByFlags(BBitVector flags);
      // remove all configs which have this flag set
      void                 removeIfFlag(long flagPos);

      // reads in a text .cfg file and sets config variables appropriately
      long                 read(long dirID, const BSimString& filename, long source, bool persistent = false, bool saveFilename = false, bool tryHardDriveFirst = false);
      
      // takes in a string (presumably the command line arguments) and processes
      // them to set config variables
      void                 processString(const BSimString &text);

      // clears all config state
      void                 clearAllState(long exceptSource = BConfigData::cSourceInvalid);
      void                 clearSource(long source);

      // given a "command" string in line-by-line format, set appropriate data
      bool                 process(BSimString &command, BSimString *pConfigName=NULL);

      // dump some or all data out to the log / console
      void                 dumpAll(OUTPUT_PROC outputFn);
      void                 dump(const BConfigData *pCfg, OUTPUT_PROC outputFn);
      void                 dumpSubstring(const char *search, OUTPUT_PROC outputFn);

      // tells the config system that this is a "persistent" config variable, and should
      // be written out to the end of user.cfg when the program terminates
      void                 setPersistent(long id, bool persist = true);   

      //set a flag on the config variable
      virtual void         setFlag(long id, long index); 
      //set all flags on a config variable
      virtual void         setAllFlags(long id, BBitVector flags); 
      //unset a flag on the config variable
      virtual void         unsetFlag(long id, long index); 

      //test if a flag is set on the config. Returns true if the config was found false if not.
      //the state value of the flag is returned throught the 'flagValue' parameter
      bool                 isSetFlag(long id, long index, bool &flagValue); 

      // set where we should save out persistent config vars to
      void                 setPersistentFile(const char *filename) { mPersistentFile.set(filename); }
      void                 setPersistentFile(long dirID, const BSimString& filename);
      BSimString              getPersistentFile(void) { return(mPersistentFile); };

      // looks up a BConfigData by name, or returns NULL if no match
      BConfigData          *findConfig(const char *name, bool findDeleted = false);
      BConfigData          *findConfig(long id, bool findDeleted = false);

      bool                 isFormal(long configIndex) const;  // checks to see whether a config is part of the formal list or not

      // send to output the names of all formal configs
      void                 configHelp(const char *filter, bool substring, OUTPUT_PROC outputFn);

      // dump all persistent configs to the various cfg files
      void                 writePersistent(void);

      long                 getConfigAmount(void) { return mConfigs.getNumber(); }
      const BConfigData    *getConfig(long index) { BASSERT(index >= 0 && index < mConfigs.getNumber()); return const_cast<const BConfigData *>(&mConfigs[index]); }

      //void                 addConfigTable(const BConfigTable &t) { mConfigTables.add(t); }

      long                 getNumConfigEnums(void) const { return mConfigFormals.getNumber(); }

      // inlined for speed
      inline BConfigFormal *getFormal(long id) const
      {
         // make sure this is a valid access
#ifdef _DEBUG
         if ((id < 0) || (id >= getNumConfigEnums()) || !mTableInitted)
         {
            BASSERT(0);
            //blogtrace("Could not getFormal on id %d!",id);
            // return SOMETHING to avoid referencing NULL
            return NULL;
         }
#endif

         return(mConfigFormals[id]);
      }

      BConfigFormal        *getFormal(const char *name);

      long                 getCurrentSource()            { return mCurrentSource;   }
      void                 setCurrentSource(long source) { mCurrentSource = source; }
      void                 setEnforceOrder(bool enforce) { mEnforceOrder = enforce; }

      //
      // 1may01 - ham
      // 
      // NOTE: the following are string processing functions
      //
      //       they should be AVOIDED unless absolutely needed.
      // 
      //       if you are unsure, please ask.  thx
      //

      virtual void         define(const char *name);

      bool                 isDefined(const char * const name)
                           {
                              BASSERT(GetCurrentThreadId() == gEventDispatcher.getThreadId(cThreadIndexSim));
                              
                              const BConfigData * const pConfigData = findConfig(name);

                              if (!pConfigData)
                                 return false;

                              return !pConfigData->mFlags.isSet(BConfigData::cFlagNotSet);
                           }

      bool                 get(const char *name, long *pVal);
      bool                 get(const char *name, BSimString &string);
      bool                 get(const char *name, float *pVal);
      virtual void         set(const char *name, long val, long flags=0);
      virtual void         set(const char *name, float val, long flags=0);
      virtual void         set(const char *name, const char *val, long flags=0);
      virtual bool         remove(const char *name);
      void                 setPersistent(const char *name, bool persist = true);   
      virtual void         setFlag(const char *name, long index); 
      virtual void         setAllFlags(const char *name, BBitVector flags); 
      virtual void         unsetFlag(const char *name, long index); 
      bool                 isSetFlag(const char *name, long index, bool &flagValue); 

      const BSimString      &getCmdLine(void) const {return(mCmdLine);}

      void                 setBaseDirectoryID(const long dirID) { mBaseDirectoryID = dirID; }
      long                 getBaseDirectoryID() const { return mBaseDirectoryID; }
      
      // Variables

   protected:

      // Functions

      // do the actual processing work on one "chunk" of a command line string
      void                    processToken(BSimString &token);

      // finds an existing config with this name, or grabs a new one
      // sets the name, does everything except the actual value
      BConfigData             *setCore(const char *name);
      BConfigData             *setCore(long id);

      // creates a new config (does not look for an existing one)
      // sets the name, does everything except the actual value
      BConfigData             *createCore(const char *name);
      BConfigData             *createCore(long id);

      // Internal set functions.
      virtual void            set(BConfigData *pConfig, long val, long flags=0, long id=-1);
      virtual void            set(BConfigData *pConfig, float val, long flags=0, long id=-1);
      virtual void            set(BConfigData *pConfig, const char *val, long flags=0, long id=-1);

      bool                    removeInternal(BConfigData *pConfig, const char *name);

      // create appropriate entries for enumerated configs
      void                    createEnumTable(void);

      // Variables

      // the actual data
      BConfigArray            mConfigs;

      BSimString               mPersistentFile; //dirctory/filename of the machine 'persistent.cfg'

      // cached off copy of our startup data
      BSimString               mCmdLine;

      // each component system (core, engine, application, etc.) can add a config table here.  When we init the true list of
      // config formals, all these contributory lists are synthesized together in the specified order.  Note that the order
      // that you add to this list must parallel the order that you use to compute your enum values.
      //BDynamicSimArray<BConfigTable>    mConfigTables;

      BDynamicSimArray<BConfigFormal *> mConfigFormals;

      // any operations that modify the state of the configs are marked with the current source
      // this lets us clear or scan by source.
      long                    mCurrentSource;

      // If this is set, you are not allowed to overwrite anything tagged with a source higher priority than the current source
      long                    mEnforceOrder;


      long                    mBaseDirectoryID;

      // have we initialized the formals table?
      bool                    mTableInitted : 1;

      bool                    mAllowCallbacks : 1;
}; // BConfig

//==============================================================================
// 
//==============================================================================

extern BConfig gConfig;

#define DEFINE_CONFIG(x) \
   long x##temp = -1; \
   const long &x = x##temp;

#if CONFIG_HELP_TEXT   
   #define DECLARE_CONFIG(x, text, helpText, flags, callback) \
      x##temp = gConfig.registerConfig(text,helpText,flags,callback);
#else
   #define DECLARE_CONFIG(x, text, helpText, flags, callback) \
      x##temp = gConfig.registerConfig(text,NULL,flags,callback);
#endif      


//==============================================================================
#endif // _CONFIG_H_

//==============================================================================
// eof: config.h
//==============================================================================

