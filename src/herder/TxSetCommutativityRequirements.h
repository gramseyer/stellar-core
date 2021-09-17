#pragma once

#include "transactions/TransactionFrameBase.h"

#include "ledger/LedgerHashUtils.h"

#include <map>
#include "util/UnorderedMap.h"
#include "herder/AccountCommutativityRequirements.h"
#include "util/XDROperators.h"
#include <optional>


namespace stellar {

class AbstractLedgerTxn;
class LedgerTxnHeader;

class TxSetCommutativityRequirements {

	std::map<AccountID, AccountCommutativityRequirements> mAccountRequirements;

	bool
	canAddFee(LedgerTxnHeader& header, AbstractLedgerTxn& ltx, AccountID feeAccount, int64_t fee);

	void addFee(AccountID feeAccount, int64_t fee);

	AccountCommutativityRequirements& 
	getRequirements(AccountID account);


public:

	bool tryAddTransaction(TransactionFrameBasePtr tx, AbstractLedgerTxn& ltx);

	bool validateAndAddTransaction(TransactionFrameBasePtr tx, AbstractLedgerTxn& ltx);

	bool tryReplaceTransaction(TransactionFrameBasePtr newTx, TransactionFrameBasePtr oldTx, AbstractLedgerTxn& ltx);

	void removeTransaction(TransactionFrameBasePtr tx);

	// returns true if account has been removed from the map
	bool tryCleanAccountEntry(AccountID account);

	bool checkAccountHasSufficientBalance(AccountID account, AbstractLedgerTxn& ltx, LedgerTxnHeader& header);

#ifdef BUILD_TESTS
	std::optional<int64_t> getReq(AccountID account, Asset asset);
#endif


};

} /* namespace stellar */
