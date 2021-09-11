#include "lib/catch.hpp"

#include "speedex/test/TatonnementTestUtils.h"
#include "simulation/Simulation.h"



#include "herder/HerderImpl.h"

#include "main/Application.h"
#include "main/Config.h"
#include "test/TestAccount.h"
#include "test/TestUtils.h"
#include "test/test.h"
#include "test/TxTests.h"

#include "transactions/OperationFrame.h"
#include "transactions/TransactionFrame.h"
#include "transactions/TransactionUtils.h"

#include "ledger/LedgerTxn.h"
#include "ledger/TrustLineWrapper.h"

using namespace stellar;
using namespace stellar::txtest;

TEST_CASE("sim 2-asset orderbook against lp", "[speedexsim]")
{

	// Boilerplate setup
    SIMULATION_CREATE_NODE(0);

    Config cfg(getTestConfig(0, Config::TESTDB_DEFAULT));

    cfg.MANUAL_CLOSE = false;
    cfg.NODE_SEED = v0SecretKey;

    cfg.QUORUM_SET.threshold = 1;
    cfg.QUORUM_SET.validators.clear();
    cfg.QUORUM_SET.validators.push_back(v0NodeID);

	cfg.LEDGER_PROTOCOL_VERSION = 17;
	cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 1000;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);

    const int64_t minBalance2 = app->getLedgerManager().getLastMinBalance(2);

    auto baseTxFee = app -> getLedgerManager().getLastTxFee();

    auto feedTx = [&](TransactionFramePtr tx) {
        REQUIRE(app->getHerder().recvTransaction(tx) ==
                TransactionQueue::AddResult::ADD_STATUS_PENDING);
    };

    auto feedBadTx = [&](TransactionFramePtr tx) {
        REQUIRE(app->getHerder().recvTransaction(tx) !=
                TransactionQueue::AddResult::ADD_STATUS_PENDING);
    };

    auto waitForExternalize = [&]() {
        bool stop = false;
        auto prev = app->getLedgerManager().getLastClosedLedgerNum();
        VirtualTimer checkTimer(*app);

        auto check = [&](asio::error_code const& error) {
            REQUIRE(!error);
            REQUIRE(app->getLedgerManager().getLastClosedLedgerNum() >
                    prev);
            stop = true;
        };

        checkTimer.expires_from_now(
            Herder::EXP_LEDGER_TIMESPAN_SECONDS +
            std::chrono::seconds(1));
        checkTimer.async_wait(check);
        while (!stop)
        {
            app->getClock().crank(true);
        }
    };


    // Create an issuer account with limited asset issuance
    auto assetIssuer = getIssuanceLimitedAccount(root, "issuer", minBalance2);

    // Create trader accounts
    auto trader1 = root.create("trader1", app -> getLedgerManager().getLastMinBalance(2) + 100 * baseTxFee);
    auto trader2 = root.create("trader2", app -> getLedgerManager().getLastMinBalance(2) + 10 * baseTxFee);

    auto assets = makeAssets(2, assetIssuer);

    // Set the trader accounts to use valid trustlines
    setNonIssuerTrustlines(trader1, assets);
    setNonIssuerTrustlines(trader2, assets);

    // give the traders assets to use.
    fundTrader(trader1, assetIssuer, assets, 100000);
    fundTrader(trader2, assetIssuer, assets, 1000);

    // Create a liquidity pool and enable speedex trading using these assets
    // Lp price: price(asset0) = 10 * price(asset1)
    PoolID poolID;
    {
	    LedgerTxn ltx(app->getLedgerTxnRoot());
		poolID = createLiquidityPool(assets[0], assets[1], 10000, 100000, ltx);
		setSpeedexAssets(ltx, assets);
		ltx.commit();
	}

    VirtualTimer setupTimer(*app);

	auto setup = [&](asio::error_code const& error) {
        REQUIRE(!error);

		// demo commutativity requirements

		feedTx(
			trader2.commutativeTx({payment(trader1, assets[0], 100)}));
		feedTx(
			trader2.commutativeTx({payment(trader1, assets[0], 500)}));
		feedBadTx(
			trader2.commutativeTx({payment(trader1, assets[0], 500)}));
		feedTx(
			trader2.commutativeTx({payment(trader1, assets[0], 400)}, trader2.getLastSequenceNumber()));
		feedTx(
			trader2.tx({payment(trader1, assets[0], 10000)}));
		feedBadTx(
			trader2.commutativeTx({payment(trader1, assets[1], 100)}));

		// Cases not demonstrated:
		// - Overflow
		// - Dependence on multiple accounts
		// - Fee bump replace

		// create sell offers: sell 1 asset0 for ~10 asset1
		// Sell offer at 9:1, 9.1:1, 9.2:1,...10.9:1, 11:1
		int32_t denominator = 100;
		for (int32_t numerator = 900; numerator <= 1100; numerator += 10) {
			Price p;
			p.n = numerator;
			p.d = denominator;
			feedTx(
				trader1.commutativeTx(
					{createSpeedexIOCOffer(assets[0], assets[1], p, 100)}));
		}
	};

	setupTimer.expires_from_now(std::chrono::seconds(0));
    setupTimer.async_wait(setup);

	waitForExternalize();
}