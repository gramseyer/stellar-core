

#include "lib/catch.hpp"


static void
testTxSet(std::vector<TransactionFrameBasePtr> txs, Application::pointer app)
{
    Config cfg(getTestConfig());
    cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE = 14;
    cfg.LEDGER_PROTOCOL_VERSION = protocolVersion;
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    // set up world
    auto root = TestAccount::createRoot(*app);

    const int nbAccounts = 2;
    const int nbTransactions = 5;

    auto accounts = std::vector<TestAccount>{};

    const int64_t minBalance0 = app->getLedgerManager().getLastMinBalance(0);

    // amount only allows up to nbTransactions
    int64_t amountPop =
        nbTransactions * app->getLedgerManager().getLastTxFee() + minBalance0;

    TxSetFramePtr txSet = std::make_shared<TxSetFrame>(
        app->getLedgerManager().getLastClosedLedgerHeader().hash);

    auto genTx = [&](int nbTxs) {
        std::string accountName = fmt::format("A{}", accounts.size());
        accounts.push_back(root.create(accountName.c_str(), amountPop));
        auto& account = accounts.back();
        for (int j = 0; j < nbTxs; j++)
        {
            // payment to self
            txSet->add(account.tx({payment(account.getPublicKey(), 10000)}));
        }
    };


}

TEST_CASE("commutative payment tx set", "[txset][commutativity]")
{
	Config cfg(getTestConfig());
    VirtualClock clock;
    Application::pointer app = createTestApplication(clock, cfg);

    auto root = TestAccount::createRoot(*app);

    SECTION("xlm success")
    {
    	auto paymentSource = root.create("src", 10000);
    	auto paymentReceiver = root.create("dest", 10000);

   		TxSetFramePtr txSet = std::make_shared<TxSetFrame(
    			app-getLedgerManager(0).getLastClosedLedgerHeader().hash);

    	SECTION("one transaction good")
    	{
    		auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 100)});
    		txSet->add(tx);
    		REQUIRE(txSet -> checkValid(*app, 0, 0));
       	}
       	SECTION("many transactions good")
       	{
       		for (size_t i = 0; i < 50; i++) {
       			auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 100)});
       			txSet ->add(tx);
       			REQUIRE(txSet -> checkValid(*app, 0, 0));
       		}
       	}
       	SECTION("too much xlm spend")
       	{
       		auto tx = paymentSource.commutativeTx({payment(paymentReceiver, 5000)});
       		txSet -> add(tx);
       		REQUIRE(!txSet -> checkValid(*app, 0, 0));
       	}
    }
}