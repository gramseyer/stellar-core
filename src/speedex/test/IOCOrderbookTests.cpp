
#include "lib/catch.hpp"
#include "speedex/IOCOrderbook.h"
#include "speedex/IOCOffer.h"

#include "ledger/AssetPair.h"

#include "test/TxTests.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

using namespace stellar;
using namespace stellar::txtest;

using int128_t = __int128_t;

static AssetPair
genericAssetPair()
{
	auto acct = getAccount("asdf");

	auto sell = makeAsset(acct, "sell");
	auto buy = makeAsset(acct, "buy");

	return AssetPair {
		.selling = sell,
		.buying = buy
	};
}

static void addOffer(IOCOrderbook& orderbook, uint32_t p_n, uint32_t p_d, int64_t amount, uint64_t idx)
{
	Price p;
	p.n = p_n;
	p.d = p_d;

	AccountID acct = getAccount("blah").getPublicKey();

	auto hash = IOCOffer::offerHash(p, acct, idx, 0);
	IOCOffer offer(amount, p, hash, acct);
	orderbook.addOffer(offer);
}

TEST_CASE("empty orderbook", "[speedex]")
{
	IOCOrderbook orderbook(genericAssetPair());

	orderbook.doPriceComputationPreprocessing();

	REQUIRE(orderbook.getPriceCompStats(1, 1).cumulativeOfferedForSale == 0);
	REQUIRE(orderbook.getPriceCompStats(1, 1).cumulativeOfferedForSaleTimesPrice == 0);
}

TEST_CASE("one offer", "[speedex]")
{
	IOCOrderbook orderbook(genericAssetPair());

	int64_t amount = 10000;

	addOffer(orderbook, 100, 100, amount, 1);


	orderbook.doPriceComputationPreprocessing();


	REQUIRE(orderbook.getPriceCompStats(1, 1).cumulativeOfferedForSale == amount);
	REQUIRE(orderbook.getPriceCompStats(1, 1).cumulativeOfferedForSaleTimesPrice == ((int128_t) amount) << 32);


	REQUIRE(orderbook.getPriceCompStats(0, 1).cumulativeOfferedForSale == 0);
	REQUIRE(orderbook.getPriceCompStats(0, 1).cumulativeOfferedForSaleTimesPrice == 0);


	REQUIRE(orderbook.getPriceCompStats(100, 1).cumulativeOfferedForSale == amount);
	REQUIRE(orderbook.getPriceCompStats(100, 1).cumulativeOfferedForSaleTimesPrice == ((int128_t) amount) << 32);


	REQUIRE(orderbook.getPriceCompStats(1, 100).cumulativeOfferedForSale == 0);
	REQUIRE(orderbook.getPriceCompStats(1, 100).cumulativeOfferedForSaleTimesPrice == 0);
}

TEST_CASE("offers at identical price point", "[speedex]")
{
	IOCOrderbook orderbook(genericAssetPair());

	int64_t amount = 10000;

	addOffer(orderbook, 100, 100, amount, 1);
	addOffer(orderbook, 200, 200, amount, 2);
	addOffer(orderbook, 300, 300, amount, 3);
	addOffer(orderbook, 100, 200, amount, 4);
	addOffer(orderbook, 200, 100, amount, 5);

	orderbook.doPriceComputationPreprocessing();

	int128_t expectedHalfAmount = ((int128_t) amount) << 31;

	REQUIRE(orderbook.getPriceCompStats(1, 1).cumulativeOfferedForSale == 4 * amount);
	REQUIRE(orderbook.getPriceCompStats(1, 1).cumulativeOfferedForSaleTimesPrice == expectedHalfAmount + (((int128_t) (3 * amount)) << 32));


	REQUIRE(orderbook.getPriceCompStats(101, 100).cumulativeOfferedForSale == 4 * amount);
	REQUIRE(orderbook.getPriceCompStats(101, 100).cumulativeOfferedForSaleTimesPrice == expectedHalfAmount + (((int128_t) (3 * amount)) << 32));

	REQUIRE(orderbook.getPriceCompStats(1, 2).cumulativeOfferedForSale == amount);
	REQUIRE(orderbook.getPriceCompStats(1, 2).cumulativeOfferedForSaleTimesPrice == expectedHalfAmount);

	REQUIRE(orderbook.getPriceCompStats(1, 200).cumulativeOfferedForSale == 0);
	REQUIRE(orderbook.getPriceCompStats(1, 200).cumulativeOfferedForSaleTimesPrice == ((int128_t) 0) << 32);

	REQUIRE(orderbook.getPriceCompStats(200, 100).cumulativeOfferedForSale == 5 * amount);
	REQUIRE(orderbook.getPriceCompStats(200, 100).cumulativeOfferedForSaleTimesPrice == expectedHalfAmount + (((int128_t) (5 * amount)) << 32));

	REQUIRE(orderbook.getPriceCompStats(201, 100).cumulativeOfferedForSale == 5 * amount);
	REQUIRE(orderbook.getPriceCompStats(201, 100).cumulativeOfferedForSaleTimesPrice == expectedHalfAmount + (((int128_t) (5 * amount)) << 32));
}

