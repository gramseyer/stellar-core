#include "speedex/uint256_t.h"

namespace stellar {

uint256_t 
uint256_t::square(int128_t a)
{
	constexpr uint128_t lowbits_mask = UINT64_MAX;
	//constexpr uint128_t highbits_mask = lowbits_mask << 64;

	a = a < 0 ? -a : a;

	const uint128_t lowA = a & lowbits_mask;
	const uint128_t highA = a >> 64;

	uint128_t highOut = 0, lowOut = 0;

	highOut += highA * highA;

	lowOut += lowA * lowA;

	uint128_t mid = lowA * highA;

	uint128_t midLow = mid & lowbits_mask;
	uint128_t midHigh = mid >> 64;

	highOut += 2 * midHigh;

	midLow *= 2;
	if (midLow > lowbits_mask)
	{
		highOut++;
		midLow &= lowbits_mask;
	}

	uint128_t candidateLow = lowOut + (midLow << 64);
	if (candidateLow < lowOut)
	{
		//overflow
		highOut++;
	}
	lowOut = candidateLow;

	return uint256_t {
		.lowbits = lowOut,
		.highbits = highOut
	};
}

uint256_t
uint256_t::product(uint128_t a, uint128_t b)
{
	constexpr auto lowbits = [] (uint128_t val) -> uint128_t {
		constexpr uint128_t lowbits_mask = UINT64_MAX;
		return val & lowbits_mask;
	};
	constexpr auto highbits = [] (uint128_t val) -> uint128_t {
		return val >> 64;
	};

	/*constexpr auto printLow = [] (uint128_t val) -> void {
		std::printf("%llx\n", ((uint64_t)val));
	};

	constexpr auto print = [=] (uint128_t val) -> void {
		std::printf("%llx %llx\n", (uint64_t) highbits(val), (uint64_t) lowbits(val));
	};*/

	const uint128_t lowA = lowbits(a);
	const uint128_t lowB = lowbits(b);
	const uint128_t highA = highbits(a);
	const uint128_t highB = highbits(b);

	//std::printf("a\n");
	//printLow(highA);
	//printLow(lowA);
	//std::printf("b\n");
	//printLow(highB);
	//printLow(lowB);


	uint128_t highOut = highA * highB;
	uint128_t lowOut = lowA * lowB;

	auto addMidBits = [&] (uint128_t l, uint128_t r) -> void
	{
		uint128_t prod = l * r;
		uint128_t prodLow = lowbits(prod);
		uint128_t prodHigh = highbits(prod);

		//std::printf("mid\n");
		//printLow(prodLow);
		//printLow(prodHigh);
		
		highOut += prodHigh;
		
		prodLow <<= 64;

		uint128_t candidate = lowOut + prodLow;
		if (candidate < lowOut) {
			//overflow
			highOut ++;
		}
		lowOut = candidate;
	};
	//print(lowOut);
	//print(highOut);

	addMidBits(lowA, highB);
	addMidBits(lowB, highA);

	//print(lowOut);
	//print(highOut);

	return uint256_t {
		.lowbits = lowOut,
		.highbits = highOut
	};
}

int64_t 
uint256_t::compress(uint8_t radix) const 
{
	uint128_t out;
	constexpr uint128_t MAX = INT64_MAX;

	if (radix < 128)
	{

		out = (lowbits >> radix) | (highbits << (128 - radix));
	} 
	else
	{
		out = (highbits >> (radix - 128));
	}
	if (out >= MAX)
	{
		return INT64_MAX;
	}
	return (int64_t) out;
}


uint256_t& 
uint256_t::operator+=(uint256_t const& other)
{
	highbits += other.highbits;
	uint128_t newLow = lowbits + other.lowbits;
	if (newLow < lowbits)
	{
		highbits++;
	}
	lowbits = newLow;
	return *this;
}

std::strong_ordering 
uint256_t::operator<=>(uint256_t const& other) const 
{
	if (highbits < other.highbits)
	{
		return std::strong_ordering::less;
	}
	if (highbits > other.highbits)
	{
		return std::strong_ordering::greater;
	}
	return lowbits <=> other.lowbits;
}


void 
uint256_t::subtract_smaller(const uint256_t& other)
{
	highbits -= other.highbits;
	uint128_t newLow = lowbits - other.lowbits;
	if (newLow > lowbits)
	{
		//overflow
		highbits--;
	}
	lowbits = newLow;
}


} /* stellar */
