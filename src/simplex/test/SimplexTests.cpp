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
	std::printf("get %lf expect %lf\n", (double) solver.getRowResult(tradingPair), (double) amount);
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

static void checkAssetConstraint(TradeMaximizingSolver const& solver, Asset const& asset, std::vector<Asset> const& otherAssets)
{
	int128_t sellAmount = 0, buyAmount = 0;

	for (auto const& otherAsset : otherAssets)
	{
		if (asset != otherAsset)
		{
			sellAmount += solver.getRowResult(AssetPair{.selling = asset, .buying = otherAsset});
			buyAmount += solver.getRowResult(AssetPair{.selling = otherAsset, .buying = asset});
		}
	}
	REQUIRE(sellAmount == buyAmount);
}

static void checkAllAssetConstraints(TradeMaximizingSolver const& solver, std::vector<Asset> const& assets)
{
	for (auto const& asset : assets)
	{
		checkAssetConstraint(solver, asset, assets);
	}
}

static void checkObjective(TradeMaximizingSolver const& solver, std::vector<Asset> const& assets, int128_t expectedObjective)
{
	int128_t obj = 0;
	for (size_t i = 0; i < assets.size(); i++) {
		for (size_t j = 0; j < assets.size(); j++) {
			if (i != j) {
				obj += solver.getRowResult(AssetPair{.selling = assets[i], .buying = assets[j]});
			}
		}
	}
	REQUIRE(obj == expectedObjective);
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

TEST_CASE("simplex 3 asset", "[simplex]")
{
	std::vector<Asset> assets;
	for (auto i = 0u; i < 3; i++) {
		assets.emplace_back(makeSimplexAsset("issuer", fmt::format("A{}", i)));
	}

	TradeMaximizingSolver solver(assets);

	SECTION("ignore one asset")
	{
		setSimplexAmount(assets[0], assets[1], solver, 100);
		setSimplexAmount(assets[1], assets[0], solver, 200);

		solver.doSolve();
		checkSimplexAmount(assets[0], assets[1], solver, 100);
		checkSimplexAmount(assets[1], assets[0], solver, 100);
	}

	SECTION("third asset not tradeable")
	{
		setSimplexAmount(assets[0], assets[1], solver, 100);
		setSimplexAmount(assets[1], assets[0], solver, 200);
		SECTION("sell third asset only")
		{
			setSimplexAmount(assets[0], assets[2], solver, 100);
			solver.doSolve();

			checkSimplexAmount(assets[0], assets[1], solver, 100);
			checkSimplexAmount(assets[1], assets[0], solver, 100);
			checkSimplexAmount(assets[0], assets[2], solver, 0);
			checkSimplexAmount(assets[1], assets[2], solver, 0);
			checkSimplexAmount(assets[2], assets[0], solver, 0);
			checkSimplexAmount(assets[2], assets[1], solver, 0);
		}
		SECTION("buy third asset only")
		{
			setSimplexAmount(assets[1], assets[2], solver, 100);
			solver.doSolve();

			checkSimplexAmount(assets[0], assets[1], solver, 100);
			checkSimplexAmount(assets[1], assets[0], solver, 100);
			checkSimplexAmount(assets[0], assets[2], solver, 0);
			checkSimplexAmount(assets[1], assets[2], solver, 0);
			checkSimplexAmount(assets[2], assets[0], solver, 0);
			checkSimplexAmount(assets[2], assets[1], solver, 0);
		}
	}
	SECTION("only directed cycle")
	{
		setSimplexAmount(assets[0], assets[1], solver, 200);
		setSimplexAmount(assets[1], assets[2], solver, 100);
		setSimplexAmount(assets[2], assets[0], solver, 150);
		
		solver.doSolve();

		checkSimplexAmount(assets[0], assets[1], solver, 100);
		checkSimplexAmount(assets[1], assets[0], solver, 0);
		checkSimplexAmount(assets[0], assets[2], solver, 0);
		checkSimplexAmount(assets[1], assets[2], solver, 100);
		checkSimplexAmount(assets[2], assets[0], solver, 100);
		checkSimplexAmount(assets[2], assets[1], solver, 0);
	}
	SECTION("dir cycles both directions")
	{
		setSimplexAmount(assets[0], assets[1], solver, 200);
		setSimplexAmount(assets[1], assets[2], solver, 100);
		setSimplexAmount(assets[2], assets[0], solver, 150);

		setSimplexAmount(assets[1], assets[0], solver, 400);
		setSimplexAmount(assets[0], assets[2], solver, 400);
		setSimplexAmount(assets[2], assets[1], solver, 450);
		
		solver.doSolve();
		solver.printSolution();

		/*checkSimplexAmount(assets[0], assets[1], solver, 100);
		checkSimplexAmount(assets[1], assets[0], solver, 400);
		checkSimplexAmount(assets[0], assets[2], solver, 400);
		checkSimplexAmount(assets[1], assets[2], solver, 100);
		checkSimplexAmount(assets[2], assets[0], solver, 100);
		checkSimplexAmount(assets[2], assets[1], solver, 400);*/
		checkAllAssetConstraints(solver, assets);
		checkObjective(solver, assets, 1500);
	}
	SECTION("dir cycle plus 2-cycles, no opposite 3-cycle")
	{
		setSimplexAmount(assets[0], assets[1], solver, 200);
		setSimplexAmount(assets[1], assets[2], solver, 100);
		setSimplexAmount(assets[2], assets[0], solver, 150);

		setSimplexAmount(assets[1], assets[0], solver, 100);
		setSimplexAmount(assets[2], assets[1], solver, 100);

		solver.doSolve();
		solver.printSolution();
		checkAllAssetConstraints(solver, assets);
		checkObjective(solver, assets, 500);

		checkSimplexAmount(assets[0], assets[1], solver, 200);
		checkSimplexAmount(assets[1], assets[0], solver, 100);
		checkSimplexAmount(assets[0], assets[2], solver, 0);
		checkSimplexAmount(assets[1], assets[2], solver, 100);
		checkSimplexAmount(assets[2], assets[0], solver, 100);
		checkSimplexAmount(assets[2], assets[1], solver, 0);
	}
}

