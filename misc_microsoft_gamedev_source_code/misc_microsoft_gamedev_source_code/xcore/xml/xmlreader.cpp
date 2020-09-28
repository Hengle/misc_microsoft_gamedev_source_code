#if 0
//==============================================================================
// xmlreader.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

// Includes
#include "xcore.h"
#include "xmlreader.h"
#include "xmlwriter.h"
#include "string\converttoken.h"

// Constants
const long cDataBufferSize=1024;

//==============================================================================
// isWhiteSpace
//==============================================================================
bool isWhiteSpace(BCHAR_T c)
{
   return (c==' ' || c=='\n' || c=='\r' || c=='\t');
}

//==============================================================================
// BXMLReader::BXMLReader
//==============================================================================
BXMLReader::BXMLReader(BIXMLFile* pFile) :
   mRootNode(NULL),
   mLoadError(false),
   mFileSize(0),
   mDataBuffer(NULL),
   mBytesProcessed(0),
   mBufferSize(0),
   mBufferIndex(0),
   mUnicode(false),
   mpFile(pFile)
{
}

//==============================================================================
// BXMLReader::~BXMLReader
//==============================================================================
BXMLReader::~BXMLReader()
{
   reset();
}

//==============================================================================
// BXMLReader::reset
//==============================================================================
void BXMLReader::reset()
{
   if(mRootNode)
   {
      delete mRootNode;
      mRootNode=NULL;
   }

   if (mpFile)
   {
      mpFile->close();
      delete mpFile;
   }

   mFileSize=0;

   if(mDataBuffer)
   {
      delete mDataBuffer;
      mDataBuffer=NULL;
   }

   mBytesProcessed = 0;
   mBufferSize = 0;
   mBufferIndex = 0;

   mLoadError=false;

}

#ifdef UNICODE
//==============================================================================
// BXMLReader::loadFileSAX
//==============================================================================
bool BXMLReader::loadFileSAX(long dirID, const char *pRelativeFilename, BStringTable<BString> *conversions, bool bLoadBinary)
{
   BString fileName(pRelativeFilename);
   return loadFileSAX(dirID, fileName.getPtr(), conversions, bLoadBinary);
}
#endif

//==============================================================================
// BXMLReader::loadFileSAX
//==============================================================================
bool BXMLReader::loadFileSAX(long dirID, const BCHAR_T *pRelativeFilename, BStringTable<BString> *conversions, bool bLoadBinary)
{
   //FIXME - support conversions and bLoadBinary parameters
   bLoadBinary;
   conversions;

   reset();

   mFilename = pRelativeFilename;
   mFullFilename = pRelativeFilename;
   
   if(!mpFile->openReadOnly(dirID, mFilename))
      return false;

   mpFile->getSize(mFileSize);

   mDataBuffer=new char[cDataBufferSize];
   if(!mDataBuffer)
   {
      reset();
      return false;
   }

   mLoadError=false;
   mBytesProcessed=0;
   mBufferSize=0;
   mBufferIndex=0;
   
   //FIXME - Hack to determine if this is a unicode text file (not sure if this is valid)
   mUnicode=false;
   WORD w=0;

#ifdef XBOX
   WORD unicodeCompare = 0xfffe; // XBOX value
#else
   WORD unicodeCompare = 0xfeff; // PC value
#endif

   if(mpFile->read(&w, 2))
   {
      if(w==unicodeCompare)
         mUnicode=true;
      else
         mpFile->setOffset(0);
   }

   mRootNode=new BXMLNode(this, -1);
   if(!mRootNode)
   {
      reset();
      return false;
   }

   if(!mRootNode->load(this, false))
   {
      reset();
      return false;
   }

   mpFile->close();

   return true;
}

//==============================================================================
// BXMLReader::readNextChar
//==============================================================================
bool BXMLReader::readNextChar(BCHAR_T& c)
{
   if(mBufferIndex>=mBufferSize)
   {
      mBytesProcessed+=mBufferSize;
      if(mBytesProcessed>=mFileSize)
         return false;
      
      unsigned long ofs;
      bool success = mpFile->getOffset(ofs);
      if (!success)
         return false;
            
      DWORD bytesToRead = mFileSize - ofs;
      
      if (bytesToRead > cDataBufferSize)
         bytesToRead = cDataBufferSize;
      
      if (!bytesToRead)
         return false;
         
      success = mpFile->read(mDataBuffer, bytesToRead);
      if (!success)
         return false;
         
      DWORD bytesRead=bytesToRead;
         
      mBufferSize=bytesRead;
      mBufferIndex=0;
   }
   if(mUnicode)
   {
#ifdef XBOX
      BYTE* p=reinterpret_cast<BYTE*>(&c);
      p[0]=mDataBuffer[mBufferIndex+1];
      p[1]=mDataBuffer[mBufferIndex];
#else
      c=*(reinterpret_cast<BCHAR_T*>(mDataBuffer+mBufferIndex));
#endif
      mBufferIndex+=2;
   }
   else
   {
      c=mDataBuffer[mBufferIndex];
      mBufferIndex++;
   }
   return true;
}

//==============================================================================
// BXMLReader::writeXML
//==============================================================================
bool BXMLReader::writeXML(long dirID, const BString& filename)
{
   BXMLWriter writer;

   if(!writer.create(mpFile, dirID, filename))
      return false;

   if(mRootNode)
   {
      if(!mRootNode->write(writer))
         return false;
   }

   writer.close();

   return true;
}

//==============================================================================
// BXMLReader::save
//==============================================================================
bool BXMLReader::save(long dirID, const BCHAR_T *szFilePath)
{
   //FIXME - Need to implement binary save
   dirID;
   szFilePath;
   return true;
}

//==============================================================================
// BXMLReader::load
//==============================================================================
bool BXMLReader::load(long dirID, const BString& filePath)
{
   //FIXME - Need to implement binary load
   dirID;
   filePath;
   return true;
}

