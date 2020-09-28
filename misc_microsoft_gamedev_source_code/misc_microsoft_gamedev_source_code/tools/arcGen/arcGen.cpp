//============================================================================
//
//  File: arcGen.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//  Warning: The civ names are currently hardcoded into this utility! 
//  (Search for UNSC, cov, and gaia)
//
//============================================================================
#include "xcore.h"
#include "xcorelib.h"
#include "utils\consoleAppHelper.h"
#include "consoleOutput.h"
#include "file\win32FindFiles.h"
#include "file\win32FileUtils.h"
#include "utils\commandLineParser.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "utils\endianSwitch.h"
#include "resource\ecfUtils.h"
#include "resource\ecfHeaderIDs.h"
#include "resource\resourceTag.h"
#include "file\win32FileStream.h"
#include "stream\dynamicStream.h"
#include "stream\byteStream.h"
#include "containers\hashMap.h"
#include "ecfArchiver.h"
#include "xml\xmlDocument.h"
#include "utils\spawn.h"
#include "containers\filenameTree.h"

#define TEMP_FILENAME      "___tmp___.!tmp!"

#define PROGRAM_TITLE      "ARCGEN" 
#define ARCHIVE_EXTENSION  "ERA"
#define DEPGEN_FILENAME    "tools\\depgen\\depgen.exe"
#define ECFARC_FILENAME    "tools\\ecfarc\\ecfarc.exe"
#define ARCGEN_FILENAME    "tools\\arcgen\\arcgen.exe"

class BCmdLineParams
{
public:
   BString                          mScriptFile;
   BCommandLineParser::BStringArray mDefines;
   BCommandLineParser::BStringArray mDefineFiles;
   bool                             mDisableFindFilesCache;
         
   BCmdLineParams() 
   {
   }
   
   void clear(void)
   {
      mScriptFile.empty();
      mDefines.clear();
      mDisableFindFilesCache = false;
   }

   bool parse(BCommandLineParser::BStringArray& args)
   {
      const BCLParam clParams[] =
      {
         {"scriptFile",          cCLParamTypeBStringPtr,       &mScriptFile },
         {"define",              cCLParamTypeBStringArrayPtr,  &mDefines},
         {"defineFile",          cCLParamTypeBStringArrayPtr,  &mDefineFiles},
         {"noFindFilesCaching",  cCLParamTypeFlag,             &mDisableFindFilesCache},
         { NULL } 
      };

      BCommandLineParser parser(clParams);

      const bool success = parser.parse(args, false, false);

      if (!success)
      {
         gConsoleOutput.error("%s\n", parser.getErrorString());
         return false;
      }

      if (parser.getUnparsedParams().size())
      {
         gConsoleOutput.error("Invalid parameter: %s\n", args[parser.getUnparsedParams()[0]].getPtr());
         return false;
      }

      return true;
   }

