//==============================================================================
// localemanager.cpp
//
// Copyright (c) Ensemble Studios, 2005-2008
//==============================================================================

// Includes
#include "common.h"
#include "localemanager.h"
#include "configsgame.h"

// Globals
BLocaleManager gLocaleManager;

/// The following array provides a mapping
/// between the language DWORD and the culture name.
///
PCSTR XLipLanguageOverride[] =
{
   NULL,		// Value 0 is undefined
   "en-us",   	// XC_LANGUAGE_ENGLISH
   "ja-jp",	// XC_LANGUAGE_JAPANESE
   "de-de",	// XC_LANGUAGE_GERMAN
   "fr-fr",	// XC_LANGUAGE_FRENCH
   "es-es",	// XC_LANGUAGE_SPANISH
   "it-it",	// XC_LANGUAGE_ITALIAN
   "ko-kr",	// XC_LANGUAGE_KOREAN
   "zh-tw",	// XC_LANGUAGE_TCHINESE
   "pt-pt",	// XC_LANGUAGE_PORTUGUESE
   "zh-cn",	// XC_LANGUAGE_SCHINESE
   "pl-pl",	// XC_LANGUAGE_POLISH
   "ru-ru"		// XC_LANGUAGE_RUSSIAN
};

/// When a locale can override the language
/// preferences on the console, it will have the language override stored as a 
/// PCSTR, or NULL otherwise. When a valid locale is stored within the locale
/// information, it will be preferred over the language
/// settings of the console (XGetLanguage)
///
PCSTR XLipLocaleOverride[] = 
{
   NULL,		// XC_LOCALE_UNKNOWN
   NULL,		// XC_LOCALE_AUSTRALIA
   NULL,		// XC_LOCALE_AUSTRIA
   "fr-be",	// XC_LOCALE_BELGIUM
   NULL,		// XC_LOCALE_BRAZIL
   NULL,		// XC_LOCALE_CANADA
   NULL,		// XC_LOCALE_CHILE
   "zh-cn",	// XC_LOCALE_CHINA
   NULL,		// XC_LOCALE_COLOMBIA
   "cs-cz",	// XC_LOCALE_CZECH_REPUBLIC
   "da-dk",	// XC_LOCALE_DENMARK
   "fi-fi",	// XC_LOCALE_FINLAND
   NULL,		// XC_LOCALE_FRANCE
   NULL,		// XC_LOCALE_GERMANY
   "el-gr",	// XC_LOCALE_GREECE
   NULL,		// XC_LOCALE_HONG_KONG, "zh-hk" (not supposed to override the language setting
   "hu-hu",	// XC_LOCALE_HUNGARY
   NULL,		// XC_LOCALE_INDIA
   NULL,		// XC_LOCALE_IRELAND
   NULL,		// XC_LOCALE_ITALY
   NULL,		// XC_LOCALE_JAPAN
   //"ko-kr",	// XC_LOCALE_KOREA - This can be selected as a language, so don't put a locale override on this
   NULL,	   // XC_LOCALE_KOREA
   "es-la",	// XC_LOCALE_MEXICO
   "nl-nl",	// XC_LOCALE_NETHERLANDS
   NULL,		// XC_LOCALE_NEW_ZEALAND
   "nb-no",	// XC_LOCALE_NORWAY
   NULL,		// XC_LOCALE_POLAND
   NULL,		// XC_LOCALE_PORTUGAL
   NULL,		// XC_LOCALE_SINGAPORE, "zh-sg" (not supposed to override the language setting)
   "sk-sk",	// XC_LOCALE_SLOVAK_REPUBLIC
   NULL,		// XC_LOCALE_SOUTH_AFRICA
   NULL,		// XC_LOCALE_SPAIN
   "sv-se",	// XC_LOCALE_SWEDEN
   NULL,		// XC_LOCALE_SWITZERLAND
   // "zh-tw",	// XC_LOCALE_TAIWAN - This can be selected as a language so don't put a locale override on this.
   NULL,	   // XC_LOCALE_TAIWAN
   //"en-uk",	// XC_LOCALE_GREAT_BRITAIN - English is the default, so no need to select this.
   NULL,	      // XC_LOCALE_GREAT_BRITAIN
   // "en-us",	// XC_LOCALE_UNITED_STATES - This can be selected as a language, so don't put a locale override on this.
   NULL,	      // XC_LOCALE_UNITED_STATES
   NULL        // XC_LOCALE_RUSSIAN_FEDERATION
};

