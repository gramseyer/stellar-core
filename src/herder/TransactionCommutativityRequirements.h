#pragma once

#include "util/UnorderedMap.h"
#include <map>
#include "xdr/Stellar-ledger-entries.h"
#include "ledger/LedgerHashUtils.h"
#include "util/XDROperators.h"
#include "herder/AccountCommutativityRequirements.h"

namespace stellar {

class AbstractLedgerTxn;
class LedgerTxnHeader;

class TransactionCommutativityRequirements {

	using AccountMap = std::map<AccountID, AccountCommutativityRequirements>;

	AccountMap mRequirementsMap;

	AccountCommutativityRequirements& 
	getOrCreateAccountReqs(AccountID account);

public:

	bool checkTrustLine(AbstractLedgerTxn& ltx, AccountID account, Asset asset) const;

	void addAssetRequirement(AccountID account, Asset asset, int64_t amount);

	AccountMap const& 
	getRequirements() const;
};

} /* stellar */