//==============================================================================
// localemanager.h
//
// Copyright (c) Ensemble Studios, 2005-2008
//==============================================================================
#pragma once

// Includes

// Forward declarations
class BLocaleManager;

// Global variable for the one BLocaleManager object
extern BLocaleManager gLocaleManager;

//==============================================================================
// BLocaleManager
//==============================================================================
class BLocaleManager
{
   public:      

      BLocaleManager();
      ~BLocaleManager();

      void              initEraFileFromLanguage();
      bool              init();
      const BString&    getLanguageString() const { return mLanguageString; }
      const BString&    getLocFile();



      // void addAvailableLanguage(BString& language, long cchLanguage);
      bool getPreferredLanguage (BString& returnedLanguage);



   protected:
      void              findLanguages();


      // xlip functionality
      void              fillFallbackData();
      bool              findInAvailableLanguages(const BString& language);
      bool              findInAvailableWithFallback(const BString& preferredLanguage, BString& foundLanguage);


      DWORD             getLanguage() const { return mLanguage; }

      DWORD             mLanguage;        // This is the dashboard language.
      BString           mLanguageString;  // This is a language string that our game understands

      BSmallDynamicArray<BString> mSupportedLanguages;

      BString           mLocaleEraFile;


      BHashMap<BString, BString> mFallbackLanguages;
      BHashMap<BString, BString> mLanguageToFileMapping;
};