//==============================================================================
// BLocaleManager::BLocaleManager
//==============================================================================
BLocaleManager::BLocaleManager() :
   mLanguage(XC_LANGUAGE_ENGLISH)
{
}

//==============================================================================
// BLocaleManager::~BLocaleManager
//==============================================================================
BLocaleManager::~BLocaleManager()
{
}

//==============================================================================
//==============================================================================
void BLocaleManager::initEraFileFromLanguage()
{
   fillFallbackData();
   findLanguages();

   BString preferredLanguage;
   if (!getPreferredLanguage(preferredLanguage))
      preferredLanguage.set("default");

   mLocaleEraFile.set("locale.era");      // just in case we don't find anything (should never happen).
   BHashMap<BString, BString>::const_iterator it = mLanguageToFileMapping.find(preferredLanguage);
   if( it != mLanguageToFileMapping.end() )
   {
      // get the locale_*.era file to use for the preferred language
      mLocaleEraFile.set( it->second );
   }
}


//==============================================================================
// BLocaleManager::init()
//==============================================================================
bool BLocaleManager::init()
{
   // Grab the dashboard language
   mLanguage = XGetLanguage();

   // This all needs to be put somewhere else. I'm sure other things will need to access this too.
   switch(mLanguage)
   {
   case XC_LANGUAGE_ENGLISH:
#ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigTestFonts))
         mLanguageString="English.test";
      else
#endif
      mLanguageString="English";
      break;
   case XC_LANGUAGE_JAPANESE:
      mLanguageString="Japanese";
      break;
   case XC_LANGUAGE_GERMAN:
      mLanguageString="German";
      break;
   case XC_LANGUAGE_FRENCH:
      mLanguageString="French";
      break;
   case XC_LANGUAGE_SPANISH:
      mLanguageString="Spanish";
      break;
   case XC_LANGUAGE_ITALIAN:
      mLanguageString="Italian";
      break;
   case XC_LANGUAGE_KOREAN:
      mLanguageString="Korean";
      break;
   case XC_LANGUAGE_TCHINESE:
      mLanguageString="TChinese";
      break;
   case XC_LANGUAGE_PORTUGUESE:
      mLanguageString="Portuguese";
      break;
   case XC_LANGUAGE_SCHINESE:
      mLanguageString="SChinese";
      break;
   case XC_LANGUAGE_POLISH:
      mLanguageString="Polish";
      break;
   case XC_LANGUAGE_RUSSIAN:
      mLanguageString="Russian";
      break;
   default:
      {
         // We don't now what language this is so we need to default to something we do know.
         DWORD region = XGetGameRegion();

         // SEE XDK documentation on how this maps
         //    documentation is on the XGetLanguage() function.
         switch (region)
         {
            // XC_GAME_REGION_ASIA_JAPAN XC_LANGUAGE_JAPANESE 
         case XC_GAME_REGION_ASIA_JAPAN:
            mLanguage=XC_LANGUAGE_JAPANESE;
            mLanguageString="Japanese";
            break;

            // All non-Asian regions XC_LANGUAGE_ENGLISH 
         case XC_GAME_REGION_NA_ALL:
         case XC_GAME_REGION_EUROPE_ALL:
         case XC_GAME_REGION_EUROPE_REST:
         case XC_GAME_REGION_RESTOFWORLD_ALL:
            mLanguage=XC_LANGUAGE_ENGLISH;
            mLanguageString="English";
            break;

            // All other regions in Asia XC_LANGUAGE_KOREAN 
         case XC_GAME_REGION_ASIA_ALL:
         case XC_GAME_REGION_ASIA_CHINA:     // <---- This doesn't seem right, but it's how I interpret the XDK docs
         case XC_GAME_REGION_ASIA_REST:
            mLanguage=XC_LANGUAGE_KOREAN;
            mLanguageString="Korean";
            break;
         }
      }
   }

   // override the language if it's set
   BSimString languageString;
   if (gConfig.get(cConfigLocaleLanguage, languageString))
   {
      // copy it over, leave the language ID as is, it's only accessible via protected method
      mLanguageString = languageString;
   }


   return true;
}


