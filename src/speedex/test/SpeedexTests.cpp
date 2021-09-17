#include "lib/catch.hpp"

#include "speedex/test/TatonnementTestUtils.h"

#include "ledger/LedgerTxn.h"

#include "main/Application.h"
#include "main/Config.h"

#include "speedex/speedex.h"

#include "test/TestAccount.h"
#include "test/TestUtils.h"
#include "test/test.h"
#include "test/TxTests.h"

#include "transactions/TransactionUtils.h"

using namespace stellar;
using namespace stellar::txtest;

TEST_CASE("3 asset demo", "[speedex]")
{
	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    PoolID poolID;

    auto root = TestAccount::createRoot(*app);

    auto issuer = getIssuanceLimitedAccount(root, "issuer", app->getLedgerManager().getLastMinBalance(2));

    auto trader = root.create("trader", app -> getLedgerManager().getLastMinBalance(10));

    auto assets = makeAssets(3, issuer);

    setNonIssuerTrustlines(trader, assets);

    fundTrader(trader, issuer, assets);

    {
	    LedgerTxn ltx(app->getLedgerTxnRoot());

		setSpeedexAssets(ltx, assets);

		ltx.commit();
	}

	LedgerTxn ltx(app -> getLedgerTxnRoot());

	auto acct = trader.getPublicKey();

	for (int32_t i = 91; i <= 110; i++) {
		addOffer(ltx, acct, 2*i, 100, 100, assets[0], assets[1], i); //  200 / 100
		addOffer(ltx, acct, 3*i, 100, 200, assets[1], assets[2], i); //  600 / 200
		addOffer(ltx, acct, i, 600, 600, assets[2], assets[0], i);   //  100 / 600
	}

	auto res = runSpeedex(ltx);
}

TEST_CASE("orderbook against lp", "[speedex]")
{

	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    PoolID poolID;

    auto root = TestAccount::createRoot(*app);

    auto issuer = getIssuanceLimitedAccount(root, "issuer", app->getLedgerManager().getLastMinBalance(2));

    auto trader = root.create("trader", app -> getLedgerManager().getLastMinBalance(10));

    auto assets = makeAssets(2, issuer);

    setNonIssuerTrustlines(trader, assets);

    fundTrader(trader, issuer, assets);

    SECTION("small lp")
    {

	    {
		    LedgerTxn ltx(app->getLedgerTxnRoot());

			poolID = createLiquidityPool(assets[0], assets[1], 1000, 1000, ltx);

			setSpeedexAssets(ltx, assets);

			ltx.commit();
		}

		LedgerTxn ltx(app -> getLedgerTxnRoot());

		auto acct = trader.getPublicKey();


		for (int32_t i = 91; i <= 110; i++) {
			addOffer(ltx, acct, i, 100, 1000, assets[0], assets[1], i);
		}

		auto res = runSpeedex(ltx);

		auto lpRes = findLPClearingStatus(res, poolID);

		REQUIRE((bool) lpRes);

		auto numOffersCrossed = std::ceil((double) (*lpRes).boughtAmount / (double) 1000);

		REQUIRE(numOffersCrossed >= 1);
		REQUIRE(res.offerStatuses.size() == numOffersCrossed);

		for (int32_t i = 91; i < 91 + numOffersCrossed; i++) {
			auto offerRes = findOfferClearingStatus(res, acct, i, 0);

			REQUIRE((bool) offerRes);

			if (i != 91 + numOffersCrossed - 1)
			{
				REQUIRE((*offerRes).soldAmount == 1000);
			} else
			{
				REQUIRE((*offerRes).soldAmount >= (*lpRes).boughtAmount % 1000);
			}
		}
	}
	SECTION("large lp")
	{
		{
		    LedgerTxn ltx(app->getLedgerTxnRoot());

			poolID = createLiquidityPool(assets[0], assets[1], 1000000, 1000000, ltx);

			setSpeedexAssets(ltx, assets);

			ltx.commit();
		}

		LedgerTxn ltx(app -> getLedgerTxnRoot());

		auto acct = trader.getPublicKey();


		for (int32_t i = 90; i < 110; i++) {
			addOffer(ltx, acct, i, 100, 1000, assets[0], assets[1], i);
		}

		auto res = runSpeedex(ltx);

		auto lpRes = findLPClearingStatus(res, poolID);

		REQUIRE((bool) lpRes);

		auto numOffersCrossed = std::ceil((double) (*lpRes).boughtAmount / (double) 1000);

		REQUIRE(numOffersCrossed >= 1);
		REQUIRE(numOffersCrossed <= 11);
		REQUIRE(res.offerStatuses.size() == numOffersCrossed);

		for (int32_t i = 90; i < 90 + numOffersCrossed; i++) {
			auto offerRes = findOfferClearingStatus(res, acct, i, 0);

			REQUIRE((bool) offerRes);

			if (i != 90 + numOffersCrossed - 1)
			{
				REQUIRE((*offerRes).soldAmount == 1000);
			} else
			{
				REQUIRE((*offerRes).soldAmount >= (*lpRes).boughtAmount % 1000);
			}
		}
	}
	SECTION("two orderbooks")
	{
		{
		    LedgerTxn ltx(app->getLedgerTxnRoot());

			setSpeedexAssets(ltx, assets);

			ltx.commit();
		}

		LedgerTxn ltx(app -> getLedgerTxnRoot());

		auto acct = trader.getPublicKey();


		for (int32_t i = 91; i < 110; i++) {
			addOffer(ltx, acct, i, 100, 100, assets[0], assets[1], i);
			addOffer(ltx, acct, i, 100, 100, assets[1], assets[0], i+100);
		}

		auto res = runSpeedex(ltx);

		auto lpRes = findLPClearingStatus(res, poolID);

		REQUIRE((bool) !lpRes);
	}
	SECTION("two orderbooks interesting")
	{
		{
		    LedgerTxn ltx(app->getLedgerTxnRoot());

			setSpeedexAssets(ltx, assets);

			ltx.commit();
		}

		LedgerTxn ltx(app -> getLedgerTxnRoot());

		auto acct = trader.getPublicKey();


		for (int32_t i = 91; i < 110; i++) {
			addOffer(ltx, acct, 2*i, 100, 100, assets[0], assets[1], i);
			addOffer(ltx, acct, i, 200, 200, assets[1], assets[0], i+100);
		}

		auto res = runSpeedex(ltx);

		auto lpRes = findLPClearingStatus(res, poolID);

		REQUIRE((bool) !lpRes);
	} 
}
