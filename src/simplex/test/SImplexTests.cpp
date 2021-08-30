#include "lib/catch.hpp"

#include "test/TestAccount.h"
#include "test/TestUtils.h"
#include "test/test.h"
#include "test/TxTests.h"

#include "util/XDROperators.h"

#include "simplex/solver.h"

using namespace stellar;
using namespace stellar::txtest;

static Asset makeSimplexAsset(std::string const& issuerName, std::string const& assetName)
{
	auto sk = getAccount(issuerName.c_str());
	return makeAsset(sk, assetName);
}

using int128_t = __int128_t;

static void checkSimplexAmount(Asset const& sell, Asset const& buy, TradeMaximizingSolver const& solver, int128_t amount)
{
	AssetPair tradingPair {
		.selling = sell,
		.buying = buy
	};
	REQUIRE(solver.getRowResult(tradingPair) == amount);
}

static void setSimplexAmount(Asset const& sell, Asset const& buy, TradeMaximizingSolver& solver, int128_t amount)
{
	AssetPair tradingPair {
		.selling = sell,
		.buying = buy
	};
	solver.setUpperBound(tradingPair, amount);
}

TEST_CASE("simplex no trades", "[simplex]")
{
	std::vector<Asset> assets;
	for (auto i = 0u; i < 10; i++) {
		assets.emplace_back(makeSimplexAsset("issuer", fmt::format("A{}", i)));
	}

	TradeMaximizingSolver solver(assets);

	solver.doSolve();

	for (size_t i = 0; i < assets.size(); i++) {
		for (size_t j = 0; j < assets.size(); j++) {
			if (i != j) {
				auto& assetA = assets[i];
				auto& assetB = assets[j];
				std::printf("bar %lu %lu\n", i, j);
				AssetPair tradingPair{
					.selling = assetA,
					.buying = assetB
				};
				REQUIRE(solver.getRowResult(tradingPair) == 0);
			} else {
				std::printf("foo\n");
			}
		}
	}
}

TEST_CASE("simplex 2 asset", "[simplex]")
{
	std::vector<Asset> assets;
	for (auto i = 0u; i < 2; i++) {
		assets.emplace_back(makeSimplexAsset("issuer", fmt::format("A{}", i)));
	}

	TradeMaximizingSolver solver(assets);

	SECTION("one direction")
	{
		setSimplexAmount(assets[0], assets[1], solver, 100);

		solver.doSolve();
		checkSimplexAmount(assets[0], assets[1], solver, 0);
		checkSimplexAmount(assets[1], assets[0], solver, 0);
	}
	SECTION("two directions equal")
	{
		setSimplexAmount(assets[0], assets[1], solver, 100);
		setSimplexAmount(assets[1], assets[0], solver, 100);

		solver.doSolve();
		checkSimplexAmount(assets[0], assets[1], solver, 100);
		checkSimplexAmount(assets[1], assets[0], solver, 100);
	}
	SECTION("two directions unequal")
	{
		setSimplexAmount(assets[0], assets[1], solver, 100000);
		setSimplexAmount(assets[1], assets[0], solver, 100);

		solver.doSolve();
		checkSimplexAmount(assets[0], assets[1], solver, 100);
		checkSimplexAmount(assets[1], assets[0], solver, 100);
	}
	
}

