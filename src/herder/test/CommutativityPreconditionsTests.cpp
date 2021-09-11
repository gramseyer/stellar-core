#include "lib/catch.hpp"

#include "herder/HerderImpl.h"
#include "herder/LedgerCloseData.h"
#include "herder/TxSetFrame.h"
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

TEST_CASE("fee calculation test", "[txset][commutativity]")
{
    TxSetCommutativityRequirements reqs;

    Config cfg(getTestConfig());
    cfg.LEDGER_PROTOCOL_VERSION = 17;
    cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 100;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);
    const int64_t minBalance0 = app->getLedgerManager().getLastMinBalance(0);
    auto baseTxFee = app -> getLedgerManager().getLastTxFee();

    auto paymentSource = root.create("src", 10000 + minBalance0);

    auto xlm = getNativeAsset();

    SECTION("comm txs")
    {

        auto tx = paymentSource.commutativeTx({payment(root, xlm, 1)});

        LedgerTxn ltx(app -> getLedgerTxnRoot());
        auto header = ltx.loadHeader();
        tx -> resetResults(header.current(), baseTxFee, false);

        REQUIRE(reqs.validateAndAddTransaction(
            tx,
            ltx));

        REQUIRE(reqs.getReq(paymentSource.getPublicKey(), xlm));
        REQUIRE(*reqs.getReq(paymentSource.getPublicKey(), xlm) == baseTxFee + 1);
    }
    SECTION("noncomm")
    {

        auto tx = paymentSource.tx({payment(root, xlm, 1)});

        LedgerTxn ltx(app -> getLedgerTxnRoot());
        auto header = ltx.loadHeader();
        tx -> resetResults(header.current(), baseTxFee, false);

        REQUIRE(reqs.validateAndAddTransaction(
            tx,
            ltx));

        REQUIRE(reqs.getReq(paymentSource.getPublicKey(), xlm));
        REQUIRE(*reqs.getReq(paymentSource.getPublicKey(), xlm) == baseTxFee);
    }

}

TEST_CASE("commutative payment tx set", "[txset][commutativity]")
{
	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
	cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 100;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);

    const int64_t minBalance0 = app->getLedgerManager().getLastMinBalance(0);

    auto baseTxFee = app -> getLedgerManager().getLastTxFee();

    SECTION("xlm only tests")
    {
    	auto senderReqs = 50 * baseTxFee;

    	auto paymentSource = root.create("src", 10000 + minBalance0 + senderReqs);
    	auto paymentReceiver = root.create("dest", 10000 + minBalance0);
   		TxSetFramePtr txSet = std::make_shared<TxSetFrame>(
    			app->getLedgerManager().getLastClosedLedgerHeader().hash);

   		std::printf("sender starts with %ld\n", 10000 + minBalance0 + senderReqs);

    	SECTION("one transaction good")
    	{
    		auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 100)});
    		txSet->add(tx);
    		txSet->sortForHash();
    		REQUIRE(txSet -> checkValid(*app, 0, 0));
       	}
       	SECTION("many transactions good")
       	{
       		for (size_t i = 0; i < 50; i++) {
       			std::printf("i = %d\n", i);
       			auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 100)});
       			txSet ->add(tx);
    			txSet->sortForHash();
       			REQUIRE(txSet -> checkValid(*app, 0, 0));
       		}
       	}
       	SECTION("too much xlm spend")
       	{
       		for (size_t i = 0; i < 50; i++) {
       			auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 100)});
       			txSet ->add(tx);
       		}
       		std::printf("last tx\n");
       		auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 5000)});
       		txSet -> add(tx);
       		txSet->sortForHash();
       		REQUIRE(!txSet -> checkValid(*app, 0, 0));

       		auto trimmed = txSet->trimInvalid(*app, 0, 0);
       		REQUIRE(trimmed.size() == 51);
       	}
       	SECTION("only commutative txs get preconditions")
       	{
   			auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 10000)});
   			txSet ->add(tx);
   			tx = paymentSource.tx({payment(paymentReceiver, 100000)});
   			txSet->add(tx);

   			txSet -> sortForHash();
   			REQUIRE(txSet-> checkValid(*app, 0, 0));
       	}
    }
}

