                          
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'cpp' files in this code is as         */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants local to the class.                            */
    /*      3. Data structures local to the class.                      */
    /*      4. Data initializations.                                    */
    /*      5. Static functions.                                        */
    /*      6. Class functions.                                         */
    /*                                                                  */
    /*   The constructor is typically the first function, class         */
    /*   member functions appear in alphabetical order with the         */
    /*   destructor appearing at the end of the file.  Any section      */
    /*   or function this is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "LibraryPCH.hpp"

#ifndef DISABLE_DEBUG_HELP
	#if !defined(_XBOX) && !defined(_XENON)
		#include <dbghelp.h>
	#else
		#include <Xbdm.h>
	#endif
#endif
#include "CallStack.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Compiler options.                                              */
    /*                                                                  */
    /*   Ensure that the last function call(s) before 'StackWalk'       */
    /*   are not FPO-optimized.                                         */
    /*                                                                  */
    /********************************************************************/

#ifndef DISABLE_DEBUG_HELP
#ifndef _XENON
#pragma optimize("y", off)
#endif
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Constants local to the class.                                  */
    /*                                                                  */
    /*   The constants supplied here control the debug buffer size.     */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 MaxBufferSize			  = 4096;
CONST SBIT32 SymbolNameLength		  = 4096;
CONST SBIT32 MAX_WALKED_MODULES		  = 128;

static bool bModuleListWritten=false;

    /********************************************************************/
    /*                                                                  */
    /*   Static member initialization.                                  */
    /*                                                                  */
    /*   Static member initialization sets the initial value for all    */
    /*   static members.                                                */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CALL_STACK::Active = False;
SBIT32 CALL_STACK::Activations = 0;
#if !defined(_XENON) && !defined(_XBOX)
HANDLE CALL_STACK::Process = NULL;
#else 
SBIT32 CALL_STACK::NumModules;
DMN_MODLOAD	CALL_STACK::LoadedModules[MAX_WALKED_MODULES];
#endif
SPINLOCK CALL_STACK::Spinlock = NULL;




    /********************************************************************/
    /*                                                                  */
    /*   Xenon Implementation.                                          */
	/*                                                                  */
	/*  The description of the functions are described in PC version    */
	/*  Moved all of Xenon-Specific code due to large differences.      */
	/********************************************************************/

#if defined(_XBOX) || defined(_XENON)
CALL_STACK::CALL_STACK( VOID )
{
#if !DISABLE_DEBUG_HELP

#endif
}

SBIT32 CALL_STACK::GetCallStack(VOID *Frames[], SBIT32 MaxFrames, SBIT32 SkipFrames)
{
	REGISTER SBIT32 Count = 0; 
#if !DISABLE_DEBUG_HELP
	PVOID stackFrames[16];
	//Ensure thread safety
	Spinlock.ClaimLock();
	
	assert(_countof(stackFrames) >= MaxFrames+SkipFrames);
	//Capture the stack trace
	if (FAILED(DmCaptureStackBackTrace(MaxFrames+SkipFrames, stackFrames)))
		return 0;

	// Remove the skipped frames from the beginning

	for (;Count<MaxFrames && stackFrames[SkipFrames];++Count, ++SkipFrames)
		Frames[Count] = stackFrames[SkipFrames];

	Spinlock.ReleaseLock();

#endif
	return Count;
}


VOID CALL_STACK::FormatCallStack(CHAR *Buffer, VOID *Frames[], 
								 SBIT32 MaxBuffer, SBIT32 MaxFrames)
{
#if !DISABLE_DEBUG_HELP
	//
	//   We will load the modules if they are
	//   not already available and we try only once.
	//
	if ( ! Active && !Activations)
	{
		Activations ++;
		Active = UpdateSymbols();
	}
	
	if (Active) 
	{
		//
		//   Walk the stack frames extracting the
		//   details from each frame examined.
		//
		for (SBIT32 Count=0;Count < MaxFrames;Count ++ )
		{
			//
			//   Walk the each stack frame.
			//
			AUTO CHAR NewSymbol[ MaxBufferSize ];
			REGISTER SBIT32 Size;

			//
			//   Format the symbol.
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( FormatSymbol( Frames[ Count ],NewSymbol,MaxBufferSize ) &&
					((Size = strlen( NewSymbol )) + 1) < MaxBuffer)
			{
				//
				//   Copy the symbol into the buffer.
				//
				strcpy( Buffer,NewSymbol );
				Buffer += Size;

				strcpy( Buffer ++,"\n" );
				MaxBuffer -= (Size + 1);
			}
			else
			{ break; }
		}
	}
	else
		{ Buffer[0]='\0';}
#endif
}

