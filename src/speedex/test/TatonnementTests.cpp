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

static void createLiquidityPool(Asset const& sell, Asset const& buy, int64_t sellAmount, int64_t buyAmount, AbstractLedgerTxn& ltx, int32_t fee = LIQUIDITY_POOL_FEE_V18)
{
	PoolID id = getPoolID(sell, buy);

	LedgerEntry lpEntry;
    lpEntry.data.type(LIQUIDITY_POOL);
    auto& lp = lpEntry.data.liquidityPool();
    lp.liquidityPoolID = id;
    lp.body.type(LIQUIDITY_POOL_CONSTANT_PRODUCT);

    auto &constProd = lp.body.constantProduct();


    auto& params = constProd.params;
    params.fee = fee;

  	if (sell < buy) {
  		params.assetA = sell;
  		params.assetB = buy;

  		constProd.reserveA = sellAmount;
  		constProd.reserveB = buyAmount;
  	} 
  	else
  	{
  		params.assetA = buy;
  		params.assetB = sell;

  		constProd.reserveA = buyAmount;
  		constProd.reserveB = sellAmount;
  	}

    ltx.create(lpEntry);
}

static void addOffer(AbstractLedgerTxn& ltx, int32_t p_n, int32_t p_d, int64_t amount, Asset const& sell, Asset const& buy, uint64_t idx)
{
	Price p;
	p.n = p_n;
	p.d = p_d;

	AccountID acct = getAccount("blah").getPublicKey();

	IOCOffer offer(amount, p, acct, idx, 0);

	AssetPair tradingPair {
		.selling = sell,
		.buying = buy
	};
	ltx.addSpeedexIOCOffer(tradingPair, offer);
}

TEST_CASE("small 2-asset tatonnement run", "[speedex][tatonnement]")
{

	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);
    LedgerTxn ltx(app->getLedgerTxnRoot());

	auto assets = makeAssets(2);

	LiquidityPoolSetFrame lpFrame({}, ltx);

	for (int32_t i = 90; i < 110; i++) {
		addOffer(ltx, i, 100, 1000, assets[0], assets[1], i);
		addOffer(ltx, i, 100, 1000, assets[1], assets[0], i+100);
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

	for (int32_t i = 90; i < 110; i++) {
		addOffer(ltx, i, 100, 1000, assets[0], assets[1], i);
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