TEST_CASE("commutative txs in queue", "[speedex][herder]")
{

    Config cfg(getTestConfig());
    cfg.LEDGER_PROTOCOL_VERSION = 17;
    cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 100;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);

    const int64_t minBalance0 = app->getLedgerManager().getLastMinBalance(0);
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

    auto replacementFee = [](int prevFee) {
        return prevFee * 10;
    };

    SECTION("xlm only tests")
    {
        auto senderReqs = 50 * baseTxFee;

        auto paymentSource = root.create("src", 10000 + minBalance0 + senderReqs);
        auto paymentReceiver = root.create("dest", 10000 + minBalance0);

            

        SECTION("one transaction good")
        {
            feedTx(paymentSource.commutativeTx({payment(paymentReceiver, 100)}));
        }
        SECTION("many transactions good")
        {
            for (size_t i = 0; i < 50; i++) {
                feedTx(paymentSource.commutativeTx({payment(paymentReceiver, 100)}));
            }
        }
        SECTION("too much xlm spend")
        {
            for (size_t i = 0; i < 50; i++) {
                feedTx(paymentSource.commutativeTx({payment(paymentReceiver, 100)}));
            }
            feedBadTx(paymentSource.commutativeTx({payment(paymentReceiver, 5000)}));

        }
        SECTION("only commutative txs get preconditions")
        {
            feedTx(paymentSource.commutativeTx({payment(paymentReceiver, 10000)}));
            feedTx(paymentSource.tx({payment(paymentReceiver, 100000)}));
        }

     /*   SECTION("replace good")
        {
            auto tx1 = paymentSource.commutativeTx({payment(paymentReceiver, 10000 + 48 * baseTxFee)});
            auto tx1r = paymentSource.commutativeTx(
                {payment(paymentReceiver, 5000)},
                paymentSource.getLastSequenceNumber(),
                replacementFee(tx1->getFeeBid()));

            auto tx2 = paymentSource.commutativeTx({payment(paymentReceiver, 4000)});

            feedTx(tx1);

            SECTION("tx2 didn't work before replace")
            {
                feedBadTx(tx2);
            }
            SECTION("tx2 works after replace")
            {
                feedTx(tx1r);
                feedTx(tx2);
            }
        }
        SECTION("replace bad")
        {
            auto tx1 = paymentSource.commutativeTx({payment(paymentReceiver, 10000)});
            auto tx2 = paymentSource.commutativeTx({payment(paymentReceiver, 100000)}, 
                paymentSource.getLastSequenceNumber(),
                replacementFee(tx1->getFeeBid()));
            feedTx(tx1);
            feedBadTx(tx2);
        } */

        SECTION("comm can't follow noncomm")
        {
            feedTx(paymentSource.tx({payment(paymentReceiver, 1)}));
            feedBadTx(paymentSource.commutativeTx({payment(paymentReceiver, 1)}));
        }
    }


}
/*
Attempt to overflow int64_t when checking validity of txset
*/
TEST_CASE("txset requirements overflows", "[speedex][herder]")
{
	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
	cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 100;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);

    const int64_t minBalance2 = app->getLedgerManager().getLastMinBalance(2);

    auto baseTxFee = app -> getLedgerManager().getLastTxFee();
	TxSetFramePtr txSet = std::make_shared<TxSetFrame>(
		app->getLedgerManager().getLastClosedLedgerHeader().hash);

    auto issuer = root.create("issuer", 100000 + minBalance2);
    issuer.setAssetIssuanceLimited();

    auto asset = issuer.asset("ABCD");

    auto main_participant = root.create("main participant", 1000000 + minBalance2);
    main_participant.changeTrust(asset, INT64_MAX);

    auto receiver = root.create("receiver", 1000000 + minBalance2);
    receiver.changeTrust(asset, INT64_MAX);
    issuer.pay(main_participant, asset, 1000);
    SECTION("null hypothesis: int64max is valid")
    {
    	issuer.pay(main_participant, asset, INT64_MAX - 1000);
    	auto tx = main_participant.commutativeTx({payment(receiver, asset, INT64_MAX - 100), payment(receiver, asset, 100)});
    	txSet -> add(tx);
    	txSet -> sortForHash();
    	REQUIRE(txSet -> checkValid(*app, 0, 0));
    }

    SECTION("overflow one tx requirements")
    {
    	auto tx = main_participant.commutativeTx({payment(receiver, asset, INT64_MAX), payment(receiver, asset, 100)});
    	txSet -> add(tx);
    	txSet -> sortForHash();
    	REQUIRE(!txSet -> checkValid(*app, 0, 0));
    }

    SECTION("overflow via multiple tx")
    {
    	txSet -> add(
    		main_participant.commutativeTx({payment(receiver, asset, INT64_MAX)}));
    	txSet -> add(
    		main_participant.commutativeTx({payment(receiver, asset, 1000)}));
    	txSet -> sortForHash();
    	REQUIRE(!txSet -> checkValid(*app, 0, 0));
    }
}