#ifndef DISABLE_DEBUG_HELP
BOOLEAN CALL_STACK::FormatSymbol(VOID *Address, CHAR *Buffer, SBIT32 MaxBuffer)
{
	SBIT32 Size;
	assert(MaxBuffer);
	assert(Buffer);
	assert(Address);
	// Initialize the buffer in case there's not enough space for anything
	*Buffer = '\0';
	for (SBIT32 Count =0; Count < NumModules; ++Count)
	{
		DMN_MODLOAD Module = LoadedModules[Count];
		VOID *Start = Module.BaseAddress;
		VOID *End = ((char*)Start) + Module.Size;

		// Check that the address is in this module
		if (Address >= Start && Address < End)
		{
			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( ((Size = strlen( Module.Name )) + 25) < MaxBuffer)
			{
				strcpy( Buffer,Module.Name );
				Buffer += Size;

				// And now write the full address for looking up within the stack
				sprintf_s(Buffer,25,"@0x%8p(+0x%8p)", Address,((char*)Address-(char*)Start));
			}
			return true;
		}
	}
	if ( ((Size = strlen( "Unknown!" ))+16) < MaxBuffer)
	{
		//
		//   Copy the empty name into the buffer.
		//
		strcpy( Buffer,"Unknown!" );
		Buffer += Size;
		MaxBuffer -= Size;
		sprintf_s(Buffer,16,"@0x%12p", Address);
		return true;
	}

	return false;
}

BOOL STDCALL CALL_STACK::UpdateSymbolCallback(PSTR Module, ULONG_PTR BaseOfDLL, ULONG SizeOfDLL, VOID *Context)
{
	return true;
}

#endif

BOOLEAN CALL_STACK::UpdateSymbols( VOID )
{
	REGISTER BOOLEAN Result = True;
#ifndef DISABLE_DEBUG_HELP
	//
	//  Claim a lock to prevent multiple threads
	//  from initalizing the module walking. It's still possible for that
	//	to happen if multiple threads call this, but at least it'd be consistent.
	//
	Spinlock.ClaimLock();

	PDM_WALK_MODULES pContext = NULL;

	NumModules = 0;
	HRESULT hr;
	while ( NumModules < MAX_WALKED_MODULES && 
			XBDM_NOERR == (hr = DmWalkLoadedModules( &pContext, &LoadedModules[NumModules++] ))  );
	if(NumModules>0)
	{
		--NumModules;	// last increment was for failed enumeration
	}
	
	// We have gotten every module if we filled the full structure or reached the end
	Result = ((XBDM_ENDOFLIST==hr) || NumModules == MAX_WALKED_MODULES);

	if (FAILED(DmCloseLoadedModules( pContext )))
		Failure("Unable to close loaded modules\n");

	//We've now activated and everything from here is thread safe
	Spinlock.ReleaseLock();
#endif
	return Result;
}

// helper - copy into buffer
BOOLEAN CALL_STACK::SafeBufferAdd(CHAR **pszDest, CONST CHAR *szSource, CHAR *szStart, SBIT32 BufferLength)
{
	BOOLEAN succeeded=false;

	int nSourceLength=strlen(szSource);
	if(BufferLength > ((*pszDest-szStart) + nSourceLength))
	{
		strncpy_s(*pszDest, BufferLength-((int)(*pszDest)-(int)szStart), szSource, nSourceLength); // terminates
		*pszDest=*pszDest+nSourceLength;
		succeeded=true;
	}
	return succeeded;
}


BOOLEAN CALL_STACK::ModuleList(CHAR *Buffer, SBIT32 BufferLength)
{
	BOOLEAN succeeded=false;
	if(!bModuleListWritten)
	{
		bModuleListWritten=true; // only one copy of module list in log

		const int localBufferSize=4096;
		CHAR *szCursor = Buffer;
		CHAR moduleBuffer[localBufferSize];

		// header
		SafeBufferAdd(&szCursor, "\nModules:\n", Buffer, BufferLength);
		SafeBufferAdd(&szCursor, "\n", Buffer, BufferLength);
		SafeBufferAdd(&szCursor, "Module list start (Name, BaseAddress, Size)\n", Buffer, BufferLength);

		for (SBIT32 j=0; j < NumModules; ++j)
		{
			DMN_MODLOAD Module = LoadedModules[j];
			sprintf_s(moduleBuffer, localBufferSize, "%s 0x%08x 0x%08x\n", Module.Name, Module.BaseAddress, Module.Size);
			SafeBufferAdd(&szCursor, moduleBuffer, Buffer, BufferLength);
		}
		SafeBufferAdd(&szCursor, "Module list end\n\n", Buffer, BufferLength);
		succeeded=true;
	}
	return succeeded;
}