//==============================================================================
//==============================================================================
void BLocaleManager::findLanguages()
{
   char strFind[] = "d:\\locale*.era";
   WIN32_FIND_DATA wfd;
   HANDLE hFind;

   // Start the find and check for failure.
   hFind = FindFirstFile( strFind, &wfd );
   if( INVALID_HANDLE_VALUE == hFind )
      return;

   BString startPattern;
   startPattern.set("locale_");

   BString startPatternDefault;
   startPatternDefault.set("localeDefault_");

   // Display each file and ask for the next.
   BString filename;
   BString language;
   do
   {
      // grab the filename
      filename.format("d:\\%s", wfd.cFileName);    // get the full path
      language.set(wfd.cFileName);
      language.removeExtension();

      // check if this is the default with no language indicator
      if (filename == "d:\\locale.era")
      {
         mLanguageToFileMapping.insert("default", filename);         // fallback in case we don't find anything that matches
         continue;
      }

      // check if this is the default with a language indicator
      int i = language.findLeft(startPatternDefault.getPtr());           // we are trying to get the language based on the filename.
      if (i == 0)       // starts with...
      {
         language.crop(startPatternDefault.length(), language.length() - 1);

         if (language.length() > 0)
         {
            // we have a language other than the default.
            mSupportedLanguages.add(language);
            mLanguageToFileMapping.insert(language, filename);          // the language for the default language
            mLanguageToFileMapping.insert("default", filename);         // fallback in case we don't find anything that matches
         }
         continue;      // found default, go to the next file.
      }

      // check if this is a secondary file next
      i = language.findLeft(startPattern.getPtr());           // we are trying to get the language based on the filename.
      if (i == 0)       // starts with...
      {
         language.crop(startPattern.length(), language.length() - 1);

         if (language.length() > 0)
         {
            // we have a language other than the default.
            mSupportedLanguages.add(language);
            mLanguageToFileMapping.insert(language, filename);
         }
      }
   } 
   while( FindNextFile( hFind, &wfd ) );

   // Close the find handle.
   FindClose( hFind );
}

//==============================================================================
//==============================================================================
const BString& BLocaleManager::getLocFile()
{
   return mLocaleEraFile;
}


