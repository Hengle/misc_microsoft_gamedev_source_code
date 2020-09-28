// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#ifndef _RANDOM_H_
#define _RANDOM_H_

// Classes.

class Random
{
	private:
		unsigned int prime1;
		unsigned int prime2;
		unsigned int prime3;
		unsigned int counter;
		
		static unsigned int defaultSeed;
		static std::vector<unsigned int> primes1;
		static std::vector<unsigned int> primes2;
		static std::vector<unsigned int> primes3;
		
		static unsigned int IntSqrt(unsigned int value);
		static bool IsPrime(unsigned int value);
		static std::vector<unsigned int> GetPrimes(unsigned int lower, unsigned int upper);
		static void BuildPrimes();
		
		void PickPrimes(unsigned int seed);
		
		unsigned int NoiseBase(unsigned int x);
		
	public:
		Random();
		Random(unsigned int seed);
		
		Random(const Random &random);
		Random & operator =(const Random &random);
		~Random();
		
		unsigned int NextInteger();
		float NextFloat();
		float NextFloat(float min, float max);
};

#endif /* _RANDOM_H_ */