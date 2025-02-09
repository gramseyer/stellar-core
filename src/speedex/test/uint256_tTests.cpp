#include "lib/catch.hpp"
#include "speedex/uint256_t.h"

using namespace stellar;

TEST_CASE("uint256_t", "[speedex]")
{
	using int128_t = __int128;
	using uint128_t = unsigned __int128;

	SECTION("squaring")
	{
		int128_t query = ((int128_t)1) << 75;
		REQUIRE(query != 0);

		auto res = uint256_t::square(query);

		REQUIRE(res.highbits == 1llu << 22);
		REQUIRE(res.lowbits == 0);

		query = (((int128_t) INT64_MAX) << 64);

		res = uint256_t::square(query);

		// (2^63 - 1) ^2 = 2^126 - 2^64 + 1

		REQUIRE(res.highbits == ((int128_t)INT64_MAX) * ((int128_t) INT64_MAX));
		REQUIRE(res.lowbits == 0);

		query = (((int128_t) INT64_MAX) << 32);

		// ((2^63 - 1)* (2^32)) ^2 = (2^126 - 2^64 + 1) * 2^64
		// = 2^190 - 2^128 + 2^64

		res = uint256_t::square(query);

		REQUIRE(res.highbits == (((int128_t)1) << 62) - 1);
		REQUIRE(res.lowbits == ((int128_t)1) << 64);
	}

	SECTION("product")
	{
		uint128_t l = ((uint128_t) 1) << 75;
		uint128_t r = ((uint128_t) 1) << 80;

		auto prod = uint256_t::product(l, r);
		REQUIRE(prod.lowbits == 0);
		REQUIRE(prod.highbits == 1llu << 27);

		l = ((uint128_t)UINT64_MAX) << 15;
		prod = uint256_t::product(l, l);
		auto square = uint256_t::square(l);

		REQUIRE(square == prod);
	}

	SECTION("operator+=")
	{
		uint256_t a {
			.lowbits = ((uint128_t)1) << 127,
			.highbits = 1
		};

		uint256_t b {
			.lowbits = ((uint128_t) 1) << 127,
			.highbits = 2
		};

		a += b;

		REQUIRE(a.highbits == 4);
		REQUIRE(a.lowbits == 0);
	}
}