//==============================================================================
/// The fallback map is populated with fallback list.
/// Each value of the map entry is a fallback for it's key.
/// E.g. "en" is a fallback for "en-uk"
/// This means that if user's settings require "en-uk" language
/// but it's not available in the game then it's fallback "en" will be selected.
/// It describes a hierarchy, e.g. "zh-cn" falls back to "zh-chs" 
/// which in turn can fall back to "en" or another language. 
/// Empty fallback string means that there is no further fallback for this language.
//==============================================================================
void BLocaleManager::fillFallbackData()
{
   mFallbackLanguages.insert("en-uk", "en-us");			// US English fallback for Great Britian
   mFallbackLanguages.insert("en-us", "");				// No fallback for US English
   mFallbackLanguages.insert("es-es", "");				// no fallback for spanish
   mFallbackLanguages.insert("zh-cn", "zh-chs");		// Simplified Chinese fallback for China
   mFallbackLanguages.insert("zh-tw", "zh-cht");		// Traditional Chinese fallback for taiwan
   //   mFallbackLanguages.insert("zh-hk", "zh-cht");		// Traditional Chinese fallback for hong kon
   //   mFallbackLanguages.insert("zh-sg", "zh-cht");		// Traditional Chinese fallback for singapore
   mFallbackLanguages.insert("ru-ru", "");				// no falback
   mFallbackLanguages.insert("fr-fr", "");				// no fallback
   mFallbackLanguages.insert("ja-jp", "");				// no fallback
   mFallbackLanguages.insert("de-de", "");				// no fallback
   mFallbackLanguages.insert("it-it", "");				// no fallback
   mFallbackLanguages.insert("ko-kr", "");				// no fallback
   mFallbackLanguages.insert("pt-pt", "");				// no fallback 
   mFallbackLanguages.insert("pl-pl", "");				// no fallback 
   mFallbackLanguages.insert("fr-be", "fr-fr");			// french fallback for belgium
   mFallbackLanguages.insert("cs-cz", "");				// no fallback czech
   mFallbackLanguages.insert("da-dk", "");				// no fallback denmark
   mFallbackLanguages.insert("fi-fi", "");				// no fallback finland
   mFallbackLanguages.insert("el-gr", "");				// no fallback greece
   mFallbackLanguages.insert("hu-hu", "");				// no fallback hungary
   mFallbackLanguages.insert("es-la", "es-es");			// spanish fallback for mexico
   mFallbackLanguages.insert("nl-nl", "");				// no fallback netherlands
   mFallbackLanguages.insert("nb-no", "");				// no fallback for norwary
   mFallbackLanguages.insert("sk-sk", "cs-cz");		   // czech fallback for slovakia
   mFallbackLanguages.insert("sv-se", "");				// no fallback for sweden
}


//==============================================================================
//==============================================================================
bool BLocaleManager::findInAvailableLanguages(const BString& language)
{
   for (int i=0; i<mSupportedLanguages.getNumber(); i++)
   {
      if (mSupportedLanguages[i].compare(language)==0)
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BLocaleManager::findInAvailableWithFallback(const BString& preferredLanguage, BString& foundLanguage)
{
   // do we have the preferred language installed?
   if (findInAvailableLanguages(preferredLanguage))
   {
      foundLanguage.set(preferredLanguage);
      return true;
   }

   // No preferred language, see if we can find a fallback language for the preferred language
   BString fallbackLanguage;
   BHashMap<BString, BString>::const_iterator it = mFallbackLanguages.find(preferredLanguage);
   if( it != mFallbackLanguages.end() )
   {
      fallbackLanguage.set( it->second );
   }

   // We have a fallback language for the preferred, is it installed?
   if (fallbackLanguage.length() > 0)
   {
      if (findInAvailableLanguages(fallbackLanguage))
      {
         foundLanguage.set(fallbackLanguage);
         return true;
      }
   }

   return false;
}


//==============================================================================
//==============================================================================
bool BLocaleManager::getPreferredLanguage(BString & preferredLanguage)
{
   //-- Add the locale setting (if applicable) --//
   DWORD dwLocale = XGetLocale();
   if (dwLocale<ARRAYSIZE(XLipLocaleOverride) && XLipLocaleOverride[dwLocale])
   {
      BString requestedLanguage;
      requestedLanguage.set(XLipLocaleOverride[dwLocale]);

      if (findInAvailableWithFallback(requestedLanguage, preferredLanguage))
         return true;
   }

   //-- Add the language setting --//
   DWORD dwLanguage = XGetLanguage();
   if (dwLanguage<ARRAYSIZE(XLipLanguageOverride) && XLipLanguageOverride[dwLanguage])
   {
      BString requestedLanguage;
      requestedLanguage.set(XLipLanguageOverride[dwLanguage]);
      if (findInAvailableWithFallback(requestedLanguage, preferredLanguage))
         return true;
   }	
   return false;
}
