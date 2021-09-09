#include "lib/catch.hpp"
#include "speedex/IOCOrderbook.h"
#include "speedex/IOCOffer.h"
#include "speedex/IOCOrderbookManager.h"

#include "speedex/DemandOracle.h"
#include "speedex/TatonnementOracle.h"
#include "speedex/TatonnementControls.h"

#include "speedex/LiquidityPoolSetFrame.h"

#include "ledger/AssetPair.h"
#include "ledger/LedgerTxn.h"

#include "test/TxTests.h"

#include "test/TestUtils.h"
#include "test/test.h"

#include "transactions/TransactionUtils.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include <map>

#include "speedex/test/TatonnementTestUtils.h"

using namespace stellar;
using namespace stellar::txtest;

TEST_CASE("small 2-asset tatonnement run", "[speedex][tatonnement]")
{

	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);
    LedgerTxn ltx(app->getLedgerTxnRoot());

	auto assets = makeAssets(2);

	LiquidityPoolSetFrame lpFrame({}, ltx);


	auto acct = getAccount("blah").getPublicKey();
	for (int32_t i = 90; i < 110; i++) {
		addOffer(ltx, acct, i, 100, 1000, assets[0], assets[1], i);
		addOffer(ltx, acct, i, 100, 1000, assets[1], assets[0], i+100);
	}

	auto& manager = ltx.getSpeedexIOCOffers();

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

	DemandOracle demandOracle(manager, lpFrame);

	TatonnementOracle oracle(demandOracle);

	std::map<Asset, uint64_t> prices;
	prices[assets[0]] = 100000;
	prices[assets[1]] = 100;

	oracle.computePrices(controls, prices, 1);

	std::printf("%llu %llu\n", prices[assets[0]], prices[assets[1]]);
}

TEST_CASE("trade offers against a liquidity pool", "[speedex][tatonnement]")
{

	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);
    LedgerTxn ltx(app->getLedgerTxnRoot());

	auto assets = makeAssets(2);

	createLiquidityPool(assets[0], assets[1], 1000, 1000, ltx);

	auto acct = getAccount("blah").getPublicKey();

	for (int32_t i = 90; i < 110; i++) {
		addOffer(ltx, acct, i, 100, 1000, assets[0], assets[1], i);
	}

	LiquidityPoolSetFrame lpFrame(assets, ltx);

	auto& manager = ltx.getSpeedexIOCOffers();
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

	DemandOracle demandOracle(manager, lpFrame);

	TatonnementOracle oracle(demandOracle);

	std::map<Asset, uint64_t> prices;
	prices[assets[0]] = 100000;
	prices[assets[1]] = 100;

	oracle.computePrices(controls, prices, 1);

	std::printf("%llu %llu\n", prices[assets[0]], prices[assets[1]]);
}


