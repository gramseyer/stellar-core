#include "lib/catch.hpp"


#include "ledger/AssetPair.h"
#include "ledger/LedgerTxn.h"

#include "test/TxTests.h"

#include "test/TestUtils.h"
#include "test/test.h"

#include "xdr/Stellar-types.h"
#include "xdr/Stellar-ledger-entries.h"

#include "transactions/TransactionUtils.h"

using namespace stellar;
using namespace stellar::txtest;

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

TEST_CASE("lp trade amounts", "[speedex]")
{
	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
	cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 100;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto assets = makeAssets(10);

    LedgerTxn ltx(app->getLedgerTxnRoot());

    AssetPair pair1 {
    	.selling = assets[0],
    	.buying = assets[1]
    };
    AssetPair pair2 {
    	.selling = assets[2],
    	.buying = assets[3]
    };

    SECTION("marginal price")
    {
        createLiquidityPool(pair1.selling, pair1.buying, 1003, 1000, ltx);

        LiquidityPoolFrame frame(ltx, pair1);

        auto [sell, buy] = frame.getSellBuyAmounts();
        REQUIRE(sell == 1003);
        REQUIRE(buy == 1000);

        auto [pN, pD] = frame.getMinPriceRatioFixedPoint();

        REQUIRE(pN == 1000);
        REQUIRE(pD == 1000);
    }

    SECTION("basic computations no fee")
    {
    	createLiquidityPool(pair1.selling, pair1.buying, 1000, 1000, ltx, 0);

    	LiquidityPoolFrame frame(ltx, pair1);

        std::printf("%lf\n", (double) frame.amountOfferedForSaleTimesSellPrice(25, 1));
    	
    	REQUIRE(frame.amountOfferedForSaleTimesSellPrice(25, 1) <= 25 * 800);
    	REQUIRE(frame.amountOfferedForSaleTimesSellPrice(1, 100) == 0);
    	REQUIRE(frame.amountOfferedForSaleTimesSellPrice(100, 1) <= 100 * 900);
    	REQUIRE(frame.amountOfferedForSaleTimesSellPrice(10000, 100) <= 10000 * 900);
    }

}


