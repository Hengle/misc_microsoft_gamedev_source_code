
#include "xsystem.h"
#include "watermark.h"

// This can be filled in with a watermark, which really takes the form:
// DWORD (sentinel), DWORD (offset of this symbol), DWORD (size of encrypted data), encrypted bytes
BYTE watermark[WATERMARK_SIZE] = {0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 

//#ifdef CORPNET_AUTH
   0x00, 0x00, 0x00, 0x00
//#else
   //0xFF, 0xFF, 0xFF, 0xFF
//#endif

};
