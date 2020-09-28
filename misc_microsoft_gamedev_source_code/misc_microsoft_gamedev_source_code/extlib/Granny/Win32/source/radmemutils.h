#ifndef __RADMEMUTILSH__
  #define __RADMEMUTILSH__

  #ifndef __RADBASEH__
    #include "radbase.h"
  #endif

  RADDEFSTART


  /* 32 bit implementations */

  #ifdef __RAD32__

    #if defined(__RADPS2__)

      #define radstrcat strcat

      #define radstrcpy   strcpy

      #define radstrlen   strlen

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

//      #define radmemcpydb memmove
//      #define radmemcpy memcpy
//      #define radmemset memset
      
      void radmemcpydb( void * dest, void const * src, U32 bytes );
      void radmemcpy( void * dest, void const * src, U32 bytes );
      void radmemset( void * dest, U8 val, U32 bytes );

    #elif defined(__RADPSP__)

      #include <libvfpu.h> 
      
      #define radstrcat strcat

      #define radstrcpy strcpy

      #define radstrlen strlen

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

      #define radmemcpydb memmove
      #define radmemcpy sceVfpuMemcpy
      #define radmemset sceVfpuMemset

    #elif defined(__RADPS3__)

      #define radstrcat strcat

      #define radstrcpy strcpy

      #define radstrlen strlen

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

      #define radmemcpydb memmove
      #define radmemcpy memcpy
      #define radmemset memset

    #elif defined(__RADSPU__)

      #include <string.h>

      #define radstrcat strcat

      #define radstrcpy strcpy

      #define radstrlen strlen

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

      #define radmemcpydb memmove
      #define radmemcpy memcpy
      #define radmemset memset

    #elif defined(__RADNGC__) 

      #include <string.h>

      #define radmemcpy   memcpy

      #define radstrcat strcat

      #define radstrcpy   strcpy

      #define radstrlen   strlen

      #define radmemset   memset

      #define radmemcpydb memmove

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

    #elif defined(__RADWII__) 

      #include <string.h>

      #define radmemcpy   memcpy

      #define radstrcat strcat

      #define radstrcpy   strcpy

      #define radstrlen   strlen

      #define radmemset   memset

      #define radmemcpydb memmove

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

    #elif defined(__RADNDS__)

      #define radmemcpy   memcpy

      #define radstrcat strcat

      #define radstrcpy   strcpy

      #define radstrlen   strlen

      #define radmemset   memset

      #define radmemcpydb memmove

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

    #elif defined(__RADXENON__) 

      #define radmemcpy   memcpy

      #define radstrcat strcat

      #define radstrcpy   strcpy

      #define radstrlen   strlen

      #define radmemset   memset

      #define radmemcpydb memmove

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

    #elif defined(__RADMAC__) && !defined(__RADX86__)
      #include <string.h>   

      #define radstrcpy strcpy

      #define radstrcat strcat

      #define radmemcpy(dest,source,size) memcpy(dest,source,size)
      //BlockMoveData((Ptr)(source),(Ptr)(dest),size)

      #define radmemcpydb(dest,source,size) memmove(dest,source,size)
      //BlockMoveData((Ptr)(source),(Ptr)(dest),size)

      #define radmemcmp memcmp

      #define radmemset memset

      #define radstrlen strlen

      #define radstrcmp strcmp

      void radmemset16(void* dest,U16 value,U32 count_w);

    #else

      #ifdef __WATCOMC__

        void radmemset16(void* dest,U16 value,U32 count_w);
        #pragma aux radmemset16 = "cld" "mov bx,ax" "shl eax,16" "mov ax,bx" "mov bl,cl" "shr ecx,1" "rep stosd" "mov cl,bl" "and cl,1" "rep stosw" parm [EDI] [EAX] [ECX] modify [EAX EDX EBX ECX EDI];

        void radmemset(void* dest,U8 value,U32 size);
        #pragma aux radmemset = "cld" "mov ah,al" "mov bx,ax" "shl eax,16" "mov ax,bx" "mov bl,cl" "shr ecx,2" "and bl,3" "rep stosd" "mov cl,bl" "rep stosb" parm [EDI] [AL] [ECX] modify [EAX EDX EBX ECX EDI];

        void radmemset32(void* dest,U32 value,U32 count_d);
        #pragma aux radmemset32 = "cld" "rep stosd" parm [EDI] [EAX] [ECX] modify [EAX EDX EBX ECX EDI];

        void radmemcpy(void* dest,const void* source,U32 size);
        #pragma aux radmemcpy = "cld" "mov bl,cl" "shr ecx,2" "rep movsd" "mov cl,bl" "and cl,3" "rep movsb" parm [EDI] [ESI] [ECX] modify [EBX ECX EDI ESI];

        void __far *radfmemcpy(void __far* dest,const void __far* source,U32 size);
        #pragma aux radfmemcpy = "cld" "push es" "push ds" "mov es,cx" "mov ds,dx" "mov ecx,eax" "shr ecx,2" "rep movsd" "mov cl,al" "and cl,3" "rep movsb" "pop ds" "pop es" parm [CX EDI] [DX ESI] [EAX] modify [ECX EDI ESI] value [CX EDI];

        void radmemcpydb(void* dest,const void* source,U32 size);  //Destination bigger
        #pragma aux radmemcpydb = "std" "mov bl,cl" "lea esi,[esi+ecx-4]" "lea edi,[edi+ecx-4]" "shr ecx,2" "rep movsd" "and bl,3" "jz dne" "add esi,3" "add edi,3" "mov cl,bl" "rep movsb" "dne:" "cld" parm [EDI] [ESI] [ECX] modify [EBX ECX EDI ESI];

        char* radstrcpy(void* dest,const void* source);
        #pragma aux radstrcpy = "cld" "mov edx,edi" "lp:" "mov al,[esi]" "inc esi" "mov [edi],al" "inc edi" "cmp al,0" "jne lp" parm [EDI] [ESI] modify [EAX EDX EDI ESI] value [EDX];

        char __far* radfstrcpy(void __far* dest,const void __far* source);
        #pragma aux radfstrcpy = "cld" "push es" "push ds" "mov es,cx" "mov ds,dx" "mov edx,edi" "lp:" "lodsb" "stosb" "test al,0xff" "jnz lp" "pop ds" "pop es" parm [CX EDI] [DX ESI] modify [EAX EDX EDI ESI] value [CX EDX];

        char* radstpcpy(void* dest,const void* source);
        #pragma aux radstpcpy = "cld" "lp:" "mov al,[esi]" "inc esi" "mov [edi],al" "inc edi" "cmp al,0" "jne lp" "dec edi" parm [EDI] [ESI] modify [EAX EDI ESI] value [EDI];

        char* radstpcpyrs(void* dest,const void* source);
        #pragma aux radstpcpyrs = "cld" "lp:" "mov al,[esi]" "inc esi" "mov [edi],al" "inc edi" "cmp al,0" "jne lp" "dec esi" parm [EDI] [ESI] modify [EAX EDI ESI] value [ESI];

        U32 radstrlen(const void* dest);
        #pragma aux radstrlen = "cld" "mov ecx,0xffffffff" "xor eax,eax" "repne scasb" "not ecx" "dec ecx" parm [EDI] modify [EAX ECX EDI] value [ECX];

        char* radstrcat(void* dest,const void* source);
        #pragma aux radstrcat = "cld" "mov ecx,0xffffffff" "mov edx,edi" "xor eax,eax" "repne scasb" "dec edi" "lp:" "lodsb" "stosb" "test al,0xff" "jnz lp" \
        parm [EDI] [ESI] modify [EAX ECX EDI ESI] value [EDX];

        char* radstrchr(const void* dest,char chr);
        #pragma aux radstrchr = "cld" "lp:" "lodsb" "cmp al,dl" "je fnd" "cmp al,0" "jnz lp" "mov esi,1" "fnd:" "dec esi" parm [ESI] [DL] modify [EAX ESI] value [esi];

        S8 radmemcmp(const void* s1,const void* s2,U32 len);
        #pragma aux radmemcmp = "cld" "rep cmpsb" "setne al" "jbe end" "neg al" "end:"  parm [EDI] [ESI] [ECX] modify [ECX EDI ESI];

        S8 radstrcmp(const void* s1,const void* s2);
        #pragma aux radstrcmp = "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,ah" "jne set" "cmp al,0" "je set" "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" \
        parm [EDI] [ESI] modify [EAX EDI ESI];

        S8 radstricmp(const void* s1,const void* s2);
        #pragma aux radstricmp = "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub al,32" "c1:" "cmp ah,'a'" "jb c2" "cmp ah,'z'" "ja c2" "sub ah,32" "c2:" "cmp al,ah" "jne set" "cmp al,0" "je set" \
       "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" \
        parm [EDI] [ESI] modify [EAX EDI ESI];

        S8 radstrnicmp(const void* s1,const void* s2,U32 len);
        #pragma aux radstrnicmp = "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub al,32" "c1:" "cmp ah,'a'" "jb c2" "cmp ah,'z'" "ja c2" "sub ah,32" "c2:" "cmp al,ah" "jne set" "cmp al,0" "je set" \
        "dec ecx" "jz set" "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" \
        parm [EDI] [ESI] [ECX] modify [EAX ECX EDI ESI];

        char* radstrupr(void* s1);
        #pragma aux radstrupr = "mov ecx,edi" "lp:" "mov al,[edi]" "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub [edi],32" "c1:" "inc edi" "cmp al,0" "jne lp" parm [EDI] modify [EAX EDI] value [ecx];

        char* radstrlwr(void* s1);
        #pragma aux radstrlwr = "mov ecx,edi" "lp:" "mov al,[edi]" "cmp al,'A'" "jb c1" "cmp al,'Z'" "ja c1" "add [edi],32" "c1:" "inc edi" "cmp al,0" "jne lp" parm [EDI] modify [EAX EDI] value [ecx];

        U32 radstru32(const void* dest);
          #pragma aux radstru32 = "cld" "xor ecx,ecx" "xor ebx,ebx" "xor edi,edi" "lodsb" "cmp al,45" "jne skip2" "mov edi,1" "jmp skip" "lp:" "mov eax,10" "mul ecx" "lea ecx,[eax+ebx]" \
          "skip:" "lodsb" "skip2:" "cmp al,0x39" "ja dne" "cmp al,0x30" "jb dne" "mov bl,al" "sub bl,0x30" "jmp lp" "dne:" "test edi,1" "jz pos" "neg ecx" "pos:" \
          parm [ESI] modify [EAX EBX EDX EDI ESI] value [ecx];

        U16 GetDS();
        #pragma aux GetDS = "mov ax,ds" value [ax];

        #ifdef __RADWINEXT__

          #define _16To32(ptr16) ((void*)(((GetSelectorBase((U16)(((U32)(ptr16))>>16))+((U16)(U32)(ptr16)))-GetSelectorBase(GetDS()))))

        #endif

        #ifndef __RADWIN__
          #define int86 int386
          #define int86x int386x
        #endif

        #define u32regs x
        #define u16regs w

      #elif defined(__RADX64__)

        #define radstrcpy strcpy
        #define radstrcat strcat
        #define radmemcpy memcpy
        #define radmemcmp memcmp
        #define radmemset memset
        #define radstrlen strlen
        #define radstrchr strchr
        #define radtoupper toupper
        #define radstru32(s) ((U32)atol(s))
        #define radstrcmp strcmp
        #define radstricmp _stricmp
        #define radstrupr _strupr
        #define radstrlwr _strlwr

        extern void *memset ( void*, int, size_t );
        extern void *memcpy ( void*, const void*, size_t );
        #pragma intrinsic(memset,memcpy)

        #define radmemcpydb memmove

        static void __inline radmemset16(void* dest,U16 value,U32 count_w)
        {
          U32 v = value | ( ( (U32) value ) << 16 );
          U32 s = count_w >> 1;
          U32 * d = (U32*)dest;
          while ( s )
          {
            --s;
            *d++=v;
          }
          if ( count_w & 1 )
          {
            *(U16*)d = value;
          }
        }


        static char* radstpcpy(char* p1, char* p2) 
        {
          char c;
          do
          {
            c = *p2++;
            *p1++ = c;
          } while (c);
          return( p1 - 1 );
        }
        
      #else

        #define radstrcpy strcpy
        #define radstrcat strcat
        #define radmemcpy memcpy
        #define radmemcmp memcmp
        #define radmemset memset
        #define radstrlen strlen
        #define radstrchr strchr
        #define radtoupper toupper
        #define radstru32(s) ((U32)atol(s))
        #define radstrcmp strcmp
        #define radstrupr _strupr
        #define radstrlwr _strlwr

        #if defined(_MSC_VER) || (defined(__RADMAC__) && defined(__RADX86__))

