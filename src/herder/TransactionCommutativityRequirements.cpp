#include "herder/TransactionCommutativityRequirements.h"

#include "ledger/LedgerTxn.h"
#include "ledger/TrustLineWrapper.h"
#include "transactions/TransactionUtils.h"

namespace stellar {

AccountCommutativityRequirements& 
TransactionCommutativityRequirements::getOrCreateAccountReqs(AccountID account) {
	return (mRequirementsMap.emplace(std::make_pair(account, account))).first->second;
}

bool 
TransactionCommutativityRequirements::checkTrustLine(AbstractLedgerTxn& ltx, AccountID account, Asset asset) const {
	if (asset.type() == ASSET_TYPE_NATIVE)
	{
		return true;
	}

	auto tl = loadTrustLine(ltx, account, asset);

	if (!tl) {
		return false;
	}

	return tl.isCommutativeTxEnabledTrustLine();
}

void 
TransactionCommutativityRequirements::addAssetRequirement(AccountID account, Asset asset, int64_t amount) {
	getOrCreateAccountReqs(account).addAssetRequirement(asset, amount);
}

TransactionCommutativityRequirements::AccountMap const& 
TransactionCommutativityRequirements::getRequirements() const {
	return mRequirementsMap;
}

} /* stellar */