   void printHelp(void)
   {
      //      --------------------------------------------------------------------------------
      gConsoleOutput.printf("Usage: arcGen <options>\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("Options:\n");
      gConsoleOutput.printf(" -scriptFile filename       Script filename\n");
      gConsoleOutput.printf(" -define name=value         Define variable\n");
      gConsoleOutput.printf(" -defineFile filename       Read file containing defines\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("\nTo specify a text file's contents for any parameter accepting a\n");
      gConsoleOutput.printf("string, proceed the filename with a '@' character. Example:\n");
      gConsoleOutput.printf("\"-password @1.txt\" will read the password from file \"1.txt\".\n");
      
      BConsoleAppHelper::printHelp();
   }
}; // class BCmdLineParams

#if 0
class BVariableManager
{
public:
   BVariableManager()
   {
   }
   
   void clear(void)
   {
      mVariables.clear();
   }
   
   bool isDefined(const char* pName) const
   {
      BDEBUG_ASSERT(pName);
      
      return mVariables.find(pName) != mVariables.end();
   }
   
   void define(const char* pName, const char* pValue = NULL)
   {
      BDEBUG_ASSERT(pName);
                  
      undefine(pName);
      
      mVariables.insert(pName, pValue ? pValue : "");
   }
   
   bool undefine(const char* pName)
   {
      BDEBUG_ASSERT(pName);
      
      return mVariables.erase(pName);
   }
   
   const char* getValue(const char* pName) const
   {
      BDEBUG_ASSERT(pName);
      
      BVariableHashMap::const_iterator it = mVariables.find(pName);
      if (it == mVariables.end())
         return NULL;
      return it->second.getPtr();
   }
     
   typedef BHashMap<BString, BString> BVariableHashMap;
   
   const BVariableHashMap& getTable(void) const   { return mVariables; }
         BVariableHashMap& getTable(void)         { return mVariables; }
            
private:
   BVariableHashMap mVariables;
};
#endif

class BVariableManager
{
public:
   BVariableManager()
   {
   }

   void clear(void)
   {
      mVariables.clear();
   }

   bool isDefined(const char* pName) const
   {
      BDEBUG_ASSERT(pName);

      BFixedString256 name(pName);
      name.tolower();

      return mVariables.find(name.getPtr()) != mVariables.end();
   }

   void define(const char* pName, const char* pValue = NULL)
   {
      BDEBUG_ASSERT(pName);

      BFixedString256 name(pName);
      name.tolower();

      undefine(name);

      mVariables.insert(name.getPtr(), pValue ? pValue : "");
   }

   bool undefine(const char* pName)
   {
      BDEBUG_ASSERT(pName);

      BFixedString256 name(pName);
      name.tolower();

      return mVariables.erase(name.getPtr());
   }

   const char* getValue(const char* pName) const
   {
      BDEBUG_ASSERT(pName);

      BFixedString256 name(pName);
      name.tolower();

      BVariableHashMap::const_iterator it = mVariables.find(name.getPtr());
      if (it == mVariables.end())
         return NULL;
      return it->second.getPtr();
   }

   typedef BHashMap<BString, BString> BVariableHashMap;

   const BVariableHashMap& getTable(void) const   { return mVariables; }
   BVariableHashMap& getTable(void)         { return mVariables; }

private:
   BVariableHashMap mVariables;
};

class BArcGen
{
public:
   BArcGen()
   {
      clear();
   }
   
   bool process(BCommandLineParser::BStringArray& args)
   {
      if (args.getSize() < 2)
      {
         mCmdLineParams.printHelp();
         return false;
      }

      if (!mCmdLineParams.parse(args))
         return false;
      
      if (!findWorkDirectory())
      {
         gConsoleOutput.error("Unable to find work directory!\n");
         return false;
      }
      
      if (!readConfig())
         return false;
      
      if (!initVariables())
         return false;
                                         
      if (!processScript())
         return false;
                  
      return true;   
   }
   
   int getExitStatus(void) const
   {
      return mScriptExitStatus;
   }
   
private:
   BString           mWorkDirectory;
   BString           mExecDirectory;
   BCmdLineParams    mCmdLineParams;
   BVariableManager  mVariables;
   
   typedef BDynamicArray<BString> BStringArray;
   BStringArray      mDependencyExtensions;
   
   typedef BHashMap<BString>        BStringMap;
   typedef BHashMap<BString, uint>  BStringIndexMap;
   
   int mScriptExitStatus;
   bool mTerminateScript;
   
   // Filename tree of the entire work directory
   BFilenameTree mFindFilesCache;
   bool mFindFilesCacheIsDirty;
      
   void clear(void)
   {
      mCmdLineParams.clear();
      
      mWorkDirectory.empty();
      
      mTerminateScript = false;
      mScriptExitStatus = 0;
      
      mFindFilesCache.clear();
      mFindFilesCacheIsDirty = true;
   }
   
   bool readConfig(void)
   {
      BString filename(mExecDirectory);
      filename += "arcgen.xml";
      
      BXMLDocument doc;
      if (FAILED(doc.parse(filename.getPtr())))
      {
         gConsoleOutput.error("Failed reading config file: %s\n", filename.getPtr());
         return false;
      }
      
      const BXMLDocument::BNode* pRootNode = doc.getRoot();
      
      const BXMLDocument::BNode* pDependencyExtensionNode = pRootNode->findChild("dependencyExtensions");
      
      mDependencyExtensions.clear();
      if (pDependencyExtensionNode)
      {
         for (uint i = 0; i < pDependencyExtensionNode->getNumChildren(); i++)
         {
            BString ext(pDependencyExtensionNode->getChild(i)->getText());
            ext.trimLeft();
            ext.trimRight();
            ext.toLower();
            mDependencyExtensions.pushBack(ext);
         }
      }
      
      return true;
   }

   bool processParams(BCommandLineParser::BStringArray& args)
   {
      if (args.getSize() < 2)
      {
         mCmdLineParams.printHelp();
         return false;
      }

      if (!mCmdLineParams.parse(args))
         return false;

      return true;
   }
   
   bool findWorkDirectory(void)
   {
      char buf[MAX_PATH];
      if (!GetModuleFileNameA(GetModuleHandle(NULL), buf, sizeof(buf)))
         return false;
      
      BString path, name;
      strPathSplit(BString(buf), path, name);
      mExecDirectory = path;

      mWorkDirectory = buf;
      mWorkDirectory.toLower();

      int i = mWorkDirectory.findRight("\\tools\\arcgen");
      if (i == -1)
         i = mWorkDirectory.findRight("\\tools\\arcgen");
      if (i == -1)
         return false;

      mWorkDirectory.crop(0, i);

      return true;
   }

   bool initDerivedVariables(void)
   {   
      if (mVariables.isDefined("scenariofilename"))
      {
         BString scenarioFilename(mVariables.getValue("scenariofilename"));
         if (!massageFilename(scenarioFilename, true))
            return false;
         
         if (!BWin32FileUtils::doesFileExist(scenarioFilename))
         {
            gConsoleOutput.error("Scenario file does not exist: %s\n", scenarioFilename.getPtr());
            return false;
         }

         mVariables.define("scenariofilename", scenarioFilename);
                  
         BString path;
         BString name;
         strPathSplit(scenarioFilename, path, name);

         path.removeTrailingPathSeperator();
         mVariables.define("scenariopath", path);

         name.removeExtension();
         mVariables.define("scenarioname", name);
      }
      
      return true;
   }
      
   bool initFindFilesCache()
   {
      mFindFilesCache.clear();
      mFindFilesCacheIsDirty = false;

      if (mCmdLineParams.mDisableFindFilesCache)
         return true;
               
      BString workPath(mWorkDirectory);
      workPath.removeTrailingPathSeperator();

      gConsoleOutput.printf("Initializing find files cache: %s\n", workPath.getPtr());

      BFindFiles findFiles;
      if (!findFiles.scan(workPath, "*", BFindFiles::FIND_FILES_WANT_FILES | BFindFiles::FIND_FILES_RECURSE_SUBDIRS))
      {
         gConsoleOutput.error("Unable to initialize find files cache!\n");
         return false;
      }
      
      for (uint i = 0; i < findFiles.numFiles(); i++)
      {
         const BFileDesc& fileDesc = findFiles.getFile(i);

         BString filename(fileDesc.relFilename(false));
         filename.standardizePath();

         uint fileFlags = 0;
         if (fileDesc.attributes() & FILE_ATTRIBUTE_DIRECTORY)
            fileFlags |= BFilenameTree::cDirFlag;

         BFilenameTree::eErrorCode status = mFindFilesCache.add(filename, fileFlags, i, true, NULL);
         if (status != BFilenameTree::cSucceeded)
         {
            gConsoleOutput.error("Unable to initialize find files cache!\n");
            
            mFindFilesCache.clear();
            
            return false;
         }
      }

      return true;
   }
         
   static bool unquote(BString& str)
   {
      if (!str.length())
         return true;

      if (str.getChar(0) == '"') 
      {
         if (str.length() < 2)
            return false;
            
         if (str.getChar(str.length() - 1) != '"')
         {
            gConsoleOutput.error("Missing end quote: %s\n", str.getPtr());
            return false;
         }

         str.substring(1, str.length() - 1);
      }

      return true;
   }
   
   bool processDefine(const char* pDef)
   {
      BString define(pDef);
      define.trimLeft();
      define.trimRight();
      if (define.isEmpty())
         return true;

      int i = define.findLeft("=");
      
      BString name, value;
      if (i >= 0)
      {
         name = define;
         name.left(i);

         value = define;
         value.right(i + 1);
      }     
      else
      {
         name = define;
      }
      
      if ((!unquote(name)) || (!unquote(value)))
         return false;
                  
      if (!resolveMacros(name, BString(name)))
         return false;
         
      BStringArray valueArray;
      if (!resolveText(value, valueArray))
         return false;
      
      if (valueArray.getSize() > 1)
         gConsoleOutput.warning("Define \"%s\" has multiple values - only using first value!\n", name.getPtr());
            
      name.toLower();
      
      if (valueArray.isEmpty())
         mVariables.define(name);
      else
         mVariables.define(name, valueArray[0]);
         
      return true;
   }
      
   bool initVariables(void)
   {
      mVariables.clear();
      
      BString workPath(mWorkDirectory);
      workPath.removeTrailingPathSeperator();
      mVariables.define("workpath", workPath);
      
      char buf[MAX_PATH];
      if (!mVariables.isDefined("temppath"))
      {
         if (GetTempPath(MAX_PATH, buf))
         {
            BString tempPath(buf);
            tempPath.removeTrailingPathSeperator();
            mVariables.define("temppath", tempPath);
         }
      }
      
      BString depGenFilename(mWorkDirectory);
      depGenFilename += DEPGEN_FILENAME;
      mVariables.define("depgen", depGenFilename);
      
      {
         BString path, name;
         strPathSplit(depGenFilename, path, name);
         path.removeTrailingPathSeperator();
         mVariables.define("depgenpath", path);
      }
      
      BString ecfArcFilename(mWorkDirectory);
      ecfArcFilename += ECFARC_FILENAME;
      mVariables.define("ecfarc", ecfArcFilename);
      
      {
         BString path, name;
         strPathSplit(ecfArcFilename, path, name);
         path.removeTrailingPathSeperator();
         mVariables.define("ecfarcpath", path);
      }
      
      BString arcGenFilename(mWorkDirectory);
      arcGenFilename += ARCGEN_FILENAME;
      mVariables.define("arcgen", arcGenFilename);

      {
         BString path, name;
         strPathSplit(arcGenFilename, path, name);
         path.removeTrailingPathSeperator();
         mVariables.define("arcgenpath", path);
      }
                                 
      BString path(mExecDirectory);
      path.removeTrailingPathSeperator();
      mVariables.define("execpath", path);
      
      for (uint defineIndex = 0; defineIndex < mCmdLineParams.mDefines.getSize(); defineIndex++)
      {
         if (!processDefine(mCmdLineParams.mDefines[defineIndex]))
            return false;
      }
        
      for (uint i = 0; i < mCmdLineParams.mDefineFiles.getSize(); i++)
      {
         if (mCmdLineParams.mDefineFiles[i].isEmpty())
            continue;
            
         gConsoleOutput.printf("Reading define file: %s\n", mCmdLineParams.mDefineFiles[i].getPtr());
         
         BStringArray defines;
         if (!BWin32FileUtils::readStringFile(mCmdLineParams.mDefineFiles[i], defines))
         {
            gConsoleOutput.error("Failed reading define file: %s\n", mCmdLineParams.mDefineFiles[i].getPtr());
            return false;
         }
         
         for (uint j = 0; j < defines.getSize(); j++)
         {
            if (!processDefine(defines[j]))
               return false;
         }
      }
            
      if (!initDerivedVariables())
         return false;
      
      checkPath("inputpath", mVariables.getValue("workpath"));
      checkPath("outputpath", mVariables.getValue("workpath"));
                  
      gConsoleOutput.printf("Defined variables:\n");
      const BVariableManager::BVariableHashMap& variables = mVariables.getTable();
      for (BVariableManager::BVariableHashMap::const_iterator it = variables.begin(); it != variables.end(); ++it)
         gConsoleOutput.printf("  \"%s\" = \"%s\"\n", it->first.getPtr(), it->second.getPtr());
         
      return true;
   }
   
   void checkPath(const char* pName, const char* pDefValue)
   {
      BString path;
      if (!mVariables.isDefined(pName))
      {
         if (!pDefValue)
            return;
         path = pDefValue;
      }
      else
         path = mVariables.getValue(pName);
               
      massageFilename(path, true);
      path.removeTrailingPathSeperator();
      mVariables.define(pName, path);
   }      
   
   static void createFilename(BString& filename, const char* pPath, const char* pName)
   {
      filename.set(pPath);
      
      strPathAddBackSlash(filename, false);
      
      filename += pName;
   }
      
   typedef BDynamicArray<BStringArray> BFileListStack;
   BFileListStack mFileListStack;
      
   bool processScript(void)
   {
      mTerminateScript = false;
      mScriptExitStatus = 0;
      
      if (mCmdLineParams.mScriptFile.isEmpty())
      {
         gConsoleOutput.error("No script specified to process!\n");
         return false;
      }
      
      mFileListStack.clear();
      mFileListStack.enlarge(1);
      
      BString scriptFilename(mCmdLineParams.mScriptFile.getPtr());
      
      if (!massageFilename(scriptFilename, false))
         return false;
      
      return parseScript(scriptFilename, 0);
   }
   
   bool parseScript(const char* pFilename, uint includeLevel)
   {
      if (includeLevel > 20)
      {
         gConsoleOutput.error("Max recursion level exceeded!");
         return false;
      }
      
      gConsoleOutput.printf("Reading XML file: %s\n", pFilename);
      
      BByteArray xmlData;
      if (!BWin32FileUtils::readFileData(pFilename, xmlData))
      {
         gConsoleOutput.error("Failed reading file: %s\n", pFilename);
         return false;
      }
   
      BXMLDocument xmlDoc;
      if (FAILED(xmlDoc.parse((const char*)xmlData.getPtr(), xmlData.getSizeInBytes())))
      {
         gConsoleOutput.error("Failed parsing XML file: %s\n", pFilename);
         return false;
      }
      
      const BXMLDocument::BNode* pRootNode = xmlDoc.getRoot();
      
      return parseSiblings(pFilename, includeLevel, pRootNode);
   }
   
   bool resolveMacros(BString& output, const BString& input)
   {
      output = input;
      
      for ( ; ; )
      {
         int i = output.findLeft("$(");
         if (i < 0)
         {
            i = output.findLeft("!(");
            if (i < 0)
               break;
         }
            
         int j = output.findLeft(')', i + 1);
         if (j < i)
         {
            gConsoleOutput.error("Macro substitution failed while processing string: %s\n", input.getPtr());
            return false; 
         }
         
         BString name(output);
         name.substring(i + 2, j);
         name.toLower();
         
         if (!mVariables.isDefined(name))
         {  
            gConsoleOutput.error("Undefined macro \"%s\" used in string: %s\n", name.getPtr(), input.getPtr());
            return false;
         }
         
         const char* pValue = mVariables.getValue(name);
         
         BString start(output);
         start.left(i);
         
         BString end(output);            
         end.right(j + 1);
         
         BString middle(pValue ? pValue : "");
         if ((middle.findLeft("$(") != -1) || (middle.findLeft("!(") != -1))
         {
            gConsoleOutput.error("Invalid macro value: %s\n", middle.getPtr());
            return false;
         }
         
         output = start + middle + end;
      }
      
      return true;
   }
   
   bool resolveText(const BString& text, BStringArray& strings, uint recursionLevel = 0)
   {
      if (recursionLevel > 10)
      {
         gConsoleOutput.error("Max recursion level exceeded!");
         return false;
      }
                  
      BString resolvedText;
      if (!resolveMacros(resolvedText, text))
         return false;
      resolvedText.trimLeft();
      resolvedText.trimRight();
      
      if (resolvedText.isEmpty())
         return true;
         
      if (resolvedText.getChar(0) == '@')
      {
         BString filename(resolvedText);
         filename.right(1);
         
         gConsoleOutput.printf("Reading file: %s\n", filename.getPtr());
         
         BStringArray fileStrings;
         if (!BWin32FileUtils::readStringFile(filename, fileStrings))
         {
            gConsoleOutput.error("Failed reading file: %s\n", filename.getPtr());
            return false;
         }
         
         for (uint i = 0; i < fileStrings.getSize(); i++)
         {
            BString resolvedFileText;
            resolveMacros(resolvedFileText, fileStrings[i]);
            resolvedFileText.trimLeft();
            resolvedFileText.trimRight();
            
            if (resolvedFileText.isEmpty())
               continue;
               
            if (resolvedFileText.getChar(0) == '@')
            {
               if (!resolveText(resolvedFileText, strings, recursionLevel + 1))
                  return false;
            }
            else
            {
               strings.pushBack(resolvedFileText);  
            }
         }
      }
      else
      {
         strings.pushBack(resolvedText);
      }
      
      return true;
   }
   
   bool processInclude(const char* pFilename, uint includeLevel, const BXMLDocument::BNode* pNode)
   {
      const BXMLDocument::BAttribute* pAttr = pNode->findAttribute("ignoreMissing");
      const bool ignoreMissing = pAttr && (pAttr->getText() == "1");
      
      pFilename;
      const BString& text = pNode->getText();
      
      BStringArray includes;
      if (!resolveText(text, includes))
         return false;
      
      for (uint i = 0; i < includes.getSize(); i++)
      {
         if (ignoreMissing)
         {
            if (!BWin32FileUtils::doesFileExist(includes[i]))
            {
               gConsoleOutput.warning("File does not exist: %s\n", includes[i].getPtr());
               continue;
            }
         }
         
         if (!parseScript(includes[i], includeLevel + 1))
            return false; 
      }
      
      return true;
   }
   
   bool processIfFileExists(bool doesExist, const char* pFilename, uint includeLevel, const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray fileList;
      if (!resolveText(text, fileList))
         return false;
         
      for (uint i = 0; i < fileList.getSize(); i++)
      {
         if (BWin32FileUtils::doesFileExist(fileList[i]) != doesExist)
            return true;
      }        
      
      return parseSiblings(pFilename, includeLevel + 1, pNode);
   }
   
   bool processIfDirExists(bool doesExist, const char* pFilename, uint includeLevel, const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray fileList;
      if (!resolveText(text, fileList))
         return false;

      for (uint i = 0; i < fileList.getSize(); i++)
      {
         if (BWin32FileUtils::doesDirectoryExist(fileList[i]) != doesExist)
            return true;
      }        

      return parseSiblings(pFilename, includeLevel + 1, pNode);
   }
   
   bool processIfdef(bool isDefined, const char* pFilename, uint includeLevel, const BXMLDocument::BNode* pNode)
   {
      BString text(pNode->getText());
      text.trimLeft();
      text.trimRight();
      text.toLower();
      
      if (mVariables.isDefined(text) == isDefined)
      {
         return parseSiblings(pFilename, includeLevel + 1, pNode);
      }
      
      return true;      
   }
   
   bool processIfEmpty(bool isEmpty, const char* pFilename, uint includeLevel, const BXMLDocument::BNode* pNode)
   {
      BStringArray& curFileList = mFileListStack.back();
      
      const bool listIsEmpty = curFileList.isEmpty();
      
      if (listIsEmpty == isEmpty)
      {
         return parseSiblings(pFilename, includeLevel + 1, pNode);
      }

      return true;      
   }
   
   bool processDefine(const BXMLDocument::BNode* pNode)
   {
      for (uint i = 0; i < pNode->getNumAttributes(); i++)
      {
         const BXMLDocument::BAttribute& attr = pNode->getAttribute(i);
         
         BString name(attr.getName());
         name.trimLeft();
         name.trimRight();
         name.toLower();
                  
         BString text(attr.getText());
         
         BStringArray strings;
         if (!resolveText(text, strings))
            return false;
         
         if (strings.isEmpty())
            mVariables.define(name);
         else
            mVariables.define(name, strings[0]);
      }
      
      return initDerivedVariables();
   }
   
   bool processUndef(const BXMLDocument::BNode* pNode)
   {
      BString name(pNode->getText());
      name.trimLeft();
      name.trimRight();
      name.toLower();
      
      if (!name.isEmpty())
         mVariables.undefine(name);
      
      return true;
   }
   
   bool processPush(bool dup)
   {
      mFileListStack.enlarge(1);
      
      if (dup)
         mFileListStack.back() = mFileListStack[mFileListStack.getSize() - 2];
      
      return true;
   }
   
   bool processPop(void)
   {
      if (mFileListStack.getSize() == 1)
      {
         gConsoleOutput.printf("Pop would result in an empty file list stack!");
         return false;
      }
      mFileListStack.popBack();
      return true;
   }
   
   bool processClear(void)
   {
      BStringArray& curFileList = mFileListStack.back();
      curFileList.clear();
      return true;
   }
   
   bool processPrint(const BXMLDocument::BNode* pNode)
   {
      BStringArray stringArray;
      if (!resolveText(pNode->getText().getPtr(), stringArray))
         return false;
      
      for (uint i = 0; i < stringArray.getSize(); i++)
         gConsoleOutput.printf("%s\n", stringArray[i].getPtr());
               
      return true;  
   }
   
   bool processPrintList(const BXMLDocument::BNode* pNode)
   {
      if (!processPrint(pNode))
         return false;
            
      BStringArray& curFileList = mFileListStack.back();
      
      gConsoleOutput.printf("%u file(s):\n", curFileList.getSize());
      
      for (uint i = 0; i < curFileList.getSize(); i++)
         gConsoleOutput.printf("%s\n", curFileList[i].getPtr());
      
      return true;  
   }
   
   bool massageFilename(BString& filename, bool convertToAbsolute = true, bool resolveMacros = true)
   {
      filename.trimLeft();
      filename.trimRight();
                  
      BString newFilename(filename);
      if (resolveMacros)
      {
         if (!this->resolveMacros(newFilename, filename))
            return false;
      }
      
      if ((convertToAbsolute) && (newFilename.length()))
      {
         if (!strPathIsAbsolute(newFilename))
         {
            const char* pInputPath = mVariables.getValue("inputpath");
            
            if (pInputPath) 
            {
               char c = newFilename.getChar(0);
               if ((c == '/') || (c == '\\'))
                  newFilename = BString(pInputPath) + newFilename;
               else
                  newFilename = BString(pInputPath) + "\\" + newFilename;
            }
         } 
         
         CHAR fullpath[_MAX_PATH];
         LPTSTR pFilePart = NULL;

         if (GetFullPathName(newFilename.getPtr(), _MAX_PATH, fullpath, &pFilePart))
            newFilename.set(fullpath);
         else
            gConsoleOutput.warning("Couldn't resolve path: %s\n", newFilename.getPtr());
      }            
      
      newFilename.standardizePath();
      
      filename.swap(newFilename);
      
      return true;
   }
   
   bool processRead(const BXMLDocument::BNode* pNode)
   {
      const BXMLDocument::BAttribute* pAttr = pNode->findAttribute("noAbsolutePathConversion");
      const bool noAbsolutePathConversion = pAttr && (pAttr->getText() == "1");
         
      const BString& text = pNode->getText();

      BStringArray filenames;
      if (!resolveText(text, filenames))
         return false;
      
      BStringArray& curFileList = mFileListStack.back();
      
      for (uint i = 0; i < filenames.getSize(); i++)
      {
         gConsoleOutput.printf("Reading file: %s\n", filenames[i].getPtr());
         
         uint curFileListSize = curFileList.getSize();
         
         if (!BWin32FileUtils::readStringFile(filenames[i], curFileList))
         {
            gConsoleOutput.error("Failed reading file: %s\n", filenames[i].getPtr());
            return false;
         }
         
         for (uint j = curFileListSize; j < curFileList.getSize(); j++)
         {
            if (!massageFilename(curFileList[j], !noAbsolutePathConversion))
               return false;
         }        
      }
      
      return true;
   }
   
   bool processWrite(const BXMLDocument::BNode* pNode)
   {
      bool ignoreFailure = false;
      pNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
            
      const BString& text = pNode->getText();
      
      bool append = false;
      pNode->getAttributeAsBool("append", append);

      BStringArray filenames;
      if (!resolveText(text, filenames))
         return ignoreFailure;

      BStringArray& curFileList = mFileListStack.back();

      for (uint i = 0; i < filenames.getSize(); i++)
      {
         gConsoleOutput.printf("Writing file: %s\n", filenames[i].getPtr());

         FILE* pFile = NULL;
         if (fopen_s(&pFile, filenames[i], append ? "a+" : "w"))
         {
            gConsoleOutput.error("Failed creating file: %s\n", filenames[i].getPtr());
            return ignoreFailure;
         }
         
         for (uint j = 0; j < curFileList.getSize(); j++)
            fprintf(pFile, "%s\n", curFileList[j].getPtr());
         
         if (fclose(pFile) != 0)
         {
            gConsoleOutput.error("Failed writing to file: %s\n", filenames[i].getPtr());
            return ignoreFailure;
         }
      }
      
      mFindFilesCacheIsDirty = true;

      return true;
   }
   
   bool processSort(const BXMLDocument::BNode* pNode)
   {
      pNode;
      BStringArray& curFileList = mFileListStack.back();
      curFileList.sort();
      return true;
   }
   
   bool fileHasDependencies(const BString& filename)
   {
      BString ext;
      strPathGetExtension(filename, ext);
      
      ext.toLower();
      
      if (ext == ".xmb")
      {
         BString tempFilename(filename);
         tempFilename.removeExtension();
         
         strPathGetExtension(tempFilename, ext);
         
         ext.toLower();
      }
      
      if (!ext.isEmpty())
      {
         if (ext.getChar(0) == '.') 
            ext.right(1);
         ext.trimRight();
                           
         for (uint i = 0; i < mDependencyExtensions.getSize(); i++)
            if (ext == mDependencyExtensions[i])
               return true;
      }
      
      return false;
   }
   
   void appendLogFile(const char* pLogFilename, FILE* pFile, int status, const char* pArgs, bool alwaysPrintSummary)
   { 
      FILE* pSrcFile;
      if (fopen_s(&pSrcFile, pLogFilename, "r"))
      {
         gConsoleOutput.error("Unable to open log file: %s\n", pLogFilename);
         return;
      }

      gConsoleOutput.printf("Reading log file: %s\n", pLogFilename);
      
      bool printedSummary = false;         
      if (alwaysPrintSummary)
      {
         fprintf(pFile, "------------- Tool status: %i, args: %s\n", status, pArgs);
         printedSummary = true;
      }

      for ( ; ; )
      {
         char buf[8192];
         if (!fgets(buf, sizeof(buf), pSrcFile))
            break;

         if ((strcmp(buf, "\n") == 0) || (strcmp(buf, "\n\r") == 0) || (strcmp(buf, "\r\n") == 0))
            continue;
            
         if (!printedSummary)
         {
            fprintf(pFile, "------------- Tool status: %i, args: %s\n", status, pArgs);
            printedSummary = true;
         }

         fprintf(pFile, "%s", buf);
      }

      fclose(pSrcFile);
      
      if (printedSummary)
         fprintf(pFile, "-------------\n");
   }
   
   static BString quoteString(const BString& str)
   {
      return BString("\"") + str + BString("\"");
   }
   
   bool addFileDependencies(BString filename, BStringArray& fileList)
   {
      BString ext;
      strPathGetExtension(filename, ext);
      ext.toLower();
      if (ext == ".xmb")
         filename.removeExtension();
               
      BString quotedFilename("\"");
      quotedFilename += filename;
      quotedFilename += "\"";
      
      BString logFilename(BString(mVariables.getValue("temppath")) + "\\depgenlog.txt");
      BString errorLogFilename(BString(mVariables.getValue("temppath")) + "\\depgenerrorlog.txt");
      
      //intptr_t result = _spawnl(_P_WAIT, mVariables.getValue("depgen"), quotedFilename.getPtr(), quotedFilename.getPtr(), "-logfile", logFilename.getPtr(), "-errorlogfile", errorLogFilename.getPtr(), NULL);
      
      BString args(quoteString(mVariables.getValue("depgen")));
      args += " " + quotedFilename + " -logfile " + quoteString(logFilename) + " -errorlogfile " + quoteString(errorLogFilename);
      if(BConsoleAppHelper::getQuiet())
         args += " -quiet";

      gConsoleOutput.printf("Spawning depgen: %s\n", args.getPtr());
      
      BSpawnCommand spawn(args);
      int result = spawn.run();
            
      if (BConsoleAppHelper::getLogFile())
         appendLogFile(logFilename, BConsoleAppHelper::getLogFile(), result, args.getPtr(), true);
      remove(logFilename);
      
      if (BConsoleAppHelper::getErrorLogFile())
         appendLogFile(errorLogFilename, BConsoleAppHelper::getErrorLogFile(), result, args.getPtr(), false);
      remove(errorLogFilename);
      
      if (result)
      {
         gConsoleOutput.error("%s returned a non-zero status: %i\n", mVariables.getValue("depgen"), result);
         return false;
      }
      
      BString outputFilename;
      strPathGetFilename(filename, outputFilename);
      strPathRemoveExtension(outputFilename);
      outputFilename += ".txt";
      
      uint firstFileIndex = fileList.getSize();
      
      if (!BWin32FileUtils::readStringFile(outputFilename, fileList))
      {
         gConsoleOutput.error("Failed reading output file: %s\n", outputFilename.getPtr());
         return false;
      }
      
      for (uint i = firstFileIndex; i < fileList.getSize(); i++)
      {
         if (!massageFilename(fileList[i], true))
            return false;
      }
      
      remove(outputFilename);
      
      return true;
   }
         
   bool findFiles(const BString& filename, BStringArray& files, bool recursive)
   {
      if ((filename.findLeft('*') < 0) && (filename.findLeft('?') < 0))
      {
         files.pushBack(filename);
         files.back().standardizePath();
         return true;
      }
   
      gConsoleOutput.printf("Finding files: %s\n", filename.getPtr());
      
      BString path, mask;
      strPathSplit(filename, path, mask);
      path.standardizePath();
      
      bool pathIsRelativeToWorkDirectory = false;
      if ((mWorkDirectory.length()) && (path.compare(mWorkDirectory, false, mWorkDirectory.length()) == 0))
         pathIsRelativeToWorkDirectory = true;
               
      path.removeTrailingPathSeperator();
      
      if (mFindFilesCacheIsDirty)
         initFindFilesCache();  
                        
      if ( ((pathIsRelativeToWorkDirectory) || (!strPathIsAbsolute(filename))) && (mFindFilesCache.getNumFiles()) )
      {
         BFilenameTree::BFindFilesResults results;
         
         uint flags = BFilenameTree::cFindFilesWantFiles;
         if (recursive)
            flags |= BFilenameTree::cFindFilesRecurse;
            
         BString findPath(path);
         if (pathIsRelativeToWorkDirectory)
            findPath.right(mWorkDirectory.length());
                  
         BFilenameTree::eErrorCode status = mFindFilesCache.findFiles(findPath, mask, flags, results);
         if (status == BFilenameTree::cSucceeded)
         {
            files.resize(results.getSize());
            for (uint i = 0; i < results.getSize(); i++)
            {
               BString fullFilename(mWorkDirectory);
               strPathAddBackSlash(fullFilename);
               fullFilename += results[i].mFullname;
               files[i].swap(fullFilename);
            }
            
            return true;
         }
      }
         
      BFindFiles findFiles;
            
      if (!findFiles.scan(path, mask, BFindFiles::FIND_FILES_WANT_FILES | (recursive ? BFindFiles::FIND_FILES_RECURSE_SUBDIRS : 0)))
      {
         gConsoleOutput.error("Failed finding files: %s\n", filename.getPtr());
         return false;
      }
      
      for (uint i = 0; i < findFiles.numFiles(); i++)
      {
         files.pushBack(findFiles.getFile(i).fullFilename());
         files.back().standardizePath();
      }
      
      return true;
   }
   
   bool processAdd(const BXMLDocument::BNode* pNode, bool remove = false)
   {
      // <add>file.txt<add>
      // <add>@file.txt</add>
      // <add findDependencies="1">file.txt</add>
      
      const BXMLDocument::BAttribute* pAttr = pNode->findAttribute("findDependencies");
      const bool findDepends = pAttr && (pAttr->getText() == "1");
      
      pAttr = pNode->findAttribute("recurseSubdirs");
      const bool recurse = pAttr && (pAttr->getText() == "1");
      
      pAttr = pNode->findAttribute("ignoreFailure");
      const bool ignoreFailure = pAttr && (pAttr->getText() == "1");
            
      bool noAbsolutePathConversion = false;
      pNode->getAttributeAsBool("noAbsolutePathConversion", noAbsolutePathConversion);
      
      const BString& text = pNode->getText();
      
      BStringArray names;
      if (!resolveText(text, names))
         return ignoreFailure;

      BStringArray tempArray;
      BStringArray& curFileList = remove ? tempArray : mFileListStack.back();
      
      for (uint i = 0; i < names.getSize(); i++)
      {
         BString name(names[i]);
         
         if (noAbsolutePathConversion)
         {
            if (!massageFilename(name, false))
            {
               if (!ignoreFailure)
                  return false;
            }
            
            curFileList.pushBack(name);
         }
         else
         {
            if (!massageFilename(name, true))
            {
               if (!ignoreFailure)
                  return false;
               else
                  continue;
            }
               
            BStringArray files;
            if (!findFiles(name, files, recurse))
            {
               if (!ignoreFailure)
                  return false;
               else
                  continue;
            }
            
            for (uint j = 0; j < files.getSize(); j++)
            {
               const BString& filename = files[j];
               
               curFileList.pushBack(filename);
               
               if ((findDepends) && (fileHasDependencies(filename)))
               {
                  if (!addFileDependencies(filename, curFileList))
                  {
                     if (!ignoreFailure)
                        return false;
                     else
                        continue;
                  }
               }
            }            
         }            
      }
      
      if (remove)
      {
         BStringMap stringTable;
         
         for (uint i = 0; i < curFileList.getSize(); i++)
            stringTable.insert(curFileList[i]);

         BStringArray& actualFileList = mFileListStack.back();         
         
         BStringArray newFileList;
         for (uint i = 0; i < actualFileList.getSize(); i++)
         {
            BStringMap::const_iterator it = stringTable.find(actualFileList[i]);
            
            if (it == stringTable.end())
               newFileList.pushBack(actualFileList[i]);
         }
         
         actualFileList.swap(newFileList);
      }
      
      return true;
   }
   
   bool processRemove(const BXMLDocument::BNode* pNode)
   {
      return processAdd(pNode, true);
   }
   
   bool processExcludeSubstring(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray names;
      if (!resolveText(text, names))
         return false;

      BStringArray& curFileList = mFileListStack.back();         

      BStringArray newFileList;     

      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         uint j;
         for (j = 0; j < names.getSize(); j++)
            if (curFileList[i].findLeft(names[j]))
               break;

         if (j == names.getSize())
            newFileList.pushBack(curFileList[i]);
      } 

      curFileList.swap(newFileList);

      return true;
   }
   
   bool processExclude(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray names;
      if (!resolveText(text, names))
         return false;
      for (uint i = 0; i < names.getSize(); i++)
      {
         names[i].standardizePath();
         massageFilename(names[i], true, false);
      }
         
      BStringArray& curFileList = mFileListStack.back();         

      BStringArray newFileList;     
      
      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         uint j;
         for (j = 0; j < names.getSize(); j++)
            if (wildcmp(names[j], curFileList[i]))
               break;
         
         if (j == names.getSize())
            newFileList.pushBack(curFileList[i]);
      } 
        
      curFileList.swap(newFileList);
       
      return true;
   }
   
