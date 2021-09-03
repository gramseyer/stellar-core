#include "lib/catch.hpp"
#include "speedex/IOCOrderbook.h"
#include "speedex/IOCOffer.h"
#include "speedex/IOCOrderbookManager.h"

#include "speedex/TatonnementOracle.h"
#include "speedex/TatonnementControls.h"

#include "ledger/AssetPair.h"

#include "test/TxTests.h"

#include "test/TestUtils.h"
#include "test/test.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include <map>

using namespace stellar;
using namespace stellar::txtest;

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

TEST_CASE("small 2-asset tatonnement run", "[speedex][tatonnement]")
{
	auto assets = makeAssets(2);

	IOCOrderbookManager manager;

	for (int32_t i = 90; i < 110; i++) {
		addOffer(manager, i, 100, 1000, assets[0], assets[1], i);
		addOffer(manager, i, 100, 1000, assets[1], assets[0], i+100);
	}

	manager.sealBatch();

	TatonnementControlParams controls
	{
		.mTaxRate = 5,
		.mSmoothMult = 5,
		.mMaxRounds = 1000,
		.mStepUp = 45,
		.mStepDown = 25,
		.mStepSizeRadix = 5,
		.mStepRadix = 65
	};

	TatonnementOracle oracle(manager);

	std::map<Asset, uint64_t> prices;
	prices[assets[0]] = 100000;
	prices[assets[1]] = 100;

	oracle.computePrices(controls, prices, 1);

	std::printf("%llu %llu\n", prices[assets[0]], prices[assets[1]]);
}


