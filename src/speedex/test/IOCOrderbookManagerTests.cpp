#include "lib/catch.hpp"
#include "speedex/IOCOrderbook.h"
#include "speedex/IOCOffer.h"
#include "speedex/IOCOrderbookManager.h"

#include "ledger/AssetPair.h"

#include "test/TxTests.h"

#include "test/TestUtils.h"
#include "test/test.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include <map>

using namespace stellar;
using namespace stellar::txtest;

using int128_t = __int128_t;

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

static void addOffer(IOCOrderbookManager& orderbookManager, int32_t p_n, int32_t p_d, int64_t amount, Asset const& sell, Asset const& buy, uint64_t idx)
{
	Price p;
	p.n = p_n;
	p.d = p_d;

	AccountID acct = getAccount("blah").getPublicKey();

	auto hash = IOCOffer::offerHash(p, acct, idx, 0);
	IOCOffer offer(amount, p, hash, acct);

	AssetPair tradingPair {
		.selling = sell,
		.buying = buy
	};
	orderbookManager.addOffer(tradingPair, offer);
}

TEST_CASE("two asset orderbook manager demand query", "[speedex]")
{
	using int128_t = __int128_t;

	auto assets = makeAssets(2);
	IOCOrderbookManager manager;

	int64_t amount = 10000;

	SECTION("one offer")
	{
		addOffer(manager, 300, 100, amount, assets[0], assets[1], 1);

		manager.sealBatch();

		std::map<Asset, int128_t> demands;

		std::map<Asset, uint64_t> prices;

		prices[assets[0]] = 400;
		prices[assets[1]] = 100;
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == -400 * amount);
		REQUIRE(demands[assets[1]] == 400 * amount);

		prices[assets[0]] = 400;
		prices[assets[1]] = 100;
		demands.clear();
		manager.demandQuery(prices, demands, 0, 1);
		REQUIRE(demands[assets[0]] == -200 * amount);
		REQUIRE(demands[assets[1]] == 200 * amount);
	}
	SECTION("both directions, same price")
	{
		addOffer(manager, 300, 100, amount, assets[0], assets[1], 1);
		addOffer(manager, 100, 300, 3*amount, assets[1], assets[0], 2);

		manager.sealBatch();

		std::map<Asset, int128_t> demands;

		std::map<Asset, uint64_t> prices;

		prices[assets[0]] = 300;
		prices[assets[1]] = 100;
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == 0);
		REQUIRE(demands[assets[1]] == 0);


		prices[assets[0]] = 400;
		prices[assets[1]] = 100;
		demands.clear();
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == -400 * amount);
		REQUIRE(demands[assets[1]] == 400 * amount);

		prices[assets[0]] = 200;
		prices[assets[1]] = 100;
		demands.clear();
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == 100 * 3 * amount);
		REQUIRE(demands[assets[1]] == -100 * 3 * amount);
	}

	SECTION("both directions, price gap")
	{
		addOffer(manager, 100, 300, amount, assets[0], assets[1], 1);
		addOffer(manager, 100, 300, amount, assets[1], assets[0], 2);

		manager.sealBatch();

		std::map<Asset, int128_t> demands;

		std::map<Asset, uint64_t> prices;

		prices[assets[0]] = 100;
		prices[assets[1]] = 100;
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == 0);
		REQUIRE(demands[assets[1]] == 0);


		prices[assets[0]] = 500;
		prices[assets[1]] = 100;
		demands.clear();
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == -500 * amount);
		REQUIRE(demands[assets[1]] == 500 * amount);
	}

	SECTION("tax one offer")
	{
		addOffer(manager, 100, 100, amount, assets[0], assets[1], 1);

		manager.sealBatch();

		std::map<Asset, int128_t> demands;

		std::map<Asset, uint64_t> prices;

		prices[assets[0]] = 200;
		prices[assets[1]] = 100;
		manager.demandQuery(prices, demands, 0, 0);
		REQUIRE(demands[assets[0]] == -200 * amount);
		REQUIRE(demands[assets[1]] == 200 * amount);

		demands.clear();
		manager.demandQuery(prices, demands, 1, 0);
		REQUIRE(demands[assets[0]] == -200 * amount);
		REQUIRE(demands[assets[1]] == 100 * amount);

		demands.clear();
		manager.demandQuery(prices, demands, 2, 0);
		REQUIRE(demands[assets[0]] == -200 * amount);
		REQUIRE(demands[assets[1]] == 150 * amount);

	}

}