   bool processPopAndExclude(const BXMLDocument::BNode* pNode)
   {
      pNode;
      
      if (mFileListStack.getSize() < 2)
      {
         gConsoleOutput.error("popAndExclude: File list stack must contain at least 2 lists!");
         return false;
      }
      
      BStringMap stringTable;
      
      {
         BStringArray& curFileList = mFileListStack.back();         
         for (uint i = 0; i < curFileList.getSize(); i++)
            stringTable.insert(curFileList[i]);
      }
      
      mFileListStack.popBack();
            
      BStringArray& curFileList = mFileListStack.back();         

      BStringArray newFileList;     

      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         BStringMap::const_iterator it = stringTable.find(curFileList[i]);

         if (it == stringTable.end())
            newFileList.pushBack(curFileList[i]);
      } 

      curFileList.swap(newFileList);

      return true;
   }
   
   bool processPopAndJoin(const BXMLDocument::BNode* pNode)
   {
      pNode;
      
      if (mFileListStack.getSize() < 2)
      {
         gConsoleOutput.error("popAndJoin: File list stack must contain at least 2 lists!");
         return false;
      }

      BStringArray filesToJoin;
      filesToJoin.swap(mFileListStack.back());
                  
      mFileListStack.popBack();

      BStringArray& curFileList = mFileListStack.back();         
            
      for (uint i = 0; i < filesToJoin.getSize(); i++)
         curFileList.pushBack(filesToJoin[i]);
         
      return true;
   }
         
   bool processExcludeExtension(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray extensions;
      if (!resolveText(text, extensions))
         return false;

      BStringArray& curFileList = mFileListStack.back();         

      BStringArray newFileList;     

      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         const BString& filename = curFileList[i];
         uint filenameLen = filename.length();
         
         uint j;
         for (j = 0; j < extensions.getSize(); j++)
         {
            uint extensionLen = extensions[j].length();

            if (filenameLen >= extensionLen)
            {
               BString ext(filename);
               ext.right(filenameLen - extensionLen);

               if (ext == extensions[j])
                  break;
            }
         }

         if (j == extensions.getSize())
            newFileList.pushBack(curFileList[i]);
      } 

      curFileList.swap(newFileList);

      return true;
   }
   
   bool processChangeExtension(const BXMLDocument::BNode* pNode)
   {
      BString fromExt, toExt;
      pNode->getAttributeAsString("from", fromExt);
      pNode->getAttributeAsString("to", toExt);

      if ((fromExt.isEmpty()) || (toExt.isEmpty()))
      {
         gConsoleOutput.error("changeExtension command requires \"from\" and \"to\" attributes!");
         return false;
      }
      
      BStringArray& curFileList = mFileListStack.back();         

      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         BString filename;
         strPathGetFilename(curFileList[i], filename);
         
         int c = filename.findLeft('.');
         if (c < 0)
            continue;
         
         BString ext(filename);
         ext.right(c);
         
         if (ext != fromExt)
            continue;
         
         if (c)
            filename.left(c);
                     
         BString path;
         strPathGetDirectory(curFileList[i], path, true);
         path += filename;
         path += toExt;
                  
         gConsoleOutput.printf("Changing extension: \"%s\" to \"%s\"\n", curFileList[i].getPtr(), path.getPtr());
         
         curFileList[i].swap(path);
      } 

      return true;
   }
   
   bool processFilter(const BXMLDocument::BNode* pNode)
   {
      BStringArray extensions;
      BStringArray substrings;
      for (uint i = 0; i < pNode->getNumAttributes(); i++)
      {
         if (pNode->getAttribute(i).getName() == "extension")
         {
            extensions.pushBack(pNode->getAttribute(i).getText());
            extensions.back().toLower();
         }
         else if (pNode->getAttribute(i).getName() == "substring")
         {
            substrings.pushBack(pNode->getAttribute(i).getText());
         }
      }
      
      BStringArray& curFileList = mFileListStack.back();
      
      BStringArray newFileList;
      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         const BString& filename = curFileList[i];
         uint filenameLen = filename.length();
         
         bool keep = true;
         
         if (extensions.getSize())
         {
            uint j;
            for (j = 0; j < extensions.getSize(); j++)
            {
               uint extensionLen = extensions[j].length();
               
               if (filenameLen >= extensionLen)
               {
                  BString ext(filename);
                  ext.right(filenameLen - extensionLen);
                  
                  if (ext == extensions[j])
                     break;
               }
            }
            
            if (j == extensions.getSize())
               keep = false;
         }               
            
         if ((keep) && (substrings.getSize()))
         {
            uint j;
            
            for (j = 0; j < substrings.getSize(); j++)
            {
               if (filename.findLeft(substrings[j]) != -1)
                  break;
            }
            
            if (j == substrings.getSize())
               keep = false;
         }
         
         if (keep)
            newFileList.pushBack(filename);
      }
      
      curFileList.swap(newFileList);
      
      return true;  
   }
   
   bool processAddCiv(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray names;
      if (!resolveText(text, names))
         return false;
         
      uint nameIndex;
      for (nameIndex = 0; nameIndex < names.getSize(); nameIndex++)
      {
         BString outputFilename("cache\\");
         outputFilename += names[nameIndex];
         outputFilename += ".txt";
         
         if (!BWin32FileUtils::doesFileExist(outputFilename))
            break;
      }
      
      if (nameIndex < names.getSize())
      {
         gConsoleOutput.printf("Creating civ cache files\n");
         
         BString logFilename(BString(mVariables.getValue("temppath")) + "\\depgenlog.txt");
         BString errorLogFilename(BString(mVariables.getValue("temppath")) + "\\depgenerrorlog.txt");
                     
         BString args(quoteString(mVariables.getValue("depgen")));
         args += " -civs -logfile " + quoteString(logFilename) + " -errorlogfile " + quoteString(errorLogFilename);
         if(BConsoleAppHelper::getQuiet())
            args += " -quiet";

         gConsoleOutput.printf("Spawning depgen: %s\n", args.getPtr());
         
         BSpawnCommand spawn(args);
         int result = spawn.run();

         if (BConsoleAppHelper::getLogFile())
            appendLogFile(logFilename, BConsoleAppHelper::getLogFile(), result, args.getPtr(), true);
         remove(logFilename);

         if (BConsoleAppHelper::getErrorLogFile())
            appendLogFile(errorLogFilename, BConsoleAppHelper::getErrorLogFile(), result, args.getPtr(), false);
         remove(errorLogFilename);

         if (result)
         {
            gConsoleOutput.error("%s returned a non-zero status: %i\n", mVariables.getValue("depgen"), result);
            return false;
         }
         
         // FIXME - should be data driven
         BWin32FileUtils::copyFile("gaia.txt", "cache\\gaia.txt");
         BWin32FileUtils::copyFile("unsc.txt", "cache\\unsc.txt");
         BWin32FileUtils::copyFile("covenant.txt", "cache\\covenant.txt");
      }         
      else
      {
         gConsoleOutput.printf("Using civ cache files\n");
      }

      BStringArray& curFileList = mFileListStack.back();
      
      for (uint i = 0; i < names.getSize(); i++)
      {
         BString outputFilename("cache\\");
         outputFilename += names[i];
         outputFilename += ".txt";

         uint firstFileIndex = curFileList.getSize();

         if (!BWin32FileUtils::readStringFile(outputFilename, curFileList))
         {
            gConsoleOutput.error("Failed reading output file: %s\n", outputFilename.getPtr());
            return false;
         }

         for (uint i = firstFileIndex; i < curFileList.getSize(); i++)
         {
            if (!massageFilename(curFileList[i], true))
               return false;
         }
      }         
      
      // FIXME - should be data driven
      remove("gaia.txt");
      remove("unsc.txt");
      remove("covenant.txt");

      return true;
   }
   
   bool processAddScenario(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray names;
      if (!resolveText(text, names))
         return false;
      
      BStringArray& curFileList = mFileListStack.back();
      
      for (uint i = 0; i < names.getSize(); i++)
      {
         BString name(names[i]);
                           
         if (!massageFilename(name, true))
            return false;
         
         gConsoleOutput.printf("Finding scenario dependencies: %s\n", name.getPtr());
         
         if (!addFileDependencies(name, curFileList))
            return false;
      }
         
      return true;  
   }
   
   bool processRemoveDups(const BXMLDocument::BNode* pNode)
   {
      pNode;
      
      BHashMap<BString> hashTable;
      
      BStringArray& curFileList = mFileListStack.back();
                  
      BStringArray newFileList;
      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         if (hashTable.find(curFileList[i]) != hashTable.end())
            continue;
            
         hashTable.insert(curFileList[i]);
         
         newFileList.pushBack(curFileList[i]);
      }
      
      curFileList.swap(newFileList);
      
      return true;
   }
   
   bool processRemoveMissing(const BXMLDocument::BNode* pNode)
   {
      pNode;

      BStringArray& curFileList = mFileListStack.back();

      BStringArray newFileList;
      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         BString filename(curFileList[i]);
         
         massageFilename(filename, true);
         
         if (!BWin32FileUtils::doesFileExist(curFileList[i]))
         {
            gConsoleOutput.warning("Removing missing file from file list: %s\n", curFileList[i].getPtr());
            continue;
         }

         newFileList.pushBack(curFileList[i]);
      }

      curFileList.swap(newFileList);

      return true;
   }
   
   bool processRemoveExtension(const BXMLDocument::BNode* pNode)
   {
      pNode;

      BStringArray& curFileList = mFileListStack.back();

      for (uint i = 0; i < curFileList.getSize(); i++)
         curFileList[i].removeExtension();

      return true;
   }
   
   bool processRemovePathname(const BXMLDocument::BNode* pNode)
   {
      pNode;

      BStringArray& curFileList = mFileListStack.back();

      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         BString path, name;
         strPathSplit(curFileList[i], path, name);
         
         curFileList[i] = name;
      }

      return true;
   }
   
   bool processSystem(const BXMLDocument::BNode* pNode)
   {
      BString str;
      
      if (!resolveMacros(str, pNode->getText()))
         return false;
      
      gConsoleOutput.printf("Processing system: %s\n", str.getPtr());
         
      int status = system(str);
      
      if (status != 0)
      {
         gConsoleOutput.error("system() call failed: %i\n", status);
         return false;
      }
      
      return true;
   }
   
   bool processSpawn(const BXMLDocument::BNode* pNode, bool quiet = false)
   {
      BString logFilename(BString(mVariables.getValue("temppath")) + "\\spawnlog.txt");
      BString errorLogFilename(BString(mVariables.getValue("temppath")) + "\\spawnerrorlog.txt");
           
      remove(logFilename);
      remove(errorLogFilename);
      
      mVariables.define("LogFilename", logFilename);
      mVariables.define("ErrorLogFilename", errorLogFilename);
      
      BString exec;

      bool success = resolveMacros(exec, pNode->getText());
                  
      if (!success)
      {
         mVariables.undefine("LogFilename");
         mVariables.undefine("ErrorLogFilename");
         return false;
      }

      bool ignoreFailure = false;
      pNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
      
      BStringArray args;
      args.pushBack(exec);
      
      for (uint i = 0; i < pNode->getNumChildren(); i++)
      {
         BString arg(pNode->getChild(i)->getText());
         
         if (!resolveText(arg, args))
         {
            mVariables.undefine("LogFilename");
            mVariables.undefine("ErrorLogFilename");
            return false;
         }
      }
      
      mVariables.undefine("LogFilename");
      mVariables.undefine("ErrorLogFilename");
      
      for (uint i = 0; i < args.getSize(); i++)
      {
         if ((!args[i].length()) || (args[i].getChar(0) != '\"'))
            args[i] = "\"" + args[i] + "\"";
      }
      
      BDynamicArray<const char*> ptrs;

      BString allArgs;
      for (uint i = 0; i < args.getSize(); i++)
      {
         ptrs.pushBack(args[i].getPtr());
         if (i)
            allArgs += " ";
         allArgs += args[i];
      }
         
      ptrs.pushBack(NULL);
      
      if (!quiet)
      {
         gConsoleOutput.printf("Processing spawn: ");
         for (uint i = 0; i < args.getSize(); i++)
            gConsoleOutput.printf("%s ", args[i].getPtr());
         gConsoleOutput.printf("\n");
      }
      
      int status = _spawnvp(_P_WAIT, exec.getPtr(), ptrs.getPtr());
      
      if (BConsoleAppHelper::getLogFile())
         appendLogFile(logFilename, BConsoleAppHelper::getLogFile(), status, allArgs.getPtr(), true);
      remove(logFilename);

      if (BConsoleAppHelper::getErrorLogFile())
         appendLogFile(errorLogFilename, BConsoleAppHelper::getErrorLogFile(), status, allArgs.getPtr(), false);
      remove(errorLogFilename);
      
      if (!ignoreFailure)
      {
         if (status != 0)
         {
            gConsoleOutput.error("Spawned processed returned a failure status: %i\n", status);
            return false;
         }
      }
      
      return true;
   }
   
   bool processDeleteFile(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray filenames;
      if (!resolveText(text, filenames))
         return false;
         
      for (uint i = 0; i < filenames.getSize(); i++)
      {
         gConsoleOutput.printf("Deleting file: %s\n", filenames[i].getPtr());
         remove(filenames[i]);
      }
      
      return true;
   }
   
   bool processAppend(const BXMLDocument::BNode* pNode)
   {
      const BString& text = pNode->getText();

      BStringArray strings;
      if (!resolveText(text, strings))
         return false;

      if (strings.isEmpty())
         return true;
         
      BStringArray& curFileList = mFileListStack.back();
      for (uint i = 0; i < curFileList.getSize(); i++)
         curFileList[i] += strings[0];

      return true;
   }
   
   bool processAddVisDependencies(const BXMLDocument::BNode* pNode)
   {
      bool ignoreFailure = false;
      pNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
                  
      BStringArray visFileList;
      BStringArray& curFileList = mFileListStack.back();
      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         BString filename(curFileList[i]);
         if ((filename.contains(".vis.xmb")) || (filename.contains(".vis")))
            visFileList.pushBack(filename);
      }
      gConsoleOutput.printf("processAddVisDependencies: Found %u vis files\n", visFileList.getSize());
      
      for (uint i = 0; i < visFileList.getSize(); i++)
      {
         gConsoleOutput.printf("Adding dependencies for vis file: %s\n", visFileList[i].getPtr());
         
         if (!addFileDependencies(visFileList[i], curFileList))
         {
            if (!ignoreFailure)
               return false;
         }
      }

      return true;
   }
   
   bool processReorder(const BXMLDocument::BNode* pNode)
   {     
      bool ignoreFailure = false;
      pNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
      
      BStringArray files;
      if (!resolveText(pNode->getText(), files))
         return false;
      
      BStringArray names;
      
      for (uint i = 0; i < files.getSize(); i++)
      {
         if (!BWin32FileUtils::readStringFile(files[i], names))
         {
            gConsoleOutput.error("Unable to read file: %s\n", files[i].getPtr());
            if (!ignoreFailure)
               return false;
         }
      }
      
      for (uint i = 0; i < names.getSize(); i++)
         massageFilename(names[i], true);
      
      BStringIndexMap fileListHash;
      
      BStringArray fileList(mFileListStack.back());
      for (uint i = 0; i < fileList.getSize(); i++)
         fileListHash.insert(fileList[i], i);
         
      BStringMap newFileListHash;
         
      BStringArray newFileList;
      for (uint i = 0; i < names.getSize(); i++)
      {
         BStringMap::const_iterator newFileListHashIt = newFileListHash.find(names[i]);
         if (newFileListHashIt != newFileListHash.end())
            continue;
            
         BStringIndexMap::const_iterator it = fileListHash.find(names[i]);
         if (it == fileListHash.end())
            continue;
         
         newFileList.pushBack(names[i]);
         newFileListHash.insert(names[i]);
         
         fileList[it->second].empty();
      }
      
      for (uint i = 0; i < fileList.getSize(); i++)
         if (fileList[i].length())  
            newFileList.pushBack(fileList[i]);
      
      mFileListStack.back().swap(newFileList);
      
      return true;
   }
   
   bool processForEach(const BXMLDocument::BNode* pNode)
   {
      bool ignoreFailure = false;
      pNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
      
      BStringArray& curFileList = mFileListStack.back();
      
      for (uint i = 0; i < curFileList.getSize(); i++)
      {
         BString listValue0(curFileList[i]);
         BString listValue1;
         BString listValue2;
         
         if (mFileListStack.getSize() > 1)
         {
            BStringArray& curFileList1 = mFileListStack[mFileListStack.getSize() - 2];
            if (i < curFileList1.getSize())
               listValue1 = curFileList1[i];
         }
         
         if (mFileListStack.getSize() > 2)
         {
            BStringArray& curFileList2 = mFileListStack[mFileListStack.getSize() - 3];
            if (i < curFileList2.getSize())
               listValue2 = curFileList2[i];
         }
         
         mVariables.define("listvalue", listValue0);
         mVariables.define("listvalue0", listValue0);
         mVariables.define("listvalue1", listValue1);
         mVariables.define("listvalue2", listValue2);
         
         if (!processSpawn(pNode, true))
         {
            if (!ignoreFailure)
               return false;
         }
      }
      
      mVariables.undefine("listvalue");
      mVariables.undefine("listvalue0");
      mVariables.undefine("listvalue1");
      mVariables.undefine("listvalue2");

      return true;
   }

   void siftXMLAddText(const char* p, BStringArray& newFileList, const BStringIndexMap& fileListHash, const BStringArray& fileList)
   {
      if ((!p) || (!p[0]))
         return;
         
      BString text(p);
      text.trimLeft();
      text.trimRight();
      
      BString name;
      strPathGetFilename(text, name);

      // The comparison is removing extensions.  I tried commenting out these two lines, but 
      // some xml's used with SiftXML are refering to textures without their extensions, and 
      // it's to late to fix this at this point.  [SAT]
      while (strPathHasExtension(name))
         strPathRemoveExtension(name);

      if (name.isEmpty())
         return;
         
      name.toLower();   

      BStringIndexMap::const_iterator it = fileListHash.find(name);
      if (it != fileListHash.end())
         newFileList.pushBack(fileList[it->second]);
   }
   
   void siftXML(const BXMLDocument::BNode* pNode, BStringArray& newFileList, const BStringIndexMap& fileListHash, const BStringArray& fileList)
   {
      for (uint i = 0; i < pNode->getNumAttributes(); i++)
      {
         const BXMLDocument::BAttribute& attr = pNode->getAttribute(i);
                  
         siftXMLAddText(attr.getText(), newFileList, fileListHash, fileList);
      }
      
      siftXMLAddText(pNode->getText(), newFileList, fileListHash, fileList);
                  
      for (uint i = 0; i < pNode->getNumChildren(); i++)
         siftXML(pNode->getChild(i), newFileList, fileListHash, fileList);
   }
   
   bool processSiftXML(const BXMLDocument::BNode* pNode)
   {
      bool ignoreFailure = false;
      pNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
            
      BStringIndexMap fileListHash;

      BStringArray& fileList = mFileListStack.back();
      for (uint i = 0; i < fileList.getSize(); i++)
      {
         BString name;
         
         strPathGetFilename(fileList[i], name);
         
         // The comparison is removing extensions.  I tried commenting out these two lines, but 
         // some xml's used with SiftXML are refering to textures without their extensions, and 
         // it's to late to fix this at this point.  [SAT]
         while (strPathHasExtension(name))
            strPathRemoveExtension(name);

         name.toLower();
         
         fileListHash.insert(name, i);
      }
         
      BStringArray files;
      if (!resolveText(pNode->getText(), files))
         return false;
         
      BStringArray newFileList;
         
      for (uint i = 0; i < files.getSize(); i++)
      {
         BString filename(files[i]);
         if(!massageFilename(filename, true, true))
         {
            if (!ignoreFailure)
               return false;
            else
               continue;
         }

         BStringArray wildcardFiles;
         if (!findFiles(filename, wildcardFiles, false))
         {
            if (!ignoreFailure)
               return false;
            else
               continue;
         }
         
         for (uint j = 0; j < wildcardFiles.getSize(); j++)
         {
            const BString& wildcardFilename = wildcardFiles[j];  

            BByteArray fileData;
            if (!BWin32FileUtils::readFileData(wildcardFilename, fileData))
            {
               gConsoleOutput.error("Failed reading file: %s\n", wildcardFilename.getPtr());
               if (!ignoreFailure)       
                  return false;
               else
                  continue;
            }

            BXMLDocument xmlDoc;
            
            if (FAILED(xmlDoc.parse((const char*)fileData.getPtr(), fileData.getSize())))
            {  
               gConsoleOutput.error("Failed parsing file: %s\n", wildcardFilename.getPtr());
               if (!ignoreFailure)       
                  return false;
               else
                  continue;
            }            
         
            siftXML(xmlDoc.getRoot(), newFileList, fileListHash, fileList);
         }
      }   
      
      fileList.swap(newFileList);
   
      return true;
   }
                     
   bool parseSiblings(const char* pFilename, uint includeLevel, const BXMLDocument::BNode* pNode)
   {
      if (includeLevel > 20)
      {
         gConsoleOutput.error("Max recursion level exceeded!");
         return false;
      }
      
      for (uint childIndex = 0; childIndex < pNode->getNumChildren(); childIndex++)
      {
         if (mTerminateScript)
            return true;
            
         const BXMLDocument::BNode* pChildNode = pNode->getChild(childIndex);
         
         bool ignoreFailure = false;
         pChildNode->getAttributeAsBool("ignoreFailure", ignoreFailure);
         
         const BString& name = pChildNode->getName();
                  
         bool succeeded = false;
         if (name == "include")
            succeeded = processInclude(pFilename, includeLevel, pChildNode);
         else if (name == "iffileexists")
            succeeded = processIfFileExists(true, pFilename, includeLevel, pChildNode);
         else if (name == "ifnotfileexists")
            succeeded = processIfFileExists(false, pFilename, includeLevel, pChildNode);
         else if (name == "ifdirexists")
            succeeded = processIfDirExists(true, pFilename, includeLevel, pChildNode);
         else if (name == "ifnotdirexists")
            succeeded = processIfDirExists(false, pFilename, includeLevel, pChildNode);
         else if (name == "ifdef")
            succeeded = processIfdef(true, pFilename, includeLevel, pChildNode);
         else if (name == "ifndef")
            succeeded = processIfdef(false, pFilename, includeLevel, pChildNode);
         else if (name == "ifempty")
            succeeded = processIfEmpty(true, pFilename, includeLevel, pChildNode);
         else if (name == "ifnempty")
            succeeded = processIfEmpty(false, pFilename, includeLevel, pChildNode);
         else if (name == "define")
            succeeded = processDefine(pChildNode);
         else if (name == "undef")
            succeeded = processUndef(pChildNode);
         else if (name == "add")
            succeeded = processAdd(pChildNode);
         else if (name == "addScenario")
            succeeded = processAddScenario(pChildNode);
         else if (name == "addCiv")
            succeeded = processAddCiv(pChildNode);
         else if (name == "remove")
            succeeded = processRemove(pChildNode);
         else if (name == "exclude")
            succeeded = processExclude(pChildNode);
         else if (name == "excludeExtension")
            succeeded = processExcludeExtension(pChildNode);
         else if (name == "changeExtension")
            succeeded = processChangeExtension(pChildNode);            
         else if (name == "excludeSubstring")
            succeeded = processExcludeSubstring(pChildNode);
         else if (name == "filter")
            succeeded = processFilter(pChildNode);
         else if (name == "dup")
            succeeded = processPush(true);
         else if (name == "push")
            succeeded = processPush(false);
         else if (name == "pop")
            succeeded = processPop();
         else if (name == "clear")
            succeeded = processClear();
         else if (name == "read")
            succeeded = processRead(pChildNode);
         else if (name == "write")
            succeeded = processWrite(pChildNode);
         else if (name == "reorder")
            succeeded = processReorder(pChildNode);
         else if (name == "sort")
            succeeded =  processSort(pChildNode);
         else if (name == "print")
            succeeded = processPrint(pChildNode);
         else if (name == "printList")
            succeeded = processPrintList(pChildNode);
         else if (name == "removeDups")
            succeeded = processRemoveDups(pChildNode);
         else if (name == "removeMissing")
            succeeded = processRemoveMissing(pChildNode);
         else if (name == "removeExtension")
            succeeded = processRemoveExtension(pChildNode);
         else if (name == "removePathname")
            succeeded = processRemovePathname(pChildNode);
         else if (name == "system")
            succeeded = processSystem(pChildNode);
         else if (name == "spawn")
            succeeded = processSpawn(pChildNode);
         else if (name == "deleteFile")
            succeeded = processDeleteFile(pChildNode);
         else if (name == "popAndExclude")
            succeeded = processPopAndExclude(pChildNode);
         else if (name == "popAndJoin")
            succeeded = processPopAndJoin(pChildNode);
         else if (name == "foreach")
            succeeded = processForEach(pChildNode);
         else if (name == "siftxml")
            succeeded = processSiftXML(pChildNode);
         else if (name == "append")
            succeeded = processAppend(pChildNode);
         else if (name == "addVisDependencies")
            succeeded = processAddVisDependencies(pChildNode);
         else if (name == "exit")
         {
            mTerminateScript = true;
            
            if (!pChildNode->getAttributeAsInt("status", mScriptExitStatus))
               mScriptExitStatus = 0;
            
            gConsoleOutput.printf("Script exited with status %i\n", mScriptExitStatus);
            break;
         }
         else
         {
            gConsoleOutput.error("Unrecognized command in file %s: %s\n", pFilename, name.getPtr());
            succeeded = false;
            break;
         }
         
         if (!ignoreFailure)
         {
            if (!succeeded)
               return false;
         }
      }
   
      return true;
   }
};

static int mainInternal(int argC, const char* argV[])
{
   XCoreCreate();

   BConsoleAppHelper::setup();

   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argC, argV))
   {
      BConsoleAppHelper::deinit();

      XCoreRelease();
      return EXIT_FAILURE;
   }


   gConsoleOutput.printf(PROGRAM_TITLE " Compiled %s %s\n", __DATE__, __TIME__);

   BArcGen arcGen;
   const bool success = arcGen.process(args);

   BConsoleAppHelper::deinit();

   if (success)
      gConsoleOutput.printf(PROGRAM_TITLE ": Done\n");
   else
      gConsoleOutput.error(PROGRAM_TITLE ": Failed\n");
   
   XCoreRelease();
  
   if (!success)
      return EXIT_FAILURE;

   return arcGen.getExitStatus();
}

int main(int argC, const char* argV[])
{
   int status;

#ifndef BUILD_DEBUG   
   __try
#endif   
   {
      status = mainInternal(argC, argV);
   }
#ifndef BUILD_DEBUG   
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      fprintf(stderr, PROGRAM_TITLE ": Unhandled exception!");
      return 1;
   }
#endif   

   return status;
}










