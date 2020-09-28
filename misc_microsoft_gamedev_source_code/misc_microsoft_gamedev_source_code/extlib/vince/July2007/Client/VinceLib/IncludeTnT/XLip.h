#pragma once

//$avgarcia: might have to wrap with extern "C" 
//$$avgarcia: cchBuffer for output buffer sizes could all be unsigned

/*++
	FILE:			XLip.h
	CREATION TIME:	19-Feb-07
	DESCRIPTION:	Contains the declaration of the public exported API that
					clients of the XLip library can invoke when linked against
					xlip.lib
	HISTORY:
					19-Feb-07	avgarcia	First draft
--*/

/*! \file
 * Declaration of public/exported API for XLIP.
 * Game titles using XLIP should include this file and link to XLip.lib to
 * use the LIP functionality. This is the only exported header file for XLIP.
 * <h2 align=left>History</h2>
 * <table>
 * <tr>
 *   <th>Date</th>
 *   <th>Author</th>
 *   <th width="75%">Change</th>
 * </tr>
 * <tr>
 *   <td>19-Feb-07</td>
 *   <td>avgarcia</td>
 *   <td>First draft</td>
 * </tr>
 * </table>
 */

/*! 
 * \defgroup INIT Library Initialization
 * \defgroup MEMORY Memory Management
 * \defgroup CULTURE Culture Information
 * \defgroup COMPONENT Component Registration/Query
 * \defgroup SETTINGS User/Language Settings
 */

///////////////////////////////////////////////////////////////////////////////
///						LIBRARY INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

/*!
 * \ingroup INIT
 * Options for the XLIP library that are user-configurable. All are assigned a
 * default value if not configured at startup time. To set these, use ::XLipConfigure
 */
enum XLIPCONFIGOPTION
{
	XLIPOPTION_ASSERTBEHAVIOR = 0,	/*!< Defines the behavior of internal ASSERTs. Possible values: <code>XLIPCONFIG_NONE || XLIPCONFIG_BREAK</code> */
	XLIPOPTION_ALLOCBEHAVIOR,		/*!< Defines the behavior of a failed allocation. Possible values: <code>XLIPCONFIG_ASSERT || XLIPCONFIG_THROW || XLIPCONFIG_NONE</code> */
	XLIPOPTION_ROOTHINT,			/*!< Pre-allocate N roots on the container vector. Default value: <code>(XLIPCONFIGOPTION)3</code> */
	XLIPOPTION_COMPONENTHINT,		/*!< Pre-allocate N components on the container vector. Default value: <code>(XLIPCONFIGOPTION)6</code> */
	XLIPOPTION_CULTUREHINT,			/*!< Pre-allocate N cultures on the container vector. Default value: <code>(XLIPCONFIGOPTION)15</code> */
	XLIPOPTION_LANGUAGEHINT,		/*!< Pre-allocate N languages on the container vector. Default value: <code>(XLIPCONFIGOPTION)5</code> */
	/*! \cond */
    // $$avgarcia: LAST->COUNT
    XLIPOPTION_LAST
	/*! \endcond */
};

/*!
 * \ingroup INIT
 * Possible values that can be specified when setting an ::XLIPCONFIGOPTION via ::XLipConfigure.
 * Some values for <b>XLIPCONFIGOPTION</b> are numeric, in which case they should still be casted
 * to <b>XLIPCONFIGOPTION</b>, e.g.:
   <pre>
   int value = 5;
   XLipConfigure(... , ((XLIPCONFIGOPTION)value));
   </pre>
 */

// $$$avgarcia: follow up on release vs debug behavior, make sure they are in sync if dev/test
// ignores debug spew or breaks
enum XLIPCONFIGVALUE
{
	XLIPCONFIG_NONE = 0,/*!< Either take no action or no pre-allocation occurs */
	XLIPCONFIG_BREAK,	/*!< Break on hit (via <code>DebugBreak()</code>) */
	XLIPCONFIG_ASSERT,	/*!< Assert on hit (calls the internal ASSERT implementation) */
	XLIPCONFIG_THROW	/*!< Throw a C-style exception (the content is a <code>wchar_t</code> string) */
};

/*!
 * \ingroup INIT
 * This needs to be the very first call to any XLip* API. Only 2 other API can be
 * called before this API, and those are ::XLipSetMainMemoryBuffer and
 * ::XLipSetMemoryManagement. Otherwise, if any other XLip* API is called before
 * XLipInitialize the results are <b>*undefined*</b>.
 */