//==============================================================================
// BXMLReader::output
//==============================================================================
void BXMLReader::output(OUTPUT_PROC outputFn) const
{
   const BXMLNode *pRootNode = getRootNode();
   if (pRootNode)
      pRootNode->output(0, outputFn);
}

//==============================================================================
// BXMLReader::updateBinary
//==============================================================================
bool BXMLReader::updateBinary(long dirID, const BCHAR_T *pFileName, BStringTable<BString> *conversions)
{
   //FIXME - Need to implement
   dirID;
   pFileName;
   conversions;
   return true;
}

//==============================================================================
// BXMLNode::BXMLNode
//==============================================================================
BXMLNode::BXMLNode(BXMLReader* pReader, int lineNumber) :
   mReader(pReader),
   mParentNode(NULL),
   mName(),
   mText(),
   mAttributes(),
   mChildren(),
   mLineNumber(lineNumber)
{
}

//==============================================================================
// BXMLNode::BXMLNode
//==============================================================================
BXMLNode::BXMLNode() :
   mReader(NULL),
   mParentNode(NULL),
   mName(),
   mText(),
   mAttributes(),
   mChildren()
{
}

//==============================================================================
// BXMLNode::~BXMLNode
//==============================================================================
BXMLNode::~BXMLNode()
{
   for(long i=0; i<mAttributes.getNumber(); i++)
   {
      if(mAttributes[i])
         delete mAttributes[i];
   }
   mAttributes.clear();

   for(long i=0; i<mChildren.getNumber(); i++)
   {
      if(mChildren[i])
         delete mChildren[i];
   }
   mChildren.clear();
}

