//////////////////////////////////////////////////////////////////////
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_AKASSERT_H_
#define _AK_AKASSERT_H_

#include <AK/SoundEngine/Common/AkTypes.h> //For AK_Fail/Success

#ifndef VERIFY
#define VERIFY(x)	((void)(x))
#endif

#ifdef AUDIOKINETIC
	#if defined (AKMEMORYMGR_EXPORTS) || defined (AKSTREAMMGR_EXPORTS) || defined (AKLOWERENGINE_EXPORTS) || defined (AKSOUNDENGINE_EXPORTS)

		#include <assert.h>

        #ifdef _DEBUG
		    #include <AK/SoundEngine/Common/AkSoundEngine.h>
		    extern AkInitSettings g_settings;
		    #define AssertHook(_Expression) ( (_Expression) || (g_settings.pfnAssertHook( #_Expression, __FILE__, __LINE__), 0) )
            #define AKASSERT(Condition) if ( g_settings.pfnAssertHook )   \
                                            AssertHook(Condition);          \
                                        else                                \
                                            assert(Condition)
        #else
            #define AKASSERT(Condition) ((void)0)
        #endif

		#define AKASSERT_RANGE(Value, Min, Max) (AKASSERT(((Value) >= (Min)) && ((Value) <= (Max))))

        #ifdef _DEBUG
            #define AKVERIFY AKASSERT
        #else
            #define AKVERIFY(x) (x)
        #endif
        
    #elif (RVL_OS)
    	#include <assert.h>

        #ifdef _DEBUG
		    #include <AK/SoundEngine/Common/AkSoundEngine.h>
			extern AkInitSettings g_settings;
			inline void assert_hook(
							bool bcondition,
							const char * in_pszExpression,
							const char * in_pszFileName,
							int in_lineNumber
							)
			{
				if( !bcondition )
					g_settings.pfnAssertHook( in_pszExpression, in_pszFileName, in_lineNumber);
			}
            #define AKASSERT(Condition) if ( g_settings.pfnAssertHook )   \
                                            assert_hook((bool)(Condition), #Condition, __FILE__, __LINE__); \
                                        else                                \
                                            assert(Condition)
        #else
            #define AKASSERT(Condition) ((void)0)
        #endif

		#define AKASSERT_RANGE(Value, Min, Max) (AKASSERT(((Value) >= (Min)) && ((Value) <= (Max))))

        #ifdef _DEBUG
            #define AKVERIFY AKASSERT
        #else
            #define AKVERIFY(x) (x)
        #endif

    #elif (AKAUDIOWAL_EXPORTS)

        #include <assert.h>

        #define AKASSERT(Condition) assert(Condition)

		#define AKASSERT_RANGE(Value, Min, Max) (AKASSERT(((Value) >= (Min)) && ((Value) <= (Max))))

        #ifdef _DEBUG
            #define AKVERIFY AKASSERT
        #else
            #define AKVERIFY(x) (x)
        #endif

	#else	// ! AUDIOLIB_PROJECT

        #ifdef _DEBUG

			#include "TestFrameworkDLL.h"

			// Forward declaration
			TESTFRAMEWORK_DLL void AKHandleAssert( 
					bool in_bCondition, 
					const char * in_szExpression, 
					const char * in_szFileName, 
					int in_lineNumber );

			#define AKASSERT( expression )	\
            AKHandleAssert( expression ? true : false, #expression, __FILE__, __LINE__ )

			#define AKVERIFY AKASSERT

		#else	// !_DEBUG

			#define AKASSERT(Condition) ((void)0)
			#define AKVERIFY VERIFY

		#endif	// _DEBUG

	#endif	// AUDIOLIB_PROJECT
#else

	#include <assert.h>

	#define AKVERIFY VERIFY

	#define AKASSERT(Condition) assert(Condition)

	#define AKASSERT_RANGE(Value, Min, Max) (AKASSERT(((Value) >= (Min)) && ((Value) <= (Max))))

#endif

#define AKASSERTANDRETURN( __Expression, __ErrorCode )\
	if (!(__Expression))\
	{\
		AKASSERT(__Expression);\
		return __ErrorCode;\
	}\

#define AKASSERTPOINTERORFAIL( __Pointer ) AKASSERTANDRETURN( __Pointer != NULL, AK_Fail )
#define AKASSERTSUCCESSORRETURN( __akr ) AKASSERTANDRETURN( __akr == AK_Success, __akr )

#define AKASSERTPOINTERORRETURN( __Pointer ) \
	if ((__Pointer) == NULL)\
	{\
		AKASSERT((__Pointer) == NULL);\
		return ;\
	}\

#endif //_AK_AKASSERT_H_