VOID XLipInitialize();

/*!
 * \ingroup INIT
 * This needs to be the very last call to the library. After this API has been
 * been called, no other XLip* API should be called. If any XLip API is called aftwerwards,
 * the results are <b>*undefined*</b>.
 */
VOID XLipUninitialize();

/*!
 * \ingroup INIT
 * Sets some global configuration options on the library. There is no requirement
 * to call XLipConfigure() at all, but some options might produce more efficient
 * results at runtime if correctly set by the game developer.
 */
VOID XLipConfigure (
	XLIPCONFIGOPTION option,/*!< The option being set */
	XLIPCONFIGVALUE value	/*!< The value for the specified option */
	);

///////////////////////////////////////////////////////////////////////////////
///							MEMORY MANAGEMENT
///////////////////////////////////////////////////////////////////////////////

/*!
 * \ingroup MEMORY
 * Sets the main memory buffer that the library should use for memory allocations.
 * If the library ever runs out of space of this main memory buffer, any calls
 * to the internal allocator will return NULL or throw an exception, depending on how
 * the library is configured (see ::XLipConfigure)
 */
VOID XLipSetMainMemoryBuffer (
	PVOID pvMemBuffer,		/*!< Memory buffer used during internal allocations */
	LONG cbMemBufferSize	/*!< Size of the buffer specified in <code>pvMemBuffer</code> */
	);

/*!
 * \ingroup MEMORY
 * Signature of the allocator function that can set up via ::XLipSetMemoryManagement
 */
typedef PVOID (*XLIP_ALLOCATOR)	  (LONG);

/*!
 * \ingroup MEMORY
 * Signature of the deallocator function that can set up via ::XLipSetMemoryManagement
 */
typedef VOID  (*XLIP_DEALLOCATOR) (PVOID);

/*!
 * \ingroup MEMORY
 * Sets (or unsets) the internal allocator/deallocator. If any of the 
 * parameters is NULL, the standard C allocator/deallocator functions 
 * (malloc/free) are used instead.
 */
VOID XLipSetMemoryManagement (
	XLIP_ALLOCATOR allocator,		/*!< Pointer to the allocator function */
	XLIP_DEALLOCATOR deallocator	/*!< Pointer to the deallocator function */
	);

///////////////////////////////////////////////////////////////////////////////
///							CULTURE INFORMATION
///////////////////////////////////////////////////////////////////////////////

/*!
 * \ingroup CULTURE
 * Loads the cultures from the specified file and populates the internal culture vector.
 * <p><b>Note:</b><br>
 * Each line in the file requires the following format:
 <pre>
	\<culture name\>\\t\<native name\>\\t<parent\>
 </pre>
 E.g.:
 <pre>
	en-us\\tEnglish (United States)\\ten
 </pre>
 * When there is no parent, the value for <code>\<parent\></code> should be <code>null</code>
 * \return The number of lines parsed from the specified file. -1 if an error occurs.
  */
//$avgarcia: replace -1 with something like XLIP_ERROR
// or < 0?
LONG XLipLoadCulturesFromFile(
	PCSTR pcszFileName	/*!< Name of the target file to parse. Either relative or full path. */
	);

/*!
 * \ingroup CULTURE
 * The enumeration of the possible information that can be requested via
 * ::XLipGetCultureInfo. This somehow maps to the flags supported by GetCultureInfo() 
 * under Windows.
 */
enum XLIPCULTUREINFOTYPE
{
	XLIPCULTURE_NATIVENAME = 1,	/*!< Native name of the language (e.g. "Español" for "Spanish"). Type is wide string (WCHAR*) */
	XLIPCULTURE_PARENT,			/*!< Parent (neutral) language (e.g. "en" for "en-US"). Type is string (CHAR*) */
    XLIPCULTURE_VERSION         /*!< Accepted Major Version for this culture. Type is numeric (UINT16) */
};

/*!
* \ingroup CULTURE
 * Gets information of the specified culture. This information is retrieved
 * from g_vecCultures when runing on XBOX. When running on Windows, this API
 * is just a thin wrapper around GetCultureInfo().
 * <p><b>Note:</b><br>
 * Depending on the information being requested, the returned data can either be formatted
 * as <code>char</code> or as <code>wchar_t</code> or as <code>UINT16</code>. For information
 * on the type of each culture information, see ::XLIPCULTUREINFOTYPE
 * \return The number of bytes returned in the output buffer.
 */
