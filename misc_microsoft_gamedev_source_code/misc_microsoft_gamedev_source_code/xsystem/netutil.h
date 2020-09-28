//============================================================================
// netutil.h
//
// Copyright (c) 2005, Ensemble Studios
//============================================================================

#ifndef _NETUTIL_
#define _NETUTIL_

//============================================================================
// structures
//============================================================================

#ifdef XBOX

struct  hostent 
{
   char    h_name[1024];           /* official name of host */
   char    h_aliases[1][1];  /* alias list */
   short   h_addrtype;             /* host address type */
   short   h_length;               /* length of address */
   char    h_addr_list[2][4]; /* list of addresses */
};

#endif // XBOX


//============================================================================
// functions
//============================================================================

bool        networkStartup();
void        networkCleanup();

#ifdef XBOX

void        createGUIDXbox       (GUID* pGUID);
long        stringFromGUIDXbox   (GUID& rguid, WCHAR* lpsz, int cchMax);   

hostent*    gethostbyname        (const char* name);
int         gethostname          (char* name, int namelen);

char*       inet_ntoa            (struct in_addr in);

bool        getXNAddr            (XNADDR& a);

#endif // XBOX

#endif // _NETUTIL_
