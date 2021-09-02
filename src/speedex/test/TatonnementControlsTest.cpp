#include "lib/catch.hpp"
#include "speedex/TatonnementControls.h"

#include "test/TxTests.h"

#include "test/TestUtils.h"
#include "test/test.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include <map>

using namespace stellar::txtest;
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

static std::vector<Asset> makeAssets(size_t numAssets)
{
	auto acct = getAccount("asdf");

	std::vector<Asset> out;

	for (auto i = 0u; i < numAssets; i++)
	{
		out.push_back(makeAsset(acct, fmt::format("A{}", i)));
	}

	return out;
}


TEST_CASE("objective function", "[speedex]")
{
	using int128_t = __int128;

	std::map<Asset, int128_t> demands1, demands2;

	auto assets = makeAssets(10);

	demands1[assets[0]] = 10000;
	demands2[assets[0]] = 10001;

	TatonnementObjectiveFn obj1(demands1), obj2(demands2);

	REQUIRE(obj1.isBetterThan(obj2, 0, 1));
	REQUIRE(!obj2.isBetterThan(obj1, 0, 1));

	REQUIRE(obj1.isBetterThan(obj2, 100, 101));
	REQUIRE(obj2.isBetterThan(obj1, 100, 101));


}