LONG XLipGetCultureInfo(
	PCSTR pcszCultureName,			/*!< The culture of interest (e.g. "en-US") */
	XLIPCULTUREINFOTYPE infoType,	/*!< The type of information requested. */
	PBYTE pbBuffer,					/*!< [out] The buffer where the information is returned */
	LONG cbBuffer					/*!< The size of the buffer */
	);

/*!
* \ingroup CULTURE
 * Gets information of the specified culture. This information is retrieved
 * from g_vecCultures when runing on XBOX. When running on Windows, this API
 * is just a thin wrapper around GetCultureInfo().
 * <p><b>Note:</b><br>
 * Depending on the information being requested, the returned data can either be formatted
 * as <code>char</code> or as <code>wchar_t</code>. Currently, the information types that
 * are formatted as wide strings are:
 * <ul>
 * <li><code>XLIPCULTURE_NATIVENAME</code></li>
 * </ul>
 * <h3>WARNING:</h3>
 * The returned data remains valid as long as no other call is made to 
 * ::XLipLoadCulturesFromFile. If <code>XLipLoadCulturesFromFile</code> is called one more
 * time, all the returned pointers by this API are now invalid.
 * \return A pointer to the data requested.
 */
const void* XLipGetCultureInfo(
	PCSTR pcszCultureName,			/*!< The culture of interest (e.g. "en-US") */
	XLIPCULTUREINFOTYPE infoType 	/*!< The type of information requested. */
	);


///////////////////////////////////////////////////////////////////////////////
///						COMPONENT REGISTRATION / QUERY
///////////////////////////////////////////////////////////////////////////////

/*!
 * \ingroup COMPONENT
 * Adds the specified path to the list of root paths that will be scanned for
 * languages/components. This API <b>*can be*</b> called at runtime, but the user is
 * then responsible of calling ::XLipRefreshComponents to re-scan the roots
 * for existing languages, etc.
 * \return <code>TRUE</code> if successful, <code>FALSE</code> otherwise.
 */
//$avgarcia: error handling?  Should this be more than a true/false or should we
// shove something into GetLastError()
BOOL XLipAddRoot (
	PCSTR pcszPath /*!< Path to the root being added. <i>Needs to be a valid path.</i> */
	);

/*!
 * \ingroup COMPONENT
 * Adds the component to the LIP registration contents. This component can
 * then be queried via ::XLipGetComponentPath when searching for localized
 * files. Components shouldn't be added/removed at runtime.
 * \return <code>TRUE</code> if successful, <code>FALSE</code> otherwise.
 */
 //$avgacria: doc wildcards in pcszPath
BOOL XLipAddComponent (
	PCSTR pcszName,	/*!< Name of the component. */
	PCSTR pcszPath	/*!< Path of the component (<i>relative to all the roots</i>). */
	);

/*!
 * \ingroup COMPONENT
 * Re-scans the languages existing under the specified roots and associates each
 * registered component with its available languages. Needs to be called at 
 * least once at runtime before any call to ::XLipGetComponentPath and
 * ::XLipEnumLanguages. Also needs to be called if the roots are updated at
 * runtime. It <b>*DOES NOT*</b> needs to be called if the user language preferences 
 * are changed at runtime.
 */
VOID XLipRefreshComponents();

/*!
 * \ingroup COMPONENT
 * Returns the path to the component on either the requested language or, if
 * <code>NULL</code> is specified in place of a culture name, on the language corresponding
 * to the user settings.
 * \return The following are the possible return values:
 * \return <ul><li>If any internal error occurs, the returned value is <code>\< 0</code></li><li>If the output buffer argument is <code>NULL</code>, the API returns the required buffer size.</li><li>If the output buffer argument is valid, the API returns the number of characters copied to the buffer.</li></ul>
 */

LONG XLipGetComponentPath (
	PCSTR pcszComponent,	/*!< Name of the component */
	PCSTR pcszLanguage,		/*!< Language of interest or <code>NULL</code> to use the user settings. */
	PSTR pszBuffer,			/*!< [out] Buffer where the path is returned. Can be <code>NULL</code>. */
	LONG cchBuffer			/*!< Length of the buffer passed in. */
	);