CALL_STACK::~CALL_STACK( VOID )
{
	Spinlock.ClaimLock();
	//Set all the variables to zero.
	if ( ((-- Activations) == 0) && (Active) )
	{
		Active = False;
		NumModules = 0;
	}
	Spinlock.ReleaseLock();
}


#else //End of Xenon-specific code

    /********************************************************************/
    /*                                                                  */
    /*   Class constructor.                                             */
    /*                                                                  */
    /*   Create a call stack class and initialize it.  This call is     */
    /*   not thread safe and should only be made in a single thread     */
    /*   environment.                                                   */
    /*                                                                  */
    /********************************************************************/

CALL_STACK::CALL_STACK( VOID )
    {
	//
	//   Claim a lock to prevent multiple threads
	//   from using the symbol lookup mechanism.
	//
	Spinlock.ClaimLock();

#ifndef DISABLE_DEBUG_HELP

	//
	//   We will activate the symbols if they are
	//   not already available.
	//
	if ( ! Active )
		{
		//
		//   Setup the process handle, load image help  
		//   and then load any available symbols.
		//
		Process = GetCurrentProcess();

		//
		//   Setup the image help library.
		//
		if ( ! (Active = ((BOOLEAN) SymInitialize( Process,NULL,TRUE ))) )
			{
			//
			//   We only issue the warning message once
			//   when we fail to load the symbols.
			//
			if ( Activations == 0 )
				{
				//
				//   Format the error message and output it
				//   to the debug stream.
				//
				DebugPrint
					(
					"Missing or mismatched symbols files: %x\n",
					HRESULT_FROM_WIN32( GetLastError() )
					);
				}
			}
		}

	//
	//   We keep track of the number of activations
	//   so we can delete the symbols at the
	//   required point.
	//
	Activations ++;

#endif
	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();

	//
	//   Update the available symbols.
	//
	UpdateSymbols();
    }

    /********************************************************************/
    /*                                                                  */
    /*   Extract the current call stack.                                */
    /*                                                                  */
    /*   Extract the current call stack and return it to the caller     */
    /*   so it can be used later.                                       */
    /*                                                                  */
    /********************************************************************/