#ifdef __RADXBOX__
#ifndef _XBOX_
unsigned int __stdcall XQueryMemoryProtect( void * addr );
#define PAGE_WRITECOMBINE 0x400
#endif
void __stdcall OutputDebugStringA( char const * lpOutputString );
#define OutputDebugString  OutputDebugStringA
#endif
  
        #if defined(_MSC_VER)
          #pragma warning( disable : 4035)
          extern void *memset ( void*, int, unsigned int );
          extern void *memcpy ( void*, const void*, unsigned int );
          #pragma intrinsic(memset,memcpy)
        #else
          #pragma warning( disable : 1011 )
        #endif

          typedef char* RADPCHAR;

          static void __inline radmemcpydb( void const * dest, void const * src, U32 bytes )
          {
            __asm
            {
              mov ecx,dword ptr [bytes]
              mov edi,dword ptr [dest]
              mov esi,dword ptr [src]
              std
              mov edx,ecx
              lea esi,[esi+ecx-4]
              lea edi,[edi+ecx-4]
              shr ecx,2
              rep movsd
              and edx,3
              jz dne
              add esi,3
              add edi,3
              mov ecx,edx
              rep movsb
              dne:
              cld
            }
          }

          static int __inline radstricmp( void const * s1, void const * s2 )
          {
            __asm
            {
              mov edi,dword ptr [s1]
              mov esi,dword ptr [s2]

              mov eax,1 //skips the first matched check

              zc:
              cmp eax,0
              je matched

              lp:
              movzx eax,byte ptr [edi]
              movzx edx,byte ptr [esi]
              inc esi
              inc edi
              cmp eax,edx
              je zc

              cmp eax,'a'
              jb c1
              cmp eax,'z'
              ja c1
              sub eax,32
              c1:
              cmp edx,'a'
              jb c2
              cmp edx,'z'
              ja c2
              sub edx,32
              c2:

              sub eax,edx
              je lp

              cdq
              lea eax,[edx+edx+1]

              matched:
            }
          }
          
          static RADPCHAR __inline radstpcpy(char* p1, char* p2) {
            __asm {
               mov edx,dword ptr [p1] 
               mov ecx,dword ptr [p2]
               cld
              lp:
               mov al,[ecx] 
               inc ecx 
               mov [edx],al
               inc edx
               cmp al,0 
               jne lp 
               dec edx
               mov eax,edx
            } 
          }

          static RADPCHAR __inline radstpcpyrs(char* p1, char* p2) {
            __asm {
              mov edx,dword ptr [p1]
              mov ecx,dword ptr [p2]
              cld
             lp:
              mov al,[ecx]
              inc ecx
              mov [edx],al
              inc edx
              cmp al,0 
              jne lp
              dec ecx
              mov eax,ecx 
            }
          }

          static void __inline radmemset16(void* dest,U16 value,U32 count_w) {
            __asm {
              mov edi,dword ptr [dest]
              mov ax,word ptr [value] 
              mov ecx,dword ptr [count_w]
              shl eax,16 
              cld 
              mov ax,word ptr [value] 
              mov bl,cl 
              shr ecx,1 
              rep stosd
              mov cl,bl 
              and cl,1 
              rep stosw
            }
          }

          static void __inline radmemset32(void* dest,U32 value,U32 count_d) {
            __asm {
              mov edi,dword ptr [dest]
              mov eax,dword ptr [value]
              mov ecx,dword ptr [count_d]
              cld
              rep stosd
            }
          }

        #if defined(_MSC_VER)
          #pragma warning( default : 4035)
        #else
          #pragma warning( default : 1011 )
        #endif

        #endif

      #endif

    #endif

  #else

    #ifdef __WATCOMC__

      void radmemset(void far *dest,U8 value,U32 size);
      #pragma aux radmemset = "cld" "and edi,0ffffh" "shl ecx,16" "mov cx,bx" "mov ah,al" "mov bx,ax" "shl eax,16" "mov ax,bx" "mov bl,cl" "shr ecx,2" 0x67 "rep stosd" "mov cl,bl" "and cl,3" "rep stosb" parm [ES DI] [AL] [CX BX];

      void radmemset16(void far* dest,U16 value,U32 size);
      #pragma aux radmemset16 = "cld" "and edi,0ffffh" "shl ecx,16" "mov cx,bx" "mov bx,ax" "shl eax,16" "mov ax,bx" "mov bl,cl" "shr ecx,1" "rep stosd" "mov cl,bl" "and cl,1" "rep stosw" parm [ES DI] [AX] [CX BX];

      void radmemcpy(void far* dest,const void far* source,U32 size);
      #pragma aux radmemcpy = "cld" "push ds" "mov ds,dx" "and esi,0ffffh" "and edi,0ffffh" "shl ecx,16" "mov cx,bx" "shr ecx,2" 0x67 "rep movsd" "mov cl,bl" "and cl,3" "rep movsb" "pop ds" parm [ES DI] [DX SI] [CX BX] modify [CX SI DI ES];

      S8 radmemcmp(const void far* s1,const void far* s2,U32 len);
      #pragma aux radmemcmp = "cld" "push ds" "mov ds,dx" "shl ecx,16" "mov cx,bx" "rep cmpsb" "setne al" "jbe end" "neg al" "end:" "pop ds"  parm [ES DI] [DX SI] [CX BX] modify [CX SI DI ES];

      char far* radstrcpy(void far* dest,const void far* source);
      #pragma aux radstrcpy = "cld" "push ds" "mov ds,dx" "and esi,0xffff" "and edi,0xffff" "mov dx,di" "lp:" "lodsb" "stosb" "test al,0xff" "jnz lp" "pop ds" parm [ES DI] [DX SI] modify [AX DX DI SI ES] value [es dx];

      char far* radstpcpy(void far* dest,const void far* source);
      #pragma aux radstpcpy = "cld" "push ds" "mov ds,dx" "and esi,0xffff" "and edi,0xffff" "lp:" "lodsb" "stosb" "test al,0xff" "jnz lp" "dec di" "pop ds" parm [ES DI] [DX SI] modify [DI SI ES] value [es di];

      U32 radstrlen(const void far* dest);
      #pragma aux radstrlen = "cld" "and edi,0xffff" "mov ecx,0xffffffff" "xor eax,eax" 0x67 "repne scasb" "not ecx" "dec ecx" "movzx eax,cx" "shr ecx,16" parm [ES DI] modify [AX CX DI ES] value [CX AX];

      char far* radstrcat(void far* dest,const void far* source);
      #pragma aux radstrcat = "cld" "and edi,0xffff" "mov ecx,0xffffffff" "and esi,0xffff" "push ds" "mov ds,dx" "mov dx,di" "xor eax,eax" 0x67 "repne scasb" "dec edi" "lp:" "lodsb" "stosb" "test al,0xff" "jnz lp" "pop ds" \
        parm [ES DI] [DX SI] modify [AX CX DI SI ES] value [es dx];

      char far* radstrchr(const void far* dest,char chr);
      #pragma aux radstrchr = "cld" "lp:" 0x26 "lodsb" "cmp al,dl" "je fnd" "cmp al,0" "jnz lp" "xor ax,ax" "mov es,ax" "mov si,1" "fnd:" "dec si" parm [ES SI] [DL] modify [AX SI ES] value [es si];

      S8 radstricmp(const void far* s1,const void far* s2);
      #pragma aux radstricmp = "and edi,0xffff" "push ds" "mov ds,dx" "and esi,0xffff" "lp:" "mov al,[esi]" "mov ah,[edi]" "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub al,32" "c1:" \
        "cmp ah,'a'" "jb c2" "cmp ah,'z'" "ja c2" "sub ah,32" "c2:" "cmp al,ah" "jne set" "cmp al,0" "je set" \
        "inc esi" "inc edi" "jmp lp" "set:" "setne al" "jbe end" "neg al" "end:" "pop ds" \
        parm [ES DI] [DX SI] modify [AX DI SI];

      U32 radstru32(const void far* dest);
      #pragma aux radstru32 = "cld" "xor ecx,ecx" "xor ebx,ebx" "xor edi,edi" 0x26 "lodsb" "cmp al,45" "jne skip2" "mov edi,1" "jmp skip" "lp:" "mov eax,10" "mul ecx" "lea ecx,[eax+ebx]" \
        "skip:" 0x26 "lodsb" "skip2:" "cmp al,0x39" "ja dne" "cmp al,0x30" "jb dne" "mov bl,al" "sub bl,0x30" "jmp lp" "dne:" "test edi,1" "jz pos" "neg ecx" "pos:" \
        "movzx eax,cx" "shr ecx,16" parm [ES SI] modify [AX BX DX DI SI] value [cx ax];

    #endif

  #endif

  #define uintneg1 ((UINTa)(SINTa)(S32)-1)
  #define RAD_align(var) var; U8 junk##var[4-(sizeof(var)&3)];
  #define RAD_align_after(var) U8 junk##var[4-(sizeof(var)&3)]={0};
  #define RAD_align_init(var,val) var=val; U8 junk##var[4-(sizeof(var)&3)]={0};
  #define RAD_align_array(var,num) var[num]; U8 junk##var[4-(sizeof(var)&3)];
  #define RAD_align_string(var,str) char var[]=str; U8 junk##var[4-(sizeof(var)&3)]={0};

  #ifdef __WATCOMC__

    U8 radtoupper(U8 p);
    #pragma aux radtoupper = "cmp al,'a'" "jb c1" "cmp al,'z'" "ja c1" "sub al,32" "c1:" parm [al] value [al];

  #else

  // for multi-processor machines

  #ifdef __RADNT__

    #ifdef __RADX64__

      long _InterlockedExchangeAdd(
        long volatile * Addend,
        long Value
      );
      
      #define LockedIncrement(var) _InterlockedExchangeAdd( (S32*) &(var), 1 )
      #define LockedDecrement(var) _InterlockedExchangeAdd( (S32*) &(var), -1 )

      #define LockedIncrementFunc(ptr) _InterlockedExchangeAdd( (S32*) (ptr), 1 )
      #define LockedDecrementFunc(ptr) _InterlockedExchangeAdd( (S32*) (ptr), -1 )

      #define LockedAddFunc(ptr,val) _InterlockedExchangeAdd( (S32*) (ptr), val )

    #else


      #define LockedIncrement(var) __asm { lock inc [var] }
      #define LockedDecrement(var) __asm { lock dec [var] }
      static void __inline LockedIncrementFunc(void PTR4* var) {
        __asm {
          mov eax,[var]
          lock inc [eax]
        }
      }

      static void __inline LockedDecrementFunc(void PTR4* var) {
        __asm {
           mov eax,[var]
           lock dec [eax]
        }
      }

      static void __inline LockedAddFunc(void PTR4* var,U32 val) {
        __asm {
          mov eax,[var]
          mov edx,[val]
          lock add [eax],edx
        }
      }
    #endif

  #else

    #if defined(__RADMAC__)

      #ifndef __OPENTRANSPORT__
        extern long OTAtomicAdd32( long toAdd, long *  dest);
      #endif

      #define LockedIncrement(var) OTAtomicAdd32(  1, (long*) &(var) )
      #define LockedDecrement(var) OTAtomicAdd32( -1, (long*) &(var) )

      #define LockedIncrementFunc(ptr) OTAtomicAdd32( 1, (long*) (ptr) )
      #define LockedDecrementFunc(ptr) OTAtomicAdd32( -1, (long*) (ptr) )
      
      #define LockedAddFunc(ptr,val) OTAtomicAdd32( val, (long*) (ptr) )

    #elif defined(__RADNGC__)

      #define LockedIncrement(var) {++(var);}
      #define LockedDecrement(var) {--(var);}

      #define LockedIncrementFunc(ptr) {++(*((U32*)(ptr)));}
      #define LockedDecrementFunc(ptr) {--(*((U32*)(ptr)));}
      
      #define LockedAddFunc(ptr,val) {(*((U32*)(ptr)))+=(val);}

    #elif defined(__RADWII__)
      #include <revolution/os.h>

      #define LockedIncrement(var) {BOOL i=OSDisableInterrupts(); ++(var);  OSRestoreInterrupts(i);}
      #define LockedDecrement(var) {BOOL i=OSDisableInterrupts(); --(var);  OSRestoreInterrupts(i);}

      #define LockedIncrementFunc(ptr) {BOOL i=OSDisableInterrupts(); ++(*((U32*)(ptr))); OSRestoreInterrupts(i); }
      #define LockedDecrementFunc(ptr) {BOOL i=OSDisableInterrupts(); --(*((U32*)(ptr))); OSRestoreInterrupts(i); }
      
      #define LockedAddFunc(ptr,val) {BOOL i=OSDisableInterrupts(); (*((U32*)(ptr)))+=(val); OSRestoreInterrupts(i); }

    #elif defined(__RADNDS__)

      #define LockedIncrement(var) {++(var);}
      #define LockedDecrement(var) {--(var);}

      #define LockedIncrementFunc(ptr) {++(*((U32*)(ptr)));}
      #define LockedDecrementFunc(ptr) {--(*((U32*)(ptr)));}
      
      #define LockedAddFunc(ptr,val) {(*((U32*)(ptr)))+=(val);}

    #elif defined(__RADSPU__)

      #define LockedIncrement(var) {++(var);}
      #define LockedDecrement(var) {--(var);}

      #define LockedIncrementFunc(ptr) {++(*((U32*)(ptr)));}
      #define LockedDecrementFunc(ptr) {--(*((U32*)(ptr)));}
      
      #define LockedAddFunc(ptr,val) {(*((U32*)(ptr)))+=(val);}

    #elif defined(__RADXENON__)