//==============================================================================
// BXMLNode::load
//==============================================================================
bool BXMLNode::load(BXMLReader* reader, BCHAR_T startChar)
{
   enum
   {
      cFindNodeName,
      cReadNodeName,
      cFindAttrName,
      cReadAttrName,
      cFindAttrValue,
      cReadAttrValue,
      cFindText,
      cReadText,
      cReadChild,
      cFindChild,
      cReadCommentStart,
      cReadCommentText,
      cReadCommentEnd,
      cReadEnd,
      cReadXMLHeader,
   };

   const long cTokenBufferSize=1024;
   BCHAR_T tokenBuffer[cTokenBufferSize+1];
   long tokenIndex=0;

   long tokenMode;
   if(startChar)
   {
      tokenMode=cReadNodeName;
      tokenBuffer[0]=startChar;
      tokenIndex=1;
   }
   else
      tokenMode=cFindNodeName;

   BString attrName;
   BString attrValue;
   BString tempStr;
   BCHAR_T quoteType=0;
   long textReplacementCount=0;

   long commentFromMode=-1; 
   int lineNumber = 1;

   for(;;)
   {
      BCHAR_T c;
      if(!reader->readNextChar(c))
         break;
         
      if (c == 0x0A)
         lineNumber++;

      switch(tokenMode)
      {
         case cFindNodeName:
            if(c=='<')
            {
               tokenIndex=0;
               tokenMode=cReadNodeName;
            }
            break;

         case cReadXMLHeader:
            if (c=='>')
            {
               //-- we are at the end of the header info set the mode to 
               //-- read the node name again
               tokenMode = cFindNodeName;
            }            
            break;

         case cReadNodeName:
            if(tokenIndex==0 && c=='!')
            {
               commentFromMode=cReadNodeName;
               tokenMode=cReadCommentStart;
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
            }
            else if(tokenIndex==0 && c=='?')
            {
               tokenMode = cReadXMLHeader;
               tokenIndex++;
            }
            else if(isWhiteSpace(c))
            {
               tokenBuffer[tokenIndex]=0;
               mName.set(tokenBuffer);
               mName.trimLeft();
               if(mName==B("!DOCTYPE"))
               {
                  tokenMode = cReadXMLHeader;
                  tokenIndex++;
               }
               else
                  tokenMode=cFindAttrName;
            }
            else if(c=='>')
            {
               tokenBuffer[tokenIndex]=0;
               mName.set(tokenBuffer);
               mName.trimLeft();
               mName.trimRight();
               tokenIndex=0;
               tokenMode=cFindText;
            }
            else if (c=='/')
            {
               // Xemu [10/29/2003] -- Special terminator for <foo/> style nodes
               tokenBuffer[tokenIndex]=0;
               mName.set(tokenBuffer);
               mName.trimLeft();
               mName.trimRight();
               tokenMode=cReadEnd;
            }
            else
            {
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
               if(tokenIndex==cTokenBufferSize-1)
               {
                  reader->setLoadError();
                  return false;
               }
            }
            break;

         case cFindAttrName:
            if(isWhiteSpace(c))
               ;
            else if(c=='/')
            {
               tokenIndex=0;
               tokenMode=cReadEnd;
            }
            else if(c=='>')
            {
               tokenIndex=0;
               tokenMode=cFindText;
            }
            else
            {
               tokenBuffer[0]=c;
               tokenIndex=1;
               tokenMode=cReadAttrName;
            }
            break;

         case cReadAttrName:
            if(isWhiteSpace(c) || c=='=')
            {
               tokenBuffer[tokenIndex]=0;
               attrName.set(tokenBuffer);
               tokenMode=cFindAttrValue;
            }
            else
            {
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
               if(tokenIndex==cTokenBufferSize-1)
               {
                  reader->setLoadError();
                  return false;
               }
            }
            break;

         case cFindAttrValue:
            if(c=='\"' || c == '\'')
            {
               quoteType=c;
               tokenIndex=0;
               tokenMode=cReadAttrValue;
            }
            break;

         case cReadAttrValue:
            if(c==quoteType)
            {
               tokenBuffer[tokenIndex]=0;
               attrValue.set(tokenBuffer);
               addAttribute(attrName.asANSI(), attrValue.asANSI());
               tokenMode=cFindAttrName;
            }
            else
            {
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
               if(tokenIndex==cTokenBufferSize-1)
               {
                  reader->setLoadError();
                  return false;
               }
            }
            break;

         case cFindText:
            if(c=='<')
            {
               tokenIndex=0;
               tokenMode=cReadChild;
            }
            else if(isWhiteSpace(c))
               ;
            else
            {
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
               if(tokenIndex==cTokenBufferSize-1)
               {
                  reader->setLoadError();
                  return false;
               }
               tokenMode=cReadText;
               textReplacementCount=(c=='&' ? 1 : 0);
            }
            break;

         case cReadText:
            // Handle text replacement (such as converting "&lt;" to "<" or "&gt;" to ">")
            if(textReplacementCount>0)
            {
               if(c==';')
               {
                  if(textReplacementCount==3)
                  {
                     char replacementChar=0;
                     if(tokenBuffer[tokenIndex-2]=='l' && tokenBuffer[tokenIndex-1]=='t')
                        replacementChar='<';
                     else if(tokenBuffer[tokenIndex-2]=='g' && tokenBuffer[tokenIndex-1]=='t')
                        replacementChar='>';
                     if(replacementChar!=0)
                     {
                        tokenIndex-=textReplacementCount;
                        tokenBuffer[tokenIndex]=replacementChar;
                        tokenIndex++;
                        textReplacementCount=0;
                        continue;
                     }
                  }
                  textReplacementCount=0;
               }
               else if(isWhiteSpace(c))
                  textReplacementCount=0;
               else
               {
                  textReplacementCount++;
                  if(textReplacementCount>3)
                     textReplacementCount=0;
               }
            }

            if(c=='<')
            {
               if(tokenIndex>0)
               {
                  tokenBuffer[tokenIndex]=0;
                  mText.set(tokenBuffer);
               }
               tokenIndex=0;
               tokenMode=cReadChild;
            }
            else if(c=='\n' || c=='\r')
            {
               if(tokenIndex>0)
               {
                  tokenBuffer[tokenIndex]=0;
                  mText.set(tokenBuffer);
               }
               tokenIndex=0;
               tokenMode=cFindChild;
            }
            else
            {
               if(textReplacementCount==0 && c=='&')
                  textReplacementCount=1;

               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
               if(tokenIndex==cTokenBufferSize-1)
               {
                  reader->setLoadError();
                  return false;
               }
            }
            break;

         case cReadChild:
            if(tokenIndex==0 && c=='!')
            {
               commentFromMode=cReadChild;
               tokenMode=cReadCommentStart;
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
            }
            else if(c=='/')
            {
               tokenIndex=0;
               tokenMode=cReadEnd;
            }
            else if(isWhiteSpace(c))
               ;
            else
            {
               BXMLNode* child=new BXMLNode(reader, lineNumber);
               if(!child)
                  return false;
               child->setParent(this);
               if(!child->load(reader, c))
               {
                  delete child;
                  return false;
               }
               if(mChildren.add(child)==-1)
               {
                  delete child;
                  return false;
               }
               tokenMode=cFindChild;
            }
            break;

         case cFindChild:
            if(c=='<')
            {
               tokenIndex=0;
               tokenMode=cReadChild;
            }
            break;

         case cReadCommentStart:
            // Comment syntax: <!--comment-->
            tokenBuffer[tokenIndex]=c;
            tokenIndex++;
            if(tokenIndex==cTokenBufferSize-1)
            {
               reader->setLoadError();
               return false;
            }

            if((tokenIndex==2 && c!='-') ||
               (tokenIndex==3 && c!='-'))
            {
               tokenMode=commentFromMode;
            }
            else if(tokenIndex==3)
               tokenMode=cReadCommentText;
            break;

         case cReadCommentText:
            if(c=='-')
            {
               tokenMode=cReadCommentEnd;
               tokenIndex=1;
            }
            break;

         case cReadCommentEnd:
            if((tokenIndex==0 && c!='-') ||
               (tokenIndex==1 && c!='-') ||
               (tokenIndex==2 && c!='>'))
            {
               tokenMode=cReadCommentText;
            }
            else if(tokenIndex==2)
            {
               if(commentFromMode==cReadNodeName)
                  tokenMode=cFindNodeName;
               else if(commentFromMode==cReadChild)
                  tokenMode=cFindChild;
               else
               {
                  BASSERT(0);
                  reader->setLoadError();
                  return false;
               }
            }
            else
               tokenIndex++;
            break;

         case cReadEnd:
            if(c=='>')
            {
               tokenBuffer[tokenIndex]=0;
               tempStr.set(tokenBuffer);
               tempStr.trimRight();
               tempStr.trimLeft();
               if(tokenIndex==0 || tempStr==mName)
                  return true;
               else
               {
                  reader->setLoadError();
                  return false;
               }
            }
            else
            {
               tokenBuffer[tokenIndex]=c;
               tokenIndex++;
               if(tokenIndex==cTokenBufferSize-1)
               {
                  reader->setLoadError();
                  return false;
               }
            }
            break;
      }
   }

   return (!mName.isEmpty() && !reader->getLoadError());
}

//==============================================================================
// BXMLNode::getTextAsString
//==============================================================================
bool BXMLNode::getTextAsString(BString &out) const
{
   out=getText();
   return(true);
}

//==============================================================================
// BXMLNode::getTextAsFloat
//==============================================================================
bool BXMLNode::getTextAsFloat(float &out) const
{
   out=getText().asFloat();
   return(true);
}

//==============================================================================
// BXMLNode::getTextAsAngle
//==============================================================================
bool BXMLNode::getTextAsAngle(float &out) const
{
   out=getText().asFloat()*cRadiansPerDegree;
   return(true);
}

//==============================================================================
// BXMLNode::getTextAsBool
//==============================================================================
bool BXMLNode::getTextAsBool(bool &out) const
{
   return(convertTokenToBool(getText(), out));
}

//==============================================================================
// BXMLNode::getTextAsLong
//==============================================================================
bool BXMLNode::getTextAsLong(long &out) const
{
   out=getText().asLong();
   return(true);
}

//==============================================================================
// BXMLNode::getTextAsDWORD
//==============================================================================
bool BXMLNode::getTextAsDWORD(DWORD &out) const
{
   out=(DWORD)getText().asLong();
   return(true);
}

