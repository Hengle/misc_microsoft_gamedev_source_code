/*
* version 1.90
* Regexp is a class that encapsulates the Regular expression
* stuff. Hopefully this means I can plug in different regexp
* libraries without the rest of my code needing to be changed.
* Written by Jim Morris,  morris@netcom.com
*/
#ifndef	_REGEXP_H
#define _REGEXP_H
//#include    <iostream.h>

#include    "regex.h"

/*
* Note this is an inclusive range where it goes
* from start() to, and including, end()
*/
class Range
{
private:
   int st, en, len;

public:
   Range()
   {
      st=0; en= -1; len= 0;
   }

   Range(int s, int e)
   {
      st= s; en= e; len= (en - st) + 1;
      assert(st <= en && st >= 0); 
   }

   // test validity of the range
   operator void *() { return (st <= en && st >= 0)? this : 0;}
   int start(void) const { return st;}
   int end(void) const { return en;}
   int length(void) const { return len;}

   void set(int a, int b)
   {
      st= a; en= b; len= (en - st) + 1;
      assert(st <= en && st >= 0); 
   }

   int operator<(const Range& r) const // for sorting
   {
      return ((st == r.st) ? (en < r.en) : (st < r.st));
   }

   // x++ operator extends end of range by one
   void operator++(int){ en++; len++; }

   // ++x operator extends start of range by one
   void operator++(void){ st++; len--; }

#if 0
   // Don't need these yet
   Range operator&(const Range& r) const // returns intersection of two ranges
   {
      Range ret;
      if(en >= 0 && r.en >= 0 && (st <= r.en) && (en >= r.st)){  // if any overlap
         ret.st= (st > r.st) ? st : r.st;
         ret.en= (en < r.en) ? en : r.en;
      }
      return ret;
   }

   Range operator|(const Range& r) const // returns union of two ranges if consecutive
   {
      Range ret;
      if(en >= 0 && r.en >= 0 && (st <= r.en+1) && (en >= r.st-1)){  // if any overlap or contiguous
         ret.st= (st < r.st) ? st : r.st;
         ret.en= (en > r.en) ? en : r.en;
      }
      return ret;
   }
#endif

   //    friend ostream& operator<<(ostream& os, const Range& r)
   //    {
   //	os << r.st << " - " << r.en << " (" << ((r.en - r.st)+1) << ")";
   //        return os;
   //    }
};

class Regexp
{
public:
   enum options {def=0, nocase=1};

private:
   regexp *repat;
   const char *target; // only used as a base address to get an offset
   int res;
   int iflg;
#ifndef	__TURBOC__
   void strlwr(char *s)
   {
      while(*s){
         *s= (char)tolower(*s);
         s++;
      }
   }
#endif    
public:
   Regexp(const char *rege, int ifl= 0)
   {
      iflg= ifl;
      if(iflg == nocase)// lowercase fold
      { 
         long length = strlen(rege) + 1;
         char *r= new char[length];
         StringCchCopyA(r, length, rege);
         strlwr(r);
         repat = regcomp(r);
         if(repat == NULL)
         {
            BString temp;

            temp.format(B("regexp init failed on string: %s"), BStrConv::toB(rege));

            BFAIL(temp.getPtr());
         }
         delete [] r;
      }
      else
      {
         repat=regcomp ((char *) rege);
         if(repat == NULL)
         {
            BString temp;

            temp.format(B("regexp init failed on string: %s"), BStrConv::toB(rege));

            BFAIL(temp.getPtr());
         }
      }
   }

   ~Regexp()
   {
      free(repat);
   }    

   int match(const char *targ)
   {
      int res;
      if(iflg == nocase){ // fold lowercase
         long length = strlen(targ) + 1;
         char *r= new char[length];
         StringCchCopyA(r, length, targ);
         strlwr(r);
         res= regexec(repat, r); 
         target= r; // looks bad but is really ok, really
         delete [] r;
      }else{
         res= regexec(repat, (char *) targ);
         target= targ;
      }

      return ((res == 0) ? 0 : 1);
   }

#ifdef __TURBOC__
   int groups(void) const;
#else
   int groups(void) const
   {
      int res= 0;
      for (int i=0; i<NSUBEXP; i++) {
         if(repat->startp[i] == NULL) break;
         res++;
      }
      return res;
   }
#endif

   Range getgroup(int n) const
   {
      assert(n < NSUBEXP);
      if(repat->endp[n] == repat->startp[n]) // pointing to end of string
         return Range((int)(repat->startp[n] - (char *)target),
         (int)(repat->endp[n] - (char *)target));
      else
         return Range((int)(repat->startp[n] - (char *)target),
         (int)(repat->endp[n] - (char *)target) - 1);
   }
};
#endif