void __stdcall OutputDebugStringA( char const * lpOutputString );
#define OutputDebugString  OutputDebugStringA

long
__stdcall
_InterlockedIncrement(
      long volatile *lpAddend
    );


long
__stdcall
_InterlockedDecrement(
      long volatile *lpAddend
    );

long
__stdcall
_InterlockedExchangeAdd(
      long volatile *lpAddend, long amt
    );

#ifndef _XBOX_H_
unsigned int __stdcall XQueryMemoryProtect( void const * addr );
#define PAGE_WRITECOMBINE 0x400
#endif

  void __emit(unsigned int opcode);
  #define __lwsync()      __emit(0x7C2004AC)

  #define LockedIncrement(var) { _InterlockedIncrement( (long*)&var ); __lwsync(); }
  #define LockedDecrement(var) { __lwsync(); _InterlockedDecrement( (long*)&var ); }

  #define LockedIncrementFunc(ptr) { _InterlockedIncrement( (long*)(ptr) ); __lwsync(); }
  #define LockedDecrementFunc(ptr) { __lwsync(); _InterlockedDecrement( (long*)(ptr) ); }

      
      #define LockedAddFunc(ptr,val) _InterlockedExchangeAdd( (S32*) (ptr), val )

    #elif defined(__RADPS2__)

      #include <eekernel.h>

      static inline void AtomicAdd32( S32 toAdd, S32 * where )
      {
        int old = DI();
        *where += toAdd;
        if( old )
        {
          EI();
        }
      }

      #define LockedIncrement(var) AtomicAdd32(  1, (S32*) &(var) )
      #define LockedDecrement(var) AtomicAdd32( -1, (S32*) &(var) )

      #define LockedIncrementFunc(ptr) AtomicAdd32( 1, (S32*) (ptr) )
      #define LockedDecrementFunc(ptr) AtomicAdd32( -1, (S32*) (ptr) )

      #define LockedAddFunc(ptr,val) AtomicAdd32( val, (S32*) (ptr) )

    #elif defined(__RADPSP__)

      #include <kernel.h>

      static inline void AtomicAdd32( S32 toAdd, S32 * where )
      {
        int old = sceKernelCpuSuspendIntr();
        *where += toAdd;
        sceKernelCpuResumeIntrWithSync(old);
      }

      #define LockedIncrement(var) AtomicAdd32(  1, (S32*) &(var) )
      #define LockedDecrement(var) AtomicAdd32( -1, (S32*) &(var) )

      #define LockedIncrementFunc(ptr) AtomicAdd32( 1, (S32*) (ptr) )
      #define LockedDecrementFunc(ptr) AtomicAdd32( -1, (S32*) (ptr) )

      #define LockedAddFunc(ptr,val) AtomicAdd32( val, (S32*) (ptr) )

    #elif defined(__RADPS3__)

      static inline U32 AtomicAdd32(U32 *ea, U32 value)
      {
        U32 old, tmp;

        __asm__ volatile(
          "# AtomicAdd32(ea=%[ea],old=%[old],val=%[value],tmp=%[tmp])\n"
          ".loop%=:\n"
          "  lwarx   %[old], 0, %[ea]\n"
          "  add     %[tmp], %[value], %[old]\n"
          "  stwcx.  %[tmp], 0, %[ea]\n"
          "  bne-    .loop%=\n"
          "  lwsync\n"
          : [old]"=&r"(old), [tmp]"=&r"(tmp)
          : [ea]"b"(ea), [value]"r"(value)
          : "cc", "memory");
        return old;
      }

      #define LockedIncrement(var) AtomicAdd32( (U32*) &(var),  1 )
      #define LockedDecrement(var) AtomicAdd32( (U32*) &(var), -1 )

      #define LockedIncrementFunc(ptr) AtomicAdd32( (U32*) (ptr),  1 )
      #define LockedDecrementFunc(ptr) AtomicAdd32( (U32*) (ptr), -1 )

      #define LockedAddFunc(ptr,val) AtomicAdd32( (U32*) (ptr), val )

    #elif defined(__RADLINUX__)

      #define radmemcpydb memmove

      // todo: need real interlocks here.  They're kind of hard to
      // come by on linux
      #define LockedIncrement(var) { ++var; }
      #define LockedDecrement(var) { --var; }
      static void __inline LockedIncrementFunc(void PTR4* var) { ++(*(U32*)var); }
      static void __inline LockedDecrementFunc(void PTR4* var) { --(*(U32*)var); }
      //static void __inline LockedAddFunc(void PTR4* var,U32 val)

    #else

      #define LockedIncrement(var) __asm { inc [var] }
      #define LockedDecrement(var) __asm { dec [var] }
      static void __inline LockedIncrementFunc(void PTR4* var) { __asm { mov eax,[var]
                                                                  inc [eax] } }
      static void __inline LockedDecrementFunc(void PTR4* var) { __asm { mov eax,[var]
                                                                  dec [eax] } }
      static void __inline LockedAddFunc(void PTR4* var,U32 val) {
        __asm {
          mov eax,[var]
          mov edx,[val]
          add [eax],edx
        }
      }

    #endif

  #endif

  #endif

  RADDEFEND

#endif
