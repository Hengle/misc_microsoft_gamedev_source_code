//////////////////////////////////////////////////////////////////////
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_AKASSERT_H_
#define _AK_AKASSERT_H_

#if ! defined( AKASSERT )

	#include <AK/SoundEngine/Common/AkTypes.h> //For AK_Fail/Success
	#include <assert.h>

	#if ! defined ( VERIFY )
		#define VERIFY(x)	((void)(x))
	#endif

	#if defined( _DEBUG )

		#if defined( __SPU__ )

			#define AKASSERT(Condition) assert(Condition)

		#else // defined( __SPU__ )

			#include <AK/SoundEngine/Common/AkSoundEngine.h>

			extern AkInitSettings g_settings;

			#if defined( RVL_OS )

					inline void _AkAssertHook(
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
													_AkAssertHook((bool)(Condition), #Condition, __FILE__, __LINE__); \
												else                                \
													assert(Condition)

			#else // defined( RVL_OS )
				
				#define _AkAssertHook(_Expression) ( (_Expression) || (g_settings.pfnAssertHook( #_Expression, __FILE__, __LINE__), 0) )

				#define AKASSERT(Condition) if ( g_settings.pfnAssertHook )   \
												_AkAssertHook(Condition);          \
											else                                \
												assert(Condition)

			#endif // defined( RVL_OS )

		#endif // defined( __SPU__ )

		#define AKVERIFY AKASSERT

	#else // defined( _DEBUG )

		#define AKASSERT(Condition) ((void)0)
		#define AKVERIFY(x) (x)

	#endif // defined( _DEBUG )

	#define AKASSERT_RANGE(Value, Min, Max) (AKASSERT(((Value) >= (Min)) && ((Value) <= (Max))))

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

#endif // ! defined( AKASSERT )

#endif //_AK_AKASSERT_H_

