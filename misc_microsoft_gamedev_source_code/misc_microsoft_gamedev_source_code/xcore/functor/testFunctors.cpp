#include <stdlib.h>
#include <string>
#include <iostream>

#include "TestFunctors.h"

int blah(const std::string& s, double t)
{
   printf("blah: %s, %f\n", s.c_str(), t);
   return 2;
}

double bf3(int i, char c, double f) { return f + i + (int)c; }

int main(int /*argc*/, char* /*argv[]*/)
{
#if 0
	TestFunctors::TestFunctors();
	char ch;
	std::cin >> ch;
#endif

   std::string s("2222");

   typedef GF::Functor<int, TYPELIST_2(std::string, double)> FunctorType;
   FunctorType f(&blah);
   
   typedef GF::Functor<int, TYPELIST_1(double)> BasicFunctor;
   
   BasicFunctor* bfun = new BasicFunctor(GF::Bind<0>(f, s));
   BasicFunctor bfun2(*bfun);
   delete bfun;
      
   printf("%u\n", bfun2(2.0));
   
   //FunctorType* p = new FunctorType(&blah);
   //FunctorType q(*p);
   //delete p;
   
   //printf("%u\n", q(100));
   
   //typedef TYPELIST_3(int, char, double) TestTL3;
   //GF::Functor<double, TestTL3> bfun3(&bf3);
   //GF::Functor<double, TYPELIST_2(char, double)> bfun14 = GF::Bind<0>(bfun3, 1);
   //XVERIFY(bfun14('A', 2.1) == bf3(1, 'A', 2.1));
   
   
   
	
	return 0;
}