/*!
 * \ingroup COMPONENT
 * Returns de number of available languages for the LIP or for a single component.
 * The behavior of this API is undefined if called before 
 * ::XLipRefreshComponents()
 * <p><b>Example:</b><br>
 <pre>
    #define MAX_CULTURE 85
	CHAR chLang[MAX_CULTURE];
	LONG cLangs = XLipCountLanguages(NULL);
	for(LONG iLang = 0; iLang \< cLangs; ++iLang)
	{
		XLipEnumLanguages(NULL,iLang,chLang,MAX_CULTURE);

		... DO SOMETHING WITH THE LANGUAGE IN chLang
	}
 </pre>
 * \return The number of supported languages or <code>/< 0</code> if an error occurs.
 */
LONG XLipCountLanguages ( PCSTR pcszComponent );

/*!
 * \ingroup COMPONENT
 * Enumerates the available languages for the LIP or for a single component.
 * The behavior of this API is undefined if called before 
 * ::XLipRefreshComponents()
 * <p><b>Example:</b><br>
 <pre>
    #define MAX_CULTURE 85
	CHAR chLang[MAX_CULTURE];
	LONG cLangs = XLipCountLanguages(NULL);
	for(LONG iLang = 0; iLang \< cLangs; ++iLang)
	{
		XLipEnumLanguages(NULL,iLang,chLang,MAX_CULTURE);

		... DO SOMETHING WITH THE LANGUAGE IN chLang
	}
 </pre>
 * \return The following are the possible return values:
 * \return <ul><li>If any internal error occurs, the returned value is <code>\< 0</code></li><li>If the passed in buffer is <code>NULL</code>, the API reutrns the required buffer size.</li><li>If the passed in buffer is valid, the API returns the number of characters written to the buffer.</li></ul>
 */
LONG XLipEnumLanguages (
	PCSTR pcszComponent,	/*!< Name of the component to enumerate (or <code>NULL</code>) */
	LONG iLanguage,			/*!< Iterator index of the language */
	PSTR pszBuffer,			/*!< [out] Buffer where the language name is returned. Can be <code>NULL</code>. */
	LONG cchBuffer			/*!< Length of the buffer passed in. */
	);

///////////////////////////////////////////////////////////////////////////////
///							SETTINGS MANAGEMENT
///////////////////////////////////////////////////////////////////////////////

/*!
 * \ingroup SETTINGS
 * Saves the specified language as preferred for the current user. Makes no 
 * distinction between different users within the console; this is a global
 * runtime setting.
 */
//$avgarcia: error code
VOID XLipSetUserSettings (
	PCSTR pcszLanguage	/*!< Language name, e.g. "en-us" */
	);

/*!
 * \ingroup SETTINGS
 * Loads the user settings from the specified memory blob. The memory passed
 * into this function can come from several places:
 * <ul>
 * <li>The registry (in Windows)</li>
 * <li>A file from disk</li>
 * <li>A profile setting (in Xbox)</li>
 * <li>Etc ...</li>
 * </ul>
 * \return TRUE if successful, FALSE otherwise.
 */
BOOL XLipLoadUserSettings (
	PVOID pvLoadBuffer,	/*!< Buffer where the settings will be loaded from */
	LONG cbBuffer		/*!< Size of the buffer */
	);

/*!
 * \ingroup SETTINGS
 * Saves the current (runtime) user settings into the specified memory blob.
 * This memory buffer can then be saved by the game developer into several
 * places:
 * <ul>
 * <li>The registry (in Windows)</li>
 * <li>A file from disk</li>
 * <li>A profile setting (in Xbox)</li>
 * <li>Etc ...</li>
 * </ul>
 * \return TRUE if successful, FALSE otherwise.
 */
//$$$avgarcia: return ULONG of size, and allow pvSaveBuffer to be NULL so that we know how big to allocate
BOOL XLipSaveUserSettings (
	PVOID pvSaveBuffer,	/*!< Buffer where the settings will be saved to */
	LONG cbBuffer		/*!< Size of the buffer */
	);


// PBERGMAN: code review left off here
/*!
 * \ingroup SETTINGS
 * Loads the user settings from the specified file. This function is only a thin
 * wrapper around ::XLipLoadUserSettings to read from the file specified.
 * \return TRUE if successful, FALSE otherwise.
 */
BOOL XLipLoadUserSettingsFromFile (
	PCSTR pcszFileName	/*!< File where the settings will be loaded from */
	);

/*!
 * \ingroup SETTINGS
 * Saves the current (runtime) user settings into the specified file.
 * \return TRUE if successful, FALSE otherwise.
 */
BOOL XLipSaveUserSettingsToFile (
	PCSTR pcszFileName	/*!< File where the settings will be saved to */
	);