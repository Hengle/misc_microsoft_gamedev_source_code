// Particle System Simulator -- Aaron Hill
// Copyright 2003 Blank Cartridge, Inc.

#include "common/core/core.h"
#include "Random.h"

// Static member data.

unsigned int Random::defaultSeed = 0;
std::vector<unsigned int> Random::primes1;
std::vector<unsigned int> Random::primes2;
std::vector<unsigned int> Random::primes3;

// Methods.

void Random::PickPrimes(unsigned int seed)
{
	srand(seed);
	prime1 = primes1[rand() % primes1.size()];
	prime2 = primes2[rand() % primes2.size()];
	prime3 = primes3[rand() % primes3.size()];
}

unsigned int Random::NoiseBase(unsigned int x)
{
	x = (x << 13) ^ x;
	return ((x * ((x * x * prime1) + prime2)) + prime3) & 0x7fffffff;
}

Random::Random()
{
	BuildPrimes();
	PickPrimes(defaultSeed++);
}

Random::Random(unsigned int seed)
{
	BuildPrimes();
	PickPrimes(seed);
}

Random::Random(const Random &random)
{
	prime1 = random.prime1;
	prime2 = random.prime2;
	prime3 = random.prime3;
	counter = random.counter;
}

Random & Random::operator =(const Random &random)
{
	prime1 = random.prime1;
	prime2 = random.prime2;
	prime3 = random.prime3;
	counter = random.counter;
	
	return *this;
}

Random::~Random()
{
}

unsigned int Random::NextInteger()
{
	return NoiseBase(counter++) % RAND_MAX;
}

float Random::NextFloat()
{
	unsigned int noise = NoiseBase(counter++);
	return static_cast<float>(noise) / 2147483647.0f;
}

float Random::NextFloat(float min, float max)
{
	return (NextFloat() * (max - min)) + min;
}

// Static methods.

unsigned int Random::IntSqrt(unsigned int value)
{
	int a = 0, b = 1, c;
	
	do
	{
		c = a;
		a = b;
		b = (a + (value / a));
		b >>= 1;
	} while (b != c);
	
	return b;
}

bool Random::IsPrime(unsigned int value)
{
	int a = IntSqrt(value);

	for (int b = 2; b <= a; ++b)
		if (value % b == 0)
			return false;
	
	return true;
}

std::vector<unsigned int> Random::GetPrimes(unsigned int lower, unsigned int upper)
{
	std::vector<unsigned int> primes;
	
	for (unsigned int value = lower; value <= upper; ++value)
		if (IsPrime(value))
			primes.push_back(value);
	
	return primes;
}

void Random::BuildPrimes()
{
	if (primes1.empty())
	{
		// 337,824 possible combinations.
		primes1 = GetPrimes(15000, 16000); // 108 primes.
		primes2 = GetPrimes(789000, 790000); // 68 primes.
		primes3 = GetPrimes(1376312000, 1376313000); // 46 primes.
	}
}