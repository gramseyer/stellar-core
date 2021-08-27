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

using namespace stellar;
using namespace stellar::txtest;

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

    SECTION("xlm success")
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
    }
}