//==============================================================================
// BXMLNode::getTextAsHexDWORD
//==============================================================================
bool BXMLNode::getTextAsHexDWORD(DWORD &out) const
{
   if(convertHexTokenToDWORD(getText().asANSI(), out))
      return(true);
   out=0;
   return(true);
}

//==============================================================================
// BXMLNode::getTextAsVector
//==============================================================================
bool BXMLNode::getTextAsVector(BVector &out) const
{
   getText().convertToVector3(reinterpret_cast<float*>(&out));
   return(true);
}

//==============================================================================
// BXMLNode::getAttributeName
//==============================================================================
bool BXMLNode::getAttributeName(long index, const BString** pString) const
{
   long num = mAttributes.getNumber();
   if ((index < 0) || (index >= num))
      return (false);

   if (mAttributes[index])
   {
      if (pString)
         *pString = &(mAttributes[index]->getName());
      return (true);
   }

   return (false);
}

//==============================================================================
// BXMLNode::getAttribute
//==============================================================================
const BXMLAttribute* BXMLNode::getAttribute(long index) const
{
   long num = mAttributes.getNumber();
   if ((index < 0) || (index >= num))
      return (NULL);

   if (mAttributes[index])
      return mAttributes[index];

   return (NULL);
}

#ifdef UNICODE
//==============================================================================
// BXMLNode::getAttribute
//==============================================================================
const BXMLAttribute* BXMLNode::getAttribute(const char *name) const
{
   BString strName(name);
   BXMLAttribute *pVal = NULL;
   long num = mAttributes.getNumber();
   for (long i = 0; i < num; i++)
   {
      pVal = mAttributes[i];
      if (pVal)
      {
         if(pVal->getName()==strName)
            return pVal;
      }
   }
   return(NULL);
}

//==============================================================================
// BXMLNode::getAttribute
//==============================================================================
bool BXMLNode::getAttribute(const char *name, const BXMLAttribute **pAttrib) const
{
   BString strName(name);
   BXMLAttribute *pVal = NULL;
   long num = mAttributes.getNumber();
   for (long i = 0; i < num; i++)
   {
      pVal = mAttributes[i];
      if (pVal)
      {
         if (pVal->getName()==strName)
         {
            if (!pAttrib)
               return (true);

            *pAttrib = pVal;
            return (true);
         }
      }
   }
   
   return(false);
}

//==============================================================================
// BXMLNode::getAttribValue
//==============================================================================
const BString* BXMLNode::getAttribValue(const char *name) const
{
   const BXMLAttribute *pAttrib=getAttribute(name);
   if(pAttrib)
      return &pAttrib->getValue();
   return(NULL);
}

//==============================================================================
// BXMLNode::getAttribValue
//==============================================================================
bool BXMLNode::getAttribValue(const char *name, const BString **pString) const
{
   const BXMLAttribute *pAttrib;
   if (getAttribute(name, &pAttrib))
   {
      if (pString != NULL)
      {
         *pString = &pAttrib->getValue();
         return(true);
      }   
   }
   return(false);
}

//==============================================================================
// BXMLNode::getAttribValueAsBool
//==============================================================================
bool BXMLNode::getAttribValueAsBool(const char *name, bool &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   if(str->compare(B("true"))==0)
   {
      out=true;
      return true;
   }

   if(str->compare(B("false"))==0)
   {
      out=false;
      return true;
   }

   // Convert to long.
   long val=strGetAsLong(*str, str->length());
   out=(val==1);
   return true;
}