SBIT32 CALL_STACK::GetCallStack
		(
		VOID						  *Frames[],
        SBIT32						  MaxFrames,
        SBIT32						  SkipFrames
		)
    {
	REGISTER SBIT32 Count = 0;

#ifndef DISABLE_DEBUG_HELP
	//
	//   We can only examine the symbol information if
	//   we were able to load image help.
	//
	if ( Active )
		{
		REGISTER CONTEXT Context;
		REGISTER HANDLE Thread;
		REGISTER SBIT32 MachineType;
		REGISTER STACKFRAME StackFrame;

		//
		//   Zero all the data structures to make
		//   sure they are clean.
		//
		ZEROMEMORY( & Context,sizeof(CONTEXT) );
		ZEROMEMORY( & StackFrame,sizeof(STACKFRAME) );

		//
		//   Setup the necessary flags and extract
		//   the thread context.
		//
		Context.ContextFlags = CONTEXT_FULL;
		MachineType = IMAGE_FILE_MACHINE_I386;
		Thread = GetCurrentThread();

		GetThreadContext( Thread,& Context );

		//
		//   Extract the details of the current
		//   stack frame.
		//
		_asm
			{
				mov StackFrame.AddrStack.Offset, esp
				mov StackFrame.AddrFrame.Offset, ebp
				mov StackFrame.AddrPC.Offset, offset DummyLabel
			DummyLabel:
			}

		StackFrame.AddrPC.Mode = AddrModeFlat;
		StackFrame.AddrStack.Mode = AddrModeFlat;
		StackFrame.AddrFrame.Mode = AddrModeFlat;

		//
		//   Claim a lock to prevent multiple threads
		//   from using the symbol lookup mechanism.
		//
		Spinlock.ClaimLock();

		//
		//   Walk the stack frames extracting the
		//   details from each frame examined.
		//
		while ( Count < MaxFrames )
			{
			//
			//   Walk the each stack frame.
			//
			if 
					(
					StackWalk
						(
						MachineType,		   
						Process,		   
						Thread,		   
						& StackFrame,
						& Context,
						NULL,
						SymFunctionTableAccess,
						SymGetModuleBase,
						NULL
						)
					)
				{
				//
				//   Examine and process the current 
				//   stack frame.
				//
				if ( SkipFrames <= 0 )
					{ 
					//
					//   Collect the current function
					//   address and store it.
					//
					Frames[ (Count ++) ] = 
						((VOID*) StackFrame.AddrPC.Offset); 
					}
				else
					{ SkipFrames --; }
				}
			else
				{ break; }
			}

		//
		//   Release the lock.
		//
		Spinlock.ReleaseLock();
		}

#endif
	return Count;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Format a call stack.                                           */
    /*                                                                  */
    /*   We format an entire call stack into a single string ready      */
    /*   for output.                                                    */
    /*                                                                  */
    /********************************************************************/

VOID CALL_STACK::FormatCallStack
		(
		CHAR						  *Buffer, 
		VOID						  *Frames[], 
		SBIT32						  MaxBuffer, 
		SBIT32						  MaxFrames 
		)
    {
#if !DISABLE_DEBUG_HELP
	//
	//   We can only examine the symbol information if
	//   we were able to load image help.
	//
	if ( Active )
		{
		REGISTER SBIT32 Count;

		//
		//   Delete any existing string.
		//
		strcpy( Buffer,"" );

		//
		//   Format each frame and then update the
		//   main buffer.
		//
		for ( Count=0;Count < MaxFrames;Count ++ )
			{
			AUTO CHAR NewSymbol[ MaxBufferSize ];
			REGISTER SBIT32 Size;

			//
			//   Format the symbol.
			//
			FormatSymbol( Frames[ Count ],NewSymbol,MaxBufferSize );

			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( ((Size = strlen( NewSymbol )) + 1) < MaxBuffer)
				{
				//
				//   Copy the symbol into the buffer.
				//
				strcpy( Buffer,NewSymbol );
				Buffer += Size;

				strcpy( Buffer ++,"\n" );

				MaxBuffer -= (Size + 1);
				}
			else if(MaxBuffer>1) // truncate - append newline
				{ 
					Size = MaxBuffer-2;
					strncpy(Buffer, NewSymbol, Size);
					Buffer[Size]='\n';
					Buffer+=(Size+1);
					MaxBuffer-=(Size+1);
				}
			}
		}
	else
		{ strcpy( Buffer,"" ); }
#else
	strcpy( Buffer,"" );
#endif
    }
#ifndef DISABLE_DEBUG_HELP

    /********************************************************************/
    /*                                                                  */
    /*   Format a single symbol.                                        */
    /*                                                                  */
    /*   We format a single simple converting it from an address to     */
    /*   a text string.                                                 */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CALL_STACK::FormatSymbol
		(
		VOID						  *Address,
        CHAR						  *Buffer,
        SBIT32						  MaxBuffer
		)
    {
    AUTO CHAR SymbolBuffer[ (sizeof(IMAGEHLP_SYMBOL) + SymbolNameLength) ];
    AUTO IMAGEHLP_MODULE Module = { 0 };
    REGISTER BOOLEAN Result = True;
    REGISTER PIMAGEHLP_SYMBOL Symbol = ((PIMAGEHLP_SYMBOL) SymbolBuffer);   

	//
	//   Setup values ready for main symbol
	//   extraction function body.
	//
    Module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

    ZEROMEMORY( Symbol,(sizeof(IMAGEHLP_SYMBOL) + SymbolNameLength) );

    Symbol -> SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    Symbol -> MaxNameLength = SymbolNameLength;

	//
	//   Claim a lock to prevent multiple threads
	//   from using the symbol lookup mechanism.
	//
	Spinlock.ClaimLock();

	//
	//   Extract the module information for the
	//   symbol and format it.
	//
    if ( SymGetModuleInfo( Process,((DWORD) Address),& Module ) )
		{
		REGISTER SBIT32 Size;

		//
		//   Make sure there is enough space in the
		//   output buffer.
		//
        if ( ((Size = strlen( Module.ModuleName )) + 1) < MaxBuffer)
			{
			//
			//   Copy the module name into the buffer.
			//
            strcpy( Buffer,Module.ModuleName );
			Buffer += Size;

            strcpy( Buffer ++,"!" );

			MaxBuffer -= (Size + 1);
			}
		}
    else
		{
		REGISTER SBIT32 Size;

		//
		//   Make sure there is enough space in the
		//   output buffer.
		//
        if ( (Size = strlen( "None!" )) < MaxBuffer)
			{
			//
			//   Copy the module name into the buffer.
			//
            strcpy( Buffer,"None!" );
			Buffer += Size;
			MaxBuffer -= Size;
			}

		//
		//  We failed to extract the module name.
		//
		Result = False;
		}

	//
	//   We will not even bother to try to decode
	//   the symbol if we can't decode the module.
	//
    if ( Result )
		{
		AUTO CHAR SymbolName[ SymbolNameLength ];
		AUTO DWORD Offset = 0;





		//
		//   Try to convert the symbol from an
		//   address to a name.
		//
        if
				(
				SymGetSymFromAddr
					(
					Process,
					(DWORD)Address,
					& Offset,
					Symbol
					)
				)
	        {
			REGISTER SBIT32 Size;

			//
			//   Try to undecorate the name.  If
			//   this fails just use the decorated
			//   name is it is better than nothing.
			//
            if ( ! SymUnDName( Symbol,SymbolName,sizeof(SymbolName) ) )
				{ lstrcpynA( SymbolName,& Symbol->Name[1],sizeof(SymbolName) ); }

			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( (Size = strlen( SymbolName )) < MaxBuffer)
				{
				//
				//   Copy the symbol name into the buffer.
				//
				strcpy( Buffer,SymbolName );
				Buffer += Size;
				MaxBuffer -= Size;
	            }
			
			//
			//   Format the offset if is is non-zero.
			//
			if ( Offset != 0 )
				{
				//
				//   Format the symbol offset.
				//
				sprintf( SymbolName,"+0x%x",Offset );

				//
				//   Make sure there is enough space in the
				//   output buffer.
				//
				if ( (Size = strlen( SymbolName )) < MaxBuffer)
					{
					//
					//   Copy the symbol name into the buffer.
					//
					strcpy( Buffer,SymbolName );
					Buffer += Size;
					MaxBuffer -= Size;
					}
				}

				// function name, filename, line numbers
				IMAGEHLP_LINE line;
				char fname[MAX_PATH] = {0};
				line.SizeOfStruct = sizeof(line);
				line.FileName = fname;
				DWORD dwLineDisplacement;
				if( SymGetLineFromAddr( Process, (DWORD)Address, &dwLineDisplacement, &line ) )
				{
					BufferAppend(&Buffer, &MaxBuffer, " ");
					BufferAppend(&Buffer, &MaxBuffer, line.FileName, "(no filename)");
					BufferAppend(&Buffer, &MaxBuffer, " ");
					BufferAppend(&Buffer, &MaxBuffer, line.LineNumber);
				}
				else
				{
					BufferAppend(&Buffer, &MaxBuffer, " SymGetLineFromAddr Error: ");
					BufferAppend(&Buffer, &MaxBuffer, GetLastError());
				}
	        }
        else
	        {
			REGISTER SBIT32 Size;

			//
			//   Format the symbol address.
			//
            sprintf( SymbolName,"0x%p",Address );

			//
			//   Make sure there is enough space in the
			//   output buffer.
			//
			if ( (Size = strlen( SymbolName )) < MaxBuffer)
				{
				//
				//   Copy the symbol name into the buffer.
				//
				strcpy( Buffer,SymbolName );
				Buffer += Size;
				MaxBuffer -= Size;
	            }

 			//
			//  We failed to extract the symbol name.
			//
           Result = False;
	       }
		}
    else
		{
		AUTO CHAR SymbolName[ SymbolNameLength ];
		REGISTER SBIT32 Size;

		//
		//   Format the symbol address.
		//
        sprintf( SymbolName,"0x%p",Address );

		//
		//   Make sure there is enough space in the
		//   output buffer.
		//
		if ( (Size = strlen( SymbolName )) < MaxBuffer)
			{
			//
			//   Copy the symbol name into the buffer.
			//
			strcpy( Buffer,SymbolName );
			Buffer += Size;
			MaxBuffer -= Size;
	        }
		}

	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();

    return Result;
	}

    /********************************************************************/
    /*                                                                  */
    /*   Load symbols callback.                                         */
    /*                                                                  */
    /*   When we load the symbols we get a callback for every module    */
    /*   that is currently loaded into the application.                 */
    /*                                                                  */
    /********************************************************************/



// helper
// Does nothing if insufficient space remaining
void BufferAppend(char **pszBuffer, SBIT32 *pnRemaining, char *sz)
{
	int nLength = strlen(sz);
	if(nLength<*pnRemaining)
	{
		strcpy(*pszBuffer, sz);  // todo: safestrings everywhere
		*pszBuffer+=nLength;
		*pnRemaining-=nLength;
	}
	else
	{
		nLength = (*pnRemaining)-1;
		strncpy(*pszBuffer, sz, nLength); // truncated
		(*pszBuffer)[nLength]=0;  // termination
		*pszBuffer+=(nLength+1);
		*pnRemaining-=(nLength+1);
	}
	return;
}
void BufferAppend(char **pszBuffer, SBIT32 *pnRemaining, int n)
{
	char szIntBuffer[32];
	sprintf(szIntBuffer, "%d", n);
	return BufferAppend(pszBuffer, pnRemaining, szIntBuffer);
}
void BufferAppend(char **pszBuffer, SBIT32 *pnRemaining, char *sz, char *szDefault)
{
	if(0==strlen(sz))
	{
		BufferAppend(pszBuffer, pnRemaining, szDefault);
	}
	else
	{
		BufferAppend(pszBuffer, pnRemaining, sz);
	}
	return;
}




BOOL STDCALL CALL_STACK::UpdateSymbolCallback
		(
		PSTR						  Module,
        ULONG_PTR					  BaseOfDLL,
        ULONG						  SizeOfDLL,
        VOID						  *Context
		)
    {
	if ( SymGetModuleBase( Process,BaseOfDLL ) == 0 )
		{ SymLoadModule( Process,NULL,Module,NULL,BaseOfDLL,SizeOfDLL ); }

	return TRUE;
    }
#endif

    /********************************************************************/
    /*                                                                  */
    /*   Load the symbols.                                              */
    /*                                                                  */
    /*   Load the symbols for the current process so we can translate   */
    /*   code addresses into names.                                     */
    /*                                                                  */
    /********************************************************************/

BOOLEAN CALL_STACK::UpdateSymbols( VOID )
    {
	REGISTER BOOLEAN Result = True;
#ifndef DISABLE_DEBUG_HELP
	//
	//   We can only examine the symbol information if
	//   we were able to load image help.
	//
	if ( Active )
		{
		//
		//   Claim a lock to prevent multiple threads
		//   from using the symbol lookup mechanism.
		//
		Spinlock.ClaimLock();

		//
		//   Enumaerate all of the loaded modules and
		//   cascade load all of the symbols.
		//
		if ( ! EnumerateLoadedModules( Process,UpdateSymbolCallback,NULL ) )
			{
			//
			//   Format the error message and output it
			//   to the debug window.
			//
			DebugPrint
				(
				"EnumerateLoadedModules returned: %x\n",
				HRESULT_FROM_WIN32( GetLastError() )
				);

			Result = False;
			}

		//
		//   Release the lock.
		//
		Spinlock.ReleaseLock();
		}
#endif

	return Result;
    }

    /********************************************************************/
    /*                                                                  */
    /*   Class destructor.                                              */
    /*                                                                  */
    /*   Destory the call stack.  This call is not thread safe and      */
    /*   should only be made in a single thread environment.            */
    /*                                                                  */
    /********************************************************************/

CALL_STACK::~CALL_STACK( VOID )
	{ 
	//
	//   Claim a lock to prevent multiple threads
	//   from using the symbol lookup mechanism.
	//
	Spinlock.ClaimLock();

#ifndef DISABLE_DEBUG_HELP
	//
	//   Cleanup the symbol library.
	//
	if ( ((-- Activations) == 0) && (Active) )
		{
		Active = False;

		//
		//   I don't understand why this does not work at
		//   the moment so I will fix it later.
		//
		// SymCleanup( Process ); 

		//
		//   Just to be neat lets zero everything.
		//
		Process = NULL;
		}

#endif
	//
	//   Release the lock.
	//
	Spinlock.ReleaseLock();
	}

BOOLEAN CALL_STACK::ModuleList(CHAR *Buffer, SBIT32 BufferLength)
{
	*Buffer = NULL;
	return false;
}

#endif //end of _XENON