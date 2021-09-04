#pragma once

#include <cstdint>
#include <compare>

namespace stellar {

struct uint256_t
{
	using int128_t = __int128;
	using uint128_t = unsigned __int128;

	constexpr static uint128_t UINT128_MAX = ((uint128_t) 0) - 1;
	constexpr static uint128_t INT128_MAX = UINT128_MAX >> 1;

	uint128_t lowbits;
	uint128_t highbits;

	static uint256_t square(int128_t a);

	static uint256_t product(uint128_t a, uint128_t b);

	uint256_t& operator+=(uint256_t const& other);

	std::strong_ordering operator<=>(uint256_t const& other) const;

	bool operator==(uint256_t const& other) const = default;

	//requires that other <= this
	void subtract_smaller(const uint256_t& other);

	int64_t compress(uint8_t radix) const;
};

} /* stellar */
