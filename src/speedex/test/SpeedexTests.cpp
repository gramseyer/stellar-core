#include "lib/catch.hpp"

#include "speedex/test/TatonnementTestUtils.h"

using namespace stellar;
using namespace stellar::txtest;


TEST_CASE("orderbook against lp", "[speedex]")
{

	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);
    LedgerTxn ltx(app->getLedgerTxnRoot());

	auto assets = makeAssets(2);

	auto poolID = createLiquidityPool(assets[0], assets[1], 1000, 1000, ltx);

	auto acct = getAccount("blah").getPublicKey();

	for (int32_t i = 90; i < 110; i++) {
		addOffer(ltx, acct, i, 100, 1000, assets[0], assets[1], i);
	}

	auto res = runSpeedex(ltx);

	auto lpRes = findLPClearingStatus(res, poolID);

	REQUIRE((bool) lpRes);

	auto numOffersCrossed = std::ceil((double) (*lpRes).boughtAmount / (double) 1000);

	REQUIRE(numOffersCrossed >= 1);
	REQUIRE(res.offerStatuses.size() == numOffersCrossed);

	for (int32_t i = 90; i < 90 + numOffersCrossed; i++) {
		auto offerRes = findOfferClearingStatus(res, acct, i, 0);

		REQUIRE((bool) offerRes);

		if (i != 90 + numOffersCrossed - 1)
		{
			REQUIRE(*offerRes.soldAmount == 1000);
		} else
		{
			REQUIRE(*offerRes.soldAmount >= (*lpRes).boughtAmount % 1000);
		}
	}

}
