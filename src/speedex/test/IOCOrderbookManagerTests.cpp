#include "lib/catch.hpp"
#include "speedex/IOCOrderbook.h"
#include "speedex/IOCOffer.h"
#include "speedex/IOCOrderbookManager.h"
#include "speedex/DemandUtils.h"

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

		std::map<Asset, uint64_t> prices;

		SupplyDemand sd1;

		prices[assets[0]] = 400;
		prices[assets[1]] = 100;
		manager.demandQuery(prices, sd1, 0);
		REQUIRE(sd1.getDelta(assets[0]) == -400 * amount);
		REQUIRE(sd1.getDelta(assets[1]) == 400 * amount);

		prices[assets[0]] = 400;
		prices[assets[1]] = 100;

		SupplyDemand sd2;
		manager.demandQuery(prices, sd2, 1);
		REQUIRE(sd2.getDelta(assets[0]) == -200 * amount);
		REQUIRE(sd2.getDelta(assets[1]) == 200 * amount);
	}
	SECTION("both directions, same price")
	{
		addOffer(manager, 300, 100, amount, assets[0], assets[1], 1);
		addOffer(manager, 100, 300, 3*amount, assets[1], assets[0], 2);

		manager.sealBatch();

		std::map<Asset, uint64_t> prices;

		SupplyDemand sd;

		SECTION("eq")
		{
			prices[assets[0]] = 300;
			prices[assets[1]] = 100;
			manager.demandQuery(prices, sd, 0);
			REQUIRE(sd.getDelta(assets[0]) == 0);
			REQUIRE(sd.getDelta(assets[1]) == 0);
		}
		SECTION("above eq")
		{
			prices[assets[0]] = 400;
			prices[assets[1]] = 100;
			manager.demandQuery(prices, sd, 0);
			REQUIRE(sd.getDelta(assets[0]) == -400 * amount);
			REQUIRE(sd.getDelta(assets[1]) == 400 * amount);
		}

		SECTION("below eq")
		{
			prices[assets[0]] = 200;
			prices[assets[1]] = 100;
			manager.demandQuery(prices, sd, 0);
			REQUIRE(sd.getDelta(assets[0]) == 100 * 3 * amount);
			REQUIRE(sd.getDelta(assets[1]) == -100 * 3 * amount);
		}
	}

	SECTION("both directions, price gap")
	{
		addOffer(manager, 100, 300, amount, assets[0], assets[1], 1);
		addOffer(manager, 100, 300, amount, assets[1], assets[0], 2);

		manager.sealBatch();

		std::map<Asset, uint64_t> prices;

		SupplyDemand sd1;

		prices[assets[0]] = 100;
		prices[assets[1]] = 100;
		manager.demandQuery(prices, sd1, 0);
		REQUIRE(sd1.getDelta(assets[0]) == 0);
		REQUIRE(sd1.getDelta(assets[1]) == 0);


		prices[assets[0]] = 500;
		prices[assets[1]] = 100;

		SupplyDemand sd2;
		manager.demandQuery(prices, sd2, 0);
		REQUIRE(sd2.getDelta(assets[0]) == -500 * amount);
		REQUIRE(sd2.getDelta(assets[1]) == 500 * amount);
	}

	/*SECTION("tax one offer")
	{
		addOffer(manager, 100, 100, amount, assets[0], assets[1], 1);

		manager.sealBatch();

		std::map<Asset, uint64_t> prices;

		SupplyDemand sd;

		prices[assets[0]] = 200;
		prices[assets[1]] = 100;
		SECTION("smooth 0")
		{
			manager.demandQuery(prices, sd, 0, 0);
			REQUIRE(sd.getDelta(assets[0]) == -200 * amount);
			REQUIRE(sd.getDelta(assets[1]) == 200 * amount);
		}
		SECTION("smooth 1")
		{
			manager.demandQuery(prices, demands, 1, 0);
			REQUIRE(sd.getDelta(assets[0]) == -200 * amount);
			REQUIRE(sd.getDelta(assets[1]) == 100 * amount);
		}

		SECTION("smooth 2")
		{
			manager.demandQuery(prices, demands, 2, 0);
			REQUIRE(sd.getDelta(assets[0]) == -200 * amount);
			REQUIRE(sd.getDelta(assets[1]) == 150 * amount);
		}

	}*/

}