TEST_CASE("asset issuance limits", "[herder][txset][commutativity]")
{
	Config cfg(getTestConfig());
	cfg.LEDGER_PROTOCOL_VERSION = 17;
	cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 100;

    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);

    const int64_t minBalance0 = app->getLedgerManager().getLastMinBalance(0);

    auto baseTxFee = app -> getLedgerManager().getLastTxFee();

    auto feedTx = [&](TransactionFramePtr& tx) {
        REQUIRE(app->getHerder().recvTransaction(tx) ==
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

	TxSetFramePtr txSet = std::make_shared<TxSetFrame>(
		app->getLedgerManager().getLastClosedLedgerHeader().hash);

    SECTION("create asset issuance limit")
    {
    	auto issuer = root.create("issuer", 100000 + minBalance0);
    	issuer.setAssetIssuanceLimited();

    	auto asset = issuer.asset("ABCD");


    	SECTION("proper trustlines for users")
    	{

	    	std::vector<TestAccount> receiverAccounts;
	    	for (auto i = 0u; i < 10u; i++) {
	    		receiverAccounts.push_back(root.create(fmt::format("receiver{}", i), app -> getLedgerManager().getLastMinBalance(2)));
	    		receiverAccounts.back().changeTrust(asset, INT64_MAX);
	    	}

	    	SECTION("can receive asset")
	    	{
	    		for (auto& acct : receiverAccounts) {
	    			auto tx = issuer.commutativeTx({payment(acct, asset, 10000)});
	    			txSet -> add(tx);
	    		}
	    		txSet -> sortForHash();
	    		REQUIRE(txSet -> checkValid(*app, 0, 0));
	    	}

	    	SECTION("can send asset")
	    	{
	    		for (auto& acct : receiverAccounts) {
	    			issuer.pay(acct, asset, 10000);
	    			auto tx = acct.commutativeTx({payment(issuer, asset, 10000)});
	    			txSet -> add(tx);
	    		}
	    		txSet -> sortForHash();
	    		REQUIRE(txSet -> checkValid(*app, 0, 0));
	    	}
    	}
    	SECTION("misconfigured trustlines")
    	{
    		auto badTrustlineAcct = root.create("badAccount", app -> getLedgerManager().getLastMinBalance(2));

    		SECTION("can't receive with no trustline")
    		{
    			txSet -> add(issuer.commutativeTx({payment(badTrustlineAcct, asset, 100)}));
    			txSet -> sortForHash();
    			REQUIRE(!txSet -> checkValid(*app, 0, 0));
    		}

    		badTrustlineAcct.changeTrust(asset, 1000);

    		SECTION("can't receive in commutative section")
    		{
    			txSet -> add(issuer.commutativeTx({payment(badTrustlineAcct, asset, 100)}));
    			txSet -> sortForHash();
    			REQUIRE(!txSet -> checkValid(*app, 0, 0));
    		}
    		SECTION("can receive in noncommutative section")
    		{
    			txSet -> add(issuer.tx({payment(badTrustlineAcct, asset, 100)}));
    			txSet -> sortForHash();
    			REQUIRE(txSet -> checkValid(*app, 0, 0));
    		}
    	}
    }

    SECTION("issuance limit tracking", "[txset][commutativitity]")
    {
    	auto issuer = root.create("issuer", 100000 + minBalance0);
    	issuer.setAssetIssuanceLimited();

    	auto asset = issuer.asset("ABCD");

    	auto receiver = root.create("receiver", app -> getLedgerManager().getLastMinBalance(1) + 10000);
    	receiver.changeTrust(asset, INT64_MAX);

    	SECTION("no payment as yet")
    	{
    		LedgerTxn ltx(app->getLedgerTxnRoot());

    		auto trustLine = stellar::loadTrustLine(ltx, receiver, asset);
    		REQUIRE(!!trustLine);
    		REQUIRE(trustLine.isCommutativeTxEnabledTrustLine());
    		REQUIRE(trustLine.getBalance() == 0);

    		auto tlIssuer = stellar::loadTrustLine(ltx, issuer, asset);

    		REQUIRE(!!tlIssuer);
    		REQUIRE(tlIssuer.isCommutativeTxEnabledTrustLine());
    		REQUIRE(tlIssuer.getBalance() == INT64_MAX);

    	}

    	const int64_t paymentAmount1 = 100000;

    	issuer.pay(receiver, asset, paymentAmount1);

		SECTION("after payment")
    	{
    		LedgerTxn ltx(app->getLedgerTxnRoot());
    		auto header = ltx.loadHeader();

    		auto trustLine = stellar::loadTrustLine(ltx, issuer, asset);
    		REQUIRE(!!trustLine);
    		REQUIRE(trustLine.isCommutativeTxEnabledTrustLine());
    		REQUIRE(trustLine.getBalance() == INT64_MAX - paymentAmount1);

    		REQUIRE(trustLine.addBalance(header, paymentAmount1));

    		REQUIRE(trustLine.getBalance() == INT64_MAX);

    		int64_t bigTransfer = ((int64_t)1) << 62;

    		REQUIRE(trustLine.addBalance(header, -bigTransfer));
    		REQUIRE(!trustLine.addBalance(header, -bigTransfer));
    		REQUIRE(!trustLine.addBalance(header, bigTransfer + 1));
    		REQUIRE(trustLine.getBalance() == INT64_MAX - bigTransfer);
    	}
    }
}