//==============================================================================
// BXMLNode::getAttribValueAsLong
//==============================================================================
bool BXMLNode::getAttribValueAsLong(const char *name, long &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to long.
   out=strGetAsLong(*str, str->length());
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsString
//==============================================================================
bool BXMLNode::getAttribValueAsString(const char *name, BString &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   out=*str;
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsFloat
//==============================================================================
bool BXMLNode::getAttribValueAsFloat(const char *name, float &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to float.
   out=strGetAsFloat(*str, str->length());
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsAngle
//==============================================================================
bool BXMLNode::getAttribValueAsAngle(const char *name, float &out) const
{
   if(!getAttribValueAsFloat(name, out))
      return false;
   out*=cRadiansPerDegree;
   return true;
}

//==============================================================================
// BXMLNode::getAttribValueAsDWORD
//==============================================================================
bool BXMLNode::getAttribValueAsDWORD(const char *name, DWORD &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to long.
   long longVal=strGetAsLong(*str, str->length());
   out=(DWORD)longVal;
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsVector
//==============================================================================
bool BXMLNode::getAttribValueAsVector(const char *name, BVector &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to vector.
   bool ok=str->convertToVector3(reinterpret_cast<float*>(&out));
   return(ok);
}
#endif

//==============================================================================
// BXMLNode::getAttribute
//==============================================================================
const BXMLAttribute* BXMLNode::getAttribute(const BCHAR_T* name) const
{
   BXMLAttribute *pVal = NULL;
   long num = mAttributes.getNumber();
   for (long i = 0; i < num; i++)
   {
      pVal = mAttributes[i];
      if (pVal)
      {
         if(pVal->getName()==name)
            return pVal;
      }
   }
   return(NULL);
}

//==============================================================================
// BXMLNode::getAttribute
//==============================================================================
bool BXMLNode::getAttribute(const BCHAR_T* name, const BXMLAttribute **pAttrib) const
{
   BXMLAttribute *pVal = NULL;
   long num = mAttributes.getNumber();
   for (long i = 0; i < num; i++)
   {
      pVal = mAttributes[i];
      if (pVal)
      {
         if (pVal->getName()==name)
         {
            if (!pAttrib)
               return (true);

            *pAttrib = pVal;
            return (true);
         }
      }
   }

   return(false);
}

//==============================================================================
// BXMLNode::getAttribValue
//==============================================================================
const BString* BXMLNode::getAttribValue(const BCHAR_T* name) const
{
   const BXMLAttribute *pAttrib=getAttribute(name);
   if(pAttrib)
      return &pAttrib->getValue();
   return(NULL);
}

//==============================================================================
// BXMLNode::getAttribValue
//==============================================================================
bool BXMLNode::getAttribValue(const BCHAR_T* name, const BString **pString) const
{
   const BXMLAttribute *pAttrib;
   if (getAttribute(name, &pAttrib))
   {
      if (pString != NULL)
      {
         *pString = &pAttrib->getValue();
         return(true);
      }   
   }
   return(false);
}

//==============================================================================
// BXMLNode::getAttribValueAsBool
//==============================================================================
bool BXMLNode::getAttribValueAsBool(const BCHAR_T* name, bool &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   if(str->compare(B("true"))==0)
   {
      out=true;
      return true;
   }

   if(str->compare(B("false"))==0)
   {
      out=false;
      return true;
   }

   // Convert to long.
   long val=strGetAsLong<BString::charType>(*str, str->length());
   out=(val==1);
   return true;
}


//==============================================================================
// BXMLNode::getAttribValueAsLong
//==============================================================================
bool BXMLNode::getAttribValueAsLong(const BCHAR_T* name, long &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to long.
   out=strGetAsLong<BString::charType>(*str, str->length());
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsString
//==============================================================================
bool BXMLNode::getAttribValueAsString(const BCHAR_T* name, BString &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   out=*str;
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsFloat
//==============================================================================
bool BXMLNode::getAttribValueAsFloat(const BCHAR_T* name, float &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to float.
   out=strGetAsFloat<BString::charType>(*str, str->length());
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsAngle
//==============================================================================
bool BXMLNode::getAttribValueAsAngle(const BCHAR_T* name, float &out) const
{
   if(!getAttribValueAsFloat(name, out))
      return false;
   out*=cRadiansPerDegree;
   return true;
}

//==============================================================================
// BXMLNode::getAttribValueAsDWORD
//==============================================================================
bool BXMLNode::getAttribValueAsDWORD(const BCHAR_T* name, DWORD &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to long.
   long longVal=strGetAsLong<BString::charType>(*str, str->length());
   out=(DWORD)longVal;
   return(true);
}

//==============================================================================
// BXMLNode::getAttribValueAsVector
//==============================================================================
bool BXMLNode::getAttribValueAsVector(const BCHAR_T* name, BVector &out) const
{
   // Look up attrib string.
   const BString *str=getAttribValue(name);
   if(!str || str->isEmpty())
      return(false);

   // Convert to vector.
   bool ok=str->convertToVector3(reinterpret_cast<float*>(&out));
   return(ok);
}

//==============================================================================
// BXMLNode::getChild
//==============================================================================
BXMLNode *BXMLNode::getChild(long index) const
{
   if (index <= mChildren.getNumber())
      return(mChildren[index]);
   else
      return(NULL);
}

#ifdef UNICODE
//==============================================================================
// BXMLNode::getChildNode
//==============================================================================
BXMLNode* BXMLNode::getChildNode(const char* name) const
{
   BString strName(name);
   return getChildNode(strName.getPtr());
}
#endif

//==============================================================================
// BXMLNode::getChildNode
//==============================================================================
BXMLNode* BXMLNode::getChildNode(const BCHAR_T* name) const
{
   long n;
   long len;
   len = mChildren.getNumber();
   for (n=0; n < len; n++)
   {
      BXMLNode *node = mChildren[n];
      if (node == NULL)
         continue;
      if (node->getName()==name)
         return node;
   }
   return(NULL);
}

#ifdef UNICODE
//==============================================================================
// BXMLNode::getChild
//==============================================================================
bool BXMLNode::getChild(const char* name, const BXMLNode **pNode) const
{
   BString strName(name);
   return getChild(strName.getPtr(), pNode);
}
#endif

//==============================================================================
// BXMLNode::getChild
//==============================================================================
bool BXMLNode::getChild(const BCHAR_T* name, const BXMLNode **pNode) const
{
   long n;
   long len;
   len = mChildren.getNumber();
   for (n=0; n < len; n++)
   {
      BXMLNode *node = mChildren[n];
      if (node == NULL)
         continue;
      if (node->getName()==name)
      {
         if (pNode != NULL)
            *pNode = node;
         return(true);
      }
   }
   return(NULL);
}

//==============================================================================
// BXMLNode::getChildValue
//==============================================================================
bool BXMLNode::getChildValue(const char *name, DWORD& value) const
{
   const BXMLNode* pChild = getChildNode(name);
   if (pChild)
      return pChild->getTextAsDWORD(value);

   return false;
}

//==============================================================================
// BXMLNode::getChildValue
//==============================================================================
bool BXMLNode::getChildValue(const char *name, bool& value) const
{
   const BXMLNode* pChild = getChildNode(name);
   if (pChild)
      return pChild->getTextAsBool(value);

   return false;
}

//==============================================================================
// BXMLNode::getChildValue
//==============================================================================
bool BXMLNode::getChildValue(const char *name, long& value) const
{
   const BXMLNode* pChild = getChildNode(name);
   if (pChild)
      return pChild->getTextAsLong(value);

   return false;
}

//==============================================================================
// BXMLNode::getChildValue
//==============================================================================
bool BXMLNode::getChildValue(const char *name, float& value) const
{
   const BXMLNode* pChild = getChildNode(name);
   if (pChild)
      return pChild->getTextAsFloat(value);

   return false;
}

//==============================================================================
// BXMLNode::getChildValue
//==============================================================================
bool BXMLNode::getChildValue(const char *name, const BString **pString) const
{
   const BXMLNode *pChild = getChildNode(name);
   if (pChild)
   {
      if (pString != NULL)
      {
         *pString = &pChild->getText();
         return(true);
      }

   }
   return(false);
}

//==============================================================================
// BXMLNode::setText
//==============================================================================
bool BXMLNode::setText(const BCHAR_T* szText, long textLen)
{
   if (!szText)
      return (false);

   if (mText.isEmpty())
      mText.set(szText, textLen);

   return (true);
}

//==============================================================================
// BXMLNode::appendText
//==============================================================================
bool BXMLNode::appendText(const BCHAR_T *szText, long textLen)
{
   if (!szText)
      return (false);

   BString temp;
   temp.set(szText, textLen);

   mText.append(temp);

   return (true);
}

//==============================================================================
// BXMLNode::addAttribute
//==============================================================================
BXMLAttribute *BXMLNode::addAttribute(long id, const BCHAR_T *szValue)
{
   id;
   szValue;
   //FIXME - From Rocket XML reader, but not supported here
   BASSERT(0);
   return NULL;
}

#ifdef UNICODE
//==============================================================================
// BXMLNode::addAttribute
//==============================================================================
BXMLAttribute* BXMLNode::addAttribute(const char *szAttributeName, const char *szValue)
{
   BXMLAttribute *pAttribute = new BXMLAttribute();
   if (!pAttribute)
      return NULL;

   pAttribute->setName(szAttributeName);
   pAttribute->setValue(szValue);
   long index=mAttributes.add(pAttribute);
   if(index==-1)
   {
      delete pAttribute;
      return NULL;
   }

   return pAttribute;
}
#endif

//==============================================================================
// BXMLNode::addAttribute
//==============================================================================
BXMLAttribute* BXMLNode::addAttribute(const BCHAR_T* szAttributeName, const BCHAR_T* szValue)
{
   BXMLAttribute *pAttribute = new BXMLAttribute();
   if (!pAttribute)
      return NULL;

   pAttribute->setName(szAttributeName);
   pAttribute->setValue(szValue);
   long index=mAttributes.add(pAttribute);
   if(index==-1)
   {
      delete pAttribute;
      return NULL;
   }

   return pAttribute;
}

//==============================================================================
// BXMLNode::addAttribute
//==============================================================================
BXMLAttribute* BXMLNode::addAttribute(const BCHAR_T *szAttributeName, long dwValue)
{
   BString szValue;
   szValue.format(B("%d"), dwValue);
   return addAttribute(szAttributeName, szValue);
}

//==============================================================================
// BXMLNode::addAttribute
//==============================================================================
BXMLAttribute* BXMLNode::addAttribute(const BCHAR_T *szAttributeName, float fValue)
{
   BString szValue;
   szValue.format(B("%0.4f"), fValue);
   return addAttribute(szAttributeName, szValue);
}

//==============================================================================
// BXMLNode::addChild
//==============================================================================
bool BXMLNode::addChild(BXMLNode *pNode)
{
   if (!pNode)
      return (false);

   long ret = mChildren.add(pNode);

   if (ret < 0)
      return (false);

   pNode->setParent(this);

   return (true);
}

//==============================================================================
// BXMLNode::addChild
//==============================================================================
BXMLNode *BXMLNode::addChild(BXMLDocument *pDocument, const BCHAR_T *szChildName)
{
   if (!pDocument)
      return (NULL);

   if (!szChildName)
      return (NULL);

   return pDocument->createNode(szChildName, this);
}

//==============================================================================
// BXMLNode::removeChild
//==============================================================================
bool BXMLNode::removeChild(long index)
{
   bool bSuccess = false;

   if (index>=0 && index < mChildren.getNumber())
   {
      BXMLNode* pChild = mChildren[index];

      // Delete the child
      if (pChild != NULL)
         releaseInstance(pChild);

      // Remove it from the list
      bSuccess = mChildren.removeIndex(index);
   }

   return bSuccess;
}

//==============================================================================
// BXMLNode::removeChild
//==============================================================================
bool BXMLNode::removeChild(const BXMLNode *pChild)
{
   bool bSuccess = false;
   long n;
   for (n = 0; n < mChildren.getNumber(); n++)
      if (pChild == mChildren[n])
         break;
   if (n == mChildren.getNumber())
      return bSuccess;
   return removeChild(n);
}

//==============================================================================
// BXMLNode::write
//==============================================================================
bool BXMLNode::write(BXMLWriter& writer)
{
   writer.startItem(mName.asANSI(), mText.asANSI());

   for(long i=0; i<mAttributes.getNumber(); i++)
   {
      const BXMLAttribute* attr=mAttributes[i];
      writer.addAttribute(attr->getName().asANSI(), attr->getValue().asANSI());
   }

   for(long i=0; i<mChildren.getNumber(); i++)
   {
      BXMLNode* node=mChildren[i];
      if(!node->write(writer))
         return false;
   }

   writer.endItem();

   return true;
}

//==============================================================================
// BXMLNode::getInstance
//==============================================================================
BXMLNode* BXMLNode::getInstance()
{
   return new BXMLNode;
}

//==============================================================================
// BXMLNode::releaseInstance
//==============================================================================
void BXMLNode::releaseInstance(BXMLNode* node)
{
   if(node)
      delete node;
}

//==============================================================================
// BXMLNode::output
//==============================================================================
void BXMLNode::output(long spacing, OUTPUT_PROC outputFn) const
{
   if(!outputFn)
      return;

   BString temp;
   BString spaces;
   long n;
   BString attrs;


   long num = mAttributes.getNumber();
   for (long i = 0; i < num; i++)
   {
      if (mAttributes[i])
      {
         attrs.append(B(" "));
         attrs.append(mAttributes[i]->getName(mReader));
         attrs.append(B("=\""));
         attrs.append(mAttributes[i]->getValue());
         attrs.append(B("\""));
      }
   }

   for (n=0; n < spacing; n++)
      spaces.append(B(" "));
   if (mChildren.getNumber() == 0)
   {
      temp.format(B("%s[%s%s]%s[\\%s]"), BStrConv::toB(spaces), BStrConv::toB(getName()), BStrConv::toB(attrs), BStrConv::toB(mText), BStrConv::toB(getName()));
      outputFn(temp.asANSI());
      return;
   }

   temp.format(B("%s[%s%s]%s"),BStrConv::toB(spaces), BStrConv::toB(getName()), BStrConv::toB(attrs), BStrConv::toB(mText));
   outputFn(temp.asANSI());

   // children
   for (n=0; n < mChildren.getNumber(); n++)
   {
      mChildren[n]->output(spacing + 3, outputFn);
   }
   temp.format(B("%s[\\%s]"),BStrConv::toB(spaces), BStrConv::toB(getName()));
   outputFn(temp.asANSI());
}

//==============================================================================
// BXMLAttribute::BXMLAttribute
//==============================================================================
BXMLAttribute::BXMLAttribute() :
   mName(),
   mValue()
{
}

//==============================================================================
// BXMLAttribute::~BXMLAttribute
//==============================================================================
BXMLAttribute::~BXMLAttribute()
{
}

//=============================================================================
// BXMLAttribute::getValueAsLong
//=============================================================================
bool BXMLAttribute::getValueAsLong(long &val) const
{
   val=strGetAsLong(getValue().getPtr(), getValue().length());
   return(true);
}

//=============================================================================
// BXMLAttribute::getValueAsFloat
//=============================================================================
bool BXMLAttribute::getValueAsFloat(float &val) const
{
   val=strGetAsFloat(getValue().getPtr(), getValue().length());
   return(true);
}

//=============================================================================
// BXMLAttribute::getValueAsDWORD
//=============================================================================
bool BXMLAttribute::getValueAsDWORD(DWORD &val) const
{
   val=(DWORD)strGetAsLong(getValue().getPtr(), getValue().length());
   return(true);
}

//==============================================================================
// class BXMLDocument
//==============================================================================

//==============================================================================
// BXMLDocument::BXMLDocument
//==============================================================================
BXMLDocument::BXMLDocument() :
   mpRootNode(NULL),
   mbOwnRootNode(false),
   mReader(NULL),
   mbOwnReader(false),
   mUnicode(false)
{
}

//==============================================================================
// BXMLDocument::~BXMLDocument
//==============================================================================
BXMLDocument::~BXMLDocument()
{
   if (mpRootNode && mbOwnRootNode)
   {
      BXMLNode::releaseInstance(mpRootNode);
      mpRootNode = NULL;
   }

   if (mReader && mbOwnReader)
   {
      delete mReader;
      mReader = NULL;
   }
}


//==============================================================================
// BXMLDocument::createRootNode
//==============================================================================
BXMLNode *BXMLDocument::createRootNode(BIXMLFile* pFile, const BCHAR_T *szName)
{
   if (!szName)
      return (NULL);

   //-- if we don't have a reader, then create one
   if (mReader)
   {
      if (mbOwnReader)
         delete mReader;
      mReader = NULL;
   }

   //-- create a reader to passed on to all nodes we create
   mReader = new BXMLReader(pFile);
   mbOwnReader = true;

   BXMLNode *pNewNode = BXMLNode::getInstance();

   if (!pNewNode)
      return (NULL);

   pNewNode->setReader(mReader);
   pNewNode->setName(szName);
   mpRootNode = pNewNode;



   //-- mark that we need to delete this (and the reader) later
   mbOwnRootNode = true;
   return (pNewNode);
}

//==============================================================================
// BXMLDocument::setRootNode
//==============================================================================
bool BXMLDocument::setRootNode(BXMLReader  *pReader)
{
   if (!pReader)
      return (false);

   BXMLNode *pNode = pReader->getRootNode();

   if (!pNode)
      return (false);

   // Delete old reader if necessary and set new reader
   if (mbOwnReader && (mReader != NULL))
      delete mReader;

   mReader = pReader;
   mbOwnReader = false;


   // Delete old root node if necessary
   if (mbOwnRootNode && (mpRootNode != NULL))
      BXMLNode::releaseInstance(mpRootNode);

   mpRootNode = pNode;
   //-- make sure we don't double delete the root node
   mbOwnRootNode = false;

   return (true);
}

//==============================================================================
// BXMLDocument::addChild
// Takes a Node and attaches it to this document, at the specified node.
// The parent node specified *must* be an existing node in the tree associated
// with this document.  However, the child can come from any other reader.  
// New nodes will be created in the current tree that are copies of the
// nodes that are in the tree passed in.
//==============================================================================
bool BXMLDocument::addChild(BXMLNode *pParent, const BXMLNode *pChild)
{
   // Sanity checks.  If we do not have a root node or a reader yet,
   // bail.
   if (!mReader)
      return false;
   if (!mpRootNode)
      return false;

   // Check the reader.  If the Parent's reader is not our reader,
   // the parent node is not in this document.
   if (pParent->getReader() != mReader)
      return false;

   // Create a new node, with the old child's name..
   BXMLNode *pNewNode = createNode(pChild->getName(), pParent);
   if (!pNewNode)
   {
      BFAIL("BXMLDocument::addchild -- Failed to allocate a new node");
      return false;
   }

   // Text
   pNewNode->setText(pChild->getText());

   // Attributes
   long n = pChild->getAttributeCount();
   for (long m = 0; m < n; m++)
   {
      const BString *pstrName = NULL;
      bool bRet = FALSE;
      bRet = pChild->getAttributeName(m, &pstrName);
      if (!bRet || pstrName == NULL)
      {
         BFAIL("BXMLDocument::addChild -- Failed retrieve attribute name text.");
         continue;
      }
      const BString *pstrValue = NULL;
      bRet = pChild->getAttribValue(pstrName->asANSI(), &pstrValue);
      if (!bRet || pstrValue == NULL)
      {
         BFAIL("BXMLDocument::addChild -- Failed to retrieve attribute value text.");
         continue;
      }
      if (!pNewNode->addAttribute(pstrName->getPtr(), pstrValue->getPtr()))
      {
         BFAIL("BXMLDocument::addChild -- Failed to add attribute to node.");
         continue;
      }
   }

   // If this node has children.. add it too.
   n = pChild->getNumberChildren();
   for (long m = 0; m < n; m++)
   {
      if (!addChild(pNewNode, pChild->getChild(m)))
      {
         BFAIL("BXMLDocument::addChild -- Failed to add a child node");
         return false;
      }
   }
   return true;

}


//==============================================================================
// BXMLDocument::HtmlTranslateChar
//==============================================================================
bool BXMLDocument::HtmlTranslateChar(const BString &src, BString &dest)
{
   dest = src;

   dest.findAndReplace(B("&"), B("&amp;"));
   dest.findAndReplace(B("<"), B("&lt;"));
   dest.findAndReplace(B(">"), B("&gt;"));
   dest.findAndReplace(B("\""), B("&quot;"));
   dest.findAndReplace(B("'"), B("&apos;"));

   return true;
}





//==============================================================================
// BXMLDocument::getRootNode
//==============================================================================
BXMLNode *BXMLDocument::getRootNode(void) const
{
   return mpRootNode;
}


//==============================================================================
// BXMLDocument::createNode
// 
//==============================================================================
BXMLNode *BXMLDocument::createNode(const BCHAR_T *szNodeName, BXMLNode *pParent /*=NULL*/) 
{
   BXMLNode *pNode = BXMLNode::getInstance();
   if (!pNode)
      return (NULL);

   if (!mReader)
      return (NULL);

   //-- set up the parent relationship
   pNode->setParent(pParent);

   //-- pass on the reader
   pNode->setReader(mReader);

   pNode->setName(szNodeName);
   if (pParent)
   {
      if (!pParent->addChild(pNode))
      {
         BXMLNode::releaseInstance(pNode);
         return (NULL);
      }
   }

   return pNode;

}


//==============================================================================
// BXMLDocument::writeToFile
//==============================================================================
bool BXMLDocument::writeToFile(BIXMLFile &file, bool unicode)
{
   //Save the unicode nature.
   mUnicode=unicode;

   //XML header.  Plop the unicode tag first if we're unicode.
   if (mUnicode == true)
   {
#ifdef UNICODE   
      file.fprintf( B( "\xFEFF" ) );  // write BOM
      file.fprintf( B( "<?xml version=\"1.0\" encoding=\"UTF-16\"?>\r\n" ) );
#else
      file.fprintf( B( "<?xml version=\"1.0\"?>\r\n" ) );
#endif      
   }
   else
      file.fprintf("<?xml version=\"1.0\"?>\r\n");

   //Write the file.
   walk(getRootNode(), file, 0);

   //Done.
   return(true);
}


//============================================================================
// BXMLDocument::walk 
//============================================================================
void BXMLDocument::walk(const BXMLNode *pNode, BIXMLFile &file, int level)
{
   if (!pNode)
      return;


   //-- get the name of the node
   const BString &szNodeName = pNode->getName();

   //-- allocate a string for growing
   BString szAttributeString;

   // look through the attributes
   const BSimpleArray<BXMLAttribute*> & attributes = pNode->getAttributes();
   int attrCount = attributes.getNumber();
   for (int a = 0; a < attrCount; a++)
   {
      BXMLAttribute *pAttribute = attributes[a];
      if (!pAttribute)
         continue;

      BString szTemp;
      BString szAttribValue;

      makeSafeXMLString(pAttribute->getValue(), szAttribValue);
      szTemp.format(B(" %s =\"%s\""), BStrConv::toB(pAttribute->getName(mReader)), BStrConv::toB(szAttribValue));
      szAttributeString.append(szTemp);
   }

   // compute the tabs
   BString szTabString(B("\r\n"));
   //Dump out any blank lines we might want.
   for (int a=0; a < pNode->getPrependBlankLineCount(); a++)
   {
      if (mUnicode == true)
         file.fputs(szTabString.getPtr());
      else
         file.fputs(szTabString.asANSI());
   }

   for (long tabindex = 0; tabindex < level; tabindex++)
   {
      szTabString.append(B("\t"));
   }

   BString szOpenNode;
   BString temp;
   szOpenNode.format(B("%s<%s%s>"), BStrConv::toB(szTabString), BStrConv::toB(szNodeName), BStrConv::toB(szAttributeString));
   if (mUnicode == true)
      file.fputs(szOpenNode.getPtr());
   else
      file.fputs(szOpenNode.asANSI()); 

   long dwNumber = pNode->getNumberChildren();

   BString szCloseNode;
   bool bTextNode = false;

   // write out the text
   const BString &text = pNode->getText();
   if (text.length() != 0)
   {
      if (validateText(text.getPtr()))
      {
         HtmlTranslateChar(text, temp);
         if (mUnicode == true)
            file.fputs(temp.getPtr());
         else
            file.fputs(temp.asANSI());
      }
   }

   for (long i = 0; i < dwNumber; i++)
   {
      BXMLNode *pChild = pNode->getChild(i);
      if (!pChild)
         continue;

      walk(pChild, file, (level+1) );
   }

   if ((dwNumber == 0) ||  ((dwNumber == 1) && (bTextNode)))
   {
      szCloseNode.format(B("</%s>"), BStrConv::toB(szNodeName)); 
   }
   else
   {
      szCloseNode.format(B("%s</%s>"), BStrConv::toB(szTabString), BStrConv::toB(szNodeName)); 
   }

   if (mUnicode == true)
      file.fputs(szCloseNode.getPtr());
   else
      file.fputs(szCloseNode.asANSI()); 
}




//==============================================================================
// BXMLDocument::validateText
//==============================================================================
bool BXMLDocument::validateText(const BCHAR_T *szText)
{
   const BCHAR_T * szDelimit =  B("\r\n\t");

   if (bcsspn(szText, szDelimit) > 0)
      return (false);

   return (true);
}


//==============================================================================
// BXMLDocument::makeSafeXMLString
//==============================================================================
bool BXMLDocument::makeSafeXMLString(const BString &szIn, BString &szOut)
{
   //-- copy the string
   long len = szIn.length();

   szOut.empty();

   const BCHAR_T *text = szIn.getPtr();
   BCHAR_T buff[2];
   buff[1] = 0;

   for (long i = 0; i < len; i++)
   {
      if (text[i] != B('\"'))
      {
         buff[0] = text[i];
         szOut.append(buff);
      }
   }

   return (true);
}
#